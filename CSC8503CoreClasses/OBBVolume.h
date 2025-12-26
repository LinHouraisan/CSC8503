#pragma once
#include "CollisionVolume.h"
#include "Vector.h"

namespace NCL {
class OBBVolume : CollisionVolume {
public:
    OBBVolume(const Maths::Vector3 &halfDims)
        : CollisionVolume(NCL::Maths::Vector::GetMaxElement(halfDims) * sqrt(2.0)) {
        type = VolumeType::OBB;
        halfSizes = halfDims;
    }
    ~OBBVolume() {}

    Maths::Vector3 GetHalfDimensions() const {
        return halfSizes;
    }

protected:
    Maths::Vector3 halfSizes;
};
} // namespace NCL
