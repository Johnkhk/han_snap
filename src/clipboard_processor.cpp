#include "../include/clipboard_processor.h"
#include <wx/log.h>
#include <wx/mstream.h>  // for wxMemoryInputStream
#include <wx/image.h>    // for wxImage loading from stream
#include <wx/file.h>
#include <wx/filename.h>  // For wxFileName
#include <wx/datetime.h> // For timestamp functionality

// Define a struct to hold clipboard content with timestamp
struct ClipboardData {
    wxString textContent;
    wxBitmap imageContent;
    bool hasText;
    bool hasImage;
    wxDateTime timestamp;
    
    ClipboardData() : hasText(false), hasImage(false) {
        timestamp = wxDateTime::Now();
    }
};

ClipboardProcessor::ClipboardProcessor()
    : m_initialized(false)
{
    // Make sure image handlers are initialized
    wxInitAllImageHandlers();
    
    // Initialize the current clipboard data
    m_clipboardData = std::make_shared<ClipboardData>();
}

ClipboardProcessor::~ClipboardProcessor()
{
    Stop();
}

bool ClipboardProcessor::Initialize(std::function<void(const wxString&, const wxDateTime&)> textCallback, 
                                   std::function<void(const wxBitmap&, const wxDateTime&)> imageCallback)
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
        
    if (!wxTheClipboard->Open()) {
        wxLogWarning("Failed to open clipboard");
        return false;
    }
    
    // Log available formats
    LogAvailableFormats();
    
    // Process the clipboard content based on format
    bool processed = false;
    
    // Check format priority: text, then images
    if (HasTextFormat()) {
        processed = ProcessTextFormat();
    }
    else if (HasImageFormat()) {
        processed = ProcessImageFormat();
    }
    else {
        wxLogDebug("Clipboard does not contain supported format (text or image)");
    }
    
    wxTheClipboard->Close();
    return processed;
}

void ClipboardProcessor::Notify()
{
    // This is called by the timer - check the clipboard
    ProcessClipboard();
}

// FORMAT DETECTION METHODS

void ClipboardProcessor::LogAvailableFormats()
{
    // Debug what formats are available - only log the ones that are present
    bool anyFormatFound = false;
    
    if (HasFormatType(wxDF_TEXT)) {
        wxLogDebug("Clipboard has text format");
        anyFormatFound = true;
    }
    
    if (HasFormatType(wxDF_BITMAP)) {
        wxLogDebug("Clipboard has bitmap format");
        anyFormatFound = true;
    }
    
    if (HasFormatType(wxDF_UNICODETEXT)) {
        wxLogDebug("Clipboard has unicode text format");
        anyFormatFound = true;
    }
    
    // Check for specific macOS formats
    #ifdef __WXMAC__
    if (HasFormatName("public.png")) {
        wxLogDebug("Clipboard has public.png format");
        anyFormatFound = true;
    }
    
    if (HasFormatName("PNG")) {
        wxLogDebug("Clipboard has PNG format");
        anyFormatFound = true;
    }
    
    if (HasFormatName("image/png")) {
        wxLogDebug("Clipboard has image/png format");
        anyFormatFound = true;
    }
    
    if (HasFormatName("public.tiff")) {
        wxLogDebug("Clipboard has public.tiff format");
        anyFormatFound = true;
    }
    #endif
    
    if (!anyFormatFound) {
        wxLogDebug("Clipboard does not contain any recognized format");
    }
}

bool ClipboardProcessor::HasFormatType(const wxDataFormat& format)
{
    return wxTheClipboard->IsSupported(format);
}

bool ClipboardProcessor::HasFormatName(const wxString& formatId)
{
    return wxTheClipboard->IsSupported(wxDataFormat(formatId));
}

bool ClipboardProcessor::HasTextFormat()
{
    return HasFormatType(wxDF_TEXT) || HasFormatType(wxDF_UNICODETEXT);
}

bool ClipboardProcessor::HasImageFormat()
{
    if (HasFormatType(wxDF_BITMAP))
        return true;
        
    #ifdef __WXMAC__
    if (HasFormatName("public.png") || HasFormatName("PNG") || 
        HasFormatName("image/png") || HasFormatName("public.tiff"))
        return true;
    #endif
    
    return false;
}

// TEXT PROCESSING METHODS

bool ClipboardProcessor::ProcessTextFormat()
{
    wxString clipboardText;
    
    if (HasFormatType(wxDF_TEXT)) {
        clipboardText = GetClipboardText();
    }
    else if (HasFormatType(wxDF_UNICODETEXT)) {
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        clipboardText = data.GetText();
    }
    
    if (clipboardText.empty()) {
        return false;
    }
    
    wxLogDebug("Clipboard contains text: %s", clipboardText);
    
    // Check if text content has changed
    if (clipboardText != m_lastClipboardContent) {
        wxLogDebug("Clipboard text content changed");
        m_lastClipboardContent = clipboardText;
        
        // Update clipboard data with timestamp
        m_clipboardData = std::make_shared<ClipboardData>();
        m_clipboardData->textContent = clipboardText;
        m_clipboardData->hasText = true;
        m_clipboardData->timestamp = wxDateTime::Now();
        
        // Call the text callback if provided
        if (m_textCallback) {
            m_textCallback(clipboardText, m_clipboardData->timestamp);
        }
        return true;
    }
    
    return false;
}

