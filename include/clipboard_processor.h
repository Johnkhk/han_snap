#ifndef CLIPBOARD_PROCESSOR_H
#define CLIPBOARD_PROCESSOR_H

#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/timer.h>
#include <functional>
#include <string>

class ClipboardProcessor : public wxTimer
{
public:
    ClipboardProcessor();
    virtual ~ClipboardProcessor();

    // Initialize the processor with optional callbacks for different data types
    bool Initialize(std::function<void(const wxString&)> textCallback = nullptr,
                   std::function<void(const wxBitmap&)> imageCallback = nullptr);
    
    // Start monitoring the clipboard
    bool Start(int checkIntervalMs = 500);
    
    // Stop monitoring the clipboard
    void Stop() override;

    // Check clipboard content (can be called manually)
    bool ProcessClipboard();

private:
    // wxTimer notification override
    void Notify() override;

    // Format detection methods
    void LogAvailableFormats();
    bool HasFormatType(const wxDataFormat& format);
    bool HasFormatName(const wxString& formatId);
    bool HasTextFormat();
    bool HasImageFormat();
    
    // Text processing methods
    bool ProcessTextFormat();
    wxString GetClipboardText();
    
    // Image processing methods
    bool ProcessImageFormat();
    bool ProcessBitmapFormat();
    wxBitmap GetClipboardImage();
    
    #ifdef __WXMAC__
    bool ProcessPngFormat();
    bool ProcessTiffFormat();
    bool TryImageFromMemoryStream(const void* data, size_t len, wxBitmapType type = wxBITMAP_TYPE_PNG);
    bool TryImageFromTempFile(const void* data, size_t len, const wxString& extension);
    #endif
    
    bool TryGetImageFromClipboard(wxBitmap& bitmap);

    // Member variables
    wxString m_lastClipboardContent;
    std::function<void(const wxString&)> m_textCallback;
    std::function<void(const wxBitmap&)> m_imageCallback;
    bool m_initialized;
};

#endif // CLIPBOARD_PROCESSOR_H
