#pragma once

#include "custom-types/shared/macros.hpp"

#include "UnityEngine/MonoBehaviour.hpp"

DECLARE_CLASS_CODEGEN(MetaCore, ObjectSignal, UnityEngine::MonoBehaviour) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_INSTANCE_METHOD(void, OnEnable);
    DECLARE_INSTANCE_METHOD(void, OnDisable);

   public:
    std::function<void()> onEnable = nullptr;
    std::function<void()> onDisable = nullptr;

    static std::unordered_map<int, std::function<void()>> onDestroys;
};

DECLARE_CLASS_CODEGEN(MetaCore, MainThreadScheduler, UnityEngine::MonoBehaviour) {
    DECLARE_DEFAULT_CTOR();

    DECLARE_INSTANCE_METHOD(void, Update);

   public:
    static void Schedule(std::function<void()> callback);
    static void Schedule(std::function<bool()> wait, std::function<void()> callback);
    static void AddUpdate(std::function<void()> callback);

    template <class T>
    static void Await(T task, std::function<void()> callback) {
        Schedule([task]() { return task->IsCompleted; }, std::move(callback));
    }
};
