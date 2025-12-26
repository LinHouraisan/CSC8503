#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include "Debug.h"

constexpr float CHASE_DISTANCE = 80.0f;

using namespace NCL;
using namespace CSC8503;
StateGameObject ::StateGameObject(const std::string &name, FindPathFunc findPathFunc)
    : GameObject(name), findPathFunc(findPathFunc) {
    counter = 0.0f;
    stateMachine = new StateMachine();

    /*

    UpdateWaypoints  ChaseTarget

    */

    State *stateMoveLeft = new State([&](float dt) -> void {
        this->MoveLeft(dt);
    });
    State *stateMoveRight = new State([&](float dt) -> void {
        this->MoveRight(dt);
    });

    State *stateChasePlayer = new State([this](float dt) {
        this->ChaseTarget(dt);
    });

    State *stateUpdateWaypoints = new State([this](float dt) {
        this->UpdateWaypoints(dt);
    });

    stateMachine->AddState(stateMoveLeft);
    stateMachine->AddState(stateMoveRight);
    stateMachine->AddState(stateChasePlayer);
    stateMachine->AddState(stateUpdateWaypoints);

    stateMachine->AddTransition(new StateTransition(stateUpdateWaypoints, stateChasePlayer, [&]() -> bool {
        return true;
    }));

    stateMachine->AddTransition(new StateTransition(stateChasePlayer, stateUpdateWaypoints, [&]() -> bool {
        return this->counter < 0.0f;
    }));

    stateMachine->AddTransition(new StateTransition(stateMoveLeft, stateMoveRight, [&]() -> bool {
        return this->counter > 3.0f;
    }));

    stateMachine->AddTransition(new StateTransition(stateMoveRight, stateMoveLeft, [&]() -> bool {
        return this->counter < 0.0f;
    }));

    stateMachine->AddTransition(new StateTransition(stateChasePlayer, stateMoveLeft, [&]() -> bool {
        if (chaseTarget == nullptr) {
            return true;
        }

        if (Vector::Length(GetTransform().GetPosition() - chaseTarget->GetTransform().GetPosition()) > CHASE_DISTANCE) {
            std::cout << "to hang out" << std::endl;
            return true;
        }
        return false;
    }));

    stateMachine->AddTransition(new StateTransition(stateMoveLeft, stateChasePlayer, [&]() -> bool {
        if (chaseTarget == nullptr) {
            return false;
        }

        if (Vector::Length(GetTransform().GetPosition() - chaseTarget->GetTransform().GetPosition()) < CHASE_DISTANCE) {

            std::cout << "to chase" << std::endl;
            return true;
        }
        return false;
    }));
}
StateGameObject ::~StateGameObject() {
    delete stateMachine;
}

void NCL::CSC8503::StateGameObject::DrawWaypoints() const {
    Vector3 lastWaypoint;
    if (!waypoints.empty())
        lastWaypoint = waypoints[0];
    for (auto &waypoint : waypoints) {
        lastWaypoint.y = 1.0f;
        Vector3 endpoint = waypoint;
        endpoint.y = 1.0f;
        Debug::DrawLine(lastWaypoint, endpoint, Debug::RED);
        lastWaypoint = waypoint;
        break;
    }
}

void StateGameObject ::Update(float dt) {
    stateMachine->Update(dt);

    // orientate to the xz-direction of velocity
    Vector3 v = GetPhysicsObject()->GetLinearVelocity();
    // std::cout << v.x << " " << v.y << " " << v.z << std::endl;
    float yaw = atan2(-v.x, v.z);
    // std::cout << yaw << std::endl;
    GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(yaw)));
}
void StateGameObject ::MoveLeft(float dt) {
    GetPhysicsObject()->AddForce({-100, 0, 0});
    counter += dt;
}

void StateGameObject ::MoveRight(float dt) {
    GetPhysicsObject()->AddForce({100, 0, 0});
    counter -= dt;
}

void NCL::CSC8503::StateGameObject::UpdateWaypoints(float dt) {
    if (!chaseTarget) {
        return;
    }

    counter = 5.0;
    std::cout << "UpdateWaypoints" << std::endl;
    waypoints = findPathFunc(GetTransform().GetPosition(), chaseTarget->GetTransform().GetPosition());
    if (waypoints.empty()) {
        std::cout << "can't find path" << std::endl;
        return;
    }
}

void NCL::CSC8503::StateGameObject::ChaseTarget(float dt) {
    Debug::Print("chase", Vector2(5, 92), Debug::RED);

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
