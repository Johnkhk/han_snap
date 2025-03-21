#include <iostream>
#include <string>
#include <cstdlib> // For getenv()
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>  // For file handling
#include <sstream>  // For string stream

// Use the nlohmann json namespace
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

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
        std::cerr << "Error: LLM_API_KEY environment variable not set." << std::endl;
        return "ERROR: API key not found in environment variables";
    }
    
    CURL* curl = curl_easy_init();
    std::string response_data;
    
    if (curl) {
        std::string url = "https://api.openai.com/v1/chat/completions";
        std::cout << "Calling ChatGPT API..." << std::endl;
        
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
        std::cout << "Raw API response: " << rawResponse << std::endl;
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
                (void)parsed; // Avoid unused variable warning
                return content;
            } catch (const json::parse_error& e) {
                return "{ \"error\": \"Invalid JSON in response content: " + std::string(e.what()) + "\" }";
            }
        }
        
        return "{ \"error\": \"Couldn't extract JSON content from response\" }";
    } catch (const std::exception& e) {
        return "{ \"error\": \"Error parsing response: " + std::string(e.what()) + "\" }";
    }
}