wxString ClipboardProcessor::GetClipboardText()
{
    wxString clipboardText = wxEmptyString;
    
    // The clipboard should already be open when this is called
    if (HasFormatType(wxDF_TEXT)) {
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        clipboardText = data.GetText();
    }
    
    return clipboardText;
}

// IMAGE PROCESSING METHODS

bool ClipboardProcessor::ProcessImageFormat()
{
    wxBitmap newImage;
    bool gotImage = false;
    
    // Try to get the image in various formats
    if (HasFormatType(wxDF_BITMAP)) {
        wxBitmapDataObject data;
        if (wxTheClipboard->GetData(data)) {
            newImage = data.GetBitmap();
            gotImage = newImage.IsOk();
        }
    }
    
    #ifdef __WXMAC__
    // Try Mac-specific formats if standard bitmap format didn't work
    wxLogDebug("000000000000000000000000");
    if (!gotImage) {
        if (HasFormatName("public.png") || HasFormatName("PNG") || HasFormatName("image/png")) {
            gotImage = ProcessPngFormat();
            // return gotImage; // Mac format handlers call the callback directly
        }
        
        if (HasFormatName("public.tiff")) {
            gotImage = ProcessTiffFormat();
            // return gotImage; // Mac format handlers call the callback directly
        }
    }
    #endif
    wxLogDebug("999999999999999999999999");
    // Log the status of image retrieval
    wxLogDebug("Image retrieval status: gotImage=%d, newImage.IsOk()=%d", 
              gotImage, newImage.IsOk());
    
    // If we successfully got an image, check if it's new and process it
    if (gotImage && newImage.IsOk()) {
        wxLogDebug("Clipboard contains a bitmap image: %d x %d", 
                  newImage.GetWidth(), newImage.GetHeight());
        
        // Check if this is a new image
        bool isNewImage = false;
        wxLogDebug("111111111111111111111111");
        
        if (!m_clipboardData || !m_clipboardData->hasImage) {
            isNewImage = true;
            wxLogDebug("222222222222222222222222");
        } else {
            // Always treat as new image if dimensions changed
            wxLogDebug("333333333333333333333333");
            if (m_clipboardData->imageContent.GetWidth() != newImage.GetWidth() ||
                m_clipboardData->imageContent.GetHeight() != newImage.GetHeight()) {
                isNewImage = true;
            } else {
                // Compare actual image data for images of same size
                // We'll use a simple hash-based approach by converting to PNG and checking size
                wxMemoryOutputStream stream1;
                wxMemoryOutputStream stream2;
                m_clipboardData->imageContent.ConvertToImage().SaveFile(stream1, wxBITMAP_TYPE_PNG);
                newImage.ConvertToImage().SaveFile(stream2, wxBITMAP_TYPE_PNG);
                
                if (stream1.GetLength() != stream2.GetLength()) {
                    isNewImage = true;
                } else {
                    // Compare first few bytes as an additional check
                    const size_t bytesToCheck = std::min<size_t>(static_cast<size_t>(stream1.GetLength()), static_cast<size_t>(1024));
                    wxStreamBuffer* buf1 = stream1.GetOutputStreamBuffer();
                    wxStreamBuffer* buf2 = stream2.GetOutputStreamBuffer();
                    
                    if (memcmp(buf1->GetBufferStart(), buf2->GetBufferStart(), bytesToCheck) != 0) {
                        isNewImage = true;
                    }
                }
            }
        }
        
        if (isNewImage) {
            // Update clipboard data with timestamp only for new images
            m_clipboardData = std::make_shared<ClipboardData>();
            m_clipboardData->imageContent = newImage;
            m_clipboardData->hasImage = true;
            m_clipboardData->timestamp = wxDateTime::Now();
            
            // Call the image callback if provided
            if (m_imageCallback) {
                m_imageCallback(newImage, m_clipboardData->timestamp);
            }
            return true;
        }
    }
    
    return false;
}

bool ClipboardProcessor::ProcessBitmapFormat()
{
    wxBitmap clipboardImage = GetClipboardImage();
    
    if (clipboardImage.IsOk()) {
        wxLogDebug("Processing bitmap format: %d x %d", 
                  clipboardImage.GetWidth(), clipboardImage.GetHeight());
        
        // The comparison and callback is now handled in ProcessImageFormat
        return true;
    }
    
    return false;
}

wxBitmap ClipboardProcessor::GetClipboardImage()
{
    wxBitmap bitmap;
    
    // The clipboard should already be open when this is called
    if (HasFormatType(wxDF_BITMAP)) {
        wxBitmapDataObject data;
        wxTheClipboard->GetData(data);
        bitmap = data.GetBitmap();
    }
    
    return bitmap;
}

