#include "internals.hpp"

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/BeatmapCallbacksController.hpp"
#include "GlobalNamespace/BeatmapCallbacksUpdater.hpp"
#include "GlobalNamespace/BeatmapDataSortedListForTypeAndIds_1.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/GameplayModifierParamsSO.hpp"
#include "GlobalNamespace/GameplayModifiersModelSO.hpp"
#include "GlobalNamespace/IGameEnergyCounter.hpp"
#include "GlobalNamespace/ISortedList_1.hpp"
#include "GlobalNamespace/PlayerAllOverallStatsData.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Collections/Generic/LinkedList_1.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Transform.hpp"
#include "events.hpp"
#include "main.hpp"
#include "stats.hpp"
#include "types.hpp"
#include "unity.hpp"

using namespace GlobalNamespace;
using namespace MetaCore;
using namespace UnityEngine;

static std::string lastBeatmap;
static float timeSinceSlowUpdate;

static std::pair<int, int> GetNoteCount(BeatmapCallbacksUpdater* updater, bool left) {
    using LinkedList = System::Collections::Generic::LinkedList_1<NoteData*>;

    if (!updater)
        return {0, 0};

    int noteCount = 0;
    int totalCount = 0;

    auto bcc = updater->_beatmapCallbacksController;
    auto songTime = bcc->_startFilterTime;

    auto data = il2cpp_utils::try_cast<BeatmapData>(bcc->_beatmapData).value_or(nullptr);
    if (!data) {
        logger.warn("IReadonlyBeatmapData was {} not BeatmapData", il2cpp_functions::class_get_name(((Il2CppObject*) bcc->_beatmapData)->klass));
        return {0, 0};
    }

    auto noteDataItemsList = (LinkedList*) data->_beatmapDataItemsPerTypeAndId->GetList(csTypeOf(NoteData*), 0)->items;
    auto enumerator = noteDataItemsList->GetEnumerator();
    while (enumerator.MoveNext()) {
        auto noteData = (NoteData*) enumerator.Current;
        if (Stats::ShouldCountNote(noteData) && (noteData->colorType == ColorType::ColorA) == left) {
            totalCount++;
            if (noteData->time >= songTime)
                noteCount++;
        }
    }
    return {noteCount, totalCount};
}

static int GetMaxScore(BeatmapCallbacksUpdater* updater) {
    if (!updater)
        return 0;
    return ScoreModel::ComputeMaxMultipliedScoreForBeatmap(updater->_beatmapCallbacksController->_beatmapData);
}

static float GetSongLength(ScoreController* controller) {
    if (!controller)
        return 0;
    return controller->_audioTimeSyncController->_initData->audioClip->length;
}

static int GetFailCount(PlayerDataModel* data) {
    if (!data)
        return 0;
    return data->playerData->playerAllOverallStatsData->get_allOverallStatsData()->failedLevelsCount;
}

static float GetPositiveMods(ScoreController* controller) {
    if (!controller || !controller->_gameplayModifierParams)
        return 0;
    float ret = 0;
    auto mods = ListW<GameplayModifierParamsSO*>(controller->_gameplayModifierParams);
    for (auto& mod : mods) {
        float mult = mod->multiplier;
        if (mult > 0)
            ret += mult;
    }
    return ret;
}

static float GetNegativeMods(ScoreController* controller) {
    if (!controller || !controller->_gameplayModifierParams)
        return 0;
    float ret = 0;
    auto mods = ListW<GameplayModifierParamsSO*>(controller->_gameplayModifierParams);
    for (auto& mod : mods) {
        if (mod == controller->_gameplayModifiersModel->_noFailOn0Energy.ptr()) {
            Internals::startingState.noFail = true;
            continue;
        }
        float mult = mod->multiplier;
        if (mult < 0)
            ret += mult;
    }
    return ret;
}

static int GetHighScore(PlayerDataModel* data, GameplayCoreInstaller* installer) {
    if (!data || !installer || !installer->_sceneSetupData)
        return -1;
    auto beatmap = installer->_sceneSetupData->beatmapKey;
    if (!data->playerData->levelsStatsData->ContainsKey(beatmap))
        return -1;
    return data->playerData->levelsStatsData->get_Item(beatmap)->highScore;
}

static float GetHealth(ScoreController* controller) {
    if (!controller)
        return 1;
    return controller->_gameEnergyCounter->energy;
}

