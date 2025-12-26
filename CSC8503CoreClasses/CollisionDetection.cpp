#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "Window.h"
#include "Maths.h"
#include "Debug.h"

#include <cassert>

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray &r, const Plane &p, RayCollision &collisions) {
    float ln = Vector::Dot(p.GetNormal(), r.GetDirection());

    if (ln == 0.0f) {
        return false; // direction vectors are perpendicular!
    }

    Vector3 planePoint = p.GetPointOnPlane();

    Vector3 pointDir = planePoint - r.GetPosition();

    float d = Vector::Dot(pointDir, p.GetNormal()) / ln;

    collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

    return true;
}

bool CollisionDetection::RayIntersection(const Ray &r, GameObject &object, RayCollision &collision) {
    bool hasCollided = false;

    const Transform &worldTransform = object.GetTransform();
    const CollisionVolume *volume = object.GetBoundingVolume();

    if (!volume) {
        return false;
    }

    switch (volume->type) {
    case VolumeType::AABB:
        hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume &)*volume, collision);
        break;
    case VolumeType::OBB:
        hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume &)*volume, collision);
        break;
    case VolumeType::Sphere:
        hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume &)*volume, collision);
        break;

    case VolumeType::Capsule:
        hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume &)*volume, collision);
        break;
    }

    return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray &r, const Vector3 &boxPos, const Vector3 &boxSize,
                                            RayCollision &collision) {
    Vector3 boxMin = boxPos - boxSize;
    Vector3 boxMax = boxPos + boxSize;

    Vector3 rayPos = r.GetPosition();
    Vector3 rayDir = r.GetDirection();

    Vector3 tVals(-1, -1, -1);

    for (int i = 0; i < 3; ++i) { // get best 3 intersections
        if (rayDir[i] > 0) {
            tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
        } else if (rayDir[i] < 0) {
            tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
        }
    }
    float bestT = Vector::GetMaxElement(tVals);
    if (bestT < 0.0f) {
        return false; // no backwards rays !
    }
    Vector3 intersection = rayPos + (rayDir * bestT);
    const float epsilon = 0.0001f; // an amount of leeway in our calcs
    for (int i = 0; i < 3; ++i) {
        if (intersection[i] + epsilon < boxMin[i] || intersection[i] - epsilon > boxMax[i]) {
            return false; // best intersection doesn 't touch the box !
        }
    }
    collision.collidedAt = intersection;
    collision.rayDistance = bestT;
    return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray &r, const Transform &worldTransform, const AABBVolume &volume,
                                             RayCollision &collision) {
    Vector3 boxPos = worldTransform.GetPosition();
    Vector3 boxSize = volume.GetHalfDimensions();
    return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray &r, const Transform &worldTransform, const OBBVolume &volume,
                                            RayCollision &collision) {
    Quaternion orientation = worldTransform.GetOrientation();
    Vector3 position = worldTransform.GetPosition();

    Matrix3 transform = Quaternion::RotationMatrix<Matrix3>(orientation);
    Matrix3 invTransform = Quaternion::RotationMatrix<Matrix3>(orientation.Conjugate());

    Vector3 localRayPos = r.GetPosition() - position;

    Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());

    bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);

    if (collided) {
        collision.collidedAt = transform * collision.collidedAt + position;
    }
    return collided;
}

bool CollisionDetection::RaySphereIntersection(const Ray &r, const Transform &worldTransform,
                                               const SphereVolume &volume, RayCollision &collision) {
    Vector3 spherePos = worldTransform.GetPosition();
    float sphereRadius = volume.GetRadius();

    // Get the direction between the ray origin and the sphere origin
    Vector3 dir = (spherePos - r.GetPosition());

    // Then project the sphere 's origin onto our ray direction vector
    float sphereProj = Vector ::Dot(dir, r.GetDirection());

    if (sphereProj < 0.0f) {
        return false; // point is behind the ray !
    }

    // Get closest point on ray line to sphere
    Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);

    float sphereDist = Vector ::Length(point - spherePos);

    if (sphereDist > sphereRadius) {
        return false;
    }

    float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));

    collision.rayDistance = sphereProj - (offset);
    collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);
    return true;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray &r, const Transform &worldTransform,
                                                const CapsuleVolume &volume, RayCollision &collision) {
    return false;
}

