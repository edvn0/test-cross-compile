// describe() overloads for error types owned by the gpu module. Declared in
// error_describe.hxx (via error_types.def), defined here so this module's
// error headers stay private to it -- see error_describe.cxx's file comment.
#include "error_describe.hxx"

#include <format>

#include "device_error.hxx"
#include "gpu_resource_table.hxx"
#include "image.hxx"
#include "image_storage.hxx"
#include "pipeline.hxx"
#include "pipeline_storage.hxx"
#include "sampler_storage.hxx"
#include "shader_object.hxx"
#include "shader_object_storage.hxx"

auto describe(DeviceError const &error) -> std::string {
    auto const message = error.message.empty() ? std::string_view{"(no message)"} : error.message.view();

    return std::format("DeviceError({}): {} [VkResult={}] ({}:{})", error.type, message,
                       static_cast<int>(error.vk_result), error.location.file_name(), error.location.line());
}

auto describe(ImageError const &error) -> std::string {
    auto head = std::format("ImageError({})", error.type);

    if (error.cause.has_value()) {
        return head + " -> " + describe(*error.cause);
    }

    return head;
}

auto describe(ImageStorageError const &error) -> std::string {
    auto head = std::format("ImageStorageError({})", error.type);

    if (error.cause.has_value()) {
        return head + " -> " + describe(*error.cause);
    }

    return head;
}

auto describe(PipelineError const &error) -> std::string {
    auto head = std::format("PipelineError({})", error.type);

    if (error.context.has_value()) {
        return head + " -> " + describe(*error.context);
    }

    return head;
}

auto describe(PipelineStorageError const &error) -> std::string {
    auto head = std::format("PipelineStorageError({})", error.type);

    if (error.cause.has_value()) {
        return head + " -> " + describe(*error.cause);
    }

    return head;
}

auto describe(ShaderObjectError const &error) -> std::string {
    auto head = std::format("ShaderObjectError({})", error.type);

    if (error.context.has_value()) {
        return head + " -> " + describe(*error.context);
    }

    return head;
}

auto describe(ShaderObjectStorageError const &error) -> std::string {
    auto head = std::format("ShaderObjectStorageError({})", error.type);

    if (error.cause.has_value()) {
        return head + " -> " + describe(*error.cause);
    }

    return head;
}

auto describe(GpuResourceTableError const &error) -> std::string {
    auto head = std::format("GpuResourceTableError({})", error.type);

    if (error.context.has_value()) {
        return head + " -> " + describe(*error.context);
    }

    return head;
}

auto describe(SamplerStorageError const &error) -> std::string {
    auto head = std::format("SamplerStorageError({})", error.type);

    if (error.context.has_value()) {
        return head + " -> " + describe(*error.context);
    }

    return head;
}
