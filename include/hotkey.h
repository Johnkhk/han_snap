#ifndef HOTKEY_H
#define HOTKEY_H

#include <wx/wx.h>
#include <functional>

// Callback type for hotkey events
typedef std::function<void()> HotkeyCallback;

/**
 * HotkeyManager handles global hotkey registration and events
 * in a cross-platform manner.
 */
class HotkeyManager {
public:
    // Get the singleton instance
    static HotkeyManager* Get();
    
    // Register a hotkey with a callback
    bool RegisterHotkey(int modifiers, int keyCode, HotkeyCallback callback);
    
    // Unregister all hotkeys
    void UnregisterAll();
    
    // Platform-specific implementation details - made public for callback access
    class Impl;
    
private:
    HotkeyManager();
    ~HotkeyManager();
    
    Impl* m_impl;
    
    // Singleton instance
    static HotkeyManager* s_instance;
};

// Modifier key constants for better cross-platform compatibility
namespace HotkeyModifier {
    static const int CTRL  = 1;
    static const int ALT   = 2;
    static const int SHIFT = 4;
    static const int CMD   = 8;  // macOS Command key
}

#endif // HOTKEY_H 