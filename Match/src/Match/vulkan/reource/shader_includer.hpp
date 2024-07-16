#pragma once

#include <Match/commons.hpp>
#include <Match/core/utils.hpp>
#include <Match/vulkan/resource/volume_data.hpp>
#include <glslang/Public/ShaderLang.h>
#include <filesystem>

namespace Match {
    class ShaderIncluder final : public glslang::TShader::Includer {
    public:
        ShaderIncluder(const std::filesystem::path &filename) {
            filepath = filename.parent_path();
        }

        IncludeResult* includeSystem(const char* header_name, const char* includer_name, size_t inclusion_depth) {
            std::string *contents;

            if (strcmp(header_name, "MatchTypes") == 0) {
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
                    "\n"
                    "struct MatchGLTFPrimitiveInstanceData {"
                    "    uint first_index;\n"
                    "    uint first_vertex;\n"
                    "    uint material_index;\n"
                    "};\n"
                    "\n"
                    "struct MatchGLTFMaterial {\n"
                    "    vec4 base_color_factor;\n"
                    "    int base_color_texture;\n"
                    "    vec3 emissive_factor;\n"
                    "    int emissive_texture;\n"
                    "    int normal_texture;\n"
                    "    float metallic_factor;\n"
                    "    float roughness_factor;\n"
                    "    int metallic_roughness_texture;\n"
                    "};\n"
                );
            } else if (strcmp(header_name, "MatchRandom") == 0) {
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
            } else if (strcmp(header_name, "MatchVolume") == 0) {
                contents = new std::string(""
                    "int pos_to_volume_data_idx(ivec3 idx_pos) {\n"
                    "    if (idx_pos.x < 0 || idx_pos.y < 0 || idx_pos.z < 0) return -1;\n"
                    "    if (idx_pos.x >= $ || idx_pos.y >= $ || idx_pos.z >= $) return -1;\n"
                    "    return idx_pos.x * $ * $ + idx_pos.y * $ + idx_pos.z;\n"
                    "}\n"
                    "\n"
                    "const int volume_data_size = $ * $ * $;\n"
                    "const int volume_data_resolution = $;\n"
                );
                auto p = contents->find_first_of("$");
                auto vrdr = std::to_string(Match::volume_raw_data_resolution);
                while (p != contents->npos) {
                    contents->replace(p, 1, vrdr);
                    p = contents->find_first_of("$");
                }
            } else {
                header_name = "null";
                contents = new std::string("");
            }

            return new IncludeResult(header_name, contents->c_str(), contents->length(), static_cast<void *>(contents));
        }

        IncludeResult* includeLocal(const char* header_name, const char* includer_name, size_t inclusion_depth) {
            auto data = read_binary_file((filepath / std::string(header_name)).string());
            auto *contents = new std::string(data.data(), data.size());
            return new IncludeResult(header_name, contents->c_str(), contents->length(), static_cast<void *>(contents));
        }

        void releaseInclude(IncludeResult *result) {
            if (result) {
                delete static_cast<std::string *>(result->userData);
                delete result;
            }
        }

        ~ShaderIncluder() {}
    private:
        std::filesystem::path filepath;
    };
}
