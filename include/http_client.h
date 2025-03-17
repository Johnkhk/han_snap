#pragma once

#include <wx/string.h>

/**
 * Simple HTTP client utility using libcurl
 */
class HttpClient {
public:
    /**
     * Sends a GET request to a URL
     * 
     * @param url The complete URL to send request to (e.g., "http://localhost:8080/api/data")
     * @return Response text or error message
     */
    static wxString Get(const wxString& url);
    
    /**
     * Sends a POST request with JSON data
     * 
     * @param url The complete URL to send request to
     * @param jsonData The JSON data to send in the request body
     * @return Response text or error message
     */
    static wxString Post(const wxString& url, const wxString& jsonData);
    
private:
    // Helper function used to accumulate response data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
}; 