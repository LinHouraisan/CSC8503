#include "LevelGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"

#include "Assets.h"

#include <cassert>

using namespace NCL;
using namespace CSC8503;

LevelGame::LevelGame(GameMode gameMode)
    : gameMode(gameMode), controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()),
      grid(Assets::DATADIR + "/level1.txt"), navigationGrid(grid), isDebug(false), playerBornPositions(PLAYER_LIMIT),
      lastReportPosTime(std::chrono::high_resolution_clock::now()) {

    if (isDebug) {
        usePhysics = false;
    } else {
        usePhysics = true;
    }

    switch (gameMode) {
    case GameMode::SINGLE_PLAYER:
        showMouse = false;
        gameStatus = GameStatus::START_GAME;
        break;
    case GameMode::MULTI_PLAYER:
        showMouse = true;
        gameStatus = GameStatus::WAIT_OTHER_PLAYERS;
        cli = new client("localhost", PORT);
        break;
    }

    world = new GameWorld(true);
#ifdef USEVULKAN
    renderer = new GameTechVulkanRenderer(*world);
    renderer->Init();
    renderer->InitStructures();
#else
    renderer = new GameTechRenderer(*world);
#endif

    physics = new PhysicsSystem(*world);

    forceMagnitude = 10.0f;
    useGravity = true;
    physics->UseGravity(useGravity);

    world->GetMainCamera().SetController(controller);
    world->GetThirdPersonCamera().SetController(controller);

    Window::GetWindow()->ShowOSPointer(showMouse);
    Window::GetWindow()->LockMouseToWindow(!showMouse);

    controller.MapAxis(0, "Sidestep");
    controller.MapAxis(1, "UpDown");
    controller.MapAxis(2, "Forward");

    controller.MapAxis(3, "XLook");
    controller.MapAxis(4, "YLook");

    InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void LevelGame::InitialiseAssets() {
    cubeMesh = renderer->LoadMesh("cube.msh");
    sphereMesh = renderer->LoadMesh("sphere.msh");
    catMesh = renderer->LoadMesh("ORIGAMI_Chat.msh");
    kittenMesh = renderer->LoadMesh("Kitten.msh");

    enemyMesh = renderer->LoadMesh("Keeper.msh");
    bonusMesh = renderer->LoadMesh("19463_Kitten_Head_v1.msh");
    capsuleMesh = renderer->LoadMesh("capsule.msh");

    basicTex = renderer->LoadTexture("checkerboard.png");
    goldTex = renderer->LoadTexture("gold.jpg");
    blackTex = renderer->LoadTexture("black.jpg");
    brownTex = renderer->LoadTexture("GoatBrown.jpg");
    blueTex = renderer->LoadTexture("blue.png");
    redTex = renderer->LoadTexture("red.png");
    greenTex = renderer->LoadTexture("green.png");
    doggeTex = renderer->LoadTexture("Default.jpg");
    brickTex = renderer->LoadTexture("brick.jpg");
    basicShader = renderer->LoadShader("scene.vert", "scene.frag");

    InitWorld();
    InitCamera();
}

LevelGame::~LevelGame() {
    delete cubeMesh;
    delete sphereMesh;
    delete catMesh;
    delete kittenMesh;
    delete enemyMesh;
    delete bonusMesh;

    delete basicTex;
    delete basicShader;

    delete physics;
    delete renderer;
    delete world;
}

void LevelGame::UpdateGame(float dt) {

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::M)) {
        showMouse = !showMouse;
        Window::GetWindow()->ShowOSPointer(showMouse);
        Window::GetWindow()->LockMouseToWindow(!showMouse);
    }
    Debug::Print(std::string("(M)show mouse: ") + (showMouse ? "on" : "off"), Vector2(5, 90), Debug::RED);

    if (isDebug) {
        if (Window::GetKeyboard()->KeyPressed(KeyCodes::P)) {
            usePhysics = !usePhysics;
        }
        Debug::Print(std::string("(P)use physics: ") + (usePhysics ? "on" : "off"), Vector2(5, 98), Debug::RED);
    }

    switch (gameStatus) {
    case GameStatus::WAIT_OTHER_PLAYERS: {
        Debug::Print(cli->currentStatusString(), Vector2(40, 60), Debug::RED);
        if (cli->currentStatus() == client::Status::PLAYING) {
            gameStatus = GameStatus::START_GAME;
        }
        break;
    }
    case GameStatus::START_GAME: {
        int playerNums = gameMode == GameMode::SINGLE_PLAYER ? 1 : PLAYER_LIMIT;
        int selfIndex = gameMode == GameMode::SINGLE_PLAYER ? 0 : cli->getPlayerIndex();

        assert(selfPlayer == nullptr);
        for (int i = 0; i < playerNums; ++i) {
            auto player = AddPlayerToWorld(playerBornPositions[i], *playerTextures[i], i);
            players.push_back(player);
            if (i == selfIndex) {
                selfPlayer = player;
                lockedObject = selfPlayer;
            }
        }

        gameStatus = GameStatus::PLAYING;
        showMouse = false;
        Window::GetWindow()->ShowOSPointer(showMouse);
        Window::GetWindow()->LockMouseToWindow(!showMouse);
        break;
    }
    case GameStatus::PLAYING: {
        if (gameMode == GameMode::MULTI_PLAYER) {
            HandleMessages();
        }

        std::string playerPosStr = "pos:" + std::to_string(selfPlayer->GetTransform().GetPosition().x) + "," +
                                   std::to_string(selfPlayer->GetTransform().GetPosition().y) + "," +
                                   std::to_string(selfPlayer->GetTransform().GetPosition().z);
        Debug::Print(playerPosStr, Vector2{50, 2}, playerColors[selfPlayer->selfIndex]);

        std::string bonusStr = "remaining bonus:" + std::to_string(bonuses.size() * 10);
        Debug::Print(bonusStr, Vector2{50, 5}, Debug::RED);

        UpdateKeys();

        switch (currCameraMode) {
        case CameraMode::FREE:
            world->GetMainCamera().UpdateAngleByAxis(dt);
            world->GetMainCamera().UpdatePositionByKey(dt);
            break;
        case CameraMode::GAMING: {
            assert(lockedObject);
            Vector3 objPos = lockedObject->GetTransform().GetPosition();

            world->GetThirdPersonCamera().Update(objPos);
            break;
        }
        }
        break;
    }
    default:
        assert(0);
    }

    if (useGravity) {
        Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
    } else {
        Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
    }
    // This year we can draw debug textures as well!
    // Debug::DrawTex(*basicTex, Vector2(10, 10), Vector2(5, 5), Debug::MAGENTA);

    // RayCollision closestCollision;
    //  if (Window::GetKeyboard()->KeyPressed(KeyCodes::K) && selectionObject) {
    //      Vector3 rayPos;
    //      Vector3 rayDir;

    //    rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

    //    rayPos = selectionObject->GetTransform().GetPosition();

    //    Ray r = Ray(rayPos, rayDir);

    //    if (world->Raycast(r, closestCollision, true, selectionObject)) {
    //        if (objClosest) {
    //            objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
    //        }
    //        objClosest = (GameObject *)closestCollision.node;

    //        objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
    //    }
    //}

    for (auto &player : players) {
        std::string s;
        if (player == selfPlayer) {
            s += "*";
        }
        s += "player" + std::to_string(player->selfIndex) + "(HP" + std::to_string(player->health) +
             ") score: " + std::to_string(player->score);
        Debug::Print(s, Vector2{50, static_cast<float>(player->selfIndex * 5 + 10)}, playerColors[player->selfIndex]);
    }

    Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));
    Debug::DrawLine(Vector3(), Vector3(100, 0, 0), Vector4(0, 1, 0, 1));

    for (auto &enemy : enemies) {
        std::vector<GameObject *> tempPlayers, tempBonuses;
        tempPlayers.insert(tempPlayers.end(), players.begin(), players.end());
        tempBonuses.insert(tempBonuses.end(), bonuses.begin(), bonuses.end());

        enemy->UpdateKnowledge(tempPlayers, tempBonuses);
        enemy->Update(dt);
    }

    world->UpdateWorld(dt);

    if (usePhysics) {
        auto [beginCollisions, endCollistions] = physics->Update(dt);
        for (auto &c : endCollistions) {
            if (c.a->GetName() == "player" || c.b->GetName() == "player") {
                Player *playerTemp = nullptr;
                GameObject *other = nullptr;
                if (c.a->GetName() == "player") {
                    playerTemp = dynamic_cast<Player *>(c.a);
                    other = c.b;
                } else if (c.b->GetName() == "player") {
                    playerTemp = dynamic_cast<Player *>(c.b);
                    other = c.a;
                }

                if (other->GetName() == "bonus") {
                    playerTemp->score += 10;
                    world->RemoveGameObject(other);
                    bonuses.erase(other);
                    continue;
                }
                if (other->GetName() == "enemy") {
                    playerTemp->health -= 10;
                    if (playerTemp->health <= 0) {
                        // FIXME: goto gameover
                    }
                }
                continue;
            }

            if (c.a->GetName() == "enemy" || c.b->GetName() == "enemy") {
                GameObject *enemyTemp = nullptr;
                GameObject *other = nullptr;
                if (c.a->GetName() == "enemy") {
                    enemyTemp = dynamic_cast<GameObject *>(c.a);
                    other = c.b;
                } else if (c.b->GetName() == "enemy") {
                    enemyTemp = dynamic_cast<GameObject *>(c.b);
                    other = c.a;
                }

                if (other->GetName() == "bonus") {
                    // only remove it
                    world->RemoveGameObject(other);
                    bonuses.erase(other);
                    continue;
                }
                continue;
            }
        }
    }

    for (auto &bonus : bonuses) {
        auto q = bonus->GetTransform().GetOrientation();
        q = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 5) * q;
        bonus->GetTransform().SetOrientation(q);
    }

    if (gameMode == GameMode::MULTI_PLAYER && gameStatus == GameStatus::PLAYING) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - lastReportPosTime > std::chrono::milliseconds(200)) {
            assert(selfPlayer);
            UpdatePosMessage msg;
            Vector3 pos = selfPlayer->GetTransform().GetPosition();
            Quaternion quat = selfPlayer->GetTransform().GetOrientation();
            msg.playerIndex = selfPlayer->selfIndex;
            memcpy(msg.pos, pos.array, sizeof(msg.pos));
            memcpy(msg.quat, &quat, sizeof(msg.quat));

            nlohmann::json j;
            nlohmann::to_json(j, msg);
            cli->write(j);
        }
    }

    renderer->Render();
    Debug::UpdateRenderables(dt);
}

