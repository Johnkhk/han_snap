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
    if (m_tessApi) {
        tesseract::TessBaseAPI* api = static_cast<tesseract::TessBaseAPI*>(m_tessApi);
        api->End();
        delete api;
        m_tessApi = nullptr;
        m_initialized = false;
    }
}

wxString OcrEngine::ExtractTextFromBitmap(const wxBitmap& bitmap) {
    if (!IsInitialized()) {
        wxLogError("OCR engine not initialized");
        return wxEmptyString;
    }
    
    tesseract::TessBaseAPI* api = static_cast<tesseract::TessBaseAPI*>(m_tessApi);
    
    // Convert wxBitmap to a format Tesseract can understand
    wxImage image = bitmap.ConvertToImage();
    
    // Create a Leptonica PIX object from the wxImage
    int width = image.GetWidth();
    int height = image.GetHeight();
    
    // Always create a 32bpp image for Leptonica (RGBA format)
    PIX* pix = pixCreate(width, height, 32);
    if (!pix) {
        wxLogError("Failed to create PIX object for OCR");
        return wxEmptyString;
    }
    
    // Copy the pixel data from wxImage to PIX
    const unsigned char* imageData = image.GetData();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (y * width + x) * 3;
            
            // RGB values from image data
            int r = imageData[offset];
            int g = imageData[offset + 1];
            int b = imageData[offset + 2];
            
            // Set pixel in PIX with alpha = 255 (fully opaque)
            pixSetRGBPixel(pix, x, y, r, g, b);
        }
    }
    
    // Set the PIX image for OCR
    api->SetImage(pix);
    
    // Get the recognized text
    char* text = api->GetUTF8Text();
    wxString result(text);
    
    // Log the result with proper type casting or format specifiers
    if (result.IsEmpty()) {
        wxLogDebug("OCR completed but no text was recognized");
    } else {
        wxLogDebug("OCR successfully recognized text (%zu characters)", 
                  (size_t)result.Length());
    }
    
    // Clean up
    delete[] text;
    pixDestroy(&pix);
    
    return result;
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