bool CollisionDetection::ObjectIntersection(GameObject *a, GameObject *b, CollisionInfo &collisionInfo) {
    const CollisionVolume *volA = a->GetBoundingVolume();
    const CollisionVolume *volB = b->GetBoundingVolume();

    if (!volA || !volB) {
        return false;
    }

    Transform &transformA = a->GetTransform();
    Transform &transformB = b->GetTransform();

    // float dist = Vector::Length(transformA.GetPosition() + transformB.GetPosition());

    // float nearst = 1.0; // TODO: use speed

    // if (dist - nearst > volA->BoundingRadius() + volB->BoundingRadius()) {
    //     return false;
    // }

    collisionInfo.a = a;
    collisionInfo.b = b;

    VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

    // Two AABBs
    if (pairType == VolumeType::AABB) {
        return AABBIntersection((AABBVolume &)*volA, transformA, (AABBVolume &)*volB, transformB, collisionInfo);
    }
    // Two Spheres
    if (pairType == VolumeType::Sphere) {
        return SphereIntersection((SphereVolume &)*volA, transformA, (SphereVolume &)*volB, transformB, collisionInfo);
    }
    // Two OBBs
    if (pairType == VolumeType::OBB) {
        return OBBIntersection((OBBVolume &)*volA, transformA, (OBBVolume &)*volB, transformB, collisionInfo);
    }
    // Two Capsules

    // AABB vs Sphere pairs
    if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
        return AABBSphereIntersection((AABBVolume &)*volA, transformA, (SphereVolume &)*volB, transformB,
                                      collisionInfo);
    }
    if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
        collisionInfo.a = b;
        collisionInfo.b = a;
        return AABBSphereIntersection((AABBVolume &)*volB, transformB, (SphereVolume &)*volA, transformA,
                                      collisionInfo);
    }

    // OBB vs sphere pairs
    if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
        return OBBSphereIntersection((OBBVolume &)*volA, transformA, (SphereVolume &)*volB, transformB, collisionInfo);
    }
    if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
        collisionInfo.a = b;
        collisionInfo.b = a;
        return OBBSphereIntersection((OBBVolume &)*volB, transformB, (SphereVolume &)*volA, transformA, collisionInfo);
    }

    // Capsule vs other interactions
    if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
        return SphereCapsuleIntersection((CapsuleVolume &)*volA, transformA, (SphereVolume &)*volB, transformB,
                                         collisionInfo);
    }
    if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
        collisionInfo.a = b;
        collisionInfo.b = a;
        return SphereCapsuleIntersection((CapsuleVolume &)*volB, transformB, (SphereVolume &)*volA, transformA,
                                         collisionInfo);
    }

    if (volA->type == VolumeType::Capsule && volB->type == VolumeType::AABB) {
        return AABBCapsuleIntersection((CapsuleVolume &)*volA, transformA, (AABBVolume &)*volB, transformB,
                                       collisionInfo);
    }
    if (volB->type == VolumeType::Capsule && volA->type == VolumeType::AABB) {
        collisionInfo.a = b;
        collisionInfo.b = a;
        return AABBCapsuleIntersection((CapsuleVolume &)*volB, transformB, (AABBVolume &)*volA, transformA,
                                       collisionInfo);
    }

    return false;
}

bool CollisionDetection::AABBTest(const Vector3 &posA, const Vector3 &posB, const Vector3 &halfSizeA,
                                  const Vector3 &halfSizeB) {
    Vector3 delta = posB - posA;
    Vector3 totalSize = halfSizeA + halfSizeB;

    if (abs(delta.x) < totalSize.x && abs(delta.y) < totalSize.y && abs(delta.z) < totalSize.z) {
        return true;
    }
    return false;
}

// AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume &volumeA, const Transform &worldTransformA,
                                          const AABBVolume &volumeB, const Transform &worldTransformB,
                                          CollisionInfo &collisionInfo) {
    Vector3 boxAPos = worldTransformA.GetPosition();
    Vector3 boxBPos = worldTransformB.GetPosition();

    Vector3 boxASize = volumeA.GetHalfDimensions();
    Vector3 boxBSize = volumeB.GetHalfDimensions();

    bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);
    if (overlap) {
        static const Vector3 faces[6] = {
            Vector3(-1, 0, 0), Vector3(1, 0, 0),  Vector3(0, -1, 0),
            Vector3(0, 1, 0),  Vector3(0, 0, -1), Vector3(0, 0, 1),
        };

        Vector3 maxA = boxAPos + boxASize;
        Vector3 minA = boxAPos - boxASize;

        Vector3 maxB = boxBPos + boxBSize;
        Vector3 minB = boxBPos - boxBSize;

        float distances[6] = {
            (maxB.x - minA.x), // distance of box 'b' to 'left ' of 'a '.
            (maxA.x - minB.x), // distance of box 'b' to 'right ' of 'a '.
            (maxB.y - minA.y), // distance of box 'b' to 'bottom ' of 'a '.
            (maxA.y - minB.y), // distance of box 'b' to 'top ' of 'a '.
            (maxB.z - minA.z), // distance of box 'b' to 'far ' of 'a '.
            (maxA.z - minB.z)  // distance of box 'b' to 'near ' of 'a '.
        };
        float penetration = FLT_MAX;
        Vector3 bestAxis;

        for (int i = 0; i < 6; i++) {
            if (distances[i] < penetration) {
                penetration = distances[i];
                bestAxis = faces[i];
            }
        }
        collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
        return true;
    }
    return false;
}

// Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume &volumeA, const Transform &worldTransformA,
                                            const SphereVolume &volumeB, const Transform &worldTransformB,
                                            CollisionInfo &collisionInfo) {
    float radii = volumeA.GetRadius() + volumeB.GetRadius();
    Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();

    float deltaLength = Vector ::Length(delta);

    if (deltaLength < radii) {
        float penetration = (radii - deltaLength);
        Vector3 normal = Vector ::Normalise(delta);
        Vector3 localA = normal * volumeA.GetRadius();
        Vector3 localB = -normal * volumeB.GetRadius();

        collisionInfo.AddContactPoint(localA, localB, normal, penetration);
        return true; // we 're colliding !
    }
    return false;
}

bool CollisionDetection::BoxSphereIntersection(const Vector3 &boxSize, const Vector3 &boxPos, float sphereRadius,
                                               const Vector3 &spherePos, CollisionInfo &collisionInfo) {
    Vector3 delta = spherePos - boxPos;
    Vector3 closestPointOnBox = Vector::Clamp(delta, -boxSize, boxSize);
    Vector3 localPoint = delta - closestPointOnBox;
    float distance = Vector::Length(localPoint);
    if (distance < sphereRadius) {
        Vector3 collisionNormal = Vector::Normalise(localPoint);
        float penetration = sphereRadius - distance;
        Vector3 localA = closestPointOnBox;
        Vector3 localB = -collisionNormal * sphereRadius;

        collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
        return true;
    }
    return false;
}

// AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume &volumeA, const Transform &worldTransformA,
                                                const SphereVolume &volumeB, const Transform &worldTransformB,
                                                CollisionInfo &collisionInfo) {
    bool collided = BoxSphereIntersection(volumeA.GetHalfDimensions(), worldTransformA.GetPosition(),
                                          volumeB.GetRadius(), worldTransformB.GetPosition(), collisionInfo);
    if (collided) {
        collisionInfo.point.localA = Vector3();
        return true;
    }
    return false;
}

bool CollisionDetection::SATTest(const Vector3 &sizeA, const Matrix3 &rotationA, const Vector3 &sizeB,
                                 const Matrix3 &rotationB, const Vector3 &centerDist, const Vector3 &axis,
                                 float &overlap) {
    overlap = 0.0f;
    float localDist = Vector::Dot(centerDist, axis);
    localDist = abs(localDist);
    float radiusA = 0.0f, radiusB = 0.0f;
    for (int i = 0; i < 3; i++) {
        radiusA += sizeA[i] * abs(Vector::Dot(rotationA.GetColumn(i), axis));
        radiusB += sizeB[i] * abs(Vector::Dot(rotationB.GetColumn(i), axis));
    }
    if ((radiusA + radiusB) > localDist) {
        overlap = radiusA + radiusB - localDist;
        return true;
    }
    return false;
}