void LevelGame::UpdateKeys() {
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::C)) {
        currCameraMode = static_cast<CameraMode>(currCameraMode + 1);
        if (currCameraMode == END) {
            currCameraMode = CameraMode::FREE;
        }
    }
    {
        std::string modeStr;
        switch (currCameraMode) {
        case CameraMode::FREE:
            modeStr = "free";
            break;
        case CameraMode::GAMING:
            modeStr = "gaming";
            break;
        }
        Debug::Print("Mode[C]: " + modeStr, Vector2(5, 85));
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::F1)) {
        InitWorld(); // We can reset the simulation at any time with F1
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::F2)) {
        InitCamera(); // F2 will reset the camera to a specific default place
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::G)) {
        useGravity = !useGravity; // Toggle gravity!
        physics->UseGravity(useGravity);
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::A)) {
        // float yaw = world->GetMainCamera().GetYaw();
        // Vector3 dir{cos(yaw), 0, -sin(yaw)};
        // player->GetPhysicsObject()->AddForce(dir);
    }

    // if (lockedObject) {
    LockedObjectMovement();
    // } else {
    //     DebugObjectMovement();
    // }
}

void LevelGame::LockedObjectMovement() {
    Matrix4 view = world->GetMainCamera().BuildViewMatrix();
    Matrix4 camWorld = Matrix::Inverse(view);

    Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); // view is inverse of model!

    // forward is more tricky -  camera forward is 'into' the screen...
    // so we can take a guess, and use the cross of straight up, and
    // the right axis, to hopefully get a vector that's good enough!

    Vector3 fwdAxis = Vector::Cross(Vector3(0, 1, 0), rightAxis);
    fwdAxis.y = 0.0f;
    fwdAxis = Vector::Normalise(fwdAxis);
    Vector3 forceVec = fwdAxis;

    bool pressedDirectionKey = false;

    if (Window::GetKeyboard()->KeyDown(KeyCodes::W)) {
        pressedDirectionKey = true;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::S)) {
        pressedDirectionKey = true;
        forceVec = -fwdAxis;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::A)) {
        pressedDirectionKey = true;
        forceVec = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 90.0f) * fwdAxis;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::D)) {
        pressedDirectionKey = true;
        forceVec = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -90.0f) * fwdAxis;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::W) && Window::GetKeyboard()->KeyDown(KeyCodes::A)) {
        pressedDirectionKey = true;
        forceVec = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 45) * fwdAxis;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::W) && Window::GetKeyboard()->KeyDown(KeyCodes::D)) {
        pressedDirectionKey = true;
        forceVec = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -45) * fwdAxis;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::S) && Window::GetKeyboard()->KeyDown(KeyCodes::A)) {
        pressedDirectionKey = true;
        forceVec = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 135) * fwdAxis;
    }
    if (Window::GetKeyboard()->KeyDown(KeyCodes::S) && Window::GetKeyboard()->KeyDown(KeyCodes::D)) {
        pressedDirectionKey = true;
        forceVec = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), -135) * fwdAxis;
    }
    float cameraYawDeg = world->GetMainCamera().GetYaw();
    float cameraYawDegToObject = -cameraYawDeg + 180.0f;
    // std::cout << cameraYawDeg << std::endl;

    if (pressedDirectionKey) {

        Debug::DrawLine(lockedObject->GetTransform().GetPosition(),
                        lockedObject->GetTransform().GetPosition() + forceVec, Vector4(1, 0, 0, 1));

        float yaw = atan2(forceVec.x, forceVec.z);
        float yawToObject = -yaw + PI / 2.0f;
        // std::cout << Maths::RadiansToDegrees(yaw) << std::endl;

        Vector3 v = lockedObject->GetPhysicsObject()->GetLinearVelocity();
        v += forceVec * 0.1f;
        lockedObject->GetPhysicsObject()->SetLinearVelocity(v);

        lockedObject->GetPhysicsObject()->AddForce(forceVec * 10.0f);

        lockedObject->GetTransform().SetOrientation(
            Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(yaw))); //  +cameraYawDegToObject
    }

    if (Window::GetKeyboard()->KeyDown(KeyCodes::SPACE)) {
        lockedObject->GetPhysicsObject()->AddForce(Vector3{0, 1, 0} * 50.0f);
    }

    if (Window::GetKeyboard()->KeyDown(KeyCodes::NEXT)) {
        lockedObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
    }
}

