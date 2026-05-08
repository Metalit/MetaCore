#include "main.hpp"

#include "export.h"
#include "hooks.hpp"
#include "types.hpp"
#include "scotland2/shared/modloader.h"

#include "UnityEngine/GameObject.hpp"

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};

extern "C" METACORE_EXPORT void setup(CModInfo* info) {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(MOD_ID);
    logger.info("Completed setup!");
}

extern "C" METACORE_EXPORT void late_load() {
    custom_types::Register::AutoRegister();
    Hooks::Install();

    auto mainThread = UnityEngine::GameObject::New_ctor("MetaCoreMainThread");
    UnityEngine::Object::DontDestroyOnLoad(mainThread);
    mainThread->AddComponent<MetaCore::MainThreadScheduler*>();
}
