#pragma once

#include <any>

#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapKey.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/SaberManager.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Quaternion.hpp"

// no per-variable documentation here, sorry

namespace MetaCore::Internals {
    extern int leftScore;
    extern int rightScore;
    extern int leftMaxScore;
    extern int rightMaxScore;
    extern int songMaxScore;
    extern int leftCombo;
    extern int rightCombo;
    extern int combo;
    extern float health;
    extern float songTime;
    extern float songLength;
    extern int notesLeftCut;
    extern int notesRightCut;
    extern int notesLeftBadCut;
    extern int notesRightBadCut;
    extern int notesLeftMissed;
    extern int notesRightMissed;
    extern int bombsLeftHit;
    extern int bombsRightHit;
    extern int wallsHit;
    extern int remainingNotesLeft;
    extern int remainingNotesRight;
    extern int songNotesLeft;
    extern int songNotesRight;
    extern int leftPreSwing;
    extern int rightPreSwing;
    extern int leftPostSwing;
    extern int rightPostSwing;
    extern int leftAccuracy;
    extern int rightAccuracy;
    extern float leftTimeDependence;
    extern float rightTimeDependence;
    extern std::vector<float> leftSpeeds;
    extern std::vector<float> rightSpeeds;
    extern std::vector<float> leftAngles;
    extern std::vector<float> rightAngles;
    extern bool noFail;
    extern GlobalNamespace::GameplayModifiers* modifiers;
    extern float positiveMods;
    extern float negativeMods;
    extern int personalBest;
    extern int fails;
    extern int restarts;
    extern GlobalNamespace::ColorScheme* colors;
    extern GlobalNamespace::BeatmapLevel* beatmapLevel;
    extern GlobalNamespace::BeatmapKey beatmapKey;
    extern GlobalNamespace::BeatmapData* beatmapData;
    extern GlobalNamespace::EnvironmentInfoSO* environment;
    extern int leftMissedMaxScore;
    extern int rightMissedMaxScore;
    extern int leftMissedFixedScore;
    extern int rightMissedFixedScore;
    extern UnityEngine::Quaternion prevRotLeft;
    extern UnityEngine::Quaternion prevRotRight;
    extern GlobalNamespace::SaberManager* saberManager;
    extern UnityEngine::Camera* mainCamera;

    extern bool stateValid;
    extern bool mapWasQuit;

    void Initialize();
    void DoSlowUpdate();
    void Finish(bool quit);

    extern GlobalNamespace::BeatmapKey selectedKey;
    extern GlobalNamespace::BeatmapLevel* selectedLevel;
    extern GlobalNamespace::BeatmapLevelPack* selectedPlaylist;
    extern bool isLevelSelected;
    extern bool isPlaylistSelected;

    void SetLevel(GlobalNamespace::BeatmapKey key, GlobalNamespace::BeatmapLevel* level);
    void ClearLevel();
    void SetPlaylist(GlobalNamespace::BeatmapLevelPack* playlist);
    void ClearPlaylist();

    void SetEndDragUI(UnityEngine::Component* component, std::function<void()> callback);
    std::function<void()> SetKeyboardCloseUI(UnityEngine::Component* component, std::function<void()> onClosed, std::function<void()> onOk);

    bool IsAprilFirst();
}
