#include <iostream>
#include <string>
#include <cstdlib> // For getenv()
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>  // For file handling
#include <sstream>  // For string stream
#include <vector>
#include <cstring>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include "../../common/include/logger.h"
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>  // Add this line for base64 functions

// Use the nlohmann json namespace
using json = nlohmann::json;

// Module-level static logger initialization
static std::shared_ptr<spdlog::logger> getLLMLogger() {
    static std::shared_ptr<spdlog::logger> logger = hansnap::Logger::getInstance().createLogger("llm");
    return logger;
}

// Convenience macros
#define LLM_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_INFO(...) SPDLOG_LOGGER_INFO(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_WARNING(...) SPDLOG_LOGGER_WARN(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(getLLMLogger(), __VA_ARGS__)
#define LLM_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(getLLMLogger(), __VA_ARGS__)

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

json generateAudioLinks(const json& jsonResponse);
std::string generateSpeech(const std::string& text, const std::string& language, const std::string& voice);
std::string generateSpeechGoogle(const std::string& text, const std::string& languageCode, const std::string& voice);
std::string base64_decode(const std::string& encoded);

/**
 * Call ChatGPT API with a prompt and expected schema to get a JSON response
 * 
 * @param prompt The text prompt to send to ChatGPT
 * @param schemaJson The expected JSON schema for the response (can be empty json object)
 * @return Raw response from the API
 */
