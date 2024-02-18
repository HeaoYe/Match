#pragma once

#include <vector>  // IWYU pragma: export
#include <map>  // IWYU pragma: export
#include <set>  // IWYU pragma: export
#include <memory>  // IWYU pragma: export
#include <string>  // IWYU pragma: export
#include <glm/glm.hpp>  // IWYU pragma: export

#if defined (MATCH_INNER_VISIBLE)
    #define INNER_VISIBLE public
    #define INNER_PROTECT public
#else
    #define INNER_VISIBLE private
    #define INNER_PROTECT protected
#endif

#if defined (PLATFORM_WINDOWS)
    #if defined (WINDOWS_DLL)
        #define MATCH_API __declspec(dllexport)
    #else
        #define MATCH_API __declspec(dllimport)
    #endif
#else
    #define MATCH_API
#endif

namespace Match {
    #define no_copy_construction(cls_name) cls_name(const cls_name &) = delete;
    #define no_move_construction(cls_name) cls_name(cls_name &&) = delete;
    #define default_construction(cls_name) public: \
        cls_name() = default;
    #define no_copy_move_construction(cls_name) no_copy_construction(cls_name) no_move_construction(cls_name)
    #define default_no_copy_move_construction(cls_name) default_construction(cls_name) no_copy_construction(cls_name) no_move_construction(cls_name)
}

#include <Match/types.hpp>
#include <Match/core/logger.hpp>