Internals::State Internals::startingState;
Internals::State Internals::currentState;
bool Internals::stateValid = false;
bool Internals::mapWasQuit = false;

void Internals::Initialize() {
    auto beatmapCallbacksUpdater = Object::FindObjectOfType<BeatmapCallbacksUpdater*>(true);
    auto scoreController = Object::FindObjectOfType<ScoreController*>(true);
    auto playerDataModel = Object::FindObjectOfType<PlayerDataModel*>(true);

    auto gameplayCoreInstallers = Resources::FindObjectsOfTypeAll<GameplayCoreInstaller*>();
    GameplayCoreInstaller* gameplayCoreInstaller;
    for (auto& installer : gameplayCoreInstallers) {
        if (installer->isActiveAndEnabled && installer->_sceneSetupData != nullptr) {
            gameplayCoreInstaller = installer;
            break;
        }
    }
    if (!gameplayCoreInstaller)
        gameplayCoreInstaller = gameplayCoreInstallers->FirstOrDefault();
    auto setupData = gameplayCoreInstaller ? gameplayCoreInstaller->_sceneSetupData : nullptr;

    startingState.leftScore = 0;
    startingState.rightScore = 0;
    startingState.leftMaxScore = 0;
    startingState.rightMaxScore = 0;
    startingState.songMaxScore = GetMaxScore(beatmapCallbacksUpdater);
    startingState.leftCombo = 0;
    startingState.rightCombo = 0;
    startingState.combo = 0;
    startingState.health = GetHealth(scoreController);
    startingState.songTime = 0;
    startingState.songLength = GetSongLength(scoreController);
    startingState.notesLeftCut = 0;
    startingState.notesRightCut = 0;
    startingState.notesLeftBadCut = 0;
    startingState.notesRightBadCut = 0;
    startingState.notesLeftMissed = 0;
    startingState.notesRightMissed = 0;
    startingState.bombsLeftHit = 0;
    startingState.bombsRightHit = 0;
    startingState.wallsHit = 0;
    auto pair = GetNoteCount(beatmapCallbacksUpdater, true);
    startingState.remainingNotesLeft = pair.first;
    startingState.songNotesLeft = pair.second;
    pair = GetNoteCount(beatmapCallbacksUpdater, false);
    startingState.remainingNotesRight = pair.first;
    startingState.songNotesRight = pair.second;
    startingState.leftPreSwing = 0;
    startingState.rightPreSwing = 0;
    startingState.leftPostSwing = 0;
    startingState.rightPostSwing = 0;
    startingState.leftAccuracy = 0;
    startingState.rightAccuracy = 0;
    startingState.leftTimeDependence = 0;
    startingState.rightTimeDependence = 0;
    startingState.leftSpeeds = {};
    startingState.rightSpeeds = {};
    startingState.leftAngles = {};
    startingState.rightAngles = {};
    startingState.noFail = false;
    startingState.modifiers = scoreController ? scoreController->_gameplayModifiers : nullptr;
    // GetNegativeMods sets noFail
    startingState.positiveMods = GetPositiveMods(scoreController);
    startingState.negativeMods = GetNegativeMods(scoreController);
    startingState.personalBest = GetHighScore(playerDataModel, gameplayCoreInstaller);
    startingState.fails = GetFailCount(playerDataModel);

    logger.debug("modifiers {} -{}", startingState.positiveMods, startingState.negativeMods);

    std::string beatmap = "Unknown";
    if (setupData)
        beatmap = (std::string) setupData->beatmapKey.SerializedName();
    if (beatmap != lastBeatmap)
        startingState.restarts = 0;
    else
        startingState.restarts++;
    lastBeatmap = beatmap;
    timeSinceSlowUpdate = 0;

    startingState.colors = setupData ? setupData->colorScheme : nullptr;
    startingState.beatmapLevel = setupData ? setupData->beatmapLevel : nullptr;
    startingState.beatmapKey = setupData ? setupData->beatmapKey : BeatmapKey();
    startingState.beatmapData = beatmapCallbacksUpdater ? (BeatmapData*) beatmapCallbacksUpdater->_beatmapCallbacksController->_beatmapData : nullptr;
    startingState.environment = setupData ? setupData->targetEnvironmentInfo : nullptr;

    startingState.leftMissedMaxScore = 0;
    startingState.rightMissedMaxScore = 0;
    startingState.leftMissedFixedScore = 0;
    startingState.rightMissedFixedScore = 0;

    startingState.prevRotLeft = Quaternion::get_identity();
    startingState.prevRotRight = Quaternion::get_identity();
    startingState.saberManager = Object::FindObjectOfType<SaberManager*>(true);
    startingState.mainCamera = Camera::get_main();
    startingState.modExtraData.clear();

    currentState = startingState;
    stateValid = true;
}

