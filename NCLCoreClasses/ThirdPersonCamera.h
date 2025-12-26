#pragma once

#include "Camera.h"

namespace NCL {

class ThirdPersonCamera : public Camera {
private:
    float viewDist = 5.0f;
    Vector3 worldUp{0, 1, 0};
    Vector3 cameraTarget{0, 1.5, 0};
    Vector3 cameraFront{0, 0, -1};

    Matrix4 view;

    float fov = 45.0f;

public:
    ThirdPersonCamera();

    void Update(Vector3 rolePos);

    Vector3 &GetFront();

    virtual Matrix4 BuildViewMatrix() const override;

    void ProcessMouseMovement();

    virtual Matrix4 BuildProjectionMatrix(float aspectRatio = 1.0f) const override;
};
} // namespace NCL
