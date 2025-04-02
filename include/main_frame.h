#pragma once

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/datetime.h>
#include <memory>
#include "clipboard_processor.h"
#include "taskbar.h"
#include <nlohmann/json.hpp>

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

    // Method to update UI with translation data
    void UpdateUIWithTranslation(const nlohmann::json& response);
    
    // Method to show waiting message
    void ShowWaitingMessage();

    // Method to show error message
    void ShowError(const wxString& message, const wxString& title);

    // Method to show warning message
    void ShowWarning(const wxString& message);

    // Method to show translating message
    void ShowTranslating();

    // UI elements
    wxPanel* m_mainPanel;
    wxPanel* m_translationPanel;
    wxStaticText* m_waitingMessage;
    
    // Translation display elements
    wxStaticText* m_englishMeaningLabel;
    wxTextCtrl* m_englishMeaningText;
    
    // Left panel elements
    wxPanel* m_leftPanel;
    wxStaticText* m_pinyinLabel;
    wxTextCtrl* m_pinyinText;
    wxStaticText* m_originalTextLabel;
    wxTextCtrl* m_originalText;
    
    // Right panel elements
    wxPanel* m_rightPanel;
    wxStaticText* m_jyutpingLabel;
    wxTextCtrl* m_jyutpingText;
    wxStaticText* m_cantoneseLabel;
    wxTextCtrl* m_cantoneseText;
    
    // Image display for OCR
    wxStaticBitmap* m_imageDisplay;
    MyTaskBarIcon* m_taskBarIcon;
    
    // Timestamp tracking
    wxDateTime m_lastProcessedTimestamp;  // Tracks the last processed clipboard content
    
    // Unique pointer to the ClipboardProcessor instance.
    std::unique_ptr<ClipboardProcessor> m_clipboardProcessor;

    // Add this line to your existing member variables
    wxPanel* m_waitingPanel;

    wxButton* m_mandarinPlayButton;
    wxButton* m_cantonesePlayButton;
    std::string m_mandarinAudioData;
    std::string m_cantoneseAudioData;
    std::deque<wxString> m_tempAudioFiles;

    void OnPlayMandarin(wxCommandEvent& event);
    void OnPlayCantonese(wxCommandEvent& event);
    void PlayAudio(const std::string& base64Data, const std::string& prefix);
    void CleanupTempAudioFiles();
}; 