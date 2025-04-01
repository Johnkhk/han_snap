#include <drogon/drogon.h>
#include <curl/curl.h>  // Add this for CURL_GLOBAL_ALL
#include "../include/llm.h"  // Include the LLM header
#include "../../common/include/logger.h"

// Module-level static logger initialization for main
static std::shared_ptr<spdlog::logger> getMainLogger() {
    static std::shared_ptr<spdlog::logger> logger = hansnap::Logger::getInstance().createLogger("main");
    return logger;
}

// Convenience macros
#define MAIN_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(getMainLogger(), __VA_ARGS__)
#define MAIN_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(getMainLogger(), __VA_ARGS__)
#define MAIN_LOG_INFO(...) SPDLOG_LOGGER_INFO(getMainLogger(), __VA_ARGS__)
#define MAIN_LOG_WARNING(...) SPDLOG_LOGGER_WARN(getMainLogger(), __VA_ARGS__)
#define MAIN_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(getMainLogger(), __VA_ARGS__)
#define MAIN_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(getMainLogger(), __VA_ARGS__)

int main() {
    // Initialize the main logger
    hansnap::Logger::getInstance().initialize("hansnap_backend");
    
    // Configure log level
    const char* log_level = std::getenv("LOG_LEVEL");
    if (log_level) {
        std::string level(log_level);
        if (level == "debug") {
            hansnap::Logger::getInstance().setLevel(hansnap::Logger::Level::DEBUG);
        } else if (level == "trace") {
            hansnap::Logger::getInstance().setLevel(hansnap::Logger::Level::TRACE);
        }
    }
    
    // Add file logging
    hansnap::Logger::getInstance().addFileLogger("backend.log");
    
    // Initialize component loggers
    auto llm_logger = hansnap::Logger::getInstance().createLogger("llm");
    auto db_logger = hansnap::Logger::getInstance().createLogger("database");
    auto api_logger = hansnap::Logger::getInstance().createLogger("api");
    
    MAIN_LOG_INFO("Starting Hansnap backend server...");
    
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
            MAIN_LOG_INFO("Hello, Drogon!");
            resp->setBody("Hello, Drogon!");
            callback(resp);
        },
        {drogon::Get});

    // LLM route
    drogon::app().registerHandler("/llm", 
        [](const drogon::HttpRequestPtr& req, 
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            
            MAIN_LOG_INFO("LLM route called");
            // Create a response
            auto resp = drogon::HttpResponse::newHttpResponse();
            
            try {
                // Convert string_view to string explicitly
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
                
                // Create prompt for translation
                std::string prompt = "Translate the Chinese text \n\n'" + text + "'\n\nto English. Include:\n"
                                    "- Original text\n"
                                    "- English meaning\n"
                                    "- Mandarin pronunciation (pinyin)\n"
                                    "- Cantonese pronunciation (jyutping)\n"
                                    "- Cantonese equivalent phrase if different from input";

                // Get translation
                Translation translation = getStructuredResponse<Translation>(prompt);
                
                // Convert Translation to JSON
                json translationJson = translation;
                
                // Add audio data to the translation JSON
                json enhancedJson = addAudioToJson(translationJson);
                
                // Wrap with original text in the final response
                json responseJson = {
                    {"translation", json{
                        {"text", text},           // Add the original text
                        {"result", enhancedJson}  // The enhanced translation
                    }}
                };

                // Log the final response JSON before returning
                std::string finalResponse = responseJson.dump();

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
    spdlog::shutdown();
    return 0;
}
