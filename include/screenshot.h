#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <wx/wx.h>
#include <functional>

// Callback function type for receiving the screenshot
typedef std::function<void(wxBitmap)> ScreenshotCallback;

/**
 * Launches a screenshot selection tool that allows the user to select
 * an area of the screen. The selected area will be returned via the callback.
 * 
 * @param callback Function to call with the screenshot bitmap
 */
void LaunchScreenshotTool(ScreenshotCallback callback);

/**
 * Overloaded function to launch the screenshot tool on specific display(s).
 */
void LaunchScreenshotTool(ScreenshotCallback callback, const wxRect& displayRect);

// Internal class used by the screenshot tool
class ScreenshotFrame : public wxFrame {
public:
    // Original constructor
    ScreenshotFrame(ScreenshotCallback callback);
    // New constructor accepting a display rectangle
    ScreenshotFrame(ScreenshotCallback callback, const wxRect& displayRect);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void CaptureScreen();
    void ProcessSelection();
    
    wxPanel* m_panel;
    wxPoint m_startPoint;
    wxPoint m_currentPoint;
    wxRect m_selectionRect;
    wxRect m_virtualScreenRect;
    bool m_selecting = false;
    bool m_selectionComplete = false;
    wxBitmap m_screenshot;
    ScreenshotCallback m_callback;

    wxDECLARE_EVENT_TABLE();
};

#endif // SCREENSHOT_H
