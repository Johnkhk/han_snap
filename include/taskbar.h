#pragma once

#include <wx/wx.h>
#include <wx/taskbar.h>

// Forward declaration
class wxFrame;

// Custom taskbar icon implementation
class MyTaskBarIcon : public wxTaskBarIcon
{
public:
    MyTaskBarIcon(wxFrame* parentFrame);
    virtual ~MyTaskBarIcon();

    // This method is called when the icon is right-clicked
    virtual wxMenu* CreatePopupMenu() override;

private:
    enum
    {
        ID_SHOW_HIDE = wxID_HIGHEST + 1
    };

    void OnShowHide(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnLeftClick(wxTaskBarIconEvent& event);

    wxFrame* m_parentFrame;

    wxDECLARE_EVENT_TABLE();
}; 