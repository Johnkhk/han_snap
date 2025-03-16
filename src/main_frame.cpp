#include "../include/main_frame.h"
#include "../include/ocr.h"

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Clipboard Monitor", wxDefaultPosition, wxSize(800, 600)),
      m_lastProcessedTimestamp(wxDateTime::Now())  // Initialize with current time
{
    // Create UI components
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add timestamp display
    m_timestampDisplay = new wxStaticText(panel, wxID_ANY, "No clipboard content yet");
    mainSizer->Add(m_timestampDisplay, 0, wxEXPAND | wxALL, 5);
    
    // Text display
    m_textDisplay = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 
                                  wxTE_MULTILINE | wxTE_READONLY);
    mainSizer->Add(m_textDisplay, 1, wxEXPAND | wxALL, 5);
    
    // Image display
    m_imageDisplay = new wxStaticBitmap(panel, wxID_ANY, wxNullBitmap);
    mainSizer->Add(m_imageDisplay, 2, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
    
    // Create taskbar icon
    m_taskBarIcon = new MyTaskBarIcon(this);
    
    // Bind events
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
    
    // Initialize clipboard processor with callbacks that include timestamps
    m_clipboardProcessor = std::make_unique<ClipboardProcessor>();
    m_clipboardProcessor->Initialize(
        [this](const wxString& text, const wxDateTime& timestamp) { 
            // Only process if timestamp is newer than last processed timestamp
            if (timestamp > m_lastProcessedTimestamp) {
                OnClipboardText(text, timestamp);
                m_lastProcessedTimestamp = timestamp;
            }
        },
        [this](const wxBitmap& image, const wxDateTime& timestamp) {
            // Only process if timestamp is newer than last processed timestamp
            if (timestamp > m_lastProcessedTimestamp) {
                OnClipboardImage(image, timestamp);
                m_lastProcessedTimestamp = timestamp;
            }
        }
    );
    
    // Start monitoring clipboard
    m_clipboardProcessor->Start();
    
    // Center the window
    Centre();

        // Create and setup taskbar icon
    m_taskBarIcon = new MyTaskBarIcon(this);
    
    // Load an icon file
    wxIcon icon;
    if (icon.LoadFile("../assets/images/app_icon.png", wxBITMAP_TYPE_PNG)) {
        m_taskBarIcon->SetIcon(icon, "Clipboard Monitor");
    }
    
    // For Simplified Chinese
    // if (!OcrEngine::IsInitialized()) {
    //     if (!OcrEngine::Initialize("chi_sim")) {
    //         wxLogError("Failed to initialize OCR engine for Simplified Chinese");
    //     } else {
    //         // wxLogMessage("OCR engine initialized for Simplified Chinese");
    //     }
    // }
    
    // For Traditional Chinese
    // if (!OcrEngine::IsInitialized()) {
    //     if (!OcrEngine::Initialize("chi_tra")) {
    //         wxLogError("Failed to initialize OCR engine for Traditional Chinese");
    //     } else {
    //         wxLogMessage("OCR engine initialized for Traditional Chinese");
    //     }
    // }

    if (!OcrEngine::IsInitialized()) {
    // Both Simplified (chi_sim) and Traditional (chi_tra) Chinese
    if (!OcrEngine::Initialize("chi_sim+chi_tra")) {
        wxLogError("Failed to initialize OCR engine for Simplified and Traditional Chinese");
    } else {
        // wxLogMessage("OCR engine initialized for Simplified and Traditional Chinese");
    }
}
}

MainFrame::~MainFrame()
{
    // Clean up
    if (m_taskBarIcon) {
        m_taskBarIcon->RemoveIcon();
        delete m_taskBarIcon;
    }
}

void MainFrame::OnToggleApp(wxCommandEvent& event)
{
    // Check if we're enabling or disabling the app
    bool isEnabled = (event.GetInt() == 1);
    
    if (isEnabled) {
        // We're turning the app back on
        // Update the timestamp to current time to avoid processing old content
        m_lastProcessedTimestamp = wxDateTime::Now();
        
        // Start clipboard monitoring
        m_clipboardProcessor->Start();
        
        // Show the window
        Show();
        Raise();
    } else {
        // We're turning the app off
        // Stop clipboard monitoring
        m_clipboardProcessor->Stop();
        
        // Hide the window
        Hide();
    }
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    // Hide instead of close if the taskbar icon is present
    if (m_taskBarIcon && m_taskBarIcon->IsIconInstalled()) {
        Hide();
    } else {
        // Actually close
        Destroy();
    }
}

void MainFrame::OnClipboardText(const wxString& text, const wxDateTime& timestamp)
{
    // Update the timestamp display
    wxString timeStr = timestamp.Format("%Y-%m-%d %H:%M:%S");
    m_timestampDisplay->SetLabel("Text copied at: " + timeStr);
    
    // Update text display
    m_textDisplay->SetValue(text);
    m_textDisplay->Show();
    
    // Hide image display
    m_imageDisplay->SetBitmap(wxNullBitmap);
    m_imageDisplay->Hide();
    
    // Update layout
    Layout();
}

void MainFrame::OnClipboardImage(const wxBitmap& image, const wxDateTime& timestamp)
{
    // Update the timestamp display
    wxString timeStr = timestamp.Format("%Y-%m-%d %H:%M:%S");
    m_timestampDisplay->SetLabel("Image copied at: " + timeStr);
    
    // Hide text display
    m_textDisplay->Clear();
    m_textDisplay->Hide();
    
    // Update image display
    m_imageDisplay->SetBitmap(image);
    m_imageDisplay->Show();
    
    // Perform OCR on the image
    if (!OcrEngine::IsInitialized()) {
        if (!OcrEngine::Initialize("chi_sim")) {
            wxLogError("Failed to initialize OCR engine for Simplified Chinese");
        } else {
            wxLogMessage("OCR engine initialized for Simplified Chinese");
        }
    }
    
    if (OcrEngine::IsInitialized()) {
        wxString recognizedText = OcrEngine::ExtractTextFromBitmap(image);
        wxLogDebug("OCR Text: %s", recognizedText);
    }
    
    // Update layout
    Layout();
} 