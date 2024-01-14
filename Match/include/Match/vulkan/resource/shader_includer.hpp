#pragma once

#include <shaderc/shaderc.hpp>

namespace Match {
    class ShaderIncluder final : public shaderc::CompileOptions::IncluderInterface {
        shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override;
        void ReleaseInclude(shaderc_include_result* data) override;
    };
}