Vector3 CollisionDetection::BoxCornerDir(int index) {
    Vector3 cornerDir;
    cornerDir.x = index % 2 == 0 ? -1 : 1;
    cornerDir.z = (index / 2) % 2 == 0 ? 1 : -1;
    cornerDir.y = index / 4 == 0 ? -1 : 1;
    return cornerDir;
}

Vector3 CollisionDetection::FindClosestNormalOnBox(const Vector3 &localNormal) {
    float minDist = 1.0f;
    Vector3 minNormal;
    for (int i = 0; i < 3; i++) {
        float dist = abs(localNormal[i]);
        if (dist < minDist && dist > 0) {
            minDist = dist;
            minNormal = Vector3(0, 0, 0);
            minNormal[i] = localNormal[i] / dist;
        }
    }
    return minNormal;
}

bool CollisionDetection::BoxOBBIntersection(const Vector3 &boxSize, const Vector3 &boxPos, const OBBVolume &volumeOBB,
                                            const Transform &TransformOBB, CollisionInfo &collisionInfo) {
    static const float bias = 0.0001f;

    Vector3 posOBB = TransformOBB.GetPosition();
    Vector3 sizeOBB = volumeOBB.GetHalfDimensions();
    Quaternion orientationOBB = TransformOBB.GetOrientation();
    Matrix3 rotationOBB = Quaternion::RotationMatrix<Matrix3>(orientationOBB);
    Matrix3 invRotationOBB = Quaternion::RotationMatrix<Matrix3>(orientationOBB.Conjugate());

    Vector3 centerDistance = posOBB - boxPos;

    bool collided = false;
    Vector3 collisionNormal;
    float minPenetration = FLT_MAX;

    int indexBox = -1, indexOBB = -1;
    float overlapBox[3] = {0};
    float overlapOBB[3] = {0};

    std::vector<Vector3> TestedNormals;

    // Test Box normals
    Matrix3 rotationBox = Matrix3();
    for (int i = 0; i < 3; i++) {
        Vector3 normal = rotationBox.GetColumn(i);
        float overlap = 0.0f;
        if (!SATTest(boxSize, rotationBox, sizeOBB, rotationOBB, centerDistance, normal, overlap)) {
            return false;
        }
        overlapBox[i] = overlap;
        if (overlap < minPenetration) {
            minPenetration = overlap;
            collisionNormal = normal;
            indexBox = i;
            indexOBB = -1;
        }
        TestedNormals.push_back(normal);
    }

    // Test OBB normals
    for (int i = 0; i < 3; i++) {
        Vector3 normal = rotationOBB.GetColumn(i);
        float overlap = 0.0f;
        if (!SATTest(boxSize, rotationBox, sizeOBB, rotationOBB, centerDistance, normal, overlap)) {
            return false;
        }
        overlapOBB[i] = overlap;
        if (overlap < minPenetration) {
            minPenetration = overlap;
            collisionNormal = normal;
            indexBox = -1;
            indexOBB = i;
        }
        TestedNormals.push_back(normal);
    }

    // Test cross product of normals
    for (int i = 0; i < 3; i++) {
        Vector3 normalBox = rotationBox.GetColumn(i);
        for (int j = 0; j < 3; j++) {
            Vector3 normalOBB = rotationOBB.GetColumn(i);
            Vector3 normal = Vector::Normalise(Vector::Cross(normalBox, normalOBB));
            if (Vector::Length(normal) == 0) {
                continue;
            }
            float overlap = 0.0f;
            if (!SATTest(boxSize, rotationBox, sizeOBB, rotationOBB, centerDistance, normal, overlap)) {
                return false;
            }
            if (overlap < minPenetration) {
                minPenetration = overlap;
                collisionNormal = normal;
                indexBox = i;
                indexOBB = j;
            }
            TestedNormals.push_back(normal);
        }
    }

    Vector3 cpointBox, cpointOBB;
    float cnormalDist = Vector::Dot(centerDistance, collisionNormal);
    float dSign = cnormalDist / abs(cnormalDist);
    collisionNormal *= dSign;
    // Vector3 collisionNormalSigned = collisionNormal * dSign;

    if (indexBox >= 0 && indexOBB < 0) {
        Vector3 localNormal;
        localNormal[indexBox] = 1 * dSign;
        float planeD = -abs(Vector::Dot(localNormal, boxSize));
        Vector3 sum;
        float num = 0.0f;
        for (int i = 0; i < 8; i++) {
            Vector3 VertexOBB = rotationOBB * (BoxCornerDir(i) * sizeOBB) + centerDistance;
            float side = Vector::Dot(localNormal, VertexOBB) + planeD;
            if (side < 0) {
                if (abs(side) > minPenetration - 0.01f) {
                    num += 1.0f;
                    sum += Vector::Clamp(VertexOBB, -boxSize, boxSize);
                    // sum += VertexOBB;
                }
            }
        }
        if (num > 0.0f) {
            Vector3 cPoint = sum / num;
            cpointBox = cPoint + localNormal * minPenetration;
            cpointOBB = invRotationOBB * (cPoint - centerDistance);
        }
    } else if (indexBox < 0 && indexOBB >= 0) {
        Vector3 localNormal;
        localNormal[indexOBB] = 1 * dSign;
        float planeD = -abs(Vector::Dot(localNormal, sizeOBB));
        Vector3 sum;
        float num = 0.0f;
        for (int i = 0; i < 8; i++) {
            Vector3 VertexBox = invRotationOBB * (BoxCornerDir(i) * boxSize - centerDistance);
            float side = Vector::Dot(localNormal, VertexBox) + planeD;
            if (side < 0) {
                if (abs(side) > minPenetration - 0.01f) {
                    num += 1.0f;
                    sum += Vector::Clamp(VertexBox, -sizeOBB, sizeOBB);
                    // sum += VertexBox;
                }
            }
        }
        if (num > 0.0f) {
            Vector3 cPoint = sum / num;
            cpointOBB = cPoint + localNormal * minPenetration;
            cpointBox = rotationOBB * cPoint + centerDistance;
        }
    } else {
        // Calculate the contact point on 2 edges
        float dNormal = 0.0f;
        for (int i = 0; i < 3; i++) {
            dNormal += boxSize[i] * abs(Vector::Dot(rotationBox.GetColumn(i), collisionNormal));
        }
        Vector3 nBox = FindClosestNormalOnBox(collisionNormal);
        Vector3 localCN_OBB = invRotationOBB * (-collisionNormal);
        Vector3 nOBB_local = FindClosestNormalOnBox(localCN_OBB);
        Vector3 nOBB = rotationOBB * nOBB_local;
        float dBox = abs(Vector::Dot(boxSize, nBox));
        float dOBB = abs(Vector::Dot(
            (rotationOBB * (nOBB_local * sizeOBB) + centerDistance + collisionNormal * minPenetration), nOBB));
        /*float elementA[9];
        for (int i = 0; i < 3; i++)
        {
                elementA[i * 3] = collisionNormal[i];
                elementA[i * 3 + 1] = nBox[i];
                elementA[i * 3 + 2] = nOBB[i];
        }*/
        Matrix3 A;
        A.SetRow(0, collisionNormal);
        A.SetRow(1, nBox);
        A.SetRow(2, nOBB);
        Vector3 D(dNormal, dBox, dOBB);
        Matrix4 A4(A);
        cpointBox = Matrix::Inverse(A4) * Vector4(D, 0);
        cpointOBB = invRotationOBB * (cpointBox - collisionNormal * minPenetration - centerDistance);
    }

    // printf("cpointBox : (%f,%f,%f)\n", cpointBox.x, cpointBox.y, cpointBox.z);
    // printf("cpointOBB : (%f,%f,%f)\n", cpointOBB.x, cpointOBB.y, cpointOBB.z);
    // printf("collisionNormal : (%f,%f,%f)\n", collisionNormal.x, collisionNormal.y, collisionNormal.z);
    // printf("minPenetration : %f\n", minPenetration);

    collisionInfo.AddContactPoint(cpointBox, cpointOBB, collisionNormal, minPenetration);
    // collisionInfo.AddContactPoint(Vector3(), Vector3(), collisionNormalSigned, minPenetration);

    return true;
}

