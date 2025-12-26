#include "GameWorld.h"
#include "GameObject.h"
#include "Constraint.h"
#include "CollisionDetection.h"
#include "Camera.h"

#include "PhysicsObject.h"

using namespace NCL;
using namespace NCL::CSC8503;

GameWorld::GameWorld(bool useThirdPersonCamera) {
    shuffleConstraints = false;
    shuffleObjects = false;
    worldIDCounter = 0;
    worldStateCounter = 0;
    if (useThirdPersonCamera) {
        mainCamera = &thirdPersonCamera;
    } else {
        mainCamera = &defaultCamera;
    }
}

GameWorld::~GameWorld() {}

void GameWorld::Clear() {
    gameObjects.clear();
    constraints.clear();
    worldIDCounter = 0;
    worldStateCounter = 0;
}

void GameWorld::ClearAndErase() {
    for (auto &i : gameObjects) {
        delete i;
    }
    for (auto &i : constraints) {
        delete i;
    }
    Clear();
}

void GameWorld::AddGameObject(GameObject *o) {
    if (o->GetPhysicsObject()) {
        if (o->GetPhysicsObject()->GetInverseMass() == 0) {
            //    fixedGameObjects.emplace_back(o);
            //} else {
        }
        moveGameObjects.emplace_back(o);
    }
    gameObjects.emplace_back(o);
    o->SetWorldID(worldIDCounter++);
    worldStateCounter++;
}

void GameWorld::RemoveGameObject(GameObject *o, bool andDelete) {
    gameObjects.erase(std::remove(gameObjects.begin(), gameObjects.end(), o), gameObjects.end());
    moveGameObjects.erase(std::remove(moveGameObjects.begin(), moveGameObjects.end(), o), moveGameObjects.end());
    fixedGameObjects.erase(std::remove(fixedGameObjects.begin(), fixedGameObjects.end(), o), fixedGameObjects.end());
    if (andDelete) {
        delete o;
    }
    worldStateCounter++;
}

void GameWorld::GetObjectIterators(GameObjectIterator &first, GameObjectIterator &last) const {

    first = gameObjects.begin();
    last = gameObjects.end();
}

void NCL::CSC8503::GameWorld::GetMoveObjectIterators(GameObjectIterator &first, GameObjectIterator &last) const {
    first = moveGameObjects.begin();
    last = moveGameObjects.end();
}

void NCL::CSC8503::GameWorld::GetFixedObjectIterators(GameObjectIterator &first, GameObjectIterator &last) const {

    first = fixedGameObjects.begin();
    last = fixedGameObjects.end();
}

void GameWorld::OperateOnContents(GameObjectFunc f) {
    for (GameObject *g : gameObjects) {
        f(g);
    }
}

void GameWorld::UpdateWorld(float dt) {
    auto rng = std::default_random_engine{};

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);

    if (shuffleObjects) {
        std::shuffle(gameObjects.begin(), gameObjects.end(), e);
    }

    if (shuffleConstraints) {
        std::shuffle(constraints.begin(), constraints.end(), e);
    }
}

bool GameWorld::Raycast(Ray &r, RayCollision &closestCollision, bool closestObject, GameObject *ignoreThis) const {
    // The simplest raycast just goes through each object and sees if there's a collision
    RayCollision collision;

    for (auto &i : gameObjects) {
        if (!i->GetBoundingVolume()) { // objects might not be collideable etc...
            continue;
        }
        if (i == ignoreThis) {
            continue;
        }
        RayCollision thisCollision;
        if (CollisionDetection::RayIntersection(r, *i, thisCollision)) {

            if (!closestObject) {
                closestCollision = collision;
                closestCollision.node = i;
                return true;
            } else {
                if (thisCollision.rayDistance < collision.rayDistance) {
                    thisCollision.node = i;
                    collision = thisCollision;
                }
            }
        }
    }
    if (collision.node) {
        closestCollision = collision;
        closestCollision.node = collision.node;
        return true;
    }
    return false;
}

/*
Constraint Tutorial Stuff
*/

void GameWorld::AddConstraint(Constraint *c) {
    constraints.emplace_back(c);
}

void GameWorld::RemoveConstraint(Constraint *c, bool andDelete) {
    constraints.erase(std::remove(constraints.begin(), constraints.end(), c), constraints.end());
    if (andDelete) {
        delete c;
    }
}

void GameWorld::GetConstraintIterators(std::vector<Constraint *>::const_iterator &first,
                                       std::vector<Constraint *>::const_iterator &last) const {
    first = constraints.begin();
    last = constraints.end();
}