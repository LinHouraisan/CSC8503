#pragma once

#include "TutorialGame.h"
#include "BaseLevel.h"

#include <memory>

namespace NCL {
namespace CSC8503 {
class LevelController {
public:
    enum Level {
        NO_ACT,
        LEVEL_COVER,
        LEVEL_SETTINGS,
        LEVEL_GAME,
        LEVEL_NETWORK_GAME,
        LEVEL_TUTORIAL,
    };

    LevelController();

    void UpdateGame(float dt);

    void SetQuitFlag();

    bool ShouldQuit() const;

    void SwitchLevel(Level toLevel);

private:
    bool shouldQuit;
    Level levelAct;
    std::unique_ptr<BaseLevel> currLevel;
};
} // namespace CSC8503
} // namespace NCL