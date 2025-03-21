#include "../include/http_client.h"
#include <curl/curl.h>
#include <sstream>
#include <wx/log.h>
#include <iostream>

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), realsize);
    return realsize;
}

wxString HttpClient::Get(const wxString& url) {
    // Result string
    std::string responseString;
    
    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize curl";
    }
    
    // Convert wxString to std::string
    std::string urlStr = url.ToStdString();
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, urlStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 10 seconds timeout
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Check for errors
    if (res != CURLE_OK) {
        wxString errorMsg = wxString::Format("Error: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return errorMsg;
    }
    
    // Get HTTP response code
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    // Clean up
    curl_easy_cleanup(curl);
    
    // Log the response code
    wxLogDebug("HTTP GET request to %s completed with code %ld", url, httpCode);
    
    // Return the response
    return wxString(responseString);
}

wxString HttpClient::Post(const wxString& url, const wxString& jsonData) {
    // Result string
    std::string responseString;
    
    std::cout << "DEBUG: HTTP POST starting to " << url.ToStdString() << std::endl;
    std::cout << "DEBUG: JSON data size: " << jsonData.Length() << " characters" << std::endl;
    
    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "DEBUG: Failed to initialize curl" << std::endl;
        return "Error: Failed to initialize curl";
    }
    
    std::cout << "DEBUG: curl initialized successfully" << std::endl;
    
    try {
        // Convert wxString to std::string - potential crash point with large text
        std::cout << "DEBUG: Converting URL to std::string" << std::endl;
        std::string urlStr = url.ToStdString();
        
        std::cout << "DEBUG: Converting JSON data to std::string" << std::endl;
        std::string dataStr = jsonData.ToStdString();
        std::cout << "DEBUG: JSON data converted, size: " << dataStr.size() << " bytes" << std::endl;
        
        // Set up headers
        std::cout << "DEBUG: Setting up headers" << std::endl;
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        
        // Set curl options
        std::cout << "DEBUG: Setting curl options" << std::endl;
        curl_easy_setopt(curl, CURLOPT_URL, urlStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataStr.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // Increase timeout to 5 minutes (300 seconds)
        
        // Perform the request
        std::cout << "DEBUG: Performing curl request" << std::endl;
        CURLcode res = curl_easy_perform(curl);
        std::cout << "DEBUG: curl request completed with code: " << res << std::endl;
        
        // Check for errors
        if (res != CURLE_OK) {
            std::cout << "DEBUG: curl error: " << curl_easy_strerror(res) << std::endl;
            std::string errorStr = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            return wxString::Format("Error: %s", errorStr);
        }
        
        // Get HTTP response code
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        std::cout << "DEBUG: HTTP response code: " << httpCode << std::endl;
        std::cout << "DEBUG: Response size: " << responseString.size() << " bytes" << std::endl;
        
        // Clean up
        std::cout << "DEBUG: Cleaning up curl resources" << std::endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        
        std::cout << "DEBUG: HTTP POST completed successfully" << std::endl;
        
        // Return the response
        return wxString(responseString);
    }
    catch (const std::exception& e) {
        // Handle std::exception
        std::cout << "DEBUG: Exception in HTTP POST: " << e.what() << std::endl;
        try {
            curl_easy_cleanup(curl);
        } catch (...) {
            std::cout << "DEBUG: Exception during curl cleanup" << std::endl;
        }
        return wxString::Format("Error during HTTP request: %s", e.what());
    }
    catch (...) {
        // Handle unknown exceptions
        std::cout << "DEBUG: Unknown exception in HTTP POST" << std::endl;
        try {
            curl_easy_cleanup(curl);
        } catch (...) {
            std::cout << "DEBUG: Exception during curl cleanup after unknown error" << std::endl;
        }
        return "Unknown error during HTTP request";
    }
} 