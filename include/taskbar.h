#pragma once

#include <wx/wx.h>
#include <wx/taskbar.h>

// Forward declaration
class wxFrame;

// Custom taskbar icon implementation
class MyTaskBarIcon : public wxTaskBarIcon
{
public:
    // Make the IDs public so they can be used for event binding in other classes
    enum
    {
        ID_SHOW_HIDE = wxID_HIGHEST + 1,
        ID_TOGGLE_APP
    };

    MyTaskBarIcon(wxFrame* parentFrame);
    virtual ~MyTaskBarIcon();

    // This method is called when the icon is right-clicked
    virtual wxMenu* CreatePopupMenu() override;
    
    // Get/Set app state
    bool IsAppEnabled() const { return m_appEnabled; }
    void SetAppEnabled(bool enabled);

private:
    void OnShowHide(wxCommandEvent& event);
    void OnToggleApp(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnLeftClick(wxTaskBarIconEvent& event);

    wxFrame* m_parentFrame;
    bool m_appEnabled;

    wxDECLARE_EVENT_TABLE();
}; 