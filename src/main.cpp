#include <wx/wx.h>
#include "../include/main_frame.h"

// The main application class.
class MyApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        // wxLog::SetLogLevel(wxLOG_Info);  // Only show Info level and above (hides Debug)
        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

// Macro that defines the main() entry point.
wxIMPLEMENT_APP(MyApp);