void Internals::DoSlowUpdate() {
    if (!stateValid)
        return;

    timeSinceSlowUpdate += Time::get_deltaTime();
    if (timeSinceSlowUpdate > 1 / (float) SLOW_UPDATES_PER_SEC) {
        if (auto saberManager = currentState.saberManager; saberManager && saberManager->leftSaber && saberManager->rightSaber) {
            currentState.leftSpeeds.emplace_back(saberManager->leftSaber->bladeSpeed);
            currentState.rightSpeeds.emplace_back(saberManager->rightSaber->bladeSpeed);

            auto rotLeft = saberManager->leftSaber->transform->rotation;
            auto rotRight = saberManager->rightSaber->transform->rotation;
            // use speeds array as tracker for if prevRots have accurate values
            if (currentState.leftSpeeds.size() > 1) {
                currentState.leftAngles.emplace_back(Quaternion::Angle(rotLeft, currentState.prevRotLeft));
                currentState.rightAngles.emplace_back(Quaternion::Angle(rotRight, currentState.prevRotRight));
            }
            currentState.prevRotLeft = rotLeft;
            currentState.prevRotRight = rotRight;
        }
        timeSinceSlowUpdate = 0;
        Events::Broadcast(Events::SlowUpdate);
    }
}

void Internals::Finish(bool quit) {
    stateValid = false;
    mapWasQuit = quit;
}

BeatmapKey Internals::selectedKey = {};
BeatmapLevel* Internals::selectedLevel = nullptr;
BeatmapLevelPack* Internals::selectedPlaylist = nullptr;
bool Internals::isLevelSelected = false;
bool Internals::isPlaylistSelected = false;

void Internals::SetLevel(BeatmapKey key, BeatmapLevel* level) {
    logger.debug("set level {}", level ? level->songName : "none");
    if (!level || !key.IsValid()) {
        ClearLevel();
        return;
    }
    if (isLevelSelected && level == selectedLevel && key.Equals(selectedKey))
        return;
    selectedKey = key;
    selectedLevel = level;
    isLevelSelected = true;
    Events::Broadcast(Events::MapSelected);
}

void Internals::ClearLevel() {
    logger.debug("clear level");
    isLevelSelected = false;
    Events::Broadcast(Events::MapDeselected);
}

void Internals::SetPlaylist(BeatmapLevelPack* playlist) {
    logger.debug("set playlist {}", playlist ? playlist->packName : "none");
    if (!playlist) {
        ClearLevel();
        return;
    }
    if (isPlaylistSelected && playlist == selectedPlaylist)
        return;
    selectedPlaylist = playlist;
    isPlaylistSelected = true;
    Events::Broadcast(Events::PlaylistSelected);
}

void Internals::ClearPlaylist() {
    logger.debug("clear playlist");
    isPlaylistSelected = false;
    Events::Broadcast(Events::PlaylistDeselected);
}

void Internals::SetEndDragUI(Component* component, std::function<void()> callback) {
    Engine::GetOrAddComponent<EndDragHandler*>(component)->callback = std::move(callback);
}

std::function<void()> Internals::SetKeyboardCloseUI(Component* component, std::function<void()> onClosed, std::function<void()> onOk) {
    auto handler = Engine::GetOrAddComponent<KeyboardCloseHandler*>(component);
    handler->closeCallback = std::move(onClosed);
    handler->okCallback = std::move(onOk);
    return [handler]() {
        if (handler->okCallback)
            handler->okCallback();
        else if (handler->closeCallback)
            handler->closeCallback();
    };
}

bool Internals::IsAprilFirst() {
    auto time = std::time(nullptr);
    auto info = std::localtime(&time);
    if (!info)
        return false;
    return info->tm_mon == 3 && info->tm_mday == 1;
}
