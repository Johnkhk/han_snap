#include "../include/main_frame.h"
#include "../include/ocr.h"
#include "../include/http_client.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json GetLLMResponse(const wxString& text) {
    // Escape any special characters in the recognized text
    wxString escapedText = text;
    escapedText.Replace("\\", "\\\\");
    escapedText.Replace("\"", "\\\"");
    escapedText.Replace("\n", "\\n");
    escapedText.Replace("\r", "\\r");
    escapedText.Replace("\t", "\\t");
    
    wxString jsonStr = "{\"text\": \"" + escapedText + "\"}";
    wxString response = HttpClient::Post("http://localhost:8080/llm", jsonStr);
    
    return json::parse(response);
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "HanSnap - Chinese Translation", wxDefaultPosition, wxSize(800, 600)),
      m_lastProcessedTimestamp(wxDateTime::Now())  // Initialize with current time
{
    // Create main panel
    m_mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create waiting message (initially visible)
    m_waitingMessage = new wxStaticText(m_mainPanel, wxID_ANY, "Waiting for Clipboard Content...", 
                                        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    wxFont waitingFont = m_waitingMessage->GetFont();
    waitingFont.SetPointSize(waitingFont.GetPointSize() + 4);
    waitingFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_waitingMessage->SetFont(waitingFont);
    mainSizer->Add(m_waitingMessage, 1, wxALIGN_CENTER | wxALL, 20);
    
    // Create translation UI elements (initially hidden)
    
    // English meaning (top section)
    wxBoxSizer* englishSizer = new wxBoxSizer(wxVERTICAL);
    m_englishMeaningLabel = new wxStaticText(m_mainPanel, wxID_ANY, "English Meaning:");
    wxFont labelFont = m_englishMeaningLabel->GetFont();
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_englishMeaningLabel->SetFont(labelFont);
    
    m_englishMeaningText = new wxTextCtrl(m_mainPanel, wxID_ANY, "", 
                                         wxDefaultPosition, wxDefaultSize,
                                         wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    wxFont englishFont = m_englishMeaningText->GetFont();
    englishFont.SetPointSize(englishFont.GetPointSize() + 2);
    m_englishMeaningText->SetFont(englishFont);
    
    englishSizer->Add(m_englishMeaningLabel, 0, wxEXPAND | wxALL, 5);
    englishSizer->Add(m_englishMeaningText, 1, wxEXPAND | wxALL, 5);
    
    // Create horizontal sizer for the two-panel layout
    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel (Original text with Pinyin)
    m_leftPanel = new wxPanel(m_mainPanel);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    m_pinyinLabel = new wxStaticText(m_leftPanel, wxID_ANY, "Mandarin Pinyin:");
    m_pinyinLabel->SetFont(labelFont);
    m_pinyinText = new wxTextCtrl(m_leftPanel, wxID_ANY, "", 
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    
    m_originalTextLabel = new wxStaticText(m_leftPanel, wxID_ANY, "Original Text:");
    m_originalTextLabel->SetFont(labelFont);
    m_originalText = new wxTextCtrl(m_leftPanel, wxID_ANY, "", 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    wxFont chineseFont = m_originalText->GetFont();
    chineseFont.SetPointSize(chineseFont.GetPointSize() + 4);
    m_originalText->SetFont(chineseFont);
    
    leftSizer->Add(m_pinyinLabel, 0, wxEXPAND | wxALL, 5);
    leftSizer->Add(m_pinyinText, 1, wxEXPAND | wxALL, 5);
    leftSizer->Add(m_originalTextLabel, 0, wxEXPAND | wxALL, 5);
    leftSizer->Add(m_originalText, 2, wxEXPAND | wxALL, 5);
    m_leftPanel->SetSizer(leftSizer);
    
    // Right panel (Cantonese with Jyutping)
    m_rightPanel = new wxPanel(m_mainPanel);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    m_jyutpingLabel = new wxStaticText(m_rightPanel, wxID_ANY, "Cantonese Jyutping:");
    m_jyutpingLabel->SetFont(labelFont);
    m_jyutpingText = new wxTextCtrl(m_rightPanel, wxID_ANY, "", 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    
    m_cantoneseLabel = new wxStaticText(m_rightPanel, wxID_ANY, "Cantonese Equivalent:");
    m_cantoneseLabel->SetFont(labelFont);
    m_cantoneseText = new wxTextCtrl(m_rightPanel, wxID_ANY, "", 
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    m_cantoneseText->SetFont(chineseFont);
    
    rightSizer->Add(m_jyutpingLabel, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(m_jyutpingText, 1, wxEXPAND | wxALL, 5);
    rightSizer->Add(m_cantoneseLabel, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(m_cantoneseText, 2, wxEXPAND | wxALL, 5);
    m_rightPanel->SetSizer(rightSizer);
    
    // Add both panels to the horizontal sizer
    horizontalSizer->Add(m_leftPanel, 1, wxEXPAND | wxALL, 5);
    horizontalSizer->Add(m_rightPanel, 1, wxEXPAND | wxALL, 5);
    
    // Image display for OCR preview (hidden by default)
    m_imageDisplay = new wxStaticBitmap(m_mainPanel, wxID_ANY, wxNullBitmap);
    
    // Add components to main sizer
    mainSizer->Add(englishSizer, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(horizontalSizer, 2, wxEXPAND | wxALL, 5);
    mainSizer->Add(m_imageDisplay, 0, wxEXPAND | wxALL, 5);
    
    // Initially hide the translation UI
    m_englishMeaningLabel->Hide();
    m_englishMeaningText->Hide();
    m_leftPanel->Hide();
    m_rightPanel->Hide();
    m_imageDisplay->Hide();
    
    m_mainPanel->SetSizer(mainSizer);
    
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
        m_taskBarIcon->SetIcon(icon, "HanSnap - Chinese Translation");
    }
    
    // Initialize OCR for Chinese
    if (!OcrEngine::IsInitialized()) {
        if (!OcrEngine::Initialize("chi_sim+chi_tra")) {
            wxLogError("Failed to initialize OCR engine for Chinese");
        }
    }
    
    // Start with waiting message
    ShowWaitingMessage();
}

MainFrame::~MainFrame()
{
    // Clean up
    if (m_taskBarIcon) {
        m_taskBarIcon->RemoveIcon();
        delete m_taskBarIcon;
    }
}

void MainFrame::ShowWaitingMessage()
{
    // Show waiting message and hide translation UI
    m_waitingMessage->Show();
    
    m_englishMeaningLabel->Hide();
    m_englishMeaningText->Hide();
    m_leftPanel->Hide();
    m_rightPanel->Hide();
    m_imageDisplay->Hide();
    
    m_mainPanel->Layout();
}

void MainFrame::UpdateUIWithTranslation(const json& response)
{
    // Hide waiting message
    m_waitingMessage->Hide();
    
    // Show translation UI
    m_englishMeaningLabel->Show();
    m_englishMeaningText->Show();
    m_leftPanel->Show();
    m_rightPanel->Show();
    
    // Extract data from JSON response
    if (response.contains("translation") && 
        response["translation"].contains("text") && 
        response["translation"].contains("result")) {
        
        // Get original text
        wxString originalText = wxString::FromUTF8(response["translation"]["text"].get<std::string>());
        m_originalText->SetValue(originalText);
        
        // Get translation data
        const auto& result = response["translation"]["result"];
        
        if (result.contains("meaning_english")) {
            wxString english = wxString::FromUTF8(result["meaning_english"].get<std::string>());
            m_englishMeaningText->SetValue(english);
        }
        
        if (result.contains("pinyin_mandarin")) {
            wxString pinyin = wxString::FromUTF8(result["pinyin_mandarin"].get<std::string>());
            m_pinyinText->SetValue(pinyin);
        }
        
        if (result.contains("jyutping_cantonese")) {
            wxString jyutping = wxString::FromUTF8(result["jyutping_cantonese"].get<std::string>());
            m_jyutpingText->SetValue(jyutping);
        }
        
        if (result.contains("equivalent_cantonese")) {
            wxString cantonese = wxString::FromUTF8(result["equivalent_cantonese"].get<std::string>());
            m_cantoneseText->SetValue(cantonese);
        }
    } else {
        // Handle error in response
        m_englishMeaningText->SetValue("Error processing translation.");
        
        if (response.contains("error")) {
            wxString errorMsg = wxString::FromUTF8(response["error"].get<std::string>());
            m_englishMeaningText->SetValue("Error: " + errorMsg);
        }
    }
    
    // Update layout
    m_mainPanel->Layout();
}

void MainFrame::OnToggleApp(wxCommandEvent& event)
{
    // Check if we're enabling or disabling the app
    bool isEnabled = (event.GetInt() == 1);
    
    if (isEnabled) {
        // We're turning the app back on
        // Update 
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
    json response = GetLLMResponse(text);
    std::cout << "Response: " << response << std::endl;
    UpdateUIWithTranslation(response);
}

void MainFrame::OnClipboardImage(const wxBitmap& image, const wxDateTime& timestamp)
{
    // Show the image preview
    m_imageDisplay->SetBitmap(image);
    m_imageDisplay->Show();
    
    // Perform OCR on the image
    if (OcrEngine::IsInitialized()) {
        wxString recognizedText = OcrEngine::ExtractTextFromBitmap(image);
        
        if (!recognizedText.IsEmpty()) {
            json response = GetLLMResponse(recognizedText);
            std::cout << "Response: " << response << std::endl;
            UpdateUIWithTranslation(response);
        } else {
            // No text recognized
            ShowWaitingMessage();
            wxMessageBox("No text was recognized in the image.", "OCR Result", 
                        wxOK | wxICON_INFORMATION);
        }
    }
    
    // Update layout
    m_mainPanel->Layout();
} 
