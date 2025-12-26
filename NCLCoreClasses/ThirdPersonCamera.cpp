#include "ThirdPersonCamera.h"

namespace NCL {

ThirdPersonCamera::ThirdPersonCamera() {

    position = {0, 2.5, 0};
}

void ThirdPersonCamera::Update(Vector3 rolePos) {
    rolePos.y += 1.5f;

    ProcessMouseMovement();

    cameraTarget = rolePos;

    position = cameraTarget - cameraFront * viewDist;

    view = Matrix::View(position, cameraTarget, worldUp);
}

Vector3 &ThirdPersonCamera::GetFront() {
    return cameraFront;
}

Matrix4 ThirdPersonCamera::BuildViewMatrix() const {
    return view;
}

void ThirdPersonCamera::ProcessMouseMovement() {
    float y = activeController->GetNamedAxis("YLook");
    float x = activeController->GetNamedAxis("XLook");

    pitch -= y;
    yaw += x;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    Vector3 front;
    front.x = cos(Maths::DegreesToRadians(pitch)) * sin(Maths::DegreesToRadians(yaw)); //
    front.y = sin(Maths::DegreesToRadians(pitch));
    front.z = -cos(Maths::DegreesToRadians(pitch)) * cos(Maths::DegreesToRadians(yaw));
    cameraFront = Vector::Normalise(front);
}
Matrix4 ThirdPersonCamera::BuildProjectionMatrix(float currentAspect) const {
    return Matrix::Perspective(nearPlane, farPlane, currentAspect, fov);
}
} // namespace NCL