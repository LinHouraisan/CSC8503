#pragma once
#include "GameObject.h"

namespace NCL {
namespace CSC8503 {

using FindPathFunc = std::function<std::vector<Vector3>(Vector3 from, Vector3 to)>;

class StateMachine;
class StateGameObject : public GameObject {
public:
    StateGameObject(const std::string &name, FindPathFunc findPathFunc);
    ~StateGameObject();

    void SetChaseTarget(GameObject *target) {
        chaseTarget = target;
    }

    void DrawWaypoints() const;

    virtual void Update(float dt);

protected:
    void MoveLeft(float dt);
    void MoveRight(float dt);

    void UpdateWaypoints(float dt);
    void ChaseTarget(float dt);

    FindPathFunc findPathFunc;
    GameObject *chaseTarget = nullptr;
    StateMachine *stateMachine;
    float counter;
    std::vector<Vector3> waypoints;
};
} // namespace CSC8503
} // namespace NCL
