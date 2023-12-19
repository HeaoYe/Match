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
    enum class PlatformWindowSystem {
        eNone,
#if defined (MATCH_WAYLAND)
        eWayland,
#endif
#if defined (MATCH_XLIB)
        eXlib,
#endif
#if defined (MATCH_XCB)
        eXcb,
#endif
#if defined (MATCH_WIN32)
        eWin32,
#endif
    };

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

    enum class VertexType {
        eInt8, eInt8x2, eInt8x3, eInt8x4,
        eInt16, eInt16x2, eInt16x3, eInt16x4,
        eInt32, eInt32x2, eInt32x3, eInt32x4,
        eInt64, eInt64x2, eInt64x3, eInt64x4,
        eUint8, eUint8x2, eUint8x3, eUint8x4,
        eUint16, eUint16x2, eUint16x3, eUint16x4,
        eUint32, eUint32x2, eUint32x3, eUint32x4,
        eUint64, eUint64x2, eUint64x3, eUint64x4,
        eFloat, eFloat2, eFloat3, eFloat4,
        eDouble, eDouble2, eDouble3, eDouble4,
    };

    enum class IndexType {
        eUint16,
        eUint32,
    };

    #define no_copy_construction(cls_name) cls_name(const cls_name &) = delete;
    #define no_move_construction(cls_name) cls_name(cls_name &&) = delete;
    #define default_construction(cls_name) public: \
        cls_name() = default;
    #define no_copy_move_construction(cls_name) no_copy_construction(cls_name) no_move_construction(cls_name)
    #define default_no_copy_move_construction(cls_name) default_construction(cls_name) no_copy_construction(cls_name) no_move_construction(cls_name)
}

#include <Match/core/logger.hpp>
