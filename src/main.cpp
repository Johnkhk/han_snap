#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/image.h>
#include "../include/screenshot.h"
#include "../include/hotkey.h"

// Define event IDs at global scope
enum {
    ID_SCREENSHOT = 1
};

// Main application frame
class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);

private:
    void OnScreenshot(wxCommandEvent& event);
    void ProcessScreenshot(wxBitmap screenshot); // Placeholder for future implementation

    wxPanel* m_panel;
    wxStaticBitmap* m_imageCtrl;
    wxTextCtrl* m_pinyinText;
    wxTextCtrl* m_meaningText;

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_SCREENSHOT, MainFrame::OnScreenshot)
wxEND_EVENT_TABLE()

// Main application class
class HanSnapApp : public wxApp {
public:
    virtual bool OnInit();
    MainFrame* frame = nullptr;
};

wxIMPLEMENT_APP(HanSnapApp);

bool HanSnapApp::OnInit() {
    // Initialize image handlers
    wxImage::AddHandler(new wxPNGHandler());
    wxImage::AddHandler(new wxJPEGHandler());

    // Create main frame and show it
    frame = new MainFrame("Han Snap - Chinese Character Recognition");
    frame->Show(true);

    return true;
}

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(400, 300))
{
    m_panel = new wxPanel(this, wxID_ANY);

    // Create a button called "Take Screenshot"
    wxButton* screenshotButton = new wxButton(
        m_panel, 
        ID_SCREENSHOT, 
        wxT("Take Screenshot"), 
        wxPoint(20, 20), 
        wxSize(150, 40)
    );
}

void MainFrame::OnScreenshot(wxCommandEvent& event) {
    // For now, simply show a message box to indicate the button was clicked.
    // wxMessageBox("Take Screenshot button clicked!", "Info", wxOK | wxICON_INFORMATION);
    LaunchScreenshotTool([this](wxBitmap screenshot) {
        // Process the screenshot bitmap. For now, we just call ProcessScreenshot.
        ProcessScreenshot(screenshot);
    });
}

void MainFrame::ProcessScreenshot(wxBitmap screenshot) {
    // Future implementation: Process the screenshot.

    // For now save the screenshot to a file
    screenshot.SaveFile("screenshot.png", wxBITMAP_TYPE_PNG);
}
