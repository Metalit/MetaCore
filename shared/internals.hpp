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
    struct State {
        int leftScore;
        int rightScore;
        int leftMaxScore;
        int rightMaxScore;
        int songMaxScore;
        int leftCombo;
        int rightCombo;
        int combo;
        float health;
        float songTime;
        float songLength;
        int notesLeftCut;
        int notesRightCut;
        int notesLeftBadCut;
        int notesRightBadCut;
        int notesLeftMissed;
        int notesRightMissed;
        int bombsLeftHit;
        int bombsRightHit;
        int wallsHit;
        int remainingNotesLeft;
        int remainingNotesRight;
        int songNotesLeft;
        int songNotesRight;
        int leftPreSwing;
        int rightPreSwing;
        int leftPostSwing;
        int rightPostSwing;
        int leftAccuracy;
        int rightAccuracy;
        float leftTimeDependence;
        float rightTimeDependence;
        std::vector<float> leftSpeeds;
        std::vector<float> rightSpeeds;
        std::vector<float> leftAngles;
        std::vector<float> rightAngles;
        bool noFail;
        GlobalNamespace::GameplayModifiers* modifiers;
        float positiveMods;
        float negativeMods;
        int personalBest;
        int fails;
        int restarts;
        GlobalNamespace::ColorScheme* colors;
        GlobalNamespace::BeatmapLevel* beatmapLevel;
        GlobalNamespace::BeatmapKey beatmapKey;
        GlobalNamespace::BeatmapData* beatmapData;
        GlobalNamespace::EnvironmentInfoSO* environment;
        int leftMissedMaxScore;
        int rightMissedMaxScore;
        int leftMissedFixedScore;
        int rightMissedFixedScore;
        UnityEngine::Quaternion prevRotLeft;
        UnityEngine::Quaternion prevRotRight;
        GlobalNamespace::SaberManager* saberManager;
        UnityEngine::Camera* mainCamera;
        std::map<std::string, std::any> modExtraData;
    };

    extern State startingState;
    extern State currentState;
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

#define STATE_GETTER(variable) inline decltype(State::variable)& variable() { return currentState.variable; }

    STATE_GETTER(leftScore);
    STATE_GETTER(rightScore);
    STATE_GETTER(leftMaxScore);
    STATE_GETTER(rightMaxScore);
    STATE_GETTER(songMaxScore);
    STATE_GETTER(leftCombo);
    STATE_GETTER(rightCombo);
    STATE_GETTER(combo);
    STATE_GETTER(health);
    STATE_GETTER(songTime);
    STATE_GETTER(songLength);
    STATE_GETTER(notesLeftCut);
    STATE_GETTER(notesRightCut);
    STATE_GETTER(notesLeftBadCut);
    STATE_GETTER(notesRightBadCut);
    STATE_GETTER(notesLeftMissed);
    STATE_GETTER(notesRightMissed);
    STATE_GETTER(bombsLeftHit);
    STATE_GETTER(bombsRightHit);
    STATE_GETTER(wallsHit);
    STATE_GETTER(remainingNotesLeft);
    STATE_GETTER(remainingNotesRight);
    STATE_GETTER(songNotesLeft);
    STATE_GETTER(songNotesRight);
    STATE_GETTER(leftPreSwing);
    STATE_GETTER(rightPreSwing);
    STATE_GETTER(leftPostSwing);
    STATE_GETTER(rightPostSwing);
    STATE_GETTER(leftAccuracy);
    STATE_GETTER(rightAccuracy);
    STATE_GETTER(leftTimeDependence);
    STATE_GETTER(rightTimeDependence);
    STATE_GETTER(leftSpeeds);
    STATE_GETTER(rightSpeeds);
    STATE_GETTER(leftAngles);
    STATE_GETTER(rightAngles);
    STATE_GETTER(noFail);
    STATE_GETTER(modifiers);
    STATE_GETTER(positiveMods);
    STATE_GETTER(negativeMods);
    STATE_GETTER(personalBest);
    STATE_GETTER(fails);
    STATE_GETTER(restarts);
    STATE_GETTER(colors);
    STATE_GETTER(beatmapLevel);
    STATE_GETTER(beatmapKey);
    STATE_GETTER(beatmapData);
    STATE_GETTER(environment);
    STATE_GETTER(leftMissedMaxScore);
    STATE_GETTER(rightMissedMaxScore);
    STATE_GETTER(leftMissedFixedScore);
    STATE_GETTER(rightMissedFixedScore);
    STATE_GETTER(prevRotLeft);
    STATE_GETTER(prevRotRight);
    STATE_GETTER(saberManager);
    STATE_GETTER(mainCamera);
    STATE_GETTER(modExtraData);

#undef STATE_GETTER
}
