#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "model.h"  // Include model header for struct definitions

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
 * Get a structured response directly into a C++ struct or any type that can be
 * deserialized from JSON using nlohmann_json.
 * 
 * @param prompt The text prompt to send to ChatGPT
 * @return A structured object of type T containing the response
 */
template<typename T>
T getStructuredResponse(const std::string& prompt) {
    // Generate a simple description of what we need
    // std::cout << "Prompt: " << prompt << std::endl;
    json schema = T::responseSchema();
    std::string json_response = callChatGPTForJSON(prompt, schema); 
    // Extract just the content part
    std::string json_content = extractJSONContent(json_response);
    
    // Parse into the type
    try {
        return json::parse(json_content).get<T>();
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing response: " << e.what() << std::endl;
        return T(); // Return default-constructed object
    }
}