#include "BehaviourGameObject.h"

#include "BehaviourAction.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "PhysicsObject.h"
#include "Debug.h"

constexpr float CHASE_DISTANCE = 80.0f;

using namespace NCL;
using namespace CSC8503;
BehaviourGameObject ::BehaviourGameObject(const std::string &name, const Grid &grid, FindPathFunc findPathFunc)
    : GameObject(name), grid(grid), findPathFunc(findPathFunc) {
    counter = 0.0f;

    BehaviourAction *actionMoveLeft =
        new BehaviourAction("move left", [&](float dt, BehaviourState state) -> BehaviourState {
            switch (state) {
            case BehaviourState::Initialise:
                counter = 3.0f;
                state = Ongoing;
                std::cout << "start move left. counter=" << counter << std::endl;
                break;

            case BehaviourState::Success:
            case BehaviourState::Failure:
                break;
            case BehaviourState::Ongoing:
                std::cout << "move left... counter=" << counter << std::endl;
                this->MoveLeft(dt);
                counter -= dt;
                if (counter <= 0) {
                    state = Success;
                }
                break;
            }
            return state;
        });
    BehaviourAction *actionMoveRight =
        new BehaviourAction("move right", [&](float dt, BehaviourState state) -> BehaviourState {
            switch (state) {
            case BehaviourState::Initialise:
                counter = 3.0f;
                state = Ongoing;
                std::cout << "start move right. counter=" << counter << std::endl;
                break;

            case BehaviourState::Success:
            case BehaviourState::Failure:
                break;
            case BehaviourState::Ongoing:
                std::cout << "move right... counter=" << counter << std::endl;
                this->MoveRight(dt);
                counter -= dt;
                if (counter <= 0) {
                    state = Success;
                }
                break;
            }
            return state;
        });

    BehaviourAction *conditionPlayerIsNear =
        new BehaviourAction("conditionPlayerIsNear", [this](float dt, BehaviourState state) -> BehaviourState {
            for (auto &player : players) {
                float dist = Vector::Length(GetTransform().GetPosition() - player->GetTransform().GetPosition());
                if (dist < CHASE_DISTANCE) {
                    currTarget = player;
                    return BehaviourState::Success;
                }
            }
            return BehaviourState::Failure;
        });

    BehaviourAction *conditionBonusIsNear =
        new BehaviourAction("conditionBonusIsNear", [this](float dt, BehaviourState state) -> BehaviourState {
            for (auto &bonus : bonuses) {
                float dist = Vector::Length(GetTransform().GetPosition() - bonus->GetTransform().GetPosition());
                if (dist < CHASE_DISTANCE) {
                    currTarget = bonus;
                    return BehaviourState::Success;
                }
            }
            return BehaviourState::Failure;
        });

    BehaviourAction *conditionCaughtTarget =
        new BehaviourAction("conditionCaughtTarget", [this](float dt, BehaviourState state) -> BehaviourState {
            if (currTarget == nullptr) {
                return BehaviourState::Failure;
            }
            float dist = Vector::Length(GetTransform().GetPosition() - currTarget->GetTransform().GetPosition());
            if (dist < 1.0f) {
                return BehaviourState::Success;
            }
            return BehaviourState::Failure;
        });

    // judge if self is near by a wall, if is, update a path to escape and fill in waypoints. if not return the Failure
    // state
    BehaviourAction *conditionNearByWall =
        new BehaviourAction("conditionNearByWall", [this](float dt, BehaviourState state) -> BehaviourState {
            Vector3 selfPos = GetTransform().GetPosition();
            Vector2i coord(static_cast<int>(selfPos.x), static_cast<int>(selfPos.z));
            auto tile = this->grid.GetTile(coord);
            if (!tile.has_value()) {
                return BehaviourState::Failure;
            }

            std::vector<Vector2i> neighbours = {
                {coord.x + 1, coord.y}, {coord.x, coord.y + 1}, {coord.x - 1, coord.y}, {coord.x, coord.y - 1}};
            for (auto &neighbour : neighbours) {
                auto tile = this->grid.GetTile(neighbour);
                if (!tile.has_value()) {
                    continue;
                }

                if (tile.value() != WALL_NODE) {
                    continue;
                }

                Vector2i wallDirection = neighbour - coord;
                Vector2i escapeDirection = -wallDirection;

                waypoints = {this->grid.GetGridCoord(coord + escapeDirection, 0)};
                std::cout << "escape from wall. dest pos=" << waypoints[0].x << ", " << waypoints[0].y << ", "
                          << waypoints[0].z << std::endl;
                return BehaviourState::Success;
            }
            return BehaviourState::Failure;
        });

    auto ActionGoAlongWaypointsFunc = [this](float dt, BehaviourState state, float initCounter) -> BehaviourState {
        std::string targetName;
        if (currTarget) {
            targetName = currTarget->GetName();
        }
        switch (state) {
        case BehaviourState::Initialise:
            counter = 3.0f;
            state = Ongoing;
            std::cout << "start go along waypoints " << targetName << ". counter=" << counter << std::endl;
            break;

        case BehaviourState::Success:
        case BehaviourState::Failure:
            break;
        case BehaviourState::Ongoing:
            // std::cout << "go along waypoints " << targetName << "... counter=" << counter << std::endl;
            this->GoAlongWaypoints(dt);
            counter -= dt;
            if (counter <= 0) {
                std::cout << "end go along waypoints " << targetName << ". counter=" << counter << std::endl;
                state = Success;
            }
            break;
        }
        return state;
    };

    BehaviourAction *actionUpdateWaypoints =
        new BehaviourAction("update waypoints", [this](float dt, BehaviourState state) -> BehaviourState {
            bool ok = this->UpdateWaypoints(dt);
            return ok ? BehaviourState::Success : BehaviourState::Failure;
        });

    treeroot = new BehaviourSelector("root");

    BehaviourSequence *hangout = new BehaviourSequence("hang out");
    hangout->AddChild(actionMoveLeft);
    hangout->AddChild(actionMoveRight);

    BehaviourSequence *chaseBonus = new BehaviourSequence("chase bonus");
    chaseBonus->AddChild(conditionBonusIsNear);
    chaseBonus->AddChild(actionUpdateWaypoints);
    chaseBonus->AddChild(new BehaviourAction(
        "go along waypoints", [this, ActionGoAlongWaypointsFunc](float dt, BehaviourState state) -> BehaviourState {
            return ActionGoAlongWaypointsFunc(dt, state, 10.0f);
        }));
    chaseBonus->AddChild(conditionCaughtTarget);

    BehaviourSequence *chasePlayer = new BehaviourSequence("chase player");
    chasePlayer->AddChild(conditionPlayerIsNear);
    chasePlayer->AddChild(actionUpdateWaypoints);
    chasePlayer->AddChild(new BehaviourAction(
        "go along waypoints", [this, ActionGoAlongWaypointsFunc](float dt, BehaviourState state) -> BehaviourState {
            return ActionGoAlongWaypointsFunc(dt, state, 10.0f);
        }));
    chasePlayer->AddChild(conditionCaughtTarget);

    BehaviourSequence *goAwayFromWall = new BehaviourSequence("go away from wall");
    goAwayFromWall->AddChild(conditionNearByWall);
    goAwayFromWall->AddChild(new BehaviourAction(
        "go along waypoints", [this, ActionGoAlongWaypointsFunc](float dt, BehaviourState state) -> BehaviourState {
            return ActionGoAlongWaypointsFunc(dt, state, 3.0f);
        }));

    treeroot->AddChild(goAwayFromWall);
    treeroot->AddChild(chaseBonus);
    treeroot->AddChild(chasePlayer);
    treeroot->AddChild(hangout);
}