bool CollisionDetection::OBBSphereIntersection(const OBBVolume &volumeA, const Transform &worldTransformA,
                                               const SphereVolume &volumeB, const Transform &worldTransformB,
                                               CollisionInfo &collisionInfo) {
    Quaternion orientationA = worldTransformA.GetOrientation();
    Vector3 positionA = worldTransformA.GetPosition();
    Matrix3 transformA = Quaternion::RotationMatrix<Matrix3>(orientationA);
    Matrix3 invTranfromA = Quaternion::RotationMatrix<Matrix3>(orientationA.Conjugate());
    Vector3 localSpherePos = worldTransformB.GetPosition() - positionA;
    bool collided = BoxSphereIntersection(volumeA.GetHalfDimensions(), Vector3(), volumeB.GetRadius(),
                                          invTranfromA * localSpherePos, collisionInfo);
    if (collided) {
        collisionInfo.point.normal = transformA * collisionInfo.point.normal;
    }
    return collided;
}

bool CollisionDetection::AABBCapsuleIntersection(const CapsuleVolume &volumeA, const Transform &worldTransformA,
                                                 const AABBVolume &volumeB, const Transform &worldTransformB,
                                                 CollisionInfo &collisionInfo) {
    return false;
}

bool CollisionDetection::SphereCapsuleIntersection(const CapsuleVolume &volumeA, const Transform &worldTransformA,
                                                   const SphereVolume &volumeB, const Transform &worldTransformB,
                                                   CollisionInfo &collisionInfo) {
    return false;
}

