#pragma once
#include "GameObject.h"

#include "BehaviourSelector.h"
#include "NavigationGrid.h"

namespace NCL {
namespace CSC8503 {

using FindPathFunc = std::function<std::vector<Vector3>(Vector3 from, Vector3 to)>;

class StateMachine;
class BehaviourGameObject : public GameObject {
public:
    BehaviourGameObject(const std::string &name, const Grid &grid, FindPathFunc findPathFunc);
    ~BehaviourGameObject();

    void UpdateKnowledge(const std::vector<GameObject *> &players, const std::vector<GameObject *> &bonuses) {
        this->players = players;
        this->bonuses = bonuses;
    }

    void DrawWaypoints() const;

    virtual void Update(float dt);

protected:
    void MoveLeft(float dt);
    void MoveRight(float dt);

    bool UpdateWaypoints(float dt);
    void GoAlongWaypoints(float dt);

    const Grid &grid;
    FindPathFunc findPathFunc;

    GameObject *currTarget = nullptr;
    std::vector<GameObject *> players;
    std::vector<GameObject *> bonuses;

    BehaviourSelector *treeroot;

    float counter;
    std::vector<Vector3> waypoints;
};
} // namespace CSC8503
} // namespace NCL
