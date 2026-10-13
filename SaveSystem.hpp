#ifndef SAVE_SYSTEM_HPP
#define SAVE_SYSTEM_HPP

#include "Types.hpp"

// Disk I/O storage functions
void SaveStorageValue(int position, int value);
int LoadStorageValue(int position);

void SaveGameData(int highestScore, const PlayerUpgrades& upgrades);
void LoadGameData(int& highestScore, PlayerUpgrades& upgrades);

// Upgrades progression stats scaling
int GetStandardUpgradeCost(int currentLvl);
float GetShieldDuration(int lvl);
int GetMaxPlayerHp(int lvl);
int GetMushroomHpBonus(int lvl);
int GetMushroomCactusKills(int lvl);
float GetMushroomSpawnChance(int lvl);
int GetKillMushroomCost(int lvl);
bool IsShieldActive(float shieldEndTime);

#endif // SAVE_SYSTEM_HPP