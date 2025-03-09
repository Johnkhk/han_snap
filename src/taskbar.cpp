#include "../include/taskbar.h"

// Event table for the taskbar icon
wxBEGIN_EVENT_TABLE(MyTaskBarIcon, wxTaskBarIcon)
    EVT_MENU(wxID_EXIT, MyTaskBarIcon::OnQuit)
    EVT_MENU(MyTaskBarIcon::ID_SHOW_HIDE, MyTaskBarIcon::OnShowHide)
    EVT_TASKBAR_LEFT_DOWN(MyTaskBarIcon::OnLeftClick)
wxEND_EVENT_TABLE()

MyTaskBarIcon::MyTaskBarIcon(wxFrame* parentFrame) 
    : m_parentFrame(parentFrame) 
{
}

MyTaskBarIcon::~MyTaskBarIcon()
{
    RemoveIcon();
}

wxMenu* MyTaskBarIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu;
    menu->Append(ID_SHOW_HIDE, m_parentFrame->IsShown() ? "Hide" : "Show");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, "Quit");
    
    return menu;
}

void MyTaskBarIcon::OnShowHide(wxCommandEvent& event)
{
    if (m_parentFrame->IsShown())
        m_parentFrame->Hide();
    else
        m_parentFrame->Show();
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
