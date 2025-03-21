#include "../include/main_frame.h"
#include "../include/ocr.h"
#include "../include/http_client.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const int MAX_TEXT_LENGTH = 500;

bool TooMuchText(const wxString& text) {
    if (text.Length() > MAX_TEXT_LENGTH) {
        return true;
    }
    return false;
}
bool IsServerOnline() {
    try {
        wxString response = HttpClient::Get("http://localhost:8080/health");
        return response.Contains("ok") || response.Contains("OK");
    } catch (...) {
        return false;
    }
}

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

    // check if response is ok
    if (response.Contains("error")) {
        std::cout << "Error: " << response << std::endl;
        return json::parse("{\"error\": \"Failed to get response from LLM\"}");
    }
    
    return json::parse(response);
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "HanSnap - Chinese Translation", wxDefaultPosition, wxSize(800, 600)),
      m_lastProcessedTimestamp(wxDateTime::Now())  // Initialize with current time
{


    // Set up status bar
    CreateStatusBar();
    SetStatusText("Waiting for clipboard content...");
    
    // Create main panel with light blue-gray background for better contrast
    m_mainPanel = new wxPanel(this);
    m_mainPanel->SetBackgroundColour(wxColour(240, 245, 250));
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create waiting message panel (initially visible)
    m_waitingPanel = new wxPanel(m_mainPanel);
    wxBoxSizer* waitingSizer = new wxBoxSizer(wxVERTICAL);
    
    // Simple version without the icon for now
    m_waitingMessage = new wxStaticText(m_waitingPanel, wxID_ANY, "Waiting for Clipboard Content...", 
                                      wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    wxFont waitingFont = m_waitingMessage->GetFont();
    waitingFont.SetPointSize(waitingFont.GetPointSize() + 4);
    waitingFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_waitingMessage->SetFont(waitingFont);
    m_waitingMessage->SetForegroundColour(wxColour(30, 30, 120));  // Dark blue text
    
    waitingSizer->Add(m_waitingMessage, 0, wxALIGN_CENTER | wxALL, 20);
    
    // First check if server is online
    if (!IsServerOnline()) {
        wxMessageBox("Server is offline. Please start the server and try again.", "Error", wxOK | wxICON_ERROR);
        exit(1);
    }

    // Add a helpful instruction message
    wxStaticText* instructionText = new wxStaticText(m_waitingPanel, wxID_ANY, 
        "Copy Chinese text or an image containing Chinese text to translate it automatically.",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    instructionText->SetForegroundColour(wxColour(50, 50, 100));  // Dark blue-gray text
    waitingSizer->Add(instructionText, 0, wxALIGN_CENTER | wxALL, 10);
    
    m_waitingPanel->SetSizer(waitingSizer);
    mainSizer->Add(m_waitingPanel, 1, wxEXPAND | wxALL, 5);
    
    // Create translation panel (initially hidden)
    m_translationPanel = new wxPanel(m_mainPanel);
    wxBoxSizer* translationSizer = new wxBoxSizer(wxVERTICAL);
    
    // English meaning (top section) with styled border
    wxStaticBoxSizer* englishBox = new wxStaticBoxSizer(wxVERTICAL, m_translationPanel, "");
    wxFont boxFont = englishBox->GetStaticBox()->GetFont();
    boxFont.SetWeight(wxFONTWEIGHT_BOLD);
    englishBox->GetStaticBox()->SetFont(boxFont);
    
    // Add "Meaning" label
    wxStaticText* meaningLabel = new wxStaticText(englishBox->GetStaticBox(), wxID_ANY, "Meaning");
    wxFont labelFont = meaningLabel->GetFont();
    labelFont.SetPointSize(labelFont.GetPointSize() + 1);
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    meaningLabel->SetFont(labelFont);
    meaningLabel->SetForegroundColour(wxColour(30, 30, 120));  // Dark blue text
    
    englishBox->Add(meaningLabel, 0, wxEXPAND | wxALL, 5);
    
    m_englishMeaningText = new wxTextCtrl(englishBox->GetStaticBox(), wxID_ANY, "", 
                                         wxDefaultPosition, wxSize(-1, 100),  
                                         wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    wxFont englishFont = m_englishMeaningText->GetFont();
    englishFont.SetPointSize(englishFont.GetPointSize() + 2);
    m_englishMeaningText->SetFont(englishFont);
    m_englishMeaningText->SetForegroundColour(wxColour(0, 0, 0));
    m_englishMeaningText->SetBackgroundColour(wxColour(250, 250, 255));
    
    englishBox->Add(m_englishMeaningText, 1, wxEXPAND | wxALL, 5);
    
    // Create horizontal sizer for the two-panel layout
    wxBoxSizer* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel (Original text with Pinyin)
    wxStaticBoxSizer* leftBox = new wxStaticBoxSizer(wxVERTICAL, m_translationPanel, "");
    leftBox->GetStaticBox()->SetFont(boxFont);
    
    // Add "Mandarin" label
    wxStaticText* mandarinLabel = new wxStaticText(leftBox->GetStaticBox(), wxID_ANY, "Mandarin");
    mandarinLabel->SetFont(labelFont);
    mandarinLabel->SetForegroundColour(wxColour(30, 30, 120));  // Dark blue text
    leftBox->Add(mandarinLabel, 0, wxEXPAND | wxALL, 5);
    
    // Create pinyin text control with scrolling
    m_pinyinText = new wxTextCtrl(leftBox->GetStaticBox(), wxID_ANY, "", 
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    m_pinyinText->SetForegroundColour(wxColour(0, 0, 0));  // Black text
    m_pinyinText->SetBackgroundColour(wxColour(245, 245, 250));  // Very light purple background
    wxFont pinyinFont = m_pinyinText->GetFont();
    pinyinFont.SetPointSize(pinyinFont.GetPointSize() + 1);
    m_pinyinText->SetFont(pinyinFont);
    
    // Create original text control with scrolling
    m_originalText = new wxTextCtrl(leftBox->GetStaticBox(), wxID_ANY, "", 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    wxFont chineseFont = m_originalText->GetFont();
    chineseFont.SetPointSize(chineseFont.GetPointSize() + 4);
    m_originalText->SetFont(chineseFont);
    m_originalText->SetForegroundColour(wxColour(0, 0, 0));  // Black text
    m_originalText->SetBackgroundColour(wxColour(245, 245, 250));  // Very light purple background
    
    // Add controls to left box (pinyin directly above original text)
    leftBox->Add(m_pinyinText, 1, wxEXPAND | wxALL, 5);
    
    // Create and style the Chinese label
    wxStaticText* chineseLabel = new wxStaticText(leftBox->GetStaticBox(), wxID_ANY, "Chinese:");
    chineseLabel->SetForegroundColour(wxColour(30, 30, 120));  // Dark blue text to match other labels
    chineseLabel->SetFont(labelFont);  // Use the same font as other labels
    leftBox->Add(chineseLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
    leftBox->Add(m_originalText, 2, wxEXPAND | wxALL, 5);
    
    // Right panel (Cantonese with Jyutping)
    wxStaticBoxSizer* rightBox = new wxStaticBoxSizer(wxVERTICAL, m_translationPanel, "");
    rightBox->GetStaticBox()->SetFont(boxFont);
    
    // Add "Cantonese" label
    wxStaticText* cantoneseLabel = new wxStaticText(rightBox->GetStaticBox(), wxID_ANY, "Cantonese");
    cantoneseLabel->SetForegroundColour(wxColour(30, 30, 120));  // Dark blue text
    cantoneseLabel->SetFont(labelFont);
    rightBox->Add(cantoneseLabel, 0, wxEXPAND | wxALL, 5);
    
    // Create jyutping text control with scrolling
    m_jyutpingText = new wxTextCtrl(rightBox->GetStaticBox(), wxID_ANY, "", 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    m_jyutpingText->SetForegroundColour(wxColour(0, 0, 0));  // Black text
    m_jyutpingText->SetBackgroundColour(wxColour(245, 245, 250));  // Very light purple background
    m_jyutpingText->SetFont(pinyinFont);  // Use same font as pinyin
    
    // Create cantonese text control with scrolling
    m_cantoneseText = new wxTextCtrl(rightBox->GetStaticBox(), wxID_ANY, "", 
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL);
    m_cantoneseText->SetForegroundColour(wxColour(0, 0, 0));  // Black text
    m_cantoneseText->SetBackgroundColour(wxColour(245, 245, 250));  // Very light purple background
    m_cantoneseText->SetFont(chineseFont);
    
    // Add controls to right box (jyutping directly above cantonese text)
    rightBox->Add(m_jyutpingText, 1, wxEXPAND | wxALL, 5);
    
    // Create and style the Cantonese text label
    wxStaticText* cantoneseTextLabel = new wxStaticText(rightBox->GetStaticBox(), wxID_ANY, "Cantonese:");
    cantoneseTextLabel->SetForegroundColour(wxColour(30, 30, 120));  // Dark blue text to match other labels
    cantoneseTextLabel->SetFont(labelFont);  // Use the same font as other labels
    rightBox->Add(cantoneseTextLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
    
    rightBox->Add(m_cantoneseText, 2, wxEXPAND | wxALL, 5);
    
    // Add both panels to the horizontal sizer
    horizontalSizer->Add(leftBox, 1, wxEXPAND | wxALL, 5);
    horizontalSizer->Add(rightBox, 1, wxEXPAND | wxALL, 5);
    
    // Image display for OCR preview (hidden by default)
    wxStaticBoxSizer* imageBox = new wxStaticBoxSizer(wxVERTICAL, m_translationPanel, "OCR Image");
    imageBox->GetStaticBox()->SetFont(boxFont);
    
    m_imageDisplay = new wxStaticBitmap(imageBox->GetStaticBox(), wxID_ANY, wxNullBitmap);
    imageBox->Add(m_imageDisplay, 1, wxALIGN_CENTER | wxALL, 10);
    
    // Add components to translation sizer
    translationSizer->Add(englishBox, 1, wxEXPAND | wxALL, 10);
    translationSizer->Add(horizontalSizer, 2, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    translationSizer->Add(imageBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    m_translationPanel->SetSizer(translationSizer);
    mainSizer->Add(m_translationPanel, 1, wxEXPAND);
    
    // Initially hide the translation UI
    m_translationPanel->Hide();
    
    m_mainPanel->SetSizer(mainSizer);
    
    // Create taskbar icon
    m_taskBarIcon = new MyTaskBarIcon(this);
    
    // Bind events
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnToggleApp, this, MyTaskBarIcon::ID_TOGGLE_APP);
    
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
    
    
    // Start with waiting message
    ShowWaitingMessage();
}

MainFrame::~MainFrame()
{
    // Clean up OCR engine to prevent memory leaks
    OcrEngine::Cleanup();
    
    // Clean up taskbar icon
    if (m_taskBarIcon) {
        m_taskBarIcon->RemoveIcon();
        delete m_taskBarIcon;
    }
}

void MainFrame::ShowWaitingMessage()
{
    // Show waiting message and hide translation UI
    m_waitingPanel->Show();
    m_translationPanel->Hide();
    
    // Update status bar
    SetStatusText("Waiting for clipboard content...");
    
    m_mainPanel->Layout();
}

void MainFrame::UpdateUIWithTranslation(const json& response)
{
    // Hide waiting message and show translation UI
    m_waitingPanel->Hide();
    m_translationPanel->Show();
    
    // Update status bar
    SetStatusText("Translation completed");
    
    // Freeze window to prevent flickering during updates
    Freeze();
    
    try {
        // Extract data from JSON response
        if (!response.contains("translation") || 
            !response["translation"].contains("text") || 
            !response["translation"].contains("result")) {
            m_englishMeaningText->ChangeValue("Error: Invalid response format");
            Thaw();
            return;
        }
        
        const auto& result = response["translation"]["result"];
        
        // Update all text controls in a batch operation
        // Original text
        if (response["translation"].contains("text")) {
            std::string textStr = response["translation"]["text"].get<std::string>();
            wxString originalText = wxString::FromUTF8(textStr);
            m_originalText->ChangeValue(originalText);
        }
        
        // English meaning
        if (result.contains("meaning_english")) {
            std::string engStr = result["meaning_english"].get<std::string>();
            wxString english = wxString::FromUTF8(engStr);
            m_englishMeaningText->ChangeValue(english);
        }
        
        // Pinyin
        if (result.contains("pinyin_mandarin")) {
            std::string pinyinStr = result["pinyin_mandarin"].get<std::string>();
            wxString pinyin = wxString::FromUTF8(pinyinStr);
            m_pinyinText->ChangeValue(pinyin);
        }
        
        // Jyutping
        if (result.contains("jyutping_cantonese")) {
            std::string jyutpingStr = result["jyutping_cantonese"].get<std::string>();
            wxString jyutping = wxString::FromUTF8(jyutpingStr);
            m_jyutpingText->ChangeValue(jyutping);
        }
        
        // Cantonese
        if (result.contains("equivalent_cantonese")) {
            std::string cantoneseStr = result["equivalent_cantonese"].get<std::string>();
            wxString cantonese = wxString::FromUTF8(cantoneseStr);
            m_cantoneseText->ChangeValue(cantonese);
        }
        
        // Scroll all text controls to top
        m_englishMeaningText->ShowPosition(0);
        m_originalText->ShowPosition(0);
        m_pinyinText->ShowPosition(0);
        m_jyutpingText->ShowPosition(0);
        m_cantoneseText->ShowPosition(0);
        
    } catch (const std::exception& e) {
        // Handle any JSON parsing exceptions
        wxString errorMsg = wxString::Format("Error processing response: %s", e.what());
        wxLogError(errorMsg);
        m_englishMeaningText->ChangeValue(errorMsg);
    } catch (...) {
        // Catch any other exceptions
        wxLogError("Unknown error occurred while updating UI");
        m_englishMeaningText->ChangeValue("Unknown error occurred while updating UI");
    }
    
    // Thaw and update layout
    Thaw();
    m_mainPanel->Layout();
}

void MainFrame::OnToggleApp(wxCommandEvent& event)
{
    // Check if we're enabling or disabling the app
    bool isEnabled = (event.GetInt() == 1);
    
    if (isEnabled) {
        // We're turning the app back on
        // Update timestamp to prevent processing old clipboard content
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
    if (TooMuchText(text)) {
        ShowError("Exceeded the maximum text length of " + wxString::Format("%d", MAX_TEXT_LENGTH) + " characters.", "Error");
        return;
    }
    std::cout << "GETTING LLM RESPONSE" << std::endl;
    
    
    // while wating for LLM response, change the waiting message to "Translating..."
    ShowTranslating();
    
    json response = GetLLMResponse(text);
    
    std::cout << "Response: " << response << std::endl;
    UpdateUIWithTranslation(response);
    RequestUserAttention(wxUSER_ATTENTION_INFO);
}

void MainFrame::OnClipboardImage(const wxBitmap& image, const wxDateTime& timestamp)
{
    // Show the image preview
    // m_imageDisplay->SetBitmap(image);
    // m_imageDisplay->Show();
    // Initialize OCR for Chinese
    if (!OcrEngine::IsInitialized()) {
        if (!OcrEngine::Initialize("chi_sim+chi_tra")) {
            wxLogError("Failed to initialize OCR engine for Chinese");
        }
    }
    
    // Perform OCR on the image
    if (OcrEngine::IsInitialized()) {
        wxString recognizedText = OcrEngine::ExtractTextFromBitmap(image);
        if (TooMuchText(recognizedText)) {
            ShowError("Exceeded the maximum text length of " + wxString::Format("%d", MAX_TEXT_LENGTH) + " characters.", "Error");
            return;
        }
        if (!recognizedText.IsEmpty()) {
            ShowTranslating();
            json response = GetLLMResponse(recognizedText);
            std::cout << "Response: " << response << std::endl;
            UpdateUIWithTranslation(response);
            RequestUserAttention(wxUSER_ATTENTION_INFO);
        } else {
            // No text recognized
            ShowWaitingMessage();
            wxMessageBox("No text was recognized in the image.", "OCR Result", 
                        wxOK | wxICON_INFORMATION);
        }
    }
    
    // Update layout
    // m_mainPanel->Layout();
} 



void MainFrame::ShowError(const wxString& message, const wxString& title) {
    wxLogError("%s", message);
}

void MainFrame::ShowWarning(const wxString& message) {
    wxLogWarning("%s", message);
}

void MainFrame::ShowTranslating() {
    m_waitingMessage->SetLabel("Translating...");
    m_mainPanel->Layout();
    wxYield();
}