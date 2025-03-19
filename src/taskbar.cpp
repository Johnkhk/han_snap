#include "../include/taskbar.h"

// Event table for the taskbar icon
wxBEGIN_EVENT_TABLE(MyTaskBarIcon, wxTaskBarIcon)
    EVT_MENU(wxID_EXIT, MyTaskBarIcon::OnQuit)
    EVT_MENU(MyTaskBarIcon::ID_SHOW_HIDE, MyTaskBarIcon::OnShowHide)
    EVT_MENU(MyTaskBarIcon::ID_TOGGLE_APP, MyTaskBarIcon::OnToggleApp)
    EVT_TASKBAR_LEFT_DOWN(MyTaskBarIcon::OnLeftClick)
wxEND_EVENT_TABLE()

MyTaskBarIcon::MyTaskBarIcon(wxFrame* parentFrame) 
    : m_parentFrame(parentFrame), m_appEnabled(true)
{
}

MyTaskBarIcon::~MyTaskBarIcon()
{
    RemoveIcon();
}

wxMenu* MyTaskBarIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu;
    
    // Create a more visual toggle item with colored indicator
    wxString statusText;
    if (m_appEnabled) {
        statusText = "●  App: Active";  // Green dot (displayed as ● in the menu)
    } else {
        statusText = "○  App: Disabled";  // Empty circle (displayed as ○ in the menu)
    }
    
    menu->Append(ID_TOGGLE_APP, statusText);
    menu->AppendSeparator();
    menu->Append(ID_SHOW_HIDE, m_parentFrame->IsShown() ? "Hide Window" : "Show Window");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, "Quit");
    
    return menu;
}

void MyTaskBarIcon::SetAppEnabled(bool enabled)
{
    m_appEnabled = enabled;
    // You might want to update the icon to reflect the state
    // For example: SetIcon(enabled ? enabledIcon : disabledIcon, "Clipboard Monitor");
}

void MyTaskBarIcon::OnShowHide(wxCommandEvent& event)
{
    if (m_parentFrame->IsShown())
        m_parentFrame->Hide();
    else
        m_parentFrame->Show();
}

void MyTaskBarIcon::OnToggleApp(wxCommandEvent& event)
{
    m_appEnabled = !m_appEnabled;
    
    // Notify the main frame about the state change
    wxCommandEvent toggleEvent(wxEVT_COMMAND_MENU_SELECTED, ID_TOGGLE_APP);
    toggleEvent.SetInt(m_appEnabled ? 1 : 0);
    wxPostEvent(m_parentFrame, toggleEvent);
    
    // Update the icon's tooltip to reflect the current state
    if (IsIconInstalled()) {
        // Reload the icon and update the tooltip with clearer status
        wxIcon icon;
        if (icon.LoadFile("../assets/images/app_icon.png", wxBITMAP_TYPE_PNG)) {
            SetIcon(icon, wxString::Format("HanSnap - %s", 
                m_appEnabled ? "● Active (Monitoring)" : "○ Disabled (Not Monitoring)"));
        }
    }
}

void MyTaskBarIcon::OnQuit(wxCommandEvent& event)
{
    // Ensure app is fully disabled before quitting
    if (m_appEnabled) {
        m_appEnabled = false;
        wxCommandEvent toggleEvent(wxEVT_COMMAND_MENU_SELECTED, ID_TOGGLE_APP);
        toggleEvent.SetInt(0);  // 0 = disabled
        wxPostEvent(m_parentFrame, toggleEvent);
    }
    
    m_parentFrame->Close(true);
}

void MyTaskBarIcon::OnLeftClick(wxTaskBarIconEvent& event)
{
    m_parentFrame->Show();
    m_parentFrame->Raise();
}
