#include <wx/wx.h>
#include "../include/main_frame.h"

// The main application class.
class MyApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

// Macro that defines the main() entry point.
wxIMPLEMENT_APP(MyApp);
