#include "../include/hotkey.h"
#include <map>

#ifdef __WXMSW__
    // Windows implementation
    #include <wx/msw/private.h>
#endif

#ifdef __WXGTK__
    // Linux implementation
    #include <X11/Xlib.h>
    #include <X11/extensions/XTest.h>
    #include <gtk/gtk.h>
#endif

#ifdef __WXMAC__
    // macOS implementation
    #include <Carbon/Carbon.h>
#endif

// Singleton instance
HotkeyManager* HotkeyManager::s_instance = nullptr;

// Helper struct to store hotkey information
struct HotkeyInfo {
    int id;
    HotkeyCallback callback;
};

// Platform-specific implementation
class HotkeyManager::Impl {
public:
    Impl(wxWindow* owner);
    ~Impl();
    
    bool RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback);
    void UnregisterAll();
    void OnHotkey(wxKeyEvent& event);
    
    // Owner window for events
    wxWindow* m_owner;
    // Map of hotkey IDs to their info
    std::map<int, HotkeyInfo> m_hotkeys;
    // Next available hotkey ID
    int m_nextId;
};

#ifdef __WXMSW__
// Windows implementation
HotkeyManager::Impl::Impl(wxWindow* owner) : m_owner(owner), m_nextId(1) {
    // Create a hidden frame to receive hotkey events if no owner provided
    if (!m_owner) {
        m_owner = new wxFrame(nullptr, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR);
    }
    
    // Bind the hotkey event
    m_owner->Bind(wxEVT_HOTKEY, [this](wxKeyEvent& event) {
        this->OnHotkey(event);
    });
}

HotkeyManager::Impl::~Impl() {
    UnregisterAll();
}

bool HotkeyManager::Impl::RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback) {
    // Convert our modifiers to Windows modifiers
    int winModifiers = 0;
    if (modifiers & HotkeyModifier::CTRL)  winModifiers |= MOD_CONTROL;
    if (modifiers & HotkeyModifier::ALT)   winModifiers |= MOD_ALT;
    if (modifiers & HotkeyModifier::SHIFT) winModifiers |= MOD_SHIFT;
    if (modifiers & HotkeyModifier::CMD)   winModifiers |= MOD_WIN;
    
    // Register the hotkey with Windows
    if (::RegisterHotKey((HWND)m_owner->GetHandle(), m_nextId, winModifiers, keyCode)) {
        // Store the hotkey info
        HotkeyInfo info;
        info.id = m_nextId;
        info.callback = callback;
        m_hotkeys[m_nextId] = info;
        m_nextId++;
        return true;
    }
    return false;
}

void HotkeyManager::Impl::UnregisterAll() {
    for (auto& pair : m_hotkeys) {
        ::UnregisterHotKey((HWND)m_owner->GetHandle(), pair.first);
    }
    m_hotkeys.clear();
}

void HotkeyManager::Impl::OnHotkey(wxKeyEvent& event) {
    int id = event.GetId();
    auto it = m_hotkeys.find(id);
    if (it != m_hotkeys.end()) {
        it->second.callback();
    }
}

#elif defined(__WXMAC__)
// macOS implementation - using Carbon API for global hotkeys
// Note: For modern macOS apps, you might want to use the Cocoa API instead

// Event handler for carbon hotkeys
OSStatus macHotkeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData) {
    HotkeyManager::Impl* impl = static_cast<HotkeyManager::Impl*>(userData);
    
    EventHotKeyID hotkeyID;
    GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, NULL, 
                     sizeof(hotkeyID), NULL, &hotkeyID);
    
    // Convert to wxKeyEvent to make the interface consistent
    wxKeyEvent keyEvent;
    keyEvent.SetId(hotkeyID.id);
    impl->OnHotkey(keyEvent);
    
    return noErr;
}

HotkeyManager::Impl::Impl(wxWindow* owner) : m_owner(owner), m_nextId(1) {
    // Install carbon event handler
    EventTypeSpec eventType;
    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;
    
    InstallApplicationEventHandler(NewEventHandlerUPP(macHotkeyHandler),
                                  1, &eventType, this, NULL);
}

HotkeyManager::Impl::~Impl() {
    UnregisterAll();
}