void LevelGame::DebugObjectMovement() {
    // If we've selected an object, we can manipulate it with some key presses
    // if (inSelectionMode && selectionObject) {
    // Twist the selected object!
    // if (Window::GetKeyboard()->KeyDown(KeyCodes::LEFT)) {
    //    selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
    //}

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
    //     selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
    // }

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM7)) {
    //     selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
    // }

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM8)) {
    //     selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
    // }

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::RIGHT)) {
    //     selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
    // }

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::UP)) {
    //     selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
    // }

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::DOWN)) {
    //     selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
    // }

    // if (Window::GetKeyboard()->KeyDown(KeyCodes::NUM5)) {
    //     selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
    // }
    //}
}

void LevelGame::InitCamera() {
    world->GetMainCamera().SetNearPlane(0.1f);
    world->GetMainCamera().SetFarPlane(500.0f);
    world->GetMainCamera().SetPitch(-15.0f);
    world->GetMainCamera().SetYaw(315.0f);
    // world->GetMainCamera().SetPosition(selfPlayer->GetTransform().GetPosition());
    world->GetMainCamera().SetSpeed(50.0);

    world->GetThirdPersonCamera().SetNearPlane(0.1f);
    world->GetThirdPersonCamera().SetFarPlane(500.0f);
    world->GetThirdPersonCamera().SetPitch(-15.0f);
    world->GetThirdPersonCamera().SetYaw(315.0f);
    // world->GetThirdPersonCamera().SetPosition(selfPlayer->GetTransform().GetPosition());
    world->GetThirdPersonCamera().SetSpeed(50.0);
}

