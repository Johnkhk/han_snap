#include "../include/clipboard_processor.h"
#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/taskbar.h>
#include <memory>
#include "../include/taskbar.h"

// MainFrame class that listens for clipboard events.
class MainFrame : public wxFrame
{
public:
    MainFrame()
        : wxFrame(nullptr, wxID_ANY, "Clipboard Monitor Test")
    {
        // First create the status bar
        CreateStatusBar();
        SetStatusText("Monitoring clipboard...");
        
        // Create a text control to display clipboard content
        m_textDisplay = new wxTextCtrl(this, wxID_ANY, wxEmptyString, 
                                     wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_READONLY);
        
        // Create a bitmap display for images
        m_imageDisplay = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
        
        // Create layout
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_textDisplay, 1, wxEXPAND | wxALL, 5);
        sizer->Add(m_imageDisplay, 2, wxEXPAND | wxALL, 5);
        SetSizer(sizer);
        
        // Initialize and start clipboard processor
        m_clipboardProcessor = std::make_unique<ClipboardProcessor>();
        m_clipboardProcessor->Initialize(
            [this](const wxString& text) { OnClipboardText(text); },
            [this](const wxBitmap& image) { OnClipboardImage(image); }
        );
        m_clipboardProcessor->Start();
        
        // Create and setup taskbar icon
        m_taskBarIcon = new MyTaskBarIcon(this);
        
        // Load an icon file
        wxIcon icon;
        if (icon.LoadFile("../assets/images/app_icon.png", wxBITMAP_TYPE_PNG)) {
            m_taskBarIcon->SetIcon(icon, "Clipboard Monitor");
        }
        
        // Connect close event to hide instead of closing
        Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
        
        SetSize(600, 400);
    }
    
    ~MainFrame()
    {
        // Clean up taskbar icon
        if (m_taskBarIcon)
        {
            delete m_taskBarIcon;
        }
    }

private:
    // Event handler for clipboard text
    void OnClipboardText(const wxString& text)
    {
        m_textDisplay->SetValue(text);
        SetStatusText("Clipboard contains text (" + wxString::Format("%zu", text.length()) + " characters)");
        
        // Hide the image display when showing text
        m_imageDisplay->SetBitmap(wxNullBitmap);
    }
    
    // Event handler for clipboard images
    void OnClipboardImage(const wxBitmap& image)
    {
        wxLogDebug("Clipboard image: %d x %d", image.GetWidth(), image.GetHeight());
        m_imageDisplay->SetBitmap(image);
        SetStatusText(wxString::Format("Clipboard contains image (%d x %d)", 
                                     image.GetWidth(), image.GetHeight()));
        
        // Clear text display when showing image
        m_textDisplay->Clear();
        
        // Resize the image display if needed
        Layout();
    }
    
    // Handle window close event
    void OnClose(wxCloseEvent& event)
    {
        if (event.CanVeto())
        {
            // Hide the window instead of closing when user clicks the X
            Hide();
            event.Veto();
        }
        else
        {
            // Actually destroy the frame if we can't veto
            Destroy();
        }
    }

    // UI elements
    wxTextCtrl* m_textDisplay;
    wxStaticBitmap* m_imageDisplay;
    MyTaskBarIcon* m_taskBarIcon;
    
    // Unique pointer to the ClipboardProcessor instance.
    std::unique_ptr<ClipboardProcessor> m_clipboardProcessor;
};

// The main application class.
class MyApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

// Macro that defines the main() entry point.
wxIMPLEMENT_APP(MyApp);
