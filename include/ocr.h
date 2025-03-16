#pragma once

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <string>

/**
 * OcrEngine provides a simple interface to Tesseract OCR.
 * It allows text recognition from wxBitmap objects or image files.
 */
class OcrEngine {
public:
    /**
     * Initialize the OCR engine with a specific language.
     * 
     * @param language Language data to use (e.g., "eng" for English)
     * @return true if initialization was successful
     */
    static bool Initialize(const std::string& language = "eng");
    
    /**
     * Clean up OCR engine resources.
     */
    static void Cleanup();
    
    /**
     * Extract text from a bitmap image.
     * 
     * @param bitmap The bitmap to process
     * @return Recognized text
     */
    static wxString ExtractTextFromBitmap(const wxBitmap& bitmap);
    
    /**
     * Extract text from an image file.
     * 
     * @param filePath Path to the image file
     * @return Recognized text
     */
    static wxString ExtractTextFromFile(const wxString& filePath);
    
    /**
     * Check if the OCR engine is initialized.
     * 
     * @return true if the engine is ready for use
     */
    static bool IsInitialized();

private:
    static void* m_tessApi;  // Opaque pointer to TessBaseAPI
    static bool m_initialized;
}; 