void LevelGame::InitWorld() {
    world->ClearAndErase();
    physics->Clear();

    int rowNums = grid.height;
    int colNums = grid.width;

    for (int i = 0; i < rowNums; ++i) {
        for (int j = 0; j < colNums; ++j) {}
    }

    auto bricks = CombineToBricks(grid.data, FLOOR_NODE);

    for (auto &brick : bricks) {
        switch (brick.c) {
        case '0':
        case '1':
        case '2':
        case '3': {
            int index = brick.c - '0';
            if (index >= PLAYER_LIMIT) {
                break;
            }
            playerBornPositions[index] = grid.GetGridCoord(brick.start, 1.0f);
            break;
        }
        case 'E': {
            BehaviourGameObject *enemy = AddEnemyToWorld(grid.GetGridCoord(brick.start, 2.0f));
            enemies.push_back(enemy);
            break;
        }
        case 'B': {
            GameObject *bonus = AddBonusToWorld(grid.GetGridCoord(brick.start, 1.0f));
            bonuses.insert(bonus);
            break;
        }
        case WALL_NODE: {
            Vector3 size{static_cast<float>(brick.end.x - brick.start.x + 1.0f), 4.0f,
                         static_cast<float>(brick.end.y - brick.start.y + 1.0f)};
            AddCubeToWorld(grid.GetGridCoord(brick.GetCenter(), size.y / 2.0f), size * 0.5f, 0);
            break;
        }
        case '|': {
            Vector3 size{static_cast<float>(brick.end.x - brick.start.x + 1.0f), 4.0f,
                         static_cast<float>(brick.end.y - brick.start.y + 1.0f)};
            AddDoorToWorld(grid.GetGridCoord(brick.GetCenter(), size.y / 2.0f), size * 0.5f, 10.0f);
            break;
        }
        default:
            std::cout << "ignore node: " << brick.c << std::endl;
        }
    }

    InitDefaultFloor();
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject *LevelGame::AddFloorToWorld() {
    GameObject *floor = new GameObject("floor");

    float yHalfSize = 2.0;
    Vector3 floorSize = Vector3(2000, yHalfSize, 2000);
    AABBVolume *volume = new AABBVolume(floorSize);
    floor->SetBoundingVolume((CollisionVolume *)volume);
    floor->GetTransform().SetScale(floorSize * 2.0f).SetPosition({0, -yHalfSize, 0});

    floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
    floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

    floor->GetPhysicsObject()->SetInverseMass(0);
    floor->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(floor);

    return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject *LevelGame::AddSphereToWorld(const Vector3 &position, float radius, float inverseMass) {
    GameObject *sphere = new GameObject("sphere");

    Vector3 sphereSize = Vector3(radius, radius, radius);
    SphereVolume *volume = new SphereVolume(radius);
    sphere->SetBoundingVolume((CollisionVolume *)volume);

    sphere->GetTransform().SetScale(sphereSize).SetPosition(position);

    sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
    sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

    sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
    sphere->GetPhysicsObject()->InitSphereInertia();

    world->AddGameObject(sphere);

    return sphere;
}

GameObject *LevelGame::AddCubeToWorld(const Vector3 &position, Vector3 dimensions, float inverseMass) {
    GameObject *cube = new GameObject("cube");

    AABBVolume *volume = new AABBVolume(dimensions * 0.99f);
    cube->SetBoundingVolume((CollisionVolume *)volume);

    cube->GetTransform().SetPosition(position).SetScale(dimensions * 2.0f);

    cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, brickTex, basicShader));
    cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

    cube->GetPhysicsObject()->SetInverseMass(inverseMass);
    cube->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(cube);

    return cube;
}

