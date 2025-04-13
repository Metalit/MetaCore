#include "stats.hpp"

#include "internals.hpp"
#include "main.hpp"

bool MetaCore::Stats::IsFakeNote(GlobalNamespace::NoteData* data) {
    return data->scoringType == GlobalNamespace::NoteData::ScoringType::NoScore ||
           data->scoringType == GlobalNamespace::NoteData::ScoringType::Ignore;
}

bool MetaCore::Stats::ShouldCountNote(GlobalNamespace::NoteData* data) {
    if (IsFakeNote(data))
        return false;
    return data->gameplayType == GlobalNamespace::NoteData::GameplayType::Normal ||
           data->gameplayType == GlobalNamespace::NoteData::GameplayType::BurstSliderHead;
}

static bool IsLeft(int saber) {
    return saber == MetaCore::Stats::LeftSaber || saber == MetaCore::Stats::BothSabers;
}
static bool IsRight(int saber) {
    return saber == MetaCore::Stats::RightSaber || saber == MetaCore::Stats::BothSabers;
}

#define STATE(var) Internals::currentState.var

int MetaCore::Stats::GetScore(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(leftScore);
    if (IsRight(saber))
        ret += STATE(rightScore);
    return ret;
}

int MetaCore::Stats::GetMaxScore(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(leftMaxScore);
    if (IsRight(saber))
        ret += STATE(rightMaxScore);
    return ret;
}

int MetaCore::Stats::GetSongMaxScore() {
    return STATE(songMaxScore);
}

int MetaCore::Stats::GetCombo(int saber) {
    if (saber == BothSabers)
        return STATE(combo);
    else if (IsLeft(saber))
        return STATE(leftCombo);
    else
        return STATE(rightCombo);
}

bool MetaCore::Stats::GetFullCombo(int saber) {
    if (STATE(wallsHit) > 0)
        return false;
    if (IsLeft(saber) && STATE(bombsLeftHit) + STATE(notesLeftBadCut) + STATE(notesLeftMissed) > 0)
        return false;
    if (IsRight(saber) && STATE(bombsRightHit) + STATE(notesRightBadCut) + STATE(notesRightMissed) > 0)
        return false;
    return true;
}

int MetaCore::Stats::GetMultiplier() {
    if (STATE(combo) < 2)
        return 1;
    if (STATE(combo) < 2 + 4)
        return 2;
    if (STATE(combo) < 2 + 4 + 8)
        return 4;
    return 8;
}

float MetaCore::Stats::GetMultiplierProgress(bool allLevels) {
    if (allLevels)
        return std::min(STATE(combo) / 14.0, 1.0);
    if (STATE(combo) >= 2 + 4 + 8)
        return 1;
    if (STATE(combo) >= 2 + 4)
        return (STATE(combo) - 2 - 4) / 8.0;
    if (STATE(combo) >= 2)
        return (STATE(combo) - 2) / 4.0;
    return STATE(combo) / 2.0;
}

float MetaCore::Stats::GetHealth() {
    return STATE(health);
}

float MetaCore::Stats::GetSongTime() {
    return STATE(songTime);
}

float MetaCore::Stats::GetSongLength() {
    return STATE(songLength);
}

int MetaCore::Stats::GetTotalNotes(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(notesLeftCut) + STATE(notesLeftBadCut) + STATE(notesLeftMissed);
    if (IsRight(saber))
        ret += STATE(notesRightCut) + STATE(notesRightBadCut) + STATE(notesRightMissed);
    return ret;
}

int MetaCore::Stats::GetNotesCut(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(notesLeftCut);
    if (IsRight(saber))
        ret += STATE(notesRightCut);
    return ret;
}

int MetaCore::Stats::GetNotesMissed(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(notesLeftMissed);
    if (IsRight(saber))
        ret += STATE(notesRightMissed);
    return ret;
}

int MetaCore::Stats::GetNotesBadCut(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(notesLeftBadCut);
    if (IsRight(saber))
        ret += STATE(notesRightBadCut);
    return ret;
}

int MetaCore::Stats::GetBombsHit(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(bombsLeftHit);
    if (IsRight(saber))
        ret += STATE(bombsRightHit);
    return ret;
}

int MetaCore::Stats::GetWallsHit() {
    return STATE(wallsHit);
}

int MetaCore::Stats::GetSongNotes(int saber) {
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(songNotesLeft);
    if (IsRight(saber))
        ret += STATE(songNotesRight);
    return ret;
}

