#pragma once

#include <functional>
#include <string>

namespace MetaCore::Events {
    /// @brief The global ids of all built-in events
    enum Events {
        ScoreChanged,
        NoteCut,
        NoteMissed,
        BombCut,
        WallHit,
        ComboChanged,
        HealthChanged,
        Update,
        SlowUpdate,
        MapSelected,
        MapDeselected,
        MapStarted,
        MapPaused,
        MapUnpaused,
        MapRestarted,
        MapEnded,
        GameplaySceneEnded,
        ScoreSubmission,
        EventMax = ScoreSubmission,
    };

    /// @brief Registers a custom event for future broadcasts
    /// @param mod The unique id of the mod registering the event
    /// @param event The per-mod id of the event being registered
    /// @return The unique global id of the registered event (> EventMax), or -1 on failure
    int RegisterEvent(std::string mod, int event);

    /// @brief Finds the unique global id of a custom event
    /// @param mod The unique id of the mod that registered the event
    /// @param event The per-mod id of the registered event
    /// @return The unique global id of the event, or -1 if it does not exist
    int FindEvent(std::string mod, int event);

    /// @brief Registers a callback to an event
    /// @param event The global id of the event
    /// @param callback The function to be called when the event is broadcast
    /// @param once If the callback should only be called once and then removed
    /// @return The id for removal if the event was successfully registered to (>= 0), or -1 on failure
    int AddCallback(int event, std::function<void()> callback, bool once = false);
    /// @brief Registers a callback to an event
    /// @param mod The unique id of the mod that registered the event
    /// @param event The per-mod id of the event
    /// @param callback The function to be called when the event is broadcast
    /// @param once If the callback should only be called once and then removed
    /// @return The id for removal if the event was successfully registered to (>= 0), or -1 on failure
    int AddCallback(std::string mod, int event, std::function<void()> callback, bool once = false);
    /// @brief Registers a callback to all events that will be called before event-specific callbacks
    /// @param callback The function to be called with the global id of any broadcast event
    /// @param once If the callback should only be called once and then removed
    /// @return The id for removal (>= 0)
    int AddCallback(std::function<void(int)> callback, bool once = false);

    /// @brief Removes a previously registered callback
    /// @param id The id returned by a call to AddCallback
    void RemoveCallback(int id);

    /// @brief Globally broadcasts an event
    /// @param event The global id of the event
    /// @return If the event was successfully broadcast
    bool Broadcast(int event);
    /// @brief Globally broadcasts an event
    /// @param mod The unique id of the mod that registered the event
    /// @param event The per-mod id of the event
    /// @return If the event was successfully broadcast
    bool Broadcast(std::string mod, int event);
}

#define CONCAT_WRAPPED(x, y) x##y
#define CONCAT(x, y) CONCAT_WRAPPED(x, y)

#define AUTO_FUNCTION \
static __attribute__((constructor)) void CONCAT(auto_function_dlopen_, __LINE__)()

#define ON_EVENT(...)                                                                     \
static void CONCAT(auto_function_event_, __LINE__)();                                     \
AUTO_FUNCTION {                                                                           \
    ::MetaCore::Events::AddCallback(__VA_ARGS__, CONCAT(auto_function_event_, __LINE__)); \
}                                                                                         \
static void CONCAT(auto_function_event_, __LINE__)()