GameObject *NCL::CSC8503::LevelGame::AddDoorToWorld(const Vector3 &position, Vector3 dimensions, float inverseMass) {
    GameObject *door = new GameObject("door");

    AABBVolume *volume = new AABBVolume(dimensions * 0.99f);
    door->SetBoundingVolume((CollisionVolume *)volume);

    door->GetTransform().SetPosition(position).SetScale(dimensions * 2.0f);

    door->SetRenderObject(new RenderObject(&door->GetTransform(), cubeMesh, greenTex, basicShader));
    door->SetPhysicsObject(new PhysicsObject(&door->GetTransform(), door->GetBoundingVolume()));

    door->GetPhysicsObject()->SetInverseMass(inverseMass);
    door->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(door);

    return door;
}

Player *LevelGame::AddPlayerToWorld(const Vector3 &position, Texture *tex, int playerIndex) {
    float meshSize = 1.0f;
    float inverseMass = 0.5f;

    Player *character = new Player(playerIndex, 100, 0);
    AABBVolume *volume = new AABBVolume(Vector3{0.5, 0.5, 0.5});

    character->SetBoundingVolume((CollisionVolume *)volume);

    character->GetTransform().SetScale(Vector3(meshSize, meshSize, meshSize)).SetPosition(position);

    character->SetRenderObject(new RenderObject(&character->GetTransform(), catMesh, tex, basicShader));
    character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

    character->GetPhysicsObject()->SetInverseMass(inverseMass);
    character->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(character);

    return character;
}

