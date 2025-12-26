#pragma once
#include "CollisionVolume.h"

namespace NCL {
class CapsuleVolume : public CollisionVolume {
public:
    CapsuleVolume(float halfHeight, float radius) : CollisionVolume(halfHeight * 2.0 + radius * 2.0) {
        this->halfHeight = halfHeight;
        this->radius = radius;
        this->type = VolumeType::Capsule;
    };
    ~CapsuleVolume() {}
    float GetRadius() const {
        return radius;
    }

    float GetHalfHeight() const {
        return halfHeight;
    }

protected:
    float radius;
    float halfHeight;
};
} // namespace NCL
