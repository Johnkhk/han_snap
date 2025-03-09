#pragma once

#include <wx/wx.h>
#include <wx/filename.h>
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
    // Event handler for clipboard text
    void OnClipboardText(const wxString& text);
    
    // Event handler for clipboard images
    void OnClipboardImage(const wxBitmap& image);
    
    // Handle window close event
    void OnClose(wxCloseEvent& event);

    // UI elements
    wxTextCtrl* m_textDisplay;
    wxStaticBitmap* m_imageDisplay;
    MyTaskBarIcon* m_taskBarIcon;
    
    // Unique pointer to the ClipboardProcessor instance.
    std::unique_ptr<ClipboardProcessor> m_clipboardProcessor;
}; 