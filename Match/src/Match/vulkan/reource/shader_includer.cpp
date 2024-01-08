#include <Match/vulkan/resource/shader_includer.hpp>
#include <Match/core/logger.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) {
        auto result = new shaderc_include_result();
        if (type == shaderc_include_type_standard) {
            if (strcmp(requested_source, "MatchTypes") == 0) {
                auto contents = new std::string(
"#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable\n"
"\n"
"struct InstanceAddressInfo {\n"
"    uint64_t vertex_buffer_address;\n"
"    uint64_t index_buffer_address;\n"
"};\n"
"\n"
"struct Vertex {\n"
"    vec3 pos;\n"
"    vec3 normal;\n"
"    vec3 color;\n"
"};\n"
"\n"
"struct Index {\n"
"    uint i0; \n"
"    uint i1; \n"
"    uint i2; \n"
"};\n"
                );
                result->source_name = "MatchType";
                result->source_name_length = strlen(result->source_name);
                result->content = contents->data();
                result->content_length = contents->size();
                result->user_data = contents;
            }
        } else {
            result->source_name = requested_source;
            result->source_name_length = strlen(requested_source);
            auto data = read_binary_file("resource/shaders/" + std::string(requested_source));
            auto contents = new std::string(data.data(), data.size());
            result->content = contents->data();
            result->content_length = contents->size();
            result->user_data = contents;
        }
        return result;
    }

    void ShaderIncluder::ReleaseInclude(shaderc_include_result* data) {
        free(static_cast<std::string *>(data->user_data));
        free(data);
    }
}
