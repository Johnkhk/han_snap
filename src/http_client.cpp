#include "../include/http_client.h"
#include <curl/curl.h>
#include <sstream>
#include <wx/log.h>

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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 seconds timeout
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
    
    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize curl";
    }
    
    // Convert wxString to std::string
    std::string urlStr = url.ToStdString();
    std::string dataStr = jsonData.ToStdString();
    
    // Set up headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, urlStr.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 seconds timeout
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Check for errors
    if (res != CURLE_OK) {
        wxString errorMsg = wxString::Format("Error: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return errorMsg;
    }
    
    // Get HTTP response code
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    // Clean up
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    // Log the response code
    wxLogDebug("HTTP POST request to %s completed with code %ld", url, httpCode);
    
    // Return the response
    return wxString(responseString);
} 