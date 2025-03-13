#pragma once

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/datetime.h>
#include <memory>
#include "clipboard_processor.h"
#include "taskbar.h"

// MainFrame class that listens for clipboard events.
class MainFrame : public wxFrame
{
public:
    MainFrame();
    virtual ~MainFrame();

    // Handle taskbar toggle event
    void OnToggleApp(wxCommandEvent& event);

private:
    // Event handler for clipboard text - includes timestamp
    void OnClipboardText(const wxString& text, const wxDateTime& timestamp);
    
    // Event handler for clipboard images - includes timestamp
    void OnClipboardImage(const wxBitmap& image, const wxDateTime& timestamp);
    
    // Handle window close event
    void OnClose(wxCloseEvent& event);

    // UI elements
    wxTextCtrl* m_textDisplay;
    wxStaticBitmap* m_imageDisplay;
    wxStaticText* m_timestampDisplay;  // Added to show the timestamp
    MyTaskBarIcon* m_taskBarIcon;
    
    // Timestamp tracking
    wxDateTime m_lastProcessedTimestamp;  // Tracks the last processed clipboard content
    
    // Unique pointer to the ClipboardProcessor instance.
    std::unique_ptr<ClipboardProcessor> m_clipboardProcessor;
}; 