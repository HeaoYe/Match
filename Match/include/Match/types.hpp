#pragma once

#include <cstdint>

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
}
