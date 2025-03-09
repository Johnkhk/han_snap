#include "filemonitor.h"
#include <wx/log.h>
#include <wx/app.h>
#include <wx/event.h>
#include <wx/filename.h>

// Custom event for file changes
wxDEFINE_EVENT(wxEVT_FILE_CHANGED, wxCommandEvent);

// Platform-specific includes
#ifdef __WXMAC__
#include <CoreServices/CoreServices.h>
#elif defined(__WXGTK__)
#include <sys/inotify.h>
#include <unistd.h>
#elif defined(__WXMSW__)
#include <windows.h>
#endif

// Define the implementation class
class FileMonitor::Impl
{
public:
    Impl() : m_isMonitoring(false) {}
    ~Impl() { Reset(); }

    void Init(const wxFileName& directory);
    void Reset();

private:
    bool m_isMonitoring;

#ifdef __WXMAC__
    FSEventStreamRef m_eventStream = nullptr;
    static void FSEventCallback(
        ConstFSEventStreamRef streamRef,
        void* clientCallBackInfo,
        size_t numEvents,
        void* eventPaths,
        const FSEventStreamEventFlags eventFlags[],
        const FSEventStreamEventId eventIds[]);
#elif defined(__WXGTK__)
    int m_inotifyFd = -1;
    int m_watchDescriptor = -1;
#elif defined(__WXMSW__)
    HANDLE m_directoryHandle = INVALID_HANDLE_VALUE;
    OVERLAPPED m_overlapped;
    char m_buffer[1024];
    void StartWatch();
#endif
};

// Implementation of FileMonitor class

FileMonitor::FileMonitor(const wxFileName& directory)
{
    Init(directory);
}

FileMonitor::~FileMonitor()
{
    Reset();
}

void FileMonitor::Init(const wxFileName& directory)
{
    wxLogDebug("INITTTT1");
    if (!directory.DirExists())
    {
        wxLogError("Directory does not exist: %s", directory.GetFullPath());
        return;
    }

    m_file = directory;
    m_loadTime = wxDateTime::Now();
    
    // Create the implementation if it doesn't exist
    if (!m_impl)
    {
        m_impl = std::make_unique<Impl>();
    }
    
    // Initialize the implementation
    m_impl->Init(directory);
}

void FileMonitor::Reset()
{
    if (m_impl)
    {
        m_impl->Reset();
    }
    m_file = wxFileName();
}

void FileMonitor::NotifyFileChanged(const wxString& path)
{
    // Create an event to notify the application
    wxCommandEvent event(wxEVT_FILE_CHANGED);
    event.SetString(path);
    
    // Post the event to the application
    wxTheApp->QueueEvent(new wxCommandEvent(event));
}

// Implementation of platform-specific code

#ifdef __WXMAC__
void FileMonitor::Impl::FSEventCallback(
    ConstFSEventStreamRef streamRef,
    void* clientCallBackInfo,
    size_t numEvents,
    void* eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
    wxLogDebug("File xxxxxxchanged detected");
    char** paths = static_cast<char**>(eventPaths);
    
    for (size_t i = 0; i < numEvents; ++i)
    {
        // Convert path to wxString
        wxString path = wxString::FromUTF8(paths[i]);
        
        // Notify the application of the change
        FileMonitor::NotifyFileChanged(path);
    }
}

