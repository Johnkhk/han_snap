#pragma once

#include <string>
#include <wx/string.h>

/**
 * Simple HTTP client utility using libcurl
 */
class HttpClient {
public:
    /**
     * Sends a GET request to localhost at the specified endpoint
     * 
     * @param endpoint The endpoint path (e.g., "/api/data")
     * @param port The port number (default: 8080)
     * @return Response text or error message
     */
    static wxString GetFromLocalhost(const wxString& endpoint, int port = 8080);
    
    /**
     * Sends a GET request to a fully specified URL
     * 
     * @param url The complete URL to send request to
     * @return Response text or error message
     */
    static wxString Get(const wxString& url);
    
private:
    // Helper function used to accumulate response data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
}; 