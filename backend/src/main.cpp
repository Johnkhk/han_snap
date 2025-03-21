#include <drogon/drogon.h>
#include <curl/curl.h>  // Add this for CURL_GLOBAL_ALL
#include "../include/llm.h"  // Include the LLM header

int main() {
    // Initialize libcurl at application startup
    curl_global_init(CURL_GLOBAL_ALL);

    // Health check route
    drogon::app().registerHandler("/health", 
        [](const drogon::HttpRequestPtr& req, 
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("ok");
            callback(resp);
        },
        {drogon::Get});
    
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
            
            std::cout << "LLM route called" << std::endl;
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
                std::string text;
                
                if (reqJson.contains("text")) {
                    text = reqJson["text"].get<std::string>();
                } else {
                    json errorJson = {
                        {"error", "Missing 'text' field"}
                    };
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setBody(errorJson.dump());
                    callback(resp);
                    return;
                }
                
                
                // Create a more detailed prompt for translation
                // TODO: Use a prompt file and load it
                std::string prompt = "Translate the Chinese text \n\n'" + text + "'\n\nto English. Include:\n"
                                    "- Original text\n"
                                    "- English meaning\n"
                                    "- Mandarin pronunciation (pinyin)\n"
                                    "- Cantonese pronunciation (jyutping)\n"
                                    "- Cantonese equivalent phrase if different from input";

                // Call the GPT API using the simplified structured response template
                Translation translation = getStructuredResponse<Translation>(prompt);

                // Convert Translation to JSON and wrap with original text
                json responseJson = {
                    {"translation", json{
                        {"text", text},           // Add the original text
                        {"result", translation}   // The translation struct will be converted to JSON automatically
                    }}
                };

                // Set the JSON response
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setBody(responseJson.dump());
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
