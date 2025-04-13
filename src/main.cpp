#include "main.hpp"

#include "UnityEngine/GameObject.hpp"
#include "hooks.hpp"
#include "scotland2/shared/modloader.h"
#include "types.hpp"

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};

extern "C" void setup(CModInfo* info) {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(MOD_ID);
    logger.info("Completed setup!");
}

extern "C" void late_load() {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    Hooks::Install();

    auto mainThread = UnityEngine::GameObject::New_ctor("MetaCoreMainThread");
    UnityEngine::Object::DontDestroyOnLoad(mainThread);
    mainThread->AddComponent<MetaCore::MainThreadScheduler*>();
}
