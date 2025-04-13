#pragma once

#include "GlobalNamespace/NoteData.hpp"

namespace MetaCore::Stats {
    /// @brief Checks if a note is a fake note, such as from noodle extensions
    /// @param data The note data
    /// @return If the note is fake
    bool IsFakeNote(GlobalNamespace::NoteData* data);
    /// @brief Checks if a note should be counted in calculations involving swing parts
    /// @param data The note data
    /// @return If the note is fake
    bool ShouldCountNote(GlobalNamespace::NoteData* data);

    constexpr int LeftSaber = 0;
    constexpr int RightSaber = 1;
    constexpr int BothSabers = 2;

    // no per-function documentation here, sorry

    int GetScore(int saber);
    int GetMaxScore(int saber);
    int GetSongMaxScore();
    int GetCombo(int saber);
    bool GetFullCombo(int saber);
    int GetMultiplier();
    float GetMultiplierProgress(bool allLevels);
    float GetHealth();
    float GetSongTime();
    float GetSongLength();
    int GetTotalNotes(int saber);
    int GetNotesCut(int saber);
    int GetNotesMissed(int saber);
    int GetNotesBadCut(int saber);
    int GetBombsHit(int saber);
    int GetWallsHit();
    int GetSongNotes(int saber);
    float GetPreSwing(int saber);
    float GetPostSwing(int saber);
    float GetAccuracy(int saber);
    float GetTimeDependence(int saber);
    float GetAverageSpeed(int saber);
    float GetBestSpeed5Secs(int saber);
    float GetLastSecAngle(int saber);
    float GetHighestSecAngle(int saber);
    float GetModifierMultiplier(bool positive, bool negative);
    int GetBestScore();
    int GetFails();
    int GetRestarts();
    int GetFCScore(int saber);
}
