#pragma once

#include <vector>  // IWYU pragma: export
#include <map>  // IWYU pragma: export
#include <set>  // IWYU pragma: export
#include <memory>  // IWYU pragma: export
#include <string>  // IWYU pragma: export

#if defined (MATCH_INNER_VISIBLE)
    #define INNER_VISIBLE public
#else
    #define INNER_VISIBLE private
#endif

namespace Match {
    enum class LogLevel {
        eTrace,
        eDebug,
        eInfo,
        eError,
        eWarn,
        eFatal,
    };

    struct WindowSize {
        uint32_t width;
        uint32_t height;
    };

    #define no_copy_construction(cls_name) cls_name(const cls_name &) = delete;
    #define no_move_construction(cls_name) cls_name(cls_name &&) = delete;
    #define default_construction(cls_name) public: \
        cls_name() = default;
    #define no_copy_move_construction(cls_name) no_copy_construction(cls_name) no_move_construction(cls_name)
    #define default_no_copy_move_construction(cls_name) default_construction(cls_name) no_copy_construction(cls_name) no_move_construction(cls_name)
}

#include <Match/core/logger.hpp>
