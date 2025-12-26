#include "Config.h"

namespace NCL {
namespace CSC8503 {
std::string CollisionResponseMethodToString(CollisionResponseMethod collisionResponseMethod) {
    switch (collisionResponseMethod) {
    case CollisionResponseMethod::PROJECTION_METHOD:
        return "projection method";
    case CollisionResponseMethod::IMPULSE_METHOD:
        return "impulse method";
    case CollisionResponseMethod::PENALTY_METHOD:
        return "penalty method";
    }

    return "unknown";
}

Config &GetGlobalConfig() {
    static Config config;
    return config;
}

} // namespace CSC8503
} // namespace NCL