#ifdef __WXMAC__
bool ClipboardProcessor::ProcessPngFormat()
{
    wxLogDebug("Attempting to get macOS PNG image from clipboard");
    
    // Create a custom data object for macOS PNG format
    class PngDataObject : public wxCustomDataObject
    {
    public:
        PngDataObject() : wxCustomDataObject(wxDataFormat("public.png")) {}
    };
    
    PngDataObject dataObj;
    if (!wxTheClipboard->GetData(dataObj)) {
        wxLogDebug("Failed to get PNG data from clipboard");
        return false;
    }
    
    wxLogDebug("Successfully got PNG data from clipboard, size: %zu bytes", 
               dataObj.GetSize());
    
    // Try multiple approaches to convert the PNG data
    
    // 1. Try memory stream approach
    if (TryImageFromMemoryStream(dataObj.GetData(), dataObj.GetSize())) {
        return true;
    }
    
    // 2. Try temporary file approach
    return TryImageFromTempFile(dataObj.GetData(), dataObj.GetSize(), "png");
}

bool ClipboardProcessor::ProcessTiffFormat()
{
    wxLogDebug("Attempting to get macOS TIFF image from clipboard");
    
    class TiffDataObject : public wxCustomDataObject
    {
    public:
        TiffDataObject() : wxCustomDataObject(wxDataFormat("public.tiff")) {}
    };
    
    TiffDataObject dataObj;
    if (!wxTheClipboard->GetData(dataObj)) {
        wxLogDebug("Failed to get TIFF data from clipboard");
        return false;
    }
    
    wxLogDebug("Successfully got TIFF data from clipboard, size: %zu bytes", 
               dataObj.GetSize());
    
    // 1. Try memory stream approach
    if (TryImageFromMemoryStream(dataObj.GetData(), dataObj.GetSize(), wxBITMAP_TYPE_TIFF)) {
        return true;
    }
    
    // 2. Try temporary file approach
    return TryImageFromTempFile(dataObj.GetData(), dataObj.GetSize(), "tiff");
}

bool ClipboardProcessor::TryImageFromMemoryStream(const void* data, size_t len, wxBitmapType type)
{
    wxMemoryInputStream memStream(data, len);
    wxImage image;
    
    bool success = false;
    if (type == wxBITMAP_TYPE_ANY) {
        success = image.LoadFile(memStream);
    } else {
        success = image.LoadFile(memStream, type);
    }
    
    if (success) {
        wxBitmap bitmap(image);
        wxLogDebug("Successfully converted image data to bitmap using memory stream: %d x %d", 
                  bitmap.GetWidth(), bitmap.GetHeight());
        
        if (m_imageCallback && bitmap.IsOk()) {
            m_imageCallback(bitmap, wxDateTime::Now());
            return true;
        }
    } else {
        wxLogDebug("Failed to convert image data using memory stream");
    }
    
    return false;
}

bool ClipboardProcessor::TryImageFromTempFile(const void* data, size_t len, const wxString& extension)
{
    // Save the data to a temporary file
    wxString tempFilePath = wxFileName::GetTempDir() + wxFILE_SEP_PATH + 
                           "clipboard_temp." + extension;
    wxFile tempFile(tempFilePath, wxFile::write);
    
    if (!tempFile.IsOpened()) {
        wxLogDebug("Failed to create temporary file");
        return false;
    }
    
    tempFile.Write(data, len);
    tempFile.Close();
    wxLogDebug("Saved image data to temporary file: %s", tempFilePath);
    
    // Try to load the image from the file
    wxImage image;
    if (image.LoadFile(tempFilePath)) {
        wxBitmap bitmap(image);
        wxLogDebug("Successfully loaded image from file: %d x %d", 
                  bitmap.GetWidth(), bitmap.GetHeight());
        
        if (m_imageCallback && bitmap.IsOk()) {
            m_imageCallback(bitmap, wxDateTime::Now());
            return true;
        }
    } else {
        wxLogDebug("Failed to load image from file");
    }
    
    return false;
}
#endif

bool ClipboardProcessor::TryGetImageFromClipboard(wxBitmap& bitmap)
{
    // First try standard bitmap format
    if (HasFormatType(wxDF_BITMAP)) {
        wxBitmapDataObject data;
        if (wxTheClipboard->GetData(data)) {
            bitmap = data.GetBitmap();
            return bitmap.IsOk();
        }
    }
    
    return false;
}

// Add new accessor methods for clipboard data with timestamps
wxDateTime ClipboardProcessor::GetCurrentTimestamp() const
{
    return m_clipboardData->timestamp;
}

std::shared_ptr<ClipboardData> ClipboardProcessor::GetCurrentClipboardData() const
{
    return m_clipboardData;
}

wxString ClipboardProcessor::GetTimestampString() const
{
    if (m_clipboardData) {
        return m_clipboardData->timestamp.Format("%Y-%m-%d %H:%M:%S");
    }
    return wxEmptyString;
}
