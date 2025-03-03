#include "../include/screenshot.h"
#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <climits>

wxBEGIN_EVENT_TABLE(ScreenshotFrame, wxFrame)
    EVT_PAINT(ScreenshotFrame::OnPaint)
    EVT_LEFT_DOWN(ScreenshotFrame::OnLeftDown)
    EVT_LEFT_UP(ScreenshotFrame::OnLeftUp)
    EVT_MOTION(ScreenshotFrame::OnMouseMove)
    EVT_KEY_DOWN(ScreenshotFrame::OnKeyDown)
wxEND_EVENT_TABLE()

void LaunchScreenshotTool(ScreenshotCallback callback) {
    int displayCount = wxDisplay::GetCount();
    for (int i = 0; i < displayCount; i++) {
        wxDisplay display(i);
        wxRect rect = display.GetGeometry();
        // Create a new overlay window for each display.
        ScreenshotFrame* frame = new ScreenshotFrame(callback, rect);
        frame->SetPosition(rect.GetPosition());
        frame->SetSize(rect.GetSize());
        frame->Show(true);
    }
}


ScreenshotFrame::ScreenshotFrame(ScreenshotCallback callback, const wxRect& displayRect)
    : wxFrame(NULL, wxID_ANY, "Screenshot Tool", displayRect.GetPosition(), displayRect.GetSize(),
              wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxBORDER_NONE),
      m_callback(callback),
      m_selecting(false),
      m_selectionComplete(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Optionally, set transparency if desired:
    SetTransparent(128);  // for semi-transparent overlay
    
    // Bind events to the frame or to a panel that fills it.
    Bind(wxEVT_PAINT, &ScreenshotFrame::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &ScreenshotFrame::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &ScreenshotFrame::OnLeftUp, this);
    Bind(wxEVT_MOTION, &ScreenshotFrame::OnMouseMove, this);
    Bind(wxEVT_KEY_DOWN, &ScreenshotFrame::OnKeyDown, this);
    
    // Ensure this frame gets key events.
    SetFocus();
    
    // Capture the screen when the frame is created
    CaptureScreen();
}


void ScreenshotFrame::CaptureScreen() {
    // Get information about all displays
    int displayCount = wxDisplay::GetCount();
    std::cout << "Display count: " << displayCount << std::endl;
    
    // Find the total bounding rectangle of all displays
    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;
    
    for (int i = 0; i < displayCount; i++) {
        wxDisplay display(i);
        wxRect geometry = display.GetGeometry();
        minX = wxMin(minX, geometry.GetLeft());
        minY = wxMin(minY, geometry.GetTop());
        maxX = wxMax(maxX, geometry.GetRight());
        maxY = wxMax(maxY, geometry.GetBottom());
    }
    
    // Calculate the combined size of all displays
    int totalWidth = maxX - minX + 1;
    int totalHeight = maxY - minY + 1;
    
    std::cout << "Combined display area: " << totalWidth << " x " << totalHeight 
              << " pixels (from " << minX << "," << minY << " to " << maxX << "," << maxY << ")" << std::endl;

    // Save the virtual screen rectangle
    m_virtualScreenRect = wxRect(minX, minY, totalWidth, totalHeight);
    
    // Create a screenshot bitmap with proper dimensions
    std::cout << "Creating screenshot bitmap..." << std::endl;
    if (!m_screenshot.Create(totalWidth, totalHeight)) {
        std::cerr << "Failed to create bitmap of size " << totalWidth << "x" << totalHeight << std::endl;
        return;
    }
    
    if (!m_screenshot.IsOk()) {
        std::cerr << "Bitmap is not valid after creation!" << std::endl;
        return;
    }
    
    std::cout << "Bitmap created successfully. Size: " << m_screenshot.GetWidth() 
              << "x" << m_screenshot.GetHeight() << std::endl;
    
    // Create a memory DC to copy the screen content
    wxMemoryDC memDC;
    memDC.SelectObject(m_screenshot);
    
    // Create a device context for the screen
    wxScreenDC screenDC;
    
    // Copy the screen content to our bitmap
    std::cout << "Copying screen to bitmap..." << std::endl;
    memDC.Blit(0, 0, totalWidth, totalHeight, &screenDC, minX, minY);
    
    // Clean up
    memDC.SelectObject(wxNullBitmap);
    std::cout << "Screen capture completed" << std::endl;
}

