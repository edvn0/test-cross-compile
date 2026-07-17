#include "load_model.hxx"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace {

auto to_glm(fastgltf::math::fmat4x4 const &matrix) noexcept -> glm::mat4 {
  glm::mat4 result{1.0F};

  for (std::size_t column = 0; column < 4; ++column) {
    for (std::size_t row = 0; row < 4; ++row) {
      result[column][row] = matrix[column][row];
    }
  }

  return result;
}

auto find_attribute(fastgltf::Primitive const &primitive, std::string_view name)
    -> std::optional<std::size_t> {
  auto const iterator = primitive.findAttribute(name);

  if (iterator == primitive.attributes.end()) {
    return std::nullopt;
  }

  return iterator->accessorIndex;
}

template <typename T>
auto read_accessor(fastgltf::Asset const &asset, std::size_t accessor_index)
    -> std::vector<T> {
  auto const &accessor = asset.accessors[accessor_index];

  std::vector<T> values(accessor.count);

  fastgltf::copyFromAccessor<T>(asset, accessor, values.data());

  return values;
}

auto load_indices(fastgltf::Asset const &asset,
                  fastgltf::Primitive const &primitive,
                  std::size_t vertex_count)
    -> std::expected<std::vector<std::uint32_t>, ModelLoadError> {
  if (!primitive.indicesAccessor.has_value()) {
    std::vector<std::uint32_t> indices(vertex_count);

    for (std::uint32_t index = 0;
         index < static_cast<std::uint32_t>(vertex_count); ++index) {
      indices[index] = index;
    }

    return indices;
  }

  auto const accessor_index = *primitive.indicesAccessor;

  if (accessor_index >= asset.accessors.size()) {
    return std::unexpected(ModelLoadError{
        .type = ModelLoadErrorType::invalid_accessor,
    });
  }

  auto const &accessor = asset.accessors[accessor_index];

  std::vector<std::uint32_t> indices(accessor.count);

  fastgltf::copyFromAccessor<std::uint32_t>(asset, accessor, indices.data());

  return indices;
}

auto load_primitive(fastgltf::Asset const &asset,
                    fastgltf::Primitive const &primitive,
                    GeometryArena &geometry_arena)
    -> std::expected<ModelPrimitive, ModelLoadError> {
  if (primitive.type != fastgltf::PrimitiveType::Triangles) {
    return std::unexpected(ModelLoadError{
        .type = ModelLoadErrorType::unsupported_primitive,
    });
  }

  auto const position_accessor = find_attribute(primitive, "POSITION");

  if (!position_accessor.has_value()) {
    return std::unexpected(ModelLoadError{
        .type = ModelLoadErrorType::missing_position,
    });
  }

  auto const positions = read_accessor<glm::vec3>(asset, *position_accessor);

  std::vector<ModelVertex> vertices(positions.size());

  for (std::size_t index = 0; index < positions.size(); ++index) {
    vertices[index].position = positions[index];

    // Safe defaults for optional glTF attributes.
    vertices[index].normal = glm::vec3{0.0F, 1.0F, 0.0F};

    vertices[index].tangent = glm::vec4{1.0F, 0.0F, 0.0F, 1.0F};

    vertices[index].texcoord = glm::vec2{0.0F};
  }

  if (auto const normal_accessor = find_attribute(primitive, "NORMAL")) {
    auto const normals = read_accessor<glm::vec3>(asset, *normal_accessor);

    if (normals.size() != vertices.size()) {
      return std::unexpected(ModelLoadError{
          .type = ModelLoadErrorType::invalid_accessor,
      });
    }

    for (std::size_t index = 0; index < vertices.size(); ++index) {
      vertices[index].normal = normals[index];
    }
  }

  if (auto const tangent_accessor = find_attribute(primitive, "TANGENT")) {
    auto const tangents = read_accessor<glm::vec4>(asset, *tangent_accessor);

    if (tangents.size() != vertices.size()) {
      return std::unexpected(ModelLoadError{
          .type = ModelLoadErrorType::invalid_accessor,
      });
    }

    for (std::size_t index = 0; index < vertices.size(); ++index) {
      vertices[index].tangent = tangents[index];
    }
  }

  if (auto const texcoord_accessor = find_attribute(primitive, "TEXCOORD_0")) {
    auto const texcoords = read_accessor<glm::vec2>(asset, *texcoord_accessor);

    if (texcoords.size() != vertices.size()) {
      return std::unexpected(ModelLoadError{
          .type = ModelLoadErrorType::invalid_accessor,
      });
    }

    for (std::size_t index = 0; index < vertices.size(); ++index) {
      vertices[index].texcoord = texcoords[index];
    }
  }

  auto indices = load_indices(asset, primitive, vertices.size());

  if (!indices) {
    return std::unexpected(indices.error());
  }

  auto geometry =
      geometry_arena.allocate_mesh(std::span<const ModelVertex>{vertices},
                                   std::span<const std::uint32_t>{*indices});

  if (!geometry) {
    return std::unexpected(ModelLoadError{
        .type = ModelLoadErrorType::geometry_upload_failed,
        .geometry_error = geometry.error(),
    });
  }

  return ModelPrimitive{
      .geometry = *geometry,
      .material_index =
          primitive.materialIndex.has_value()
              ? static_cast<std::uint32_t>(*primitive.materialIndex)
              : 0,
  };
}

} // namespace

