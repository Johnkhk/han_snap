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
    menu->Append(ID_TOGGLE_APP, wxString::Format("App: %s", m_appEnabled ? "On" : "Off"));
    menu->AppendSeparator();
    menu->Append(ID_SHOW_HIDE, m_parentFrame->IsShown() ? "Hide" : "Show");
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
    
    // You can add code here to actually enable/disable app functionality
    // For example, start/stop monitoring clipboard, etc.
    
    // Example of how you might notify the main frame about the state change:
    wxCommandEvent toggleEvent(wxEVT_COMMAND_MENU_SELECTED, ID_TOGGLE_APP);
    toggleEvent.SetInt(m_appEnabled ? 1 : 0);
    wxPostEvent(m_parentFrame, toggleEvent);
}

void MyTaskBarIcon::OnQuit(wxCommandEvent& event)
{
    m_parentFrame->Close(true);
}

void MyTaskBarIcon::OnLeftClick(wxTaskBarIconEvent& event)
{
    m_parentFrame->Show();
    m_parentFrame->Raise();
}
