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


// Call ChatGPT and request structured JSON output
std::string callChatGPTForJSON(const std::string& prompt, const std::string& responseSchema) {
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
        
        // Create JSON payload using nlohmann/json
        json payload = {
            {"model", "gpt-4-turbo"},
            {"messages", json::array({
                {
                    {"role", "system"},
                    {"content", "You are a helpful assistant that always responds in JSON format. " + 
                               (responseSchema.empty() ? "Use appropriate JSON structure for the response." : 
                                                      "Follow this schema: " + responseSchema)}
                },
                {
                    {"role", "user"},
                    {"content", prompt}
                }
            })},
            {"response_format", {{"type", "json_object"}}},
            {"max_tokens", 500}
        };
        
        // Convert to string - the library handles all escaping automatically
        std::string json_payload = payload.dump();
        
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
                json::parse(content);
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

// Define the Translation struct
struct Translation {
    std::string meaning_english;
    std::string pinyin_mandarin;
    std::string jyutping_cantonese;
    std::string equivalent_cantonese;

    // Enable JSON serialization and deserialization
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Translation, meaning_english, pinyin_mandarin, jyutping_cantonese, equivalent_cantonese)
};

// Get a structured response directly into a C++ struct
template<typename T>
T getStructuredResponse(const std::string& prompt) {
    // Create a schema description from the struct's fields
    std::string schema = "{";
    // For simplicity we're just assuming string fields here
    // In a real implementation, you might want to reflect on T's structure
    schema += "\"meaning_english\": \"string\", ";
    schema += "\"pinyin_mandarin\": \"string\", ";
    schema += "\"jyutping_cantonese\": \"string\", ";
    schema += "\"equivalent_cantonese\": \"string\"";
    schema += "}";
    
    // Call the API with the schema
    std::string json_response = callChatGPTForJSON(prompt, schema);
    
    // Extract just the content part
    std::string json_content = extractJSONContent(json_response);
    
    // Parse into the struct
    try {
        return json::parse(json_content).get<T>();
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing response: " << e.what() << std::endl;
        return T(); // Return default-constructed object
    }
}


int main() {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Example 4: Using a struct for structured data
    std::string chinese_word = "你好";
    std::string struct_prompt = "Provide translation information for the Chinese word '" + 
                                chinese_word + "'. Include English meaning, Mandarin pinyin, " +
                                "Cantonese jyutping, and equivalent Cantonese expression.";
    
    Translation translation = getStructuredResponse<Translation>(struct_prompt);
    
    // Display the structured result
    std::cout << "\nStructured Translation Response:" << std::endl;
    std::cout << "English meaning: " << translation.meaning_english << std::endl;
    std::cout << "Mandarin pinyin: " << translation.pinyin_mandarin << std::endl;
    std::cout << "Cantonese jyutping: " << translation.jyutping_cantonese << std::endl;
    std::cout << "Cantonese equivalent: " << translation.equivalent_cantonese << std::endl;
    
    // Clean up libcurl
    curl_global_cleanup();
    return 0;
}