void ScreenshotFrame::OnLeftDown(wxMouseEvent& event) {
    std::cout << "OnLeftDown" << std::endl;
    // Start the selection process; coordinates are relative to our frame,
    // which now covers the entire virtual desktop.
    m_startPoint = event.GetPosition();
    m_currentPoint = m_startPoint;
    m_selecting = true;
    m_selectionComplete = false;
    Refresh();
}

void ScreenshotFrame::OnMouseMove(wxMouseEvent& event) {
    if (m_selecting) {
        m_currentPoint = event.GetPosition();
        Refresh();
    }
}

void ScreenshotFrame::OnLeftUp(wxMouseEvent& event) {
    std::cout << "OnLeftUp" << std::endl;
    if (m_selecting) {
        m_currentPoint = event.GetPosition();
        m_selectionRect = wxRect(
            wxMin(m_startPoint.x, m_currentPoint.x),
            wxMin(m_startPoint.y, m_currentPoint.y),
            abs(m_currentPoint.x - m_startPoint.x),
            abs(m_currentPoint.y - m_startPoint.y)
        );
        
        m_selecting = false;
        m_selectionComplete = true;
        
        // If selection is too small, consider it a click
        if (m_selectionRect.width < 5 || m_selectionRect.height < 5) {
            Close(true);
            return;
        }
        
        ProcessSelection();
        Refresh();
    }
}

void ScreenshotFrame::ProcessSelection() {
    std::cout << "Processing selection: " << m_selectionRect.width << "x" << m_selectionRect.height << std::endl;
    if (m_selectionRect.width <= 0 || m_selectionRect.height <= 0) {
        std::cerr << "Invalid selection rectangle size" << std::endl;
        return;
    }
    
    // Check if the screenshot bitmap is valid
    if (!m_screenshot.IsOk()) {
        std::cerr << "Screenshot bitmap is invalid!" << std::endl;
        return;
    }
    
    std::cout << "Screenshot bitmap size: " << m_screenshot.GetWidth() << "x" << m_screenshot.GetHeight() << std::endl;
    std::cout << "Creating bitmap from selected region" << std::endl;
    
    // Adjust selection rectangle to be relative to the virtual screen coordinates
    wxRect adjustedRect = m_selectionRect;
    
    std::cout << "Original selection: " << adjustedRect.x << "," << adjustedRect.y 
              << " " << adjustedRect.width << "x" << adjustedRect.height << std::endl;
    
    // If we're capturing from a multi-display setup, we need to account for
    // the offset between screen coordinates and our bitmap coordinates
    if (m_virtualScreenRect.x != 0 || m_virtualScreenRect.y != 0) {
        adjustedRect.x -= m_virtualScreenRect.x;
        adjustedRect.y -= m_virtualScreenRect.y;
        std::cout << "Adjusted for virtual screen: " << adjustedRect.x << "," << adjustedRect.y 
                  << " " << adjustedRect.width << "x" << adjustedRect.height << std::endl;
    }
    
    // Make sure the adjusted rectangle is valid
    if (adjustedRect.x < 0) {
        std::cout << "Adjusting x from " << adjustedRect.x << " to 0" << std::endl;
        adjustedRect.x = 0;
    }
    if (adjustedRect.y < 0) {
        std::cout << "Adjusting y from " << adjustedRect.y << " to 0" << std::endl;
        adjustedRect.y = 0;
    }
    
    int maxWidth = m_screenshot.GetWidth() - adjustedRect.x;
    int maxHeight = m_screenshot.GetHeight() - adjustedRect.y;
    
    if (adjustedRect.width > maxWidth) {
        std::cout << "Adjusting width from " << adjustedRect.width << " to " << maxWidth << std::endl;
        adjustedRect.width = maxWidth;
    }
    if (adjustedRect.height > maxHeight) {
        std::cout << "Adjusting height from " << adjustedRect.height << " to " << maxHeight << std::endl;
        adjustedRect.height = maxHeight;
    }
    
    // Final sanity check
    if (adjustedRect.x >= m_screenshot.GetWidth() || 
        adjustedRect.y >= m_screenshot.GetHeight() ||
        adjustedRect.width <= 0 || adjustedRect.height <= 0) {
        std::cerr << "Final rectangle is invalid: " << adjustedRect.x << "," << adjustedRect.y 
                  << " " << adjustedRect.width << "x" << adjustedRect.height << std::endl;
        Close(true);
        return;
    }
    
    std::cout << "Final selection rectangle: " << adjustedRect.x << "," << adjustedRect.y 
              << " " << adjustedRect.width << "x" << adjustedRect.height << std::endl;
    
    // Now create the bitmap from the correctly adjusted coordinates
    wxBitmap selectedBitmap;
    try {
        selectedBitmap = m_screenshot.GetSubBitmap(adjustedRect);
        std::cout << "Bitmap created successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception creating sub-bitmap: " << e.what() << std::endl;
        Close(true);
        return;
    } catch (...) {
        std::cerr << "Unknown exception creating sub-bitmap" << std::endl;
        Close(true);
        return;
    }
    
    // Check if the created bitmap is valid
    if (!selectedBitmap.IsOk()) {
        std::cerr << "Selected bitmap is invalid after creation!" << std::endl;
        Close(true);
        return;
    }
    
    // Call the callback with the selected bitmap
    if (m_callback) {
        std::cout << "Calling callback with bitmap" << std::endl;
        m_callback(selectedBitmap);
    }
    
    // Close the screenshot frame
    Close(true);
}


