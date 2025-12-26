#include "LevelCover.h"

#include "LevelController.h"
#include "RenderObject.h"
#include "PhysicsObject.h"

#include "Debug.h"
#include "Vector.h"

using namespace NCL;
using namespace CSC8503;

LevelCover::LevelCover(LevelController *levelController)
    : levelController(levelController),
      controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()),
      currSelection(Selection::SINGLE_GAME) {
    world = new GameWorld(true);
    renderer = new GameTechRenderer(*world);

    world->GetMainCamera().SetController(controller);

    Window::GetWindow()->ShowOSPointer(true);
    Window::GetWindow()->LockMouseToWindow(false);
}

LevelCover::~LevelCover() {
    delete renderer;
    delete world;
}

void LevelCover::UpdateGame(float dt) {
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::UP)) {
        if (currSelection == Selection::SINGLE_GAME) {
            currSelection = static_cast<Selection>(Selection::END);
        }
        currSelection = static_cast<Selection>(currSelection - 1);
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::DOWN)) {
        currSelection = static_cast<Selection>(currSelection + 1);
        if (currSelection == Selection::END) {
            currSelection = static_cast<Selection>(Selection::SINGLE_GAME);
        }
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN)) {
        switch (currSelection) {
        case Selection::SINGLE_GAME:
            levelController->SwitchLevel(LevelController::LEVEL_GAME);
            break;
        case Selection::NETWORK_GAME:
            levelController->SwitchLevel(LevelController::LEVEL_NETWORK_GAME);
            break;
        case Selection::TUTORIAL_GAME:
            levelController->SwitchLevel(LevelController::LEVEL_TUTORIAL);
            break;
        case Selection::SETTINGS:
            levelController->SwitchLevel(LevelController::LEVEL_SETTINGS);
            break;
        case Selection::QUIT:
            levelController->SetQuitFlag();
            break;
        }
    }

    Debug::Print("=== PAC-MAN GAME ===", Vector2(30, 20), Debug::RED);
    std::string s;

    float xPosDiff = 0;
    const float X_DIFF = 2;
    float yPos = 30;
    const float yInterval = 10;
    {
        s = "Single Game";
        if (currSelection == Selection::SINGLE_GAME) {
            s = ">" + s + "<";
            xPosDiff -= X_DIFF;
        }
        Debug::Print(s, Vector2(40 + xPosDiff, yPos), Debug::RED);
        yPos += yInterval;
    }
    {
        s = "Network Game";
        if (currSelection == Selection::NETWORK_GAME) {
            s = ">" + s + "<";
            xPosDiff -= X_DIFF;
        }
        Debug::Print(s, Vector2(38 + xPosDiff, yPos), Debug::RED);
        yPos += yInterval;
    }
    {
        s = "Tutorial Game";
        if (currSelection == Selection::TUTORIAL_GAME) {
            s = ">" + s + "<";
            xPosDiff -= X_DIFF;
        }
        Debug::Print(s, Vector2(38 + xPosDiff, yPos), Debug::RED);
        yPos += yInterval;
    }
    {
        s = "Settings";
        if (currSelection == Selection::SETTINGS) {
            s = ">" + s + "<";
            xPosDiff -= X_DIFF;
        }
        Debug::Print(s, Vector2(42 + xPosDiff, yPos), Debug::RED);
        yPos += yInterval;
    }
    {
        s = "Quit";
        if (currSelection == Selection::QUIT) {
            s = ">" + s + "<";
            xPosDiff -= X_DIFF;
        }
        Debug::Print(s, Vector2(45 + xPosDiff, yPos), Debug::RED);
        yPos += yInterval;
    }

    renderer->Render();
    Debug::UpdateRenderables(dt);
}