std::string callChatGPTForJSON(const std::string& prompt, const json& schemaJson = json()) {
    // Get API key from environment variable
    const char* api_key = std::getenv("LLM_API_KEY");
    if (!api_key) {
        LLM_LOG_ERROR("LLM_API_KEY environment variable not set");
        return "ERROR: API key not found in environment variables";
    }
    
    CURL* curl = curl_easy_init();
    std::string response_data;
    
    if (curl) {
        std::string url = "https://api.openai.com/v1/chat/completions";
        LLM_LOG_INFO("Calling ChatGPT API...");
        
        // Create JSON payload using nlohmann/json
        json payload = {
            // {"model", "gpt-4o"},
            {"model", "gpt-4o-mini"},
            {"messages", json::array({
                {
                    {"role", "system"},
                    {"content", "You are an expert translator. You are given a text and you need to translate it into English. You will respond in JSON format with fields: meaning_english, pinyin_mandarin, jyutping_cantonese, equivalent_cantonese."}
                },
                {
                    {"role", "user"},
                    {"content", prompt}
                }
            })}
        };
        
        // Add response_format section
        if (schemaJson.empty()) {
            // Basic JSON mode - just return JSON
            payload["response_format"] = {{"type", "json_object"}};
        } else {
            // Use the provided JSON object directly
            payload["response_format"] = {
                {"type", "json_schema"},
                {"json_schema", {
                    {"name", "translation_schema"},
                    {"strict", true},
                    {"schema", schemaJson}  // Use the JSON object directly
                }}
            };
        }
        
        // Convert to string
        std::string json_payload = payload.dump();
        // std::cout << "Request payload: " << json_payload << std::endl;
        
        // Set up headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // Use environment variable for Authorization header
        std::string auth_header = "Authorization: Bearer ";
        auth_header += api_key;
        headers = curl_slist_append(headers, auth_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Curl request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    return response_data;
}

// Parse JSON response and return just the JSON content (not the OpenAI wrapper)
std::string extractJSONContent(const std::string& rawResponse) {
    try {
        LLM_LOG_DEBUG("Raw API response: {}", rawResponse);
        
        json response = json::parse(rawResponse);
        
        // Check if there's an error
        if (response.contains("error")) {
            return "{ \"error\": \"" + response["error"]["message"].get<std::string>() + "\" }";
        }
        
        // Extract the message content (which should be JSON text)
        if (response.contains("choices") && !response["choices"].empty() && 
            response["choices"][0].contains("message") && 
            response["choices"][0]["message"].contains("content")) {
            
            std::string content = response["choices"][0]["message"]["content"].get<std::string>();
            
            // Validate that the content is valid JSON
            try {
                json parsed = json::parse(content);
                parsed = generateAudioLinks(parsed);  // Add audio links to the JSON
                return parsed.dump();
            } catch (const json::parse_error& e) {
                return "{ \"error\": \"Invalid JSON in response content: " + std::string(e.what()) + "\" }";
            }
        }
        
        return "{ \"error\": \"Couldn't extract JSON content from response\" }";
    } catch (const std::exception& e) {
        return "{ \"error\": \"Error parsing response: " + std::string(e.what()) + "\" }";
    }
}

/**
 * Generate a random UUID string
 * 
 * @return A string containing a random UUID
 */
std::string generateUUID() {
    boost::uuids::random_generator generator;
    boost::uuids::uuid uuid = generator();
    return boost::lexical_cast<std::string>(uuid);
}

/**
 * Generate speech using OpenAI TTS API
 * 
 * @param text The text to convert to speech
 * @param voice The voice to use (e.g., "alloy", "echo", "fable", "onyx", "nova", "shimmer")
 * @return std::string containing the filename of the generated audio file
 */
std::string generateSpeech(const std::string& text, const std::string& language, const std::string& voice) {
    // Get API key from environment variable
    const char* api_key = std::getenv("LLM_API_KEY");
    if (!api_key) {
        LLM_LOG_ERROR("LLM_API_KEY environment variable not set");
        return "";
    }
    
    // Use a simpler approach - collect data in memory first
    std::string response_data;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        LLM_LOG_ERROR("Error initializing CURL");
        return "";
    }

    // Create JSON payload for TTS API
    json payload = {
        {"model", "tts-1"},
        {"input", text},
        {"language", language},
        {"voice", voice},
        // {"instructions", "Speak in cantonese"},
        {"response_format", "mp3"}
    };
    
    std::string json_payload = payload.dump();
    std::string url = "https://api.openai.com/v1/audio/speech";
    
    // Set up headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    std::string auth_header = "Authorization: Bearer ";
    auth_header += api_key;
    headers = curl_slist_append(headers, auth_header.c_str());

    // Use the basic WriteCallback we already have
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    LLM_LOG_INFO("Generating speech for text: {}", text);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Clean up CURL
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        LLM_LOG_ERROR("TTS API request failed: {}", curl_easy_strerror(res));
        return "";
    }
    
    // Generate a unique filename with UUID
    std::string filename = "speech_" + generateUUID() + ".mp3";
    
    // Now that we have the data in memory, write it to a file
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile.is_open()) {
        LLM_LOG_ERROR("Could not open file for writing: {}", filename);
        return "";
    }
    
    outfile.write(response_data.data(), response_data.size());
    outfile.close();
    
    if (outfile.fail()) {
        LLM_LOG_ERROR("Error writing to output file");
        return "";
    }
    
    LLM_LOG_INFO("Speech generated and saved to: {}", filename);
    return filename;
}

/**
 * Generate Mandarin and Cantonese Audio Links and add onto Json Response
 * 
 * @param jsonResponse The JSON response from the API as a json object
 * @return The modified JSON with audio links added
 */
