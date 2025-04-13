#include "game.hpp"

#include "GlobalNamespace/FadeInOutController.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/ObservableVariableSO_1.hpp"
#include "GlobalNamespace/PlayerDataFileModel.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "UnityEngine/Resources.hpp"
#include "events.hpp"
#include "main.hpp"
#include "types.hpp"
#include "unity.hpp"

using namespace GlobalNamespace;

static std::set<std::string> scoreDisablers;

static std::set<std::string> cameraDisablers;

void MetaCore::Game::SetScoreSubmission(std::string mod, bool enable) {
    bool wasEmpty = scoreDisablers.empty();
    if (enable)
        scoreDisablers.erase(mod);
    else
        scoreDisablers.emplace(std::move(mod));
    if (scoreDisablers.empty() != wasEmpty)
        Events::Broadcast(Events::ScoreSubmission);
}

void MetaCore::Game::DisableScoreSubmissionOnce(std::string mod) {
    Events::AddCallback(Events::GameplaySceneEnded, [mod]() { SetScoreSubmission(std::move(mod), true); }, true);
    SetScoreSubmission(std::move(mod), false);
}

bool MetaCore::Game::IsScoreSubmissionDisabled() {
    return !scoreDisablers.empty();
}

std::set<std::string> const& MetaCore::Game::GetScoreSubmissionDisablers() {
    return scoreDisablers;
}

static FadeInOutController* GetFadeInOut() {
    static UnityW<FadeInOutController> fadeInOutController;
    if (!fadeInOutController) {
        fadeInOutController = UnityEngine::Resources::FindObjectsOfTypeAll<FadeInOutController*>()->FirstOrDefault();
        if (fadeInOutController)
            MetaCore::Engine::GetOrAddComponent<MetaCore::ObjectSignal*>(fadeInOutController)->onDestroy = []() {
                fadeInOutController = nullptr;
            };
    }
    if (!fadeInOutController)
        logger.warn("GetFadeInOut returning null");
    return fadeInOutController.unsafePtr();
}

static void UpdateFadeInOut(float duration) {
    auto controller = GetFadeInOut();
    bool fadedOut = controller->_easeValue->value == 0;
    bool toFadeOut = MetaCore::Game::IsCameraFadedOut();
    if (fadedOut == toFadeOut)
        return;
    controller->StopAllCoroutines();
    if (duration < 0)
        duration = toFadeOut ? controller->_defaultFadeOutDuration : controller->_defaultFadeInDuration;
    if (duration == 0)
        controller->_easeValue->value = toFadeOut ? 0 : 1;
    else if (toFadeOut)
        controller->StartCoroutine(controller->Fade(controller->_easeValue->value, 0, duration, 0, controller->_fadeOutCurve, nullptr));
    else
        controller->StartCoroutine(controller->Fade(0, 1, duration, controller->_fadeInStartDelay, controller->_fadeInCurve, nullptr));
}

void MetaCore::Game::SetCameraFadeOut(std::string mod, bool fadeOut, float fadeDuration) {
    bool wasEmpty = cameraDisablers.empty();
    if (fadeOut)
        cameraDisablers.emplace(std::move(mod));
    else
        cameraDisablers.erase(mod);
    if (wasEmpty == fadeOut)
        UpdateFadeInOut(fadeDuration);
}

bool MetaCore::Game::IsCameraFadedOut() {
    return !cameraDisablers.empty();
}

void MetaCore::Game::PlaySongPreview(UnityEngine::AudioClip* audio, float volume, float start, float duration) {
    GetMainFlowCoordinator()
        ->_soloFreePlayFlowCoordinator->levelSelectionNavigationController->_levelCollectionNavigationController->_levelCollectionViewController
        ->_songPreviewPlayer->CrossfadeTo(audio, volume, start, duration, nullptr);
}

BeatmapCharacteristicCollection* MetaCore::Game::GetCharacteristics() {
    return GetPlayerData()->_playerDataFileModel->_beatmapCharacteristicCollection;
}

EnvironmentsListModel* MetaCore::Game::GetEnvironments() {
    return GetPlayerData()->_playerDataFileModel->_environmentsListModel;
}

PlayerDataModel* MetaCore::Game::GetPlayerData() {
    return GetMainFlowCoordinator()->_playerDataModel;
}

UnityEngine::Material* MetaCore::Game::GetCurvedCornersMaterial() {
    static UnityW<UnityEngine::Material> material;
    if (!material)
        material = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>()->First([](auto mat) {
            return mat->name == std::string("UINoGlowRoundEdge");
        });
    return material;
}

MenuTransitionsHelper* MetaCore::Game::GetMenuTransitionsHelper() {
    static UnityW<MenuTransitionsHelper> menuTransitionsHelper;
    if (!menuTransitionsHelper) {
        menuTransitionsHelper = UnityEngine::Resources::FindObjectsOfTypeAll<MenuTransitionsHelper*>()->FirstOrDefault();
        if (menuTransitionsHelper)
            Engine::GetOrAddComponent<ObjectSignal*>(menuTransitionsHelper)->onDestroy = []() {
                menuTransitionsHelper = nullptr;
            };
    }
    if (!menuTransitionsHelper)
        logger.warn("GetMainFlowCoordinator returning null");
    return menuTransitionsHelper.unsafePtr();
}

MainFlowCoordinator* MetaCore::Game::GetMainFlowCoordinator() {
    static UnityW<MainFlowCoordinator> mainFlowCoordinator;
    if (!mainFlowCoordinator) {
        mainFlowCoordinator = UnityEngine::Resources::FindObjectsOfTypeAll<MainFlowCoordinator*>()->FirstOrDefault();
        if (mainFlowCoordinator)
            Engine::GetOrAddComponent<ObjectSignal*>(mainFlowCoordinator)->onDestroy = []() {
                mainFlowCoordinator = nullptr;
            };
    }
    if (!mainFlowCoordinator)
        logger.warn("GetMainFlowCoordinator returning null");
    return mainFlowCoordinator.unsafePtr();
}

MainSystemInit* MetaCore::Game::GetMainSystemInit() {
    static UnityW<MainSystemInit> mainSystemInit;
    if (!mainSystemInit) {
        mainSystemInit = UnityEngine::Resources::FindObjectsOfTypeAll<MainSystemInit*>()->FirstOrDefault();
        if (mainSystemInit)
            Engine::GetOrAddComponent<ObjectSignal*>(mainSystemInit)->onDestroy = []() {
                mainSystemInit = nullptr;
            };
    }
    if (!mainSystemInit)
        logger.warn("GetMainFlowCoordinator returning null");
    return mainSystemInit.unsafePtr();
}
