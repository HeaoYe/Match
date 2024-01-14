#include <Match/vulkan/resource/shader_includer.hpp>
#include <Match/core/logger.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) {
        auto result = new shaderc_include_result();
        std::string *contents;
        if (type == shaderc_include_type_standard) {
            if (strcmp(requested_source, "MatchTypes") == 0) {
                result->source_name = "MatchTypes";
                contents = new std::string(""
                    "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable\n"
                    "\n"
                    "struct MatchInstanceAddressInfo {\n"
                    "    uint64_t vertex_buffer_address;\n"
                    "    uint64_t index_buffer_address;\n"
                    "};\n"
                    "\n"
                    "struct MatchVertex {\n"
                    "    vec3 pos;\n"
                    "    vec3 normal;\n"
                    "    vec3 color;\n"
                    "};\n"
                    "\n"
                    "struct MatchIndex {\n"
                    "    uint i0; \n"
                    "    uint i1; \n"
                    "    uint i2; \n"
                    "};\n"
                    "\n"
                    "struct MatchSphere {\n"
                    "    vec3 center;\n"
                    "    float radius;\n"
                    "};\n"
                );
            } else if (strcmp(requested_source, "MatchRandom")) {
                result->source_name = "MatchRandom";
                contents = new std::string(""
                    "uint tea(uint val0, uint val1) {\n"
                    "    uint v0 = val0;\n"
                    "    uint v1 = val1;\n"
                    "    uint s0 = 0;\n"
                    "    for(uint n = 0; n < 16; n++) {\n"
                    "        s0 += 0x9e3779b9;\n"
                    "        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);\n"
                    "        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);\n"
                    "    }\n"
                    "    return v0;\n"
                    "}\n"
                    "\n"
                    "uint lcg(inout uint prev) {\n"
                    "    uint LCG_A = 1664525u;\n"
                    "    uint LCG_C = 1013904223u;\n"
                    "    prev = (LCG_A * prev + LCG_C);\n"
                    "    return prev & 0x00FFFFFF;\n"
                    "}\n"
                    "\n"
                    "float rnd(inout uint prev) {\n"
                    "    return (float(lcg(prev)) / float(0x01000000));\n"
                    "}\n"
                );
            }
        } else {
            result->source_name = requested_source;
            auto data = read_binary_file("resource/shaders/" + std::string(requested_source));
            contents = new std::string(data.data(), data.size());
        }
        result->source_name_length = strlen(result->source_name);
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