json generateAudioLinks(const json& jsonResponse) {
    json result = jsonResponse; // Make a copy to modify
    
    // Extract the text to convert to speech from the JSON
    std::string mandarinText = "";
    std::string cantoneseText = "";

    LLM_LOG_DEBUG("JSON Response: {}", result.dump());
    
    if (result.contains("original_text")) {
        mandarinText = result["original_text"].get<std::string>();
    }
    
    if (result.contains("equivalent_cantonese")) {
        cantoneseText = result["equivalent_cantonese"].get<std::string>();
    }
    
    // Generate audio files only if we have text
    if (!mandarinText.empty()) {
        std::string mandarin_audio_link = generateSpeech(mandarinText, "mandarin", "alloy");
        if (!mandarin_audio_link.empty()) {
            result["mandarin_audio_link"] = mandarin_audio_link;
        }
    }
    
    if (!cantoneseText.empty()) {
        std::string cantonese_audio_link = generateSpeechGoogle(cantoneseText, "yue-HK", "yue-HK-Standard-A");
        if (!cantonese_audio_link.empty()) {
            result["cantonese_audio_link"] = cantonese_audio_link;
        }
    }

    LLM_LOG_INFO("Generated audio links: {}", result.dump());
    return result;
}

/**
 * Generate speech using Google Cloud Text-to-Speech API
 * 
 * @param text The text to convert to speech
 * @param languageCode The language code (e.g., "en-US", "zh-CN", "yue-Hant-HK")
 * @param voice The voice name (e.g., "en-US-Standard-A")
 * @return std::string containing the filename of the generated audio file
 */
std::string generateSpeechGoogle(const std::string& text, const std::string& languageCode, const std::string& voice) {
    // Get API key from environment variable
    const char* google_api_key = std::getenv("GOOGLE_TTS_API_KEY");
    if (!google_api_key) {
        LLM_LOG_ERROR("GOOGLE_TTS_API_KEY environment variable not set");
        return "";
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        LLM_LOG_ERROR("Error initializing CURL");
        return "";
    }

    std::string response_data;
    
    // Create JSON payload for Google TTS API
    json payload = {
        {"input", {
            {"text", text}
        }},
        {"voice", {
            {"languageCode", languageCode},
            {"name", voice}
        }},
        {"audioConfig", {
            {"audioEncoding", "MP3"}
        }}
    };
    
    std::string json_payload = payload.dump();
    LLM_LOG_DEBUG("Google TTS request payload: {}", json_payload);
    
    // Google Cloud TTS endpoint
    std::string url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=";
    url += google_api_key;
    
    // Set up headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    LLM_LOG_INFO("Generating speech with Google TTS for text: {}", text);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Clean up CURL
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        LLM_LOG_ERROR("Google TTS API request failed: {}", curl_easy_strerror(res));
        return "";
    }
    
    try {
        // Parse JSON response
        json response = json::parse(response_data);
        
        // Google returns base64-encoded audio content
        if (!response.contains("audioContent")) {
            LLM_LOG_ERROR("Google TTS API response missing audioContent: {}", response_data);
            return "";
        }
        
        // Decode base64 content
        std::string base64_audio = response["audioContent"].get<std::string>();
        std::string decoded_audio = base64_decode(base64_audio);
        
        // Generate a unique filename with UUID
        std::string filename = "speech_google_" + generateUUID() + ".mp3";
        
        // Write to file
        std::ofstream outfile(filename, std::ios::binary);
        if (!outfile.is_open()) {
            LLM_LOG_ERROR("Could not open file for writing: {}", filename);
            return "";
        }
        
        outfile.write(decoded_audio.data(), decoded_audio.size());
        outfile.close();
        
        if (outfile.fail()) {
            LLM_LOG_ERROR("Error writing to output file");
            return "";
        }
        
        LLM_LOG_INFO("Google TTS speech generated and saved to: {}", filename);
        return filename;
        
    } catch (const std::exception& e) {
        LLM_LOG_ERROR("Error processing Google TTS response: {}", e.what());
        return "";
    }
}

/**
 * Helper function to decode base64 string to binary data
 * 
 * @param encoded The base64-encoded string
 * @return The decoded binary data as a string
 */
std::string base64_decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.c_str(), -1);
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    char* buffer = new char[encoded.size()];
    int decoded_size = BIO_read(bio, buffer, encoded.size());
    
    std::string result(buffer, decoded_size);
    
    delete[] buffer;
    BIO_free_all(bio);
    
    return result;
}


