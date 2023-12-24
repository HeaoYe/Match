#include <Match/core/logger.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    Logger g_logger;

    void Logger::initialize() {
        if (is_initialized) {
            return;
        }
        is_initialized = true;
        spd_logger = spdlog::stdout_color_mt("Match Core");
        spd_logger->set_level(transform<spdlog::level::level_enum>(level));
        spd_logger->info("Create Logger");
    }

    void Logger::destroy() {
        if (!is_initialized) {
            return;
        }
        is_initialized = false;
        spd_logger->info("Destroy Logger");
        // spdlog::shutdown();
    }

    void Logger::set_level(LogLevel level) {
        this->level = level;
        if (is_initialized) {
            spd_logger->set_level(transform<spdlog::level::level_enum, LogLevel>(level));
        }
    }

    void set_log_level(LogLevel level) {
        g_logger.set_level(level);
    }
}
