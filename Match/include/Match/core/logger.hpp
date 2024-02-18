#pragma once

#include <Match/commons.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Match {
    class MATCH_API Logger {
        default_no_copy_move_construction(Logger)
    public:
        void initialize();
        void destroy();

        void set_level(LogLevel level);

        template<typename... Args>
        void trace(Args &&... args) {
            spd_logger->trace(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(Args &&... args) {
            spd_logger->debug(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void info(Args &&... args) {
            spd_logger->info(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warn(Args &&... args) {
            spd_logger->warn(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(Args &&... args) {
            spd_logger->error(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void fatal(Args &&... args) {
            spd_logger->critical(std::forward<Args>(args)...);
        }
    private:
        bool is_initialized = false;
        std::shared_ptr<spdlog::logger> spd_logger;
        LogLevel level = LogLevel::eInfo;
    };

    MATCH_API extern Logger g_logger;

    MATCH_API void set_log_level(LogLevel level);
}

#define MCH_TRACE(...) ::Match::g_logger.trace(__VA_ARGS__);
#define MCH_DEBUG(...) ::Match::g_logger.debug(__VA_ARGS__);
#define MCH_INFO(...) ::Match::g_logger.info(__VA_ARGS__);
#define MCH_WARN(...) ::Match::g_logger.warn(__VA_ARGS__);
#define MCH_ERROR(...) ::Match::g_logger.error(__VA_ARGS__);
#define MCH_FATAL(...) ::Match::g_logger.fatal(__VA_ARGS__);
