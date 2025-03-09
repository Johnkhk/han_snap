#ifndef FILEMONITOR_H
#define FILEMONITOR_H

#include <wx/filename.h>
#include <wx/string.h>
#include <wx/event.h>
#include <memory>

// Declare the custom event type
wxDECLARE_EVENT(wxEVT_FILE_CHANGED, wxCommandEvent);

/**
 * Monitors a directory for changes.
 * 
 * This class provides a platform-independent way to monitor a directory
 * for changes and notify the application when files have been added, modified, or removed.
 */
class FileMonitor
{
public:
    /**
     * Default constructor.
     */
    FileMonitor() = default;

    /**
     * Constructor that immediately starts monitoring a directory.
     * 
     * @param directory The directory to monitor
     */
    FileMonitor(const wxFileName& directory);

    /**
     * Destructor.
     */
    ~FileMonitor();

    /**
     * Start monitoring a directory.
     * 
     * @param directory The directory to monitor
     */
    void Init(const wxFileName& directory);

    /**
     * Stop monitoring and clear internal state.
     */
    void Reset();

    /**
     * Check if the monitor is currently active.
     * 
     * @return true if monitoring a directory, false otherwise
     */
    bool IsOk() const { return m_file.IsOk(); }

    /**
     * Get the directory being monitored.
     * 
     * @return The monitored directory
     */
    const wxFileName& GetFileName() const { return m_file; }

    /**
     * Notify that a file has changed.
     * 
     * This is called internally when a file change is detected.
     * 
     * @param path Path to the file that changed
     */
    static void NotifyFileChanged(const wxString& path);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    wxFileName m_file;
    wxDateTime m_loadTime;
};

#endif // FILEMONITOR_H
