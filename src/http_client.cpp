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

wxString HttpClient::GetFromLocalhost(const wxString& endpoint, int port) {
    // Construct the URL
    wxString path = endpoint;
    if (!path.StartsWith("/")) {
        path = "/" + path;
    }
    
    wxString url = wxString::Format("http://localhost:%d%s", port, path);
    return Get(url);
}

wxString HttpClient::Get(const wxString& url) {
    // Result string
    std::string responseString;
    
    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error: Failed to initialize curl";
    }
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.mb_str().data());
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
    wxLogDebug("HTTP request to %s completed with code %ld", url, httpCode);
    
    // Return the response
    return wxString(responseString);
} 