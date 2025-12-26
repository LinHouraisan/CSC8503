#pragma once

#include <string>

namespace NCL {
namespace CSC8503 {

enum class CollisionResponseMethod {
    PROJECTION_METHOD,
    IMPULSE_METHOD,
    PENALTY_METHOD,
    END,
};

std::string CollisionResponseMethodToString(CollisionResponseMethod collisionResponseMethod);

struct Config {
    CollisionResponseMethod collisionResponseMethod = CollisionResponseMethod::IMPULSE_METHOD;
    bool cubeAsOBB = false;
    bool printCollisionInfo = false;
};

Config &GetGlobalConfig();

} // namespace CSC8503
} // namespace NCL