bool CollisionDetection::OBBIntersection(const OBBVolume &volumeA, const Transform &worldTransformA,
                                         const OBBVolume &volumeB, const Transform &worldTransformB,
                                         CollisionInfo &collisionInfo) {
    Quaternion orientationA = worldTransformA.GetOrientation();
    Matrix3 rotationA = Quaternion::RotationMatrix<Matrix3>(orientationA);
    Matrix3 invRotationA = Quaternion::RotationMatrix<Matrix3>(orientationA.Conjugate());

    Quaternion orientationB = worldTransformB.GetOrientation();
    Quaternion localOrientationB = orientationB * orientationA.Conjugate();
    Vector3 localPosB = worldTransformB.GetPosition() - worldTransformA.GetPosition();
    Transform localTransformB;
    localTransformB.SetOrientation(localOrientationB);
    localTransformB.SetPosition(invRotationA * localPosB);

    bool collided = BoxOBBIntersection(volumeA.GetHalfDimensions(), Vector3(), volumeB, localTransformB, collisionInfo);
    if (collided) {
        collisionInfo.point.normal = rotationA * collisionInfo.point.normal;
    }

    return collided;
}

Matrix4 GenerateInverseView(const Camera &c) {
    float pitch = c.GetPitch();
    float yaw = c.GetYaw();
    Vector3 position = c.GetPosition();

    Matrix4 iview = Matrix::Translation(position) * Matrix::Rotation(-yaw, Vector3(0, -1, 0)) *
                    Matrix::Rotation(-pitch, Vector3(-1, 0, 0));

    return iview;
}

Matrix4 GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
    float negDepth = nearPlane - farPlane;

    float invNegDepth = negDepth / (2 * (farPlane * nearPlane));

    Matrix4 m;

    float h = 1.0f / tan(fov * PI_OVER_360);

    m.array[0][0] = aspect / h;
    m.array[1][1] = tan(fov * PI_OVER_360);
    m.array[2][2] = 0.0f;

    m.array[2][3] = invNegDepth; //// +PI_OVER_360;
    m.array[3][2] = -1.0f;
    m.array[3][3] = (0.5f / nearPlane) + (0.5f / farPlane);

    return m;
}

