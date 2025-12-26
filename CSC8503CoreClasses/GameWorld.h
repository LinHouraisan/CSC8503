#pragma once
#include <random>

#include "Ray.h"
#include "CollisionDetection.h"
#include "QuadTree.h"

#include "ThirdPersonCamera.h"

namespace NCL {
class Camera;
using Maths::Ray;
namespace CSC8503 {
class GameObject;
class Constraint;

typedef std::function<void(GameObject *)> GameObjectFunc;
typedef std::vector<GameObject *>::const_iterator GameObjectIterator;

using CameraType = ThirdPersonCamera;

class GameWorld {
public:
    GameWorld(bool useThirdPersonCamera);
    ~GameWorld();

    void Clear();
    void ClearAndErase();

    void AddGameObject(GameObject *o);
    void RemoveGameObject(GameObject *o, bool andDelete = false);

    void AddConstraint(Constraint *c);
    void RemoveConstraint(Constraint *c, bool andDelete = false);

    Camera &GetMainCamera() {
        return *mainCamera;
    }

    PerspectiveCamera &GetDefaultCamera() {
        return defaultCamera;
    }

    ThirdPersonCamera &GetThirdPersonCamera() {
        return thirdPersonCamera;
    }

    void ShuffleConstraints(bool state) {
        shuffleConstraints = state;
    }

    void ShuffleObjects(bool state) {
        shuffleObjects = state;
    }

    bool Raycast(Ray &r, RayCollision &closestCollision, bool closestObject = false,
                 GameObject *ignore = nullptr) const;

    virtual void UpdateWorld(float dt);

    void OperateOnContents(GameObjectFunc f);

    void GetObjectIterators(GameObjectIterator &first, GameObjectIterator &last) const;

    void GetMoveObjectIterators(GameObjectIterator &first, GameObjectIterator &last) const;

    void GetFixedObjectIterators(GameObjectIterator &first, GameObjectIterator &last) const;

    void GetConstraintIterators(std::vector<Constraint *>::const_iterator &first,
                                std::vector<Constraint *>::const_iterator &last) const;

    int GetWorldStateID() const {
        return worldStateCounter;
    }

protected:
    std::vector<GameObject *> gameObjects;
    std::vector<GameObject *> moveGameObjects;
    std::vector<GameObject *> fixedGameObjects;
    std::vector<Constraint *> constraints;

    Camera *mainCamera;
    PerspectiveCamera defaultCamera;
    ThirdPersonCamera thirdPersonCamera;

    bool shuffleConstraints;
    bool shuffleObjects;
    int worldIDCounter;
    int worldStateCounter;
};
} // namespace CSC8503
} // namespace NCL
