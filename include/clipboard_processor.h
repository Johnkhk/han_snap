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
    void Stop();

    // Check clipboard content (can be called manually)
    bool ProcessClipboard();

private:
    // wxTimer notification override
    void Notify() override;

    // Store last clipboard content to detect changes
    wxString m_lastClipboardContent;
    
    // Callback functions for different data types
    std::function<void(const wxString&)> m_textCallback;
    std::function<void(const wxBitmap&)> m_imageCallback;
    
    // Flag to track if we're initialized
    bool m_initialized;
    
    // Helper methods
    wxString GetClipboardText();
    wxBitmap GetClipboardImage();
    bool TryGetImageFromClipboard(wxBitmap& bitmap);
};

#endif // CLIPBOARD_PROCESSOR_H