Vector3 CollisionDetection::Unproject(const Vector3 &screenPos, const PerspectiveCamera &cam) {
    Vector2i screenSize = Window::GetWindow()->GetScreenSize();

    float aspect = Window::GetWindow()->GetScreenAspect();
    float fov = cam.GetFieldOfVision();
    float nearPlane = cam.GetNearPlane();
    float farPlane = cam.GetFarPlane();

    // Create our inverted matrix! Note how that to get a correct inverse matrix,
    // the order of matrices used to form it are inverted, too.
    Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

    Matrix4 proj = cam.BuildProjectionMatrix(aspect);

    // Our mouse position x and y values are in 0 to screen dimensions range,
    // so we need to turn them into the -1 to 1 axis range of clip space.
    // We can do that by dividing the mouse values by the width and height of the
    // screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
    // and then subtracting 1 (-1.0 to 1.0).
    Vector4 clipSpace = Vector4((screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
                                (screenPos.y / (float)screenSize.y) * 2.0f - 1.0f, (screenPos.z), 1.0f);

    // Then, we multiply our clipspace coordinate by our inverted matrix
    Vector4 transformed = invVP * clipSpace;

    // our transformed w coordinate is now the 'inverse' perspective divide, so
    // we can reconstruct the final world space by dividing x,y,and z by w.
    return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const PerspectiveCamera &cam) {
    Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
    Vector2i screenSize = Window::GetWindow()->GetScreenSize();

    // We remove the y axis mouse position from height as OpenGL is 'upside down',
    // and thinks the bottom left is the origin, instead of the top left!
    Vector3 nearPos = Vector3(screenMouse.x, screenSize.y - screenMouse.y, -0.99999f);

    // We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
    // causes the unproject function to go a bit weird.
    Vector3 farPos = Vector3(screenMouse.x, screenSize.y - screenMouse.y, 0.99999f);

    Vector3 a = Unproject(nearPos, cam);
    Vector3 b = Unproject(farPos, cam);
    Vector3 c = b - a;

    c = Vector::Normalise(c);

    return Ray(cam.GetPosition(), c);
}

// http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
    Matrix4 m;

    float t = tan(fov * PI_OVER_360);

    float neg_depth = nearPlane - farPlane;

    const float h = 1.0f / t;

    float c = (farPlane + nearPlane) / neg_depth;
    float e = -1.0f;
    float d = 2.0f * (nearPlane * farPlane) / neg_depth;

    m.array[0][0] = aspect / h;
    m.array[1][1] = tan(fov * PI_OVER_360);
    m.array[2][2] = 0.0f;

    m.array[2][3] = 1.0f / d;

    m.array[3][2] = 1.0f / e;
    m.array[3][3] = -c / (d * e);

    return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera &c) {
    float pitch = c.GetPitch();
    float yaw = c.GetYaw();
    Vector3 position = c.GetPosition();

    Matrix4 iview = Matrix::Translation(position) * Matrix::Rotation(yaw, Vector3(0, 1, 0)) *
                    Matrix::Rotation(pitch, Vector3(1, 0, 0));

    return iview;
}

/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3 CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov,
                                                    const PerspectiveCamera &c) {
    // Create our inverted matrix! Note how that to get a correct inverse matrix,
    // the order of matrices used to form it are inverted, too.
    Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

    Vector2i screenSize = Window::GetWindow()->GetScreenSize();

    // Our mouse position x and y values are in 0 to screen dimensions range,
    // so we need to turn them into the -1 to 1 axis range of clip space.
    // We can do that by dividing the mouse values by the width and height of the
    // screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
    // and then subtracting 1 (-1.0 to 1.0).
    Vector4 clipSpace = Vector4((position.x / (float)screenSize.x) * 2.0f - 1.0f,
                                (position.y / (float)screenSize.y) * 2.0f - 1.0f, (position.z) - 1.0f, 1.0f);

    // Then, we multiply our clipspace coordinate by our inverted matrix
    Vector4 transformed = invVP * clipSpace;

    // our transformed w coordinate is now the 'inverse' perspective divide, so
    // we can reconstruct the final world space by dividing x,y,and z by w.
    return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}
