#pragma once
#include "../../common/include/logger.h"
#include <memory>

// Global test logger declaration
extern std::shared_ptr<spdlog::logger> test_logger;

// Test-specific logging macros
#define TEST_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(test_logger, __VA_ARGS__)
#define TEST_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(test_logger, __VA_ARGS__)
#define TEST_LOG_INFO(...) SPDLOG_LOGGER_INFO(test_logger, __VA_ARGS__)
#define TEST_LOG_WARNING(...) SPDLOG_LOGGER_WARN(test_logger, __VA_ARGS__)
#define TEST_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(test_logger, __VA_ARGS__)
#define TEST_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(test_logger, __VA_ARGS__) 