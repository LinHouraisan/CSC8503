#pragma once

#include "BaseLevel.h"

#include "GameObject.h"
#include "GameTechRenderer.h"
#include "PhysicsSystem.h"
#include "KeyboardMouseController.h"

#include <functional>

namespace NCL {
namespace CSC8503 {

class LevelController;
class LevelSettings : public BaseLevel {
public:
    struct Selection {
        std::function<std::string()> fnGetTitle;
        std::function<void()> fn;
    };

    LevelSettings(LevelController *levelController);
    ~LevelSettings();

    virtual void UpdateGame(float dt) override;

protected:
    LevelController *levelController;
    std::vector<Selection> selections;
    int currSelection;

    GameTechRenderer *renderer;
    GameWorld *world;

    KeyboardMouseController controller;
};
} // namespace CSC8503
} // namespace NCL