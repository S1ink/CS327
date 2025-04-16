#include "game.hpp"


void GameState::initRuntimeArgs(uint32_t seed, int nmon)
{
    this->state.seed = seed;
    this->state.nmon = nmon;

    this->procedural.rgen.seed(seed);
}

bool GameState::initMonDescriptions(std::istream& i)
{
    return MonDescription::parse(i, this->mon_desc);
}

bool GameState::initItemDescriptions(std::istream& i)
{
    return ItemDescription::parse(i, this->item_desc);
}

bool GameState::initDungeonFile(FILE* f)
{
    this->level.setSeed(this->nextSeed());
    this->level.loadTerrain(f);
    this->initializeEntities();
}

bool GameState::initDungeonRandom()
{
    this->level.setSeed(this->nextSeed());
    this->level.generateTerrain();
    this->initializeEntities();
}

bool GameState::exportDungeonFile(FILE* f)
{
    return !this->level.saveTerrain(f);
}

bool GameState::initializeEntities()
{

}
