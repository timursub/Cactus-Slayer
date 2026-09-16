#include "SaveSystem.hpp"
#include "raylib.h"
#include <fstream>
#include <cmath>
#include <algorithm>

void SaveStorageValue(int position, int value) {
    std::ofstream outFile("save.data", std::ios::binary | std::ios::in | std::ios::out);
    if (!outFile.is_open()) {
        outFile.open("save.data", std::ios::binary | std::ios::trunc);
    }
    outFile.seekp(position * sizeof(int));
    outFile.write(reinterpret_cast<const char*>(&value), sizeof(int));
}

int LoadStorageValue(int position) {
    std::ifstream inFile("save.data", std::ios::binary);
    if (!inFile.is_open()) return 0;
    
    inFile.seekg(position * sizeof(int));
    int value = 0;
    inFile.read(reinterpret_cast<char*>(&value), sizeof(int));
    return inFile.gcount() > 0 ? value : 0;
}

void SaveGameData(int highestScore, const PlayerUpgrades& upgrades) {
    SaveData data;
    data.highestScore = highestScore;
    data.upgrades = upgrades;
    SaveFileData("save.data", &data, sizeof(SaveData));
}

void LoadGameData(int& highestScore, PlayerUpgrades& upgrades) {
    if (FileExists("save.data")) {
        int bytesRead = 0;
        unsigned char* fileData = LoadFileData("save.data", &bytesRead);

        if (fileData != NULL && bytesRead == sizeof(SaveData)) {
            SaveData* loadedData = (SaveData*)fileData;
            highestScore = loadedData->highestScore;
            upgrades = loadedData->upgrades;
        }

        UnloadFileData(fileData);
    }
}

int GetStandardUpgradeCost(int currentLvl) {
    int baseCost = 500;
    float multiplier = 1.99f;
    return static_cast<int>(baseCost * std::pow(multiplier, currentLvl));
}

float GetShieldDuration(int lvl) {
    const float durations[] = { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
    return durations[std::min(lvl, 5)];
}

int GetMaxPlayerHp(int lvl) {
    const int hpValues[] = { 3, 4, 5, 6, 7, 8 };
    return hpValues[std::min(lvl, 5)];
}

int GetMushroomHpBonus(int lvl) {
    const int hpBonus[] = { 1, 2, 3 };
    return hpBonus[std::min(lvl, 2)];
}

int GetMushroomCactusKills(int lvl) {
    return std::min(lvl + 1, 3);
}

int GetMushroomSpawnChance(int lvl) {
    const float chances[] = { 2.0f, 2.5f, 3.0f, 3.5f, 4.5f, 5.0f };
    return chances[std::min(lvl, 5)];
}

int GetKillMushroomCost(int lvl) {
    if (lvl == 0) return 4000;
    if (lvl == 1) return 8000;
    return 0;
}

bool IsShieldActive(float shieldEndTime) {
    return GetTime() < shieldEndTime;
}