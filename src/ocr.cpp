#include "../include/ocr.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/log.h>

// Initialize static members
void* OcrEngine::m_tessApi = nullptr;
bool OcrEngine::m_initialized = false;

bool OcrEngine::Initialize(const std::string& language) {
    // Clean up any existing instance
    Cleanup();
    
    // Create a new Tesseract instance
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    
    // Initialize Tesseract with language data
    int result = api->Init(nullptr, language.c_str());
    if (result != 0) {
        wxLogError("Failed to initialize Tesseract OCR engine");
        delete api;
        return false;
    }
    
    // Store the API pointer and set initialization flag
    m_tessApi = api;
    m_initialized = true;
    // wxLogMessage("Tesseract OCR engine initialized successfully");
    return true;
}

void OcrEngine::Cleanup() {
    if (m_initialized) {
        tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
        // This forces Tesseract to clean up its cached data
        api->End();
        delete api;
        m_initialized = false;
    }
}

wxString OcrEngine::ExtractTextFromBitmap(const wxBitmap& bitmap) {
    if (!m_initialized) {
        return "";
    }
    
    // Create a new API instance for this extraction
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    
    // Initialize API with existing language data
    if (api->Init(NULL, "chi_sim+chi_tra") != 0) {
        delete api;
        return "Error initializing Tesseract";
    }
    
    try {
        // Set page segmentation mode to automatic
        api->SetPageSegMode(tesseract::PSM_AUTO);
        
        // Set a timeout limit (10 seconds)
        api->SetVariable("time_limit_per_page_ms", "10000");
        
        // Improve reliability for mixed scripts
        api->SetVariable("preserve_interword_spaces", "1");
        api->SetVariable("tessedit_char_blacklist", "");  // Clear any blacklists
        
        // Convert wxBitmap to format Tesseract can use
        wxImage image = bitmap.ConvertToImage();
        
        // Resize very large images to prevent crashes
        const int MAX_DIMENSION = 2000; // Maximum dimension in pixels
        bool resized = false;
        
        if (image.GetWidth() > MAX_DIMENSION || image.GetHeight() > MAX_DIMENSION) {
            // Calculate new dimensions while maintaining aspect ratio
            double scale = std::min(
                static_cast<double>(MAX_DIMENSION) / image.GetWidth(),
                static_cast<double>(MAX_DIMENSION) / image.GetHeight()
            );
            
            int newWidth = static_cast<int>(image.GetWidth() * scale);
            int newHeight = static_cast<int>(image.GetHeight() * scale);
            
            image.Rescale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH);
            resized = true;
        }
        
        // Get image data
        unsigned char* imageData = image.GetData();
        int width = image.GetWidth();
        int height = image.GetHeight();
        int bytesPerPixel = 3; // RGB format
        
        // Set image data
        api->SetImage(imageData, width, height, bytesPerPixel, width * bytesPerPixel);
        
        // Extract text with safer method to prevent crashes
        char* outText = nullptr;
        try {
            outText = api->GetUTF8Text();
            wxString result = wxString::FromUTF8(outText);
            delete[] outText;
            
            // Clean up
            api->Clear();
            api->End();
            delete api;
            
            if (resized) {
                result = "Note: Image was resized for processing.\n\n" + result;
            }
            
            return result;
        }
        catch (...) {
            // Handle errors during text extraction
            if (outText) {
                delete[] outText;
            }
            throw; // Re-throw to be caught by outer catch block
        }
    }
    catch (const std::exception& e) {
        // Handle any exceptions
        try {
            api->Clear();
            api->End();
            delete api;
        } catch (...) {
            // Ignore any errors during cleanup
        }
        return wxString::Format("OCR Error: %s", e.what());
    }
    catch (...) {
        // Handle any other unexpected exceptions
        try {
            api->Clear();
            api->End();
            delete api;
        } catch (...) {
            // Ignore any errors during cleanup
        }
        return "Unknown OCR error occurred";
    }
}

wxString OcrEngine::ExtractTextFromFile(const wxString& filePath) {
    if (!IsInitialized()) {
        wxLogError("OCR engine not initialized");
        return wxEmptyString;
    }
    
    tesseract::TessBaseAPI* api = static_cast<tesseract::TessBaseAPI*>(m_tessApi);
    
    // Load the image using Leptonica
    PIX* pix = pixRead(filePath.mb_str());
    if (!pix) {
        wxLogError("Failed to load image file for OCR: %s", filePath);
        return wxEmptyString;
    }
    
    // Set the image for OCR
    api->SetImage(pix);
    
    // Get the recognized text
    char* text = api->GetUTF8Text();
    wxString result(text);
    
    // Clean up
    delete[] text;
    pixDestroy(&pix);
    
    return result;
}

bool OcrEngine::IsInitialized() {
    return m_initialized && m_tessApi != nullptr;
} 