BehaviourGameObject ::~BehaviourGameObject() {
    delete treeroot;
}

void NCL::CSC8503::BehaviourGameObject::DrawWaypoints() const {
    Vector3 lastWaypoint;
    if (!waypoints.empty())
        lastWaypoint = waypoints[0];
    for (auto &waypoint : waypoints) {
        Debug::DrawLine(lastWaypoint, waypoint, Debug::RED);
        lastWaypoint = waypoint;
    }
}

void BehaviourGameObject ::Update(float dt) {
    switch (treeroot->Execute(dt)) {
    case BehaviourState::Success:
    case BehaviourState::Failure:
        treeroot->Reset();
        break;
    }

    // orientate to the xz-direction of velocity
    Vector3 v = GetPhysicsObject()->GetLinearVelocity();
    // std::cout << v.x << " " << v.y << " " << v.z << std::endl;
    float yaw = atan2(-v.x, v.z);
    // std::cout << yaw << std::endl;
    GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(yaw)));
}

void BehaviourGameObject ::MoveLeft(float dt) {
    GetPhysicsObject()->AddForce({-100, 0, 0});
}

void BehaviourGameObject ::MoveRight(float dt) {
    GetPhysicsObject()->AddForce({100, 0, 0});
}

bool NCL::CSC8503::BehaviourGameObject::UpdateWaypoints(float dt) {
    if (!currTarget) {
        return false;
    }

    std::cout << "UpdateWaypoints" << std::endl;
    waypoints = findPathFunc(GetTransform().GetPosition(), currTarget->GetTransform().GetPosition());
    if (waypoints.empty()) {
        std::cout << "can't find path" << std::endl;
        return false;
    }
    return true;
}

void NCL::CSC8503::BehaviourGameObject::GoAlongWaypoints(float dt) {
    DrawWaypoints();

    Vector3 toward;
    for (auto &waypoint : waypoints) {
        Vector3 selfPosXZ = GetTransform().GetPosition();
        selfPosXZ.y = 0;

        toward = waypoint - selfPosXZ;
        float dist = Vector::Length(toward);
        if (dist <= 1.0) {
            continue;
        }

        // std::cout << "goto " << waypoint.x << " " << waypoint.y << " " << waypoint.z << std::endl;
        break;
    }
    toward = Vector::Normalise(toward);
    // std::cout << "toward " << toward.x << " " << toward.y << " " << toward.z << std::endl;

    GetPhysicsObject()->AddForce(toward * 100.0f);

    counter -= dt;
}
