#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <memory>  // For std::shared_ptr
#include "../../common/include/logger.h"  // For spdlog
#include "model.h"  // Include model header for struct definitions

// Declare the logger function first - this lets the macros know it exists
std::shared_ptr<spdlog::logger> getLLMLogger();

// Convenience macros
#define LLM_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_INFO(...) SPDLOG_LOGGER_INFO(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_WARNING(...) SPDLOG_LOGGER_WARN(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(getLLMLogger(), __VA_ARGS__)

// Use the nlohmann json namespace
using json = nlohmann::json;

/**
 * WriteCallback for libcurl to accumulate response data
 */
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);

/**
 * Call ChatGPT API with a prompt and expected schema to get a JSON response
 * 
 * @param prompt The text prompt to send to ChatGPT
 * @param schemaJson The expected JSON schema for the response (can be empty json object)
 * @return Raw response from the API
 */
std::string callChatGPTForJSON(const std::string& prompt, const json& schemaJson = json());

/**
 * Extract the actual JSON content from the OpenAI API response
 * 
 * @param rawResponse The full raw response from OpenAI
 * @return Just the message content as JSON string
 */
std::string extractJSONContent(const std::string& rawResponse);

/**
 * Generate audio for translation text and add it to the JSON
 * 
 * @param translationJson The translation JSON to enhance with audio
 * @return Enhanced JSON with audio data
 */
json addAudioToJson(const json& translationJson);

/**
 * Get a structured response directly into a C++ struct or any type that can be
 * deserialized from JSON using nlohmann_json.
 * 
 * @param prompt The text prompt to send to ChatGPT
 * @return A structured object of type T containing the response
 */
template<typename T>
T getStructuredResponse(const std::string& prompt) {
    // Generate a simple description of what we need
    json schema = T::responseSchema();
    std::string json_response = callChatGPTForJSON(prompt, schema); 
    LLM_LOG_INFO("1111111111111111");
    // Extract just the content part
    std::string json_content = extractJSONContent(json_response);
    LLM_LOG_INFO("2222222222222222");
    // Parse into JSON and convert to the target type
    try {
        json parsed = json::parse(json_content);
        return parsed.get<T>();
    } catch (const std::exception& e) {
        LLM_LOG_ERROR("Error processing response: {}", e.what());
        return T(); // Return default-constructed object
    }
}