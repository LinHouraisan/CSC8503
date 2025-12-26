#include "LevelController.h"

#include "LevelCover.h"
#include "LevelSettings.h"
#include "TutorialGame.h"
#include "LevelGame.h"

namespace NCL {
namespace CSC8503 {
LevelController::LevelController() : shouldQuit(false), levelAct(Level::LEVEL_COVER) {}

void LevelController::UpdateGame(float dt) {
    switch (levelAct) {
    case Level::LEVEL_COVER:
        currLevel = std::unique_ptr<BaseLevel>(new LevelCover(this));
        levelAct = Level::NO_ACT;
        break;
    case Level::LEVEL_SETTINGS:
        currLevel = std::unique_ptr<BaseLevel>(new LevelSettings(this));
        levelAct = Level::NO_ACT;
        break;
    case Level::LEVEL_GAME:
        currLevel = std::unique_ptr<BaseLevel>(new LevelGame(LevelGame::GameMode::SINGLE_PLAYER));
        levelAct = Level::NO_ACT;
        break;
    case Level::LEVEL_NETWORK_GAME:
        currLevel = std::unique_ptr<BaseLevel>(new LevelGame(LevelGame::GameMode::MULTI_PLAYER));
        levelAct = Level::NO_ACT;
        break;
    case Level::LEVEL_TUTORIAL:
        currLevel = std::unique_ptr<BaseLevel>(new TutorialGame);
        levelAct = Level::NO_ACT;
        break;
    }

    currLevel->UpdateGame(dt);
}

void LevelController::SetQuitFlag() {
    shouldQuit = true;
}

bool LevelController::ShouldQuit() const {
    return shouldQuit;
}

void LevelController::SwitchLevel(Level toLevel) {
    levelAct = toLevel;
}
} // namespace CSC8503
} // namespace NCL