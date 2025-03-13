#ifndef CLIPBOARD_PROCESSOR_H
#define CLIPBOARD_PROCESSOR_H

#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/timer.h>
#include <wx/datetime.h>
#include <functional>
#include <string>
#include <memory>

// Forward declaration
struct ClipboardData;

class ClipboardProcessor : public wxTimer
{
public:
    ClipboardProcessor();
    virtual ~ClipboardProcessor();

    // Initialize the processor with optional callbacks for different data types
    // Now including timestamps in callbacks
    bool Initialize(std::function<void(const wxString&, const wxDateTime&)> textCallback = nullptr,
                   std::function<void(const wxBitmap&, const wxDateTime&)> imageCallback = nullptr);
    
    // Start monitoring the clipboard
    bool Start(int checkIntervalMs = 500);
    
    // Stop monitoring the clipboard
    void Stop() override;

    // Check clipboard content (can be called manually)
    bool ProcessClipboard();
    
    // Timestamp accessors
    wxDateTime GetCurrentTimestamp() const;
    std::shared_ptr<ClipboardData> GetCurrentClipboardData() const;
    wxString GetTimestampString() const;

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
    std::function<void(const wxString&, const wxDateTime&)> m_textCallback;
    std::function<void(const wxBitmap&, const wxDateTime&)> m_imageCallback;
    bool m_initialized;
    std::shared_ptr<ClipboardData> m_clipboardData;
};

#endif // CLIPBOARD_PROCESSOR_H
