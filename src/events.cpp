#include "events.hpp"

#include "main.hpp"
#include "maps.hpp"

static std::map<std::string, std::map<int, int>> customEvents = {};
static std::map<int, MetaCore::IndexMap<std::function<void()>>> callbacks = {};
static MetaCore::IndexMap<std::function<void(int)>> globalCallbacks = {};

static MetaCore::IndexMap<std::tuple<bool, int, int>> registrations = {};

static int maxEvent = (int) MetaCore::Events::EventMax;

int MetaCore::Events::RegisterEvent(std::string mod, int event) {
    if (!customEvents.contains(mod))
        customEvents[mod] = {};
    else if (customEvents[mod].contains(event))
        return -1;
    customEvents[mod][event] = ++maxEvent;
    return maxEvent;
}

int MetaCore::Events::FindEvent(std::string mod, int event) {
    if (!customEvents.contains(mod))
        return -1;
    if (!customEvents[mod].contains(event))
        return -1;
    return customEvents[mod][event];
}

template <bool Global, class... Ts>
static int AddCallbackImpl(MetaCore::IndexMap<std::function<void(Ts...)>>& list, std::function<void(Ts...)> callback, bool once, int event) {
    if (!once)
        return registrations.push({Global, event, list.push(std::move(callback))});

    int idx = list.push(nullptr);
    int registered = registrations.push({Global, event, idx});
    // list should be a safe reference to keep around
    list[idx] = [&list, idx, registered, callback = std::move(callback)](Ts... params) {
        callback(params...);
        // guaranteed to still have this function at idx, since it's being called
        list.erase(idx);
        registrations.erase(registered);
    };
    return registered;
}

template <bool Global, class... Ts>
static int AddCallbackImpl(std::function<void(Ts...)> callback, bool once, int event = 0) {
    if constexpr (Global)
        return AddCallbackImpl<Global>(globalCallbacks, callback, once, event);
    else
        return AddCallbackImpl<Global>(callbacks[event], callback, once, event);
}

int MetaCore::Events::AddCallback(int event, std::function<void()> callback, bool once) {
    if (event < 0 || event > maxEvent)
        return -1;
    if (!callbacks.contains(event))
        callbacks[event] = {};
    return AddCallbackImpl<false>(std::move(callback), once, event);
}

int MetaCore::Events::AddCallback(std::string mod, int event, std::function<void()> callback, bool once) {
    return AddCallback(FindEvent(mod, event), std::move(callback), once);
}

int MetaCore::Events::AddCallback(std::function<void(int)> callback, bool once) {
    return AddCallbackImpl<true>(std::move(callback), once);
}

void MetaCore::Events::RemoveCallback(int id) {
    if (!registrations.contains(id))
        return;
    auto const& [global, event, idx] = registrations[id];
    if (global)
        globalCallbacks.erase(idx);
    else if (callbacks.contains(event))
        callbacks[event].erase(idx);
}

template <class... Ts>
static inline void SafeCallCallbacks(MetaCore::IndexMap<std::function<void(Ts...)>> const& callbacks, Ts... params) {
    std::vector<std::function<void(Ts...)>> copies;
    copies.reserve(callbacks.size());
    for (auto const& [_, callback] : callbacks)
        copies.emplace_back(callback);
    for (auto const& callback : copies)
        callback(params...);
}

bool MetaCore::Events::Broadcast(int event) {
    if (event < 0 || event > maxEvent)
        return false;
    SafeCallCallbacks(globalCallbacks, event);

    if (callbacks.contains(event))
        SafeCallCallbacks(callbacks[event]);

    return true;
}

bool MetaCore::Events::Broadcast(std::string mod, int event) {
    if (!customEvents.contains(mod))
        return false;
    if (!customEvents[mod].contains(event))
        return false;
    return Broadcast(customEvents[mod][event]);
}
