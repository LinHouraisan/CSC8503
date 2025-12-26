#include "LevelSettings.h"

#include "LevelController.h"
#include "RenderObject.h"
#include "PhysicsObject.h"

#include "Debug.h"
#include "Vector.h"

#include "Config.h"

using namespace NCL;
using namespace CSC8503;

LevelSettings::LevelSettings(LevelController *levelController)
    : levelController(levelController),
      controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()), currSelection(0) {
    world = new GameWorld(true);
    renderer = new GameTechRenderer(*world);

    world->GetMainCamera().SetController(controller);

    Window::GetWindow()->ShowOSPointer(true);
    Window::GetWindow()->LockMouseToWindow(false);

    selections = {
        {[this]() -> std::string {
             return "Collision Response Method: " +
                    CollisionResponseMethodToString(GetGlobalConfig().collisionResponseMethod);
         },
         []() {
             auto &config = GetGlobalConfig();

             int method = static_cast<int>(config.collisionResponseMethod);
             method++;
             if (method == static_cast<int>(CollisionResponseMethod::END)) {
                 method = 0;
             }
             config.collisionResponseMethod = static_cast<CollisionResponseMethod>(method);
         }},
        {[this]() -> std::string {
             return std::string("Cube As OBB: ") + (GetGlobalConfig().cubeAsOBB ? "Yes" : "No");
         },
         []() {
             auto &config = GetGlobalConfig();

             config.cubeAsOBB = !config.cubeAsOBB;
         }},
        {[this]() -> std::string {
             return std::string("Print Collision Info: ") + (GetGlobalConfig().printCollisionInfo ? "Yes" : "No");
         },
         []() {
             auto &config = GetGlobalConfig();

             config.printCollisionInfo = !config.printCollisionInfo;
         }},
        {[this]() -> std::string {
             return "Back";
         },
         [this]() {
             this->levelController->SwitchLevel(LevelController::LEVEL_COVER);
         }},
    };
}

LevelSettings::~LevelSettings() {
    delete renderer;
    delete world;
}

void LevelSettings::UpdateGame(float dt) {
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::UP)) {
        if (currSelection == 0) {
            currSelection = selections.size() - 1;
        }
        currSelection--;
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::DOWN)) {
        currSelection++;
        if (currSelection == selections.size()) {
            currSelection = 0;
        }
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::RETURN)) {
        selections[currSelection].fn();
    }

    Debug::Print("Settings", Vector2(30, 20), Debug::RED);

    float xPosDiff = 0;
    const float X_DIFF = 2;
    float yPos = 30;
    const float yInterval = 10;
    for (int i = 0; i < selections.size(); ++i) {
        auto &selection = selections[i];

        std::string s = selection.fnGetTitle();
        if (i == currSelection) {
            s = ">" + s + "<";
            xPosDiff -= X_DIFF;
        }
        Debug::Print(s, Vector2(10 + xPosDiff, yPos), Debug::RED);
        yPos += yInterval;
    }

    renderer->Render();
    Debug::UpdateRenderables(dt);
}
