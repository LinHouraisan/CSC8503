#pragma once

#include "BaseLevel.h"

#include "GameObject.h"
#include "GameTechRenderer.h"
#include "PhysicsSystem.h"
#include "KeyboardMouseController.h"

namespace NCL {
namespace CSC8503 {

class LevelController;
class LevelCover : public BaseLevel {
public:
    enum Selection {
        SINGLE_GAME,
        NETWORK_GAME,
        TUTORIAL_GAME,
        SETTINGS,
        QUIT,
        END,
    };

    LevelCover(LevelController *levelController);
    ~LevelCover();

    virtual void UpdateGame(float dt) override;

protected:
    LevelController *levelController;
    Selection currSelection;

    GameTechRenderer *renderer;
    GameWorld *world;

    KeyboardMouseController controller;
};
} // namespace CSC8503
} // namespace NCL