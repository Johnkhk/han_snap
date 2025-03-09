#include "../include/clipboard_processor.h"
#include <wx/log.h>
#include <wx/mstream.h>  // for wxMemoryInputStream
#include <wx/image.h>    // for wxImage loading from stream
#include <wx/file.h>
#include <wx/filename.h>  // Add this line for wxFileName

ClipboardProcessor::ClipboardProcessor()
    : m_initialized(false)
{
    // Make sure image handlers are initialized
    // This is crucial for loading images from memory
    wxInitAllImageHandlers();
}

ClipboardProcessor::~ClipboardProcessor()
{
    Stop();
}

bool ClipboardProcessor::Initialize(std::function<void(const wxString&)> textCallback, 
                                   std::function<void(const wxBitmap&)> imageCallback)
{
    if (m_initialized)
        return true;
    
    m_textCallback = textCallback;
    m_imageCallback = imageCallback;
    m_lastClipboardContent = wxEmptyString;
    m_initialized = true;
    
    wxLogDebug("ClipboardProcessor initialized");
    return true;
}

bool ClipboardProcessor::Start(int checkIntervalMs)
{
    if (!m_initialized) {
        wxLogWarning("ClipboardProcessor not initialized before starting");
        return false;
    }
    
    // Start the timer to check clipboard periodically
    bool success = wxTimer::Start(checkIntervalMs);
    if (success) {
        wxLogDebug("ClipboardProcessor started with %d ms interval", checkIntervalMs);
        
        // Process clipboard immediately on start
        ProcessClipboard();
    }
    else {
        wxLogError("Failed to start ClipboardProcessor timer");
    }
    
    return success;
}

void ClipboardProcessor::Stop()
{
    if (IsRunning()) {
        wxTimer::Stop();
        wxLogDebug("ClipboardProcessor stopped");
    }
}

