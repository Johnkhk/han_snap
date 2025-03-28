#pragma once
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <memory>
#include <mutex>

namespace hansnap {

class Logger {
public:
    enum class Level {
        TRACE = spdlog::level::trace,
        DEBUG = spdlog::level::debug,
        INFO = spdlog::level::info,
        WARNING = spdlog::level::warn,
        ERROR = spdlog::level::err,
        CRITICAL = spdlog::level::critical,
        OFF = spdlog::level::off
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Initialize with default console logger
    void initialize(const std::string& appName = "hansnap") {
        // Guard against multiple initializations
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_initialized) return;
        
        try {
            // Create console sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            
            // Create default logger with console sink
            m_logger = std::make_shared<spdlog::logger>(appName, console_sink);
            
            // Set global default logger
            spdlog::set_default_logger(m_logger);
            
            // Set pattern: [timestamp] [level] [logger] message
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
            
            // Set default level
            spdlog::set_level(spdlog::level::info);
            
            m_initialized = true;
            m_logger->info("Logger initialized");
        }
        catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }

    // Add file logging
    bool addFileLogger(const std::string& filename, 
                       size_t maxFileSize = 5 * 1024 * 1024,    // 5MB
                       size_t maxFiles = 3) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized) {
            initialize();
        }
        
        try {
            // Create rotating file sink (5MB max size, 3 rotated files)
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filename, maxFileSize, maxFiles);
                
            // Add the file sink to the logger
            m_logger->sinks().push_back(file_sink);
            m_logger->info("File logger added: {}", filename);
            return true;
        }
        catch (const spdlog::spdlog_ex& ex) {
            if (m_logger) {
                m_logger->error("Failed to add file logger: {}", ex.what());
            } else {
                std::cerr << "Failed to add file logger: " << ex.what() << std::endl;
            }
            return false;
        }
    }

    // Set global logging level
    void setLevel(Level level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized) {
            initialize();
        }
        
        spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
        
        if (m_logger) {
            m_logger->info("Log level set to {}", spdlog::level::to_string_view(static_cast<spdlog::level::level_enum>(level)));
        }
    }

    // Get the logger
    std::shared_ptr<spdlog::logger> getLogger() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized) {
            initialize();
        }
        
        return m_logger;
    }

    // Create a new named logger (for components)
    std::shared_ptr<spdlog::logger> createLogger(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_initialized) {
            initialize();
        }
        
        // Check if logger already exists
        auto logger = spdlog::get(name);
        if (logger) {
            return logger;
        }
        
        // Create a new logger with the same sinks as the default
        logger = std::make_shared<spdlog::logger>(name, m_logger->sinks().begin(), m_logger->sinks().end());
        logger->set_level(m_logger->level());
        
        // Register it
        spdlog::register_logger(logger);
        
        return logger;
    }

private:
    Logger() : m_initialized(false) {}
    ~Logger() {
        // Shutdown spdlog to flush any pending logs
        spdlog::shutdown();
    }
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool m_initialized;
    std::shared_ptr<spdlog::logger> m_logger;
    std::mutex m_mutex;
};

// Global logging convenience macros
#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARNING(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

} // namespace hansnap 