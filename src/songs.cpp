#include "songs.hpp"

#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/IPreviewMediaData.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelCollectionViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "System/Threading/Tasks/Task.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "game.hpp"
#include "internals.hpp"
#include "main.hpp"
#include "types.hpp"

using namespace GlobalNamespace;

std::string MetaCore::Songs::GetHash(std::string levelId) {
    auto prefixIndex = levelId.find("custom_level_");
    if (prefixIndex == std::string::npos)
        return "";
    // remove prefix
    levelId = levelId.substr(prefixIndex + 13);
    auto wipIndex = levelId.find(" WIP");
    if (wipIndex != std::string::npos)
        levelId = levelId.substr(0, wipIndex);
    // make lowercase?
    return levelId;
}

std::string MetaCore::Songs::GetHash(BeatmapKey beatmap) {
    return GetHash(beatmap.levelId);
}

std::string MetaCore::Songs::GetHash(BeatmapLevel* beatmap) {
    return GetHash(beatmap->levelID);
}

static std::map<std::string, std::vector<std::function<void(IReadonlyBeatmapData*)>>> dataRequests;

void MetaCore::Songs::GetBeatmapData(BeatmapKey beatmap, std::function<void(IReadonlyBeatmapData*)> callback) {
    logger.debug("loading beatmap data for {} {} {}", beatmap.levelId, beatmap.beatmapCharacteristic->_serializedName, (int) beatmap.difficulty);

    std::string name = beatmap.SerializedName();
    if (dataRequests.contains(name)) {
        dataRequests[name].emplace_back(std::move(callback));
        return;
    }
    dataRequests.emplace(name, std::vector({std::move(callback)}));

    // I have no idea what BeatmapLevelDataVersion is for
    auto levelDataTask =
        Game::GetMenuTransitionsHelper()->_beatmapLevelsModel->LoadBeatmapLevelDataAsync(beatmap.levelId, BeatmapLevelDataVersion::Original, nullptr);

    MainThreadScheduler::Await(levelDataTask, [beatmap, name, levelDataTask]() {
        if (levelDataTask->ResultOnSuccess.isError) {
            logger.warn("failed to load beatmap data");
            for (auto& callback : dataRequests[name])
                callback(nullptr);
            dataRequests.erase(name);
        } else {
            logger.debug("got beatmap level data");
            auto beatmapDataTask = Game::GetMenuTransitionsHelper()->_beatmapDataLoader->LoadBeatmapDataAsync(
                levelDataTask->ResultOnSuccess.beatmapLevelData,
                beatmap,
                FindLevel(beatmap)->beatsPerMinute,
                false,
                nullptr,
                nullptr,
                BeatmapLevelDataVersion::Original,
                nullptr,
                nullptr,
                true
            );
            MainThreadScheduler::Await(beatmapDataTask, [beatmapDataTask, name]() {
                for (auto& callback : dataRequests[name])
                    callback(beatmapDataTask->ResultOnSuccess);
                dataRequests.erase(name);
            });
        }
    });
}

void MetaCore::Songs::GetSongCover(BeatmapLevel* beatmap, std::function<void(UnityEngine::Sprite*)> callback) {
    auto task = beatmap->previewMediaData->GetCoverSpriteAsync();
    MainThreadScheduler::Await(task, [task, callback = std::move(callback)]() { callback(task->ResultOnSuccess); });
}

// avoid back and forth string -> StringW conversions
static BeatmapLevel* FindLevelInternal(StringW levelId) {
    return MetaCore::Game::GetMenuTransitionsHelper()->_beatmapLevelsModel->GetBeatmapLevel(levelId);
}

BeatmapLevel* MetaCore::Songs::FindLevel(std::string levelId) {
    return FindLevelInternal(levelId);
}

BeatmapLevel* MetaCore::Songs::FindLevel(BeatmapKey beatmap) {
    return FindLevelInternal(beatmap.levelId);
}

BeatmapKey MetaCore::Songs::GetSelectedKey(bool last) {
    return Internals::selectedKey;
}

BeatmapLevel* MetaCore::Songs::GetSelectedLevel(bool last) {
    return Internals::selectedLevel;
}

void MetaCore::Songs::PlayLevelPreview(BeatmapLevel* beatmap) {
    // not sure if this is ok lol, might be better to cache a resources call
    Game::GetMainFlowCoordinator()
        ->_soloFreePlayFlowCoordinator->levelSelectionNavigationController->_levelCollectionNavigationController->_levelCollectionViewController
        ->SongPlayerCrossfadeToLevelAsync(beatmap, nullptr);
}