void FileMonitor::Impl::Init(const wxFileName& directory)
{
    wxLogDebug("INITTTT2");
    Reset();
    wxLogDebug("INITTTT3");
    // Create an array of paths to watch
    CFStringRef pathToWatch = CFStringCreateWithCString(
        NULL,
        directory.GetFullPath().utf8_str(),
        kCFStringEncodingUTF8
    );
    wxLogDebug("INITTTT4");
    // Log the path we're about to watch
    wxLogDebug("Setting up watch for path: %s", directory.GetFullPath());
    
    // Log the CFString representation
    char buffer[1024];
    if (CFStringGetCString(pathToWatch, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
        wxLogDebug("CFString path to watch: %s", buffer);
    } else {
        wxLogDebug("Failed to convert CFString path for logging");
    }
    CFArrayRef pathsToWatch = CFArrayCreate(
        NULL,
        (const void**)&pathToWatch,
        1,
        &kCFTypeArrayCallBacks
    );
    
    // Create the context
    FSEventStreamContext context = {0, this, NULL, NULL, NULL};
    
    // Create the stream
    m_eventStream = FSEventStreamCreate(
        NULL,
        &FileMonitor::Impl::FSEventCallback,
        &context,
        pathsToWatch,
        kFSEventStreamEventIdSinceNow,
        // 0.5,  // 500ms latency
        0.01,  // 10ms latency
        kFSEventStreamCreateFlagFileEvents
    );
    wxLogDebug("INITTTT5");
    if (m_eventStream)
    {
        wxLogDebug("INITTTT6");
        // Schedule with the run loop
        FSEventStreamScheduleWithRunLoop(
            m_eventStream,
            CFRunLoopGetCurrent(),
            kCFRunLoopDefaultMode
        );
        wxLogDebug("INITTTT7");
        // Start the stream
        FSEventStreamStart(m_eventStream);
        wxLogDebug("INITTTT8");
        m_isMonitoring = true;
    }
    
    // Release the CF objects
    CFRelease(pathsToWatch);
    CFRelease(pathToWatch);
}

void FileMonitor::Impl::Reset()
{
    if (m_isMonitoring && m_eventStream)
    {
        FSEventStreamStop(m_eventStream);
        FSEventStreamInvalidate(m_eventStream);
        FSEventStreamRelease(m_eventStream);
        m_eventStream = nullptr;
    }
    m_isMonitoring = false;
}

#elif defined(__WXGTK__)
// Linux implementation using inotify
void FileMonitor::Impl::Init(const wxFileName& directory)
{
    Reset();
    
    // Initialize inotify
    m_inotifyFd = inotify_init();
    if (m_inotifyFd < 0)
    {
        wxLogError("Failed to initialize inotify");
        return;
    }
    
    // Add the directory to watch
    m_watchDescriptor = inotify_add_watch(
        m_inotifyFd,
        directory.GetFullPath().utf8_str(),
        IN_CREATE | IN_MODIFY | IN_DELETE
    );
    
    if (m_watchDescriptor < 0)
    {
        wxLogError("Failed to add watch for directory: %s", directory.GetFullPath());
        close(m_inotifyFd);
        m_inotifyFd = -1;
        return;
    }
    
    m_isMonitoring = true;
    
    // TODO: Set up a thread or event handler to read from inotify_fd
}

void FileMonitor::Impl::Reset()
{
    if (m_isMonitoring)
    {
        if (m_watchDescriptor >= 0)
        {
            inotify_rm_watch(m_inotifyFd, m_watchDescriptor);
            m_watchDescriptor = -1;
        }
        
        if (m_inotifyFd >= 0)
        {
            close(m_inotifyFd);
            m_inotifyFd = -1;
        }
    }
    m_isMonitoring = false;
}

#elif defined(__WXMSW__)
// Windows implementation using ReadDirectoryChangesW
void FileMonitor::Impl::Init(const wxFileName& directory)
{
    Reset();
    
    // Open the directory
    m_directoryHandle = CreateFile(
        directory.GetFullPath().wc_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    
    if (m_directoryHandle == INVALID_HANDLE_VALUE)
    {
        wxLogError("Failed to open directory for monitoring: %s", directory.GetFullPath());
        return;
    }
    
    // Initialize the overlapped structure
    memset(&m_overlapped, 0, sizeof(OVERLAPPED));
    m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (m_overlapped.hEvent == NULL)
    {
        CloseHandle(m_directoryHandle);
        m_directoryHandle = INVALID_HANDLE_VALUE;
        wxLogError("Failed to create event for directory monitoring");
        return;
    }
    
    m_isMonitoring = true;
    StartWatch();
}

void FileMonitor::Impl::StartWatch()
{
    // Start monitoring changes
    BOOL success = ReadDirectoryChangesW(
        m_directoryHandle,
        m_buffer,
        sizeof(m_buffer),
        TRUE,  // Watch subtree
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL,
        &m_overlapped,
        NULL
    );
    
    if (!success)
    {
        wxLogError("Failed to start monitoring directory changes");
        Reset();
    }
    
    // TODO: Set up an event handler for when the overlapped operation completes
}

void FileMonitor::Impl::Reset()
{
    if (m_isMonitoring)
    {
        if (m_overlapped.hEvent != NULL)
        {
            CloseHandle(m_overlapped.hEvent);
            m_overlapped.hEvent = NULL;
        }
        
        if (m_directoryHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_directoryHandle);
            m_directoryHandle = INVALID_HANDLE_VALUE;
        }
    }
    m_isMonitoring = false;
}
#endif