void ScreenshotFrame::OnKeyDown(wxKeyEvent& event) {
    // Press Escape to cancel
    if (event.GetKeyCode() == WXK_ESCAPE) {
        Close(true);
    }
    event.Skip();
}

void ScreenshotFrame::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    
    if (m_screenshot.IsOk()) {
        // Draw the screenshot as the background
        dc.DrawBitmap(m_screenshot, 0, 0);
        
        // If user is making a selection or has completed one, draw the overlay
        if (m_selecting || m_selectionComplete) {
            // Draw semi-transparent overlay
            wxColour overlayColor(0, 0, 0, 128);  // Semi-transparent black
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(overlayColor));
            
            wxSize size = GetClientSize();
            wxRect selectionRect;
            
            if (m_selecting) {
                // Calculate the selection rectangle based on start and current points
                selectionRect = wxRect(
                    wxMin(m_startPoint.x, m_currentPoint.x),
                    wxMin(m_startPoint.y, m_currentPoint.y),
                    abs(m_currentPoint.x - m_startPoint.x),
                    abs(m_currentPoint.y - m_startPoint.y)
                );
            } else {
                selectionRect = m_selectionRect;
            }
            
            // Draw four rectangles to create a "cutout" effect
            
            // Top rectangle
            dc.DrawRectangle(0, 0, size.GetWidth(), selectionRect.y);
            
            // Left rectangle
            dc.DrawRectangle(0, selectionRect.y, selectionRect.x, selectionRect.height);
            
            // Right rectangle
            dc.DrawRectangle(selectionRect.x + selectionRect.width, selectionRect.y,
                            size.GetWidth() - (selectionRect.x + selectionRect.width),
                            selectionRect.height);
            
            // Bottom rectangle
            dc.DrawRectangle(0, selectionRect.y + selectionRect.height,
                            size.GetWidth(),
                            size.GetHeight() - (selectionRect.y + selectionRect.height));
            
            // Draw border around selection
            dc.SetPen(wxPen(*wxRED, 2));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(selectionRect);
        }
    } else {
        // If screenshot failed to load, draw a plain background
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(GetClientRect());
        
        // Maybe add some error text
        dc.SetTextForeground(*wxWHITE);
        dc.DrawText("Screenshot capture failed!", 20, 20);
    }
}