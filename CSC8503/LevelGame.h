#pragma once

#include "../NCLCoreClasses/KeyboardMouseController.h"

#include "GameTechRenderer.h"

#include "BaseLevel.h"
#include "PhysicsSystem.h"

#include "StateGameObject.h"
#include "BehaviourGameObject.h"
#include "NavigationGrid.h"

#include "MyGameObject.h"

#include "client.h"

namespace NCL {
namespace CSC8503 {
class LevelGame : public BaseLevel {
public:
    enum class GameMode {
        SINGLE_PLAYER,
        MULTI_PLAYER,
    };

    LevelGame(GameMode gameMode);
    ~LevelGame();

    virtual void UpdateGame(float dt) override;

protected:
    void InitialiseAssets();

    void InitCamera();
    void UpdateKeys();

    void InitWorld();

    void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
    void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3 &cubeDims);

    void InitDefaultFloor();

    void DebugObjectMovement();
    void LockedObjectMovement();

    GameObject *AddFloorToWorld();
    GameObject *AddSphereToWorld(const Vector3 &position, float radius, float inverseMass = 10.0f);
    GameObject *AddCubeToWorld(const Vector3 &position, Vector3 dimensions, float inverseMass = 10.0f);
    GameObject *AddDoorToWorld(const Vector3 &position, Vector3 dimensions, float inverseMass = 10.0f);

    Player *AddPlayerToWorld(const Vector3 &position, Texture *tex, int playerIndex);
    BehaviourGameObject *AddEnemyToWorld(const Vector3 &position);
    GameObject *AddBonusToWorld(const Vector3 &position);

    void HandleMessages();

    GameMode gameMode;
    client *cli = nullptr;
    enum class GameStatus {
        WAIT_OTHER_PLAYERS,
        START_GAME,
        PLAYING,
        WIN,
        GAMEOVER,
    };
    GameStatus gameStatus;

#ifdef USEVULKAN
    GameTechVulkanRenderer *renderer;
#else
    GameTechRenderer *renderer;
#endif
    PhysicsSystem *physics;
    GameWorld *world;

    KeyboardMouseController controller;

    Grid grid;
    NavigationGrid navigationGrid;

    enum CameraMode {
        FREE,
        GAMING,
        END,
    };
    CameraMode currCameraMode = GAMING;

    bool isDebug;
    bool useGravity;
    bool showMouse;
    bool usePhysics;

    float forceMagnitude;

    std::vector<Vector3> playerBornPositions;
    Player *selfPlayer = nullptr;
    std::vector<Player *> players;
    std::set<GameObject *> bonuses;
    std::vector<BehaviourGameObject *> enemies;

    Mesh *capsuleMesh = nullptr;
    Mesh *cubeMesh = nullptr;
    Mesh *sphereMesh = nullptr;

    Texture *basicTex = nullptr;
    Texture *blackTex = nullptr;
    Texture *brownTex = nullptr;
    Texture *blueTex = nullptr;
    Texture *redTex = nullptr;
    Texture *greenTex = nullptr;
    Texture **playerTextures[4] = {&redTex, &blueTex, &blackTex, &brownTex};
    Vector4 playerColors[4] = {Debug::RED, Debug::BLUE, Debug::BLACK, Debug::YELLOW}; // FIXME

    Texture *goldTex = nullptr;
    Texture *doggeTex = nullptr;
    Texture *brickTex = nullptr;
    Shader *basicShader = nullptr;

    // Coursework Meshes
    Mesh *catMesh = nullptr;
    Mesh *kittenMesh = nullptr;
    Mesh *enemyMesh = nullptr;
    Mesh *bonusMesh = nullptr;

    // Coursework Additional functionality
    GameObject *lockedObject = nullptr;
    Vector3 lockedOffset = Vector3(0, 2, 2);
    void LockCameraToObject(GameObject *o) {
        lockedObject = o;
    }

    GameObject *objClosest = nullptr;

    std::chrono::high_resolution_clock::time_point lastReportPosTime;
};
} // namespace CSC8503
} // namespace NCL
