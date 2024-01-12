#pragma once

#include <cstdint>
#include <typeinfo>

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

    using ClassHashCode = decltype(typeid(int).hash_code());
}