auto load_model(std::filesystem::path const &path,
                GeometryArena &geometry_arena)
    -> std::expected<Model, ModelLoadError> {
  fastgltf::GltfFileStream file_stream{path};

  if (!file_stream.isOpen()) {
    return std::unexpected(ModelLoadError{
        .type = ModelLoadErrorType::file_not_found,
    });
  }

  static fastgltf::Parser parser;

  constexpr auto options = fastgltf::Options::LoadExternalBuffers |
                           fastgltf::Options::GenerateMeshIndices;

  auto asset_result =
      parser.loadGltfBinary(file_stream, path.parent_path(), options);

  if (asset_result.error() != fastgltf::Error::None) {
    return std::unexpected(ModelLoadError{
        .type = ModelLoadErrorType::parse_error,
    });
  }

  auto asset = std::move(asset_result.get());

  Model model;
  model.meshes.reserve(asset.meshes.size());
  model.nodes.reserve(asset.nodes.size());

  for (auto const &gltf_mesh : asset.meshes) {
    ModelMesh mesh;
    mesh.primitives.reserve(gltf_mesh.primitives.size());

    for (auto const &gltf_primitive : gltf_mesh.primitives) {
      auto primitive = load_primitive(asset, gltf_primitive, geometry_arena);

      if (!primitive) {
        return std::unexpected(primitive.error());
      }

      mesh.primitives.push_back(std::move(*primitive));
    }

    model.meshes.push_back(std::move(mesh));
  }

  for (auto const &gltf_node : asset.nodes) {
    ModelNode node{
        .local_transform = to_glm(fastgltf::getTransformMatrix(gltf_node)),
    };

    if (gltf_node.meshIndex.has_value()) {
      node.mesh_index = static_cast<std::uint32_t>(*gltf_node.meshIndex);
    }

    node.children.reserve(gltf_node.children.size());

    for (auto const child : gltf_node.children) {
      node.children.push_back(static_cast<std::uint32_t>(child));
    }

    model.nodes.push_back(std::move(node));
  }

  auto const scene_index = asset.defaultScene.value_or(0);

  if (scene_index < asset.scenes.size()) {
    auto const &scene = asset.scenes[scene_index];

    model.scene_roots.reserve(scene.nodeIndices.size());

    for (auto const node_index : scene.nodeIndices) {
      model.scene_roots.push_back(static_cast<std::uint32_t>(node_index));
    }
  }

  return model;
}