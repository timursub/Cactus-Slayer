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

// Snaps raw numbers to clean, readable values
int MakePriceBeautiful(double rawPrice) {
    int price = static_cast<int>(rawPrice);

    if (price < 100)    return price;                     // 0 - 99: Exact
    if (price < 1000)   return (price / 10) * 10;         // 100 - 999: Round to nearest 10 (e.g. 520)
    if (price < 10000)  return (price / 50) * 50;         // 1,000 - 9,999: Round to nearest 50 (e.g. 3,150)
    if (price < 100000) return (price / 500) * 500;       // 10,000 - 99,999: Round to nearest 500 (e.g. 19,500)
    
    return (price / 5000) * 5000;                         // 100,000+: Round to nearest 5,000
}

int GetStandardUpgradeCost(int currentLvl) {
    int baseCost = 500;
    float multiplier = 2.50f;
    double rawCost = baseCost * std::pow(multiplier, currentLvl);
    return MakePriceBeautiful(rawCost);
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

float GetMushroomSpawnChance(int lvl) {
    const float chances[] = { 2.0f, 2.5f, 3.0f, 3.5f, 4.5f, 5.0f };
    return chances[std::min(lvl, 5)];
}

int GetKillMushroomCost(int lvl) {
    if (lvl == 0) return 4000;
    if (lvl == 1) return 12000;
    return 0;
}

bool IsShieldActive(float shieldEndTime) {
    return GetTime() < shieldEndTime;
}