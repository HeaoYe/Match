#include <Match/core/utils.hpp>
#include <Match/core/logger.hpp>

namespace Match {
    template <>
    spdlog::level::level_enum transform<spdlog::level::level_enum, LogLevel>(LogLevel src) {
        switch (src) {
        case LogLevel::eTrace:
            return spdlog::level::trace;
        case LogLevel::eDebug:
            return spdlog::level::debug;
        case LogLevel::eInfo:
            return spdlog::level::info;
        case LogLevel::eWarn:
            return spdlog::level::warn;
        case LogLevel::eError:
            return spdlog::level::err;
        case LogLevel::eFatal:
            return spdlog::level::critical;
        }
    }
}
