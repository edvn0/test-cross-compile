#pragma once

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include "material.hxx" // MaterialHandle
#include "model.hxx" // ModelHandle
#include "physics_components.hxx"
#include "transform.hxx"

namespace Components {
    struct PointLight {
        glm::vec3 colour{1.0F};
        float intensity = 1.0F;
        float range = 10.0F;
    };

    struct SpotLight {
        glm::vec3 colour{1.0F};
        float intensity = 1.0F;
        float range = 10.0F;
        float inner_cone_degrees = 20.0F;
        float outer_cone_degrees = 30.0F;
    };


    struct MaterialOverride {
        MaterialHandle material{};
    };

    struct Model {
        ModelHandle model{};
    };

    struct PlayerTag {};
} // namespace Components
