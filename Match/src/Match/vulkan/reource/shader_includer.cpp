#include <Match/vulkan/resource/shader_includer.hpp>
#include <Match/core/logger.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) {
        auto result = new shaderc_include_result();
        result->source_name = requested_source;
        result->source_name_length = strlen(requested_source);
        auto data = read_binary_file("resource/shaders/" + std::string(requested_source));
        auto contents = new std::string(data.data(), data.size());
        result->content = contents->data();
        result->content_length = contents->size();
        result->user_data = contents;
        return result;
    }

    void ShaderIncluder::ReleaseInclude(shaderc_include_result* data) {
        free(static_cast<std::string *>(data->user_data));
        free(data);
    }
}