BehaviourGameObject *LevelGame::AddEnemyToWorld(const Vector3 &position) {
    float meshSize = 3.0f;
    float inverseMass = 0.05f;

    BehaviourGameObject *character = new BehaviourGameObject("enemy", grid,
                                                             // call navigationGrid's method to find path
                                                             [this](Vector3 from, Vector3 to) -> std::vector<Vector3> {
                                                                 NavigationPath path;
                                                                 bool found = navigationGrid.FindPath(from, to, path);
                                                                 if (!found) {
                                                                     return {};
                                                                 }
                                                                 return path.GetWaypoints();
                                                             });

    AABBVolume *volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
    character->SetBoundingVolume((CollisionVolume *)volume);

    character->GetTransform().SetScale(Vector3(meshSize, meshSize, meshSize)).SetPosition(position);

    character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, doggeTex, basicShader));
    character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

    character->GetPhysicsObject()->SetInverseMass(inverseMass);
    character->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(character);

    return character;
}

GameObject *LevelGame::AddBonusToWorld(const Vector3 &position) {
    GameObject *apple = new GameObject("bonus");

    AABBVolume *volume = new AABBVolume(Vector3(0.5f, 0.5f, 0.5f));
    apple->SetBoundingVolume((CollisionVolume *)volume);
    apple->GetTransform().SetScale(Vector3(2, 2, 2)).SetPosition(position);

    apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, goldTex, basicShader));
    apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

    apple->GetPhysicsObject()->SetInverseMass(1.0f);
    apple->GetPhysicsObject()->InitSphereInertia();

    world->AddGameObject(apple);

    return apple;
}

void NCL::CSC8503::LevelGame::HandleMessages() {
    auto recvedJsons = cli->retrieve();
    for (auto &j : recvedJsons) {
        CID cid = j["cid"];
        switch (cid) {
        case CID::UPDATE_POS: {
            UpdatePosMessage msg;
            nlohmann::from_json(j, msg);

            assert(players.size() > msg.playerIndex);
            auto &player = players[msg.playerIndex];
            assert(player);
            player->GetTransform().SetPosition(Vector3(msg.pos[0], msg.pos[1], msg.pos[2]));
            Quaternion quat;
            memcpy(&quat, msg.quat, sizeof(quat));
            player->GetTransform().SetOrientation(quat);

            break;
        }
        default:
            assert(0);
        }
    }
}

void LevelGame::InitDefaultFloor() {
    AddFloorToWorld();
}

void LevelGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
    for (int x = 0; x < numCols; ++x) {
        for (int z = 0; z < numRows; ++z) {
            Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
            AddSphereToWorld(position, radius, 1.0f);
        }
    }
    AddFloorToWorld();
}

void LevelGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing,
                                  const Vector3 &cubeDims) {
    for (int x = 1; x < numCols + 1; ++x) {
        for (int z = 1; z < numRows + 1; ++z) {
            Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
            AddCubeToWorld(position, cubeDims, 1.0f);
        }
    }
}
