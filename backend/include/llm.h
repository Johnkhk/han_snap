#pragma once

#include <string>
#include <nlohmann/json.hpp>

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
 * @param responseSchema The expected JSON schema for the response
 * @return Raw response from the API
 */
std::string callChatGPTForJSON(const std::string& prompt, const std::string& responseSchema);

/**
 * Extract the actual JSON content from the OpenAI API response
 * 
 * @param rawResponse The full raw response from OpenAI
 * @return Just the message content as JSON string
 */
std::string extractJSONContent(const std::string& rawResponse);

/**
 * Translation data structure
 */
struct Translation {
    std::string meaning_english;
    std::string pinyin_mandarin;
    std::string jyutping_cantonese;
    std::string equivalent_cantonese;

    // Enable JSON serialization and deserialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Translation, meaning_english, pinyin_mandarin, jyutping_cantonese, equivalent_cantonese)
};

/**
 * Get a structured response directly into a C++ struct
 * 
 * @param prompt The text prompt to send to ChatGPT
 * @return A structured object of type T containing the response
 */
template<typename T>
T getStructuredResponse(const std::string& prompt); 