bool ClipboardProcessor::ProcessClipboard()
{
    if (!m_initialized)
        return false;
        
    bool processed = false;
    
    if (wxTheClipboard->Open()) {
        wxLogDebug("Clipboard opened successfully");
        
        // Debug what formats are available
        wxLogDebug("Clipboard has text format: %s", 
                  wxTheClipboard->IsSupported(wxDF_TEXT) ? "yes" : "no");
        wxLogDebug("Clipboard has bitmap format: %s", 
                  wxTheClipboard->IsSupported(wxDF_BITMAP) ? "yes" : "no");
        wxLogDebug("Clipboard has unicode text format: %s", 
                  wxTheClipboard->IsSupported(wxDF_UNICODETEXT) ? "yes" : "no");
        
        // Check for specific macOS formats
        #ifdef __WXMAC__
        wxLogDebug("Clipboard has public.png format: %s",
                  wxTheClipboard->IsSupported(wxDataFormat("public.png")) ? "yes" : "no");
        wxLogDebug("Clipboard has PNG format: %s",
                  wxTheClipboard->IsSupported(wxDataFormat("PNG")) ? "yes" : "no");
        wxLogDebug("Clipboard has image/png format: %s",
                  wxTheClipboard->IsSupported(wxDataFormat("image/png")) ? "yes" : "no");
        wxLogDebug("Clipboard has public.tiff format: %s",
                  wxTheClipboard->IsSupported(wxDataFormat("public.tiff")) ? "yes" : "no");
        #endif
        
        // Check for text content
        if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
            wxString clipboardText = GetClipboardText();
            wxLogDebug("Clipboard contains text: %s", clipboardText);
            
            // Check if text content has changed
            if (clipboardText != m_lastClipboardContent) {
                wxLogDebug("Clipboard text content changed");
                m_lastClipboardContent = clipboardText;
                
                // Call the text callback if provided
                if (m_textCallback) {
                    m_textCallback(clipboardText);
                }
                processed = true;
            }
        }
        // Try for unicode text if regular text check failed
        else if (wxTheClipboard->IsSupported(wxDF_UNICODETEXT)) {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            wxString clipboardText = data.GetText();
            
            wxLogDebug("Clipboard contains unicode text: %s", clipboardText);
            
            // Check if text content has changed
            if (clipboardText != m_lastClipboardContent) {
                wxLogDebug("Clipboard unicode text content changed");
                m_lastClipboardContent = clipboardText;
                
                // Call the text callback if provided
                if (m_textCallback) {
                    m_textCallback(clipboardText);
                }
                processed = true;
            }
        }
        // Check for image content - standard bitmap format
        else if (wxTheClipboard->IsSupported(wxDF_BITMAP)) {
            wxBitmap clipboardImage = GetClipboardImage();
            
            if (clipboardImage.IsOk()) {
                wxLogDebug("Clipboard contains a bitmap image: %d x %d", 
                          clipboardImage.GetWidth(), clipboardImage.GetHeight());
                
                // Call the image callback if provided
                if (m_imageCallback) {
                    m_imageCallback(clipboardImage);
                }
                processed = true;
            }
        }
        #ifdef __WXMAC__
        // Special handling for macOS screenshots using custom data object
        else if (wxTheClipboard->IsSupported(wxDataFormat("public.png"))) {
            wxLogDebug("Attempting to get macOS PNG image from clipboard");
            
            // Create a custom data object for macOS PNG format
            class PngDataObject : public wxCustomDataObject
            {
            public:
                PngDataObject() : wxCustomDataObject(wxDataFormat("public.png")) {}
            };
            
            PngDataObject dataObj;
            if (wxTheClipboard->GetData(dataObj)) {
                wxLogDebug("Successfully got PNG data from clipboard, size: %zu bytes", 
                           dataObj.GetSize());
                
                // Save the data to a temporary file for debugging
                wxString tempFilePath = wxFileName::GetTempDir() + wxFILE_SEP_PATH + "clipboard_temp.png";
                wxFile tempFile(tempFilePath, wxFile::write);
                if (tempFile.IsOpened()) {
                    tempFile.Write(dataObj.GetData(), dataObj.GetSize());
                    tempFile.Close();
                    wxLogDebug("Saved PNG data to temporary file: %s", tempFilePath);
                    
                    // Try to load the image from the file instead of memory
                    wxImage image;
                    if (image.LoadFile(tempFilePath, wxBITMAP_TYPE_PNG)) {
                        wxBitmap bitmap(image);
                        wxLogDebug("Successfully loaded PNG from file: %d x %d", 
                                  bitmap.GetWidth(), bitmap.GetHeight());
                        
                        if (m_imageCallback && bitmap.IsOk()) {
                            m_imageCallback(bitmap);
                            processed = true;
                        }
                    } else {
                        wxLogDebug("Failed to load PNG from file");
                        
                        // Try another approach - load without specifying type
                        if (image.LoadFile(tempFilePath)) {
                            wxBitmap bitmap(image);
                            wxLogDebug("Successfully loaded image from file without specifying type: %d x %d", 
                                      bitmap.GetWidth(), bitmap.GetHeight());
                            
                            if (m_imageCallback && bitmap.IsOk()) {
                                m_imageCallback(bitmap);
                                processed = true;
                            }
                        } else {
                            wxLogDebug("Failed to load image from file without specifying type");
                        }
                    }
                } else {
                    wxLogDebug("Failed to create temporary file");
                }
                
                // Also try the original memory stream approach
                wxMemoryInputStream memStream(dataObj.GetData(), dataObj.GetSize());
                wxImage image2;
                if (image2.LoadFile(memStream, wxBITMAP_TYPE_PNG)) {
                    wxBitmap bitmap(image2);
                    wxLogDebug("Successfully converted PNG data to bitmap using memory stream: %d x %d", 
                              bitmap.GetWidth(), bitmap.GetHeight());
                    
                    if (!processed && m_imageCallback && bitmap.IsOk()) {
                        m_imageCallback(bitmap);
                        processed = true;
                    }
                } else {
                    wxLogDebug("Failed to convert PNG data to image using memory stream");
                }
            } else {
                wxLogDebug("Failed to get PNG data from clipboard");
            }
        }
        #endif
        else {
            wxLogDebug("Clipboard does not contain supported format (text or image)");
        }
        
        wxTheClipboard->Close();
    }
    else {
        wxLogWarning("Failed to open clipboard");
    }
    
    return processed;
}

void ClipboardProcessor::Notify()
{
    // This is called by the timer - check the clipboard
    ProcessClipboard();
}

wxString ClipboardProcessor::GetClipboardText()
{
    wxString clipboardText = wxEmptyString;
    
    // The clipboard should already be open when this is called
    if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        clipboardText = data.GetText();
    }
    
    return clipboardText;
}

wxBitmap ClipboardProcessor::GetClipboardImage()
{
    wxBitmap bitmap;
    
    // The clipboard should already be open when this is called
    if (wxTheClipboard->IsSupported(wxDF_BITMAP)) {
        wxBitmapDataObject data;
        wxTheClipboard->GetData(data);
        bitmap = data.GetBitmap();
    }
    
    return bitmap;
}

// Updated helper method for image retrieval
bool ClipboardProcessor::TryGetImageFromClipboard(wxBitmap& bitmap)
{
    // First try standard bitmap format
    if (wxTheClipboard->IsSupported(wxDF_BITMAP)) {
        wxBitmapDataObject data;
        if (wxTheClipboard->GetData(data)) {
            bitmap = data.GetBitmap();
            return bitmap.IsOk();
        }
    }
    
    #ifdef __WXMAC__
    // On macOS, try to handle PNG format
    // Note: This is a simplified approach and may need refinement
    wxDataFormat pngFormat("public.png");
    if (wxTheClipboard->IsSupported(pngFormat)) {
        // For now, we'll try the standard bitmap approach again
        // This is a placeholder for more sophisticated handling
        wxBitmapDataObject data;
        if (wxTheClipboard->GetData(data)) {
            bitmap = data.GetBitmap();
            return bitmap.IsOk();
        }
    }
    #endif
    
    return false;
}