float MetaCore::Stats::GetPreSwing(int saber) {
    int notes = GetNotesCut(saber);
    if (notes == 0)
        return 0;
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(leftPreSwing);
    if (IsRight(saber))
        ret += STATE(rightPreSwing);
    return ret / (float) notes;
}

float MetaCore::Stats::GetPostSwing(int saber) {
    int notes = GetNotesCut(saber);
    if (notes == 0)
        return 0;
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(leftPostSwing);
    if (IsRight(saber))
        ret += STATE(rightPostSwing);
    return ret / (float) notes;
}

float MetaCore::Stats::GetAccuracy(int saber) {
    int notes = GetNotesCut(saber);
    if (notes == 0)
        return 0;
    int ret = 0;
    if (IsLeft(saber))
        ret += STATE(leftAccuracy);
    if (IsRight(saber))
        ret += STATE(rightAccuracy);
    return ret / (float) notes;
}

float MetaCore::Stats::GetTimeDependence(int saber) {
    int notes = GetNotesCut(saber);
    if (notes == 0)
        return 0;
    float ret = 0;
    if (IsLeft(saber))
        ret += STATE(leftTimeDependence);
    if (IsRight(saber))
        ret += STATE(rightTimeDependence);
    return ret / notes;
}

float MetaCore::Stats::GetAverageSpeed(int saber) {
    float ret = 0;
    int div = 0;
    if (IsLeft(saber)) {
        for (auto& val : STATE(leftSpeeds))
            ret += val;
        div += STATE(leftSpeeds).size();
    }
    if (IsRight(saber)) {
        for (auto& val : STATE(rightSpeeds))
            ret += val;
        div += STATE(rightSpeeds).size();
    }
    if (div == 0)
        return 0;
    return ret / div;
}

float MetaCore::Stats::GetBestSpeed5Secs(int saber) {
    float ret = 0;
    int back = SLOW_UPDATES_PER_SEC * 5;
    if (IsLeft(saber)) {
        int size = STATE(leftSpeeds).size();
        int start = std::max(0, size - back);
        for (int i = start; i < size; i++)
            ret = std::max(ret, STATE(leftSpeeds)[i]);
    }
    if (IsRight(saber)) {
        int size = STATE(rightSpeeds).size();
        int start = std::max(0, size - back);
        for (int i = start; i < size; i++)
            ret = std::max(ret, STATE(rightSpeeds)[i]);
    }
    return ret;
}

float MetaCore::Stats::GetLastSecAngle(int saber) {
    float ret = 0;
    int back = SLOW_UPDATES_PER_SEC;
    if (IsLeft(saber)) {
        int size = STATE(leftAngles).size();
        int start = std::max(0, size - back);
        for (int i = start; i < size; i++)
            ret += STATE(leftAngles)[i];
    }
    if (IsRight(saber)) {
        int size = STATE(rightAngles).size();
        int start = std::max(0, size - back);
        for (int i = start; i < size; i++)
            ret += STATE(rightAngles)[i];
    }
    if (saber == (int) BothSabers)
        ret /= 2;
    return ret;
}

float MetaCore::Stats::GetHighestSecAngle(int saber) {
    float ret = 0;
    if (IsLeft(saber)) {
        for (auto& val : STATE(leftAngles))
            ret = std::max(ret, val);
    }
    if (IsRight(saber)) {
        for (auto& val : STATE(rightAngles))
            ret = std::max(ret, val);
    }
    return ret * SLOW_UPDATES_PER_SEC;
}

float MetaCore::Stats::GetModifierMultiplier(bool positive, bool negative) {
    float ret = 1;
    if (positive)
        ret += STATE(positiveMods);
    if (negative)
        ret += STATE(negativeMods);
    return ret;
}

int MetaCore::Stats::GetBestScore() {
    return STATE(personalBest);
}

int MetaCore::Stats::GetFails() {
    return STATE(fails);
}

int MetaCore::Stats::GetRestarts() {
    return STATE(restarts);
}

int MetaCore::Stats::GetFCScore(int saber) {
    float swingRatio = (GetPreSwing(saber) + GetPostSwing(saber) + GetAccuracy(saber)) / 115;
    int missed = 0;
    int fixed = 0;
    if (IsLeft(saber)) {
        missed += STATE(leftMissedMaxScore);
        fixed += STATE(leftMissedFixedScore);
    }
    if (IsRight(saber)) {
        missed += STATE(rightMissedMaxScore);
        fixed += STATE(rightMissedFixedScore);
    }
    return (missed * swingRatio) + fixed + GetScore(saber);
}
