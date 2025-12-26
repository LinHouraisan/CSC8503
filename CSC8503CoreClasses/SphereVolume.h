#pragma once
#include "CollisionVolume.h"

namespace NCL {
class SphereVolume : CollisionVolume {
public:
    SphereVolume(float sphereRadius = 1.0f) : CollisionVolume(sphereRadius) {
        type = VolumeType::Sphere;
        radius = sphereRadius;
    }
    ~SphereVolume() {}

    float GetRadius() const {
        return radius;
    }

protected:
    float radius;
};
} // namespace NCL
