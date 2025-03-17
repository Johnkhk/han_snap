#include <drogon/drogon.h>
#include <curl/curl.h>  // Add this for CURL_GLOBAL_ALL
#include "../include/llm.h"  // Include the LLM header

int main() {
    // Initialize libcurl at application startup
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Simple GET route - Fix handler signature
    drogon::app().registerHandler("/hello", 
        [](const drogon::HttpRequestPtr& req, 
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            std::cout << "Hello, Drogon!" << std::endl;
            resp->setBody("Hello, Drogon!");
            callback(resp);
        },
        {drogon::Get});

    // LLM route - Fix handler signature and JSON handling
    drogon::app().registerHandler("/llm", 
        [](const drogon::HttpRequestPtr& req, 
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            
            // Create a response
            auto resp = drogon::HttpResponse::newHttpResponse();
            
            try {
                // Fix: Convert string_view to string explicitly
                std::string_view bodyView = req->getBody();
                std::string reqBody(bodyView);  // Explicit conversion
                
                json reqJson;
                
                try {
                    reqJson = json::parse(reqBody);
                } catch (const std::exception& e) {
                    // Handle invalid JSON
                    json errorJson = {
                        {"error", "Invalid JSON request"}
                    };
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setBody(errorJson.dump());
                    callback(resp);
                    return;
                }
                
                // Extract fields from the request
                std::string prompt;
                std::string schema = "{}";  // Default empty schema
                
                if (reqJson.contains("prompt")) {
                    prompt = reqJson["prompt"].get<std::string>();
                } else {
                    json errorJson = {
                        {"error", "Missing 'prompt' field"}
                    };
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setBody(errorJson.dump());
                    callback(resp);
                    return;
                }
                
                if (reqJson.contains("schema")) {
                    schema = reqJson["schema"].get<std::string>();
                }
                
                // Call the GPT API
                std::string rawResponse = callChatGPTForJSON(prompt, schema);
                std::string jsonContent = extractJSONContent(rawResponse);
                
                // Set the raw JSON string as the response body
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(jsonContent);
                callback(resp);
                
            } catch (const std::exception& e) {
                json errorJson = {
                    {"error", std::string("Exception: ") + e.what()}
                };
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(errorJson.dump());
                callback(resp);
            }
        },
        {drogon::Post});  // Specify that this is a POST endpoint

    // Configure and run the server
    drogon::app().addListener("0.0.0.0", 8080).run();
    
    // Clean up libcurl at application shutdown
    curl_global_cleanup();
    return 0;
}