bool HotkeyManager::Impl::RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback) {
    // Convert our modifiers to Carbon modifiers
    UInt32 macModifiers = 0;
    if (modifiers & HotkeyModifier::CTRL)  macModifiers |= controlKey;
    if (modifiers & HotkeyModifier::ALT)   macModifiers |= optionKey;
    if (modifiers & HotkeyModifier::SHIFT) macModifiers |= shiftKey;
    if (modifiers & HotkeyModifier::CMD)   macModifiers |= cmdKey;
    
    // Set up the hotkey ID
    EventHotKeyID hotkeyID;
    hotkeyID.signature = 'hsnp';  // A unique signature for our app
    hotkeyID.id = m_nextId;
    
    // Register the hotkey with Carbon
    EventHotKeyRef hotkeyRef;
    OSStatus status = RegisterEventHotKey(keyCode, macModifiers, hotkeyID,
                                         GetApplicationEventTarget(), 0, &hotkeyRef);
    
    if (status == noErr) {
        // Store the hotkey info
        HotkeyInfo info;
        info.id = m_nextId;
        info.callback = callback;
        m_hotkeys[m_nextId] = info;
        m_nextId++;
        return true;
    }
    return false;
}

void HotkeyManager::Impl::UnregisterAll() {
    // This is a simplification - in a real app, you'd need to store and unregister each hotkey reference
    m_hotkeys.clear();
}

void HotkeyManager::Impl::OnHotkey(wxKeyEvent& event) {
    int id = event.GetId();
    auto it = m_hotkeys.find(id);
    if (it != m_hotkeys.end()) {
        it->second.callback();
    }
}

#elif defined(__WXGTK__)
// Linux implementation
// This is a simplified version - a real implementation would be more complex

HotkeyManager::Impl::Impl(wxWindow* owner) : m_owner(owner), m_nextId(1) {
    // For GTK, you might need a more complex approach with X11 input grabbing
    if (!m_owner) {
        m_owner = new wxFrame(nullptr, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR);
    }
    
    // This is a placeholder - actual X11 event handling would be needed
}

HotkeyManager::Impl::~Impl() {
    UnregisterAll();
}

bool HotkeyManager::Impl::RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback) {
    // X11 input grabbing would go here
    // This is a simplified placeholder
    HotkeyInfo info;
    info.id = m_nextId;
    info.callback = callback;
    m_hotkeys[m_nextId] = info;
    m_nextId++;
    
    // Return true as a placeholder - actual X11 result would be used
    return true;
}

void HotkeyManager::Impl::UnregisterAll() {
    // X11 ungrabbing would go here
    m_hotkeys.clear();
}

void HotkeyManager::Impl::OnHotkey(wxKeyEvent& event) {
    int id = event.GetId();
    auto it = m_hotkeys.find(id);
    if (it != m_hotkeys.end()) {
        it->second.callback();
    }
}

#else
// Generic implementation for other platforms
HotkeyManager::Impl::Impl(wxWindow* owner) : m_owner(owner), m_nextId(1) {
    // Create a hidden frame to receive hotkey events if no owner provided
    if (!m_owner) {
        m_owner = new wxFrame(nullptr, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR);
    }
}

HotkeyManager::Impl::~Impl() {
    UnregisterAll();
}

bool HotkeyManager::Impl::RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback) {
    // Store the hotkey info as if it were registered
    HotkeyInfo info;
    info.id = m_nextId;
    info.callback = callback;
    m_hotkeys[m_nextId] = info;
    m_nextId++;
    
    // Return false since we can't really register global hotkeys generically
    return false;
}

void HotkeyManager::Impl::UnregisterAll() {
    m_hotkeys.clear();
}

void HotkeyManager::Impl::OnHotkey(wxKeyEvent& event) {
    int id = event.GetId();
    auto it = m_hotkeys.find(id);
    if (it != m_hotkeys.end()) {
        it->second.callback();
    }
}
#endif

// Public HotkeyManager implementation
HotkeyManager* HotkeyManager::Get() {
    if (!s_instance) {
        s_instance = new HotkeyManager();
    }
    return s_instance;
}

HotkeyManager::HotkeyManager() {
    m_impl = new Impl(nullptr);
}

HotkeyManager::~HotkeyManager() {
    delete m_impl;
}

bool HotkeyManager::RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback) {
    return m_impl->RegisterHotkey(modifiers, keyCode, callback);
}

void HotkeyManager::UnregisterAll() {
    m_impl->UnregisterAll();
} 