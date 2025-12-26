#pragma once

#include <GameObject.h>

namespace NCL::CSC8503 {

class Player : public GameObject {
public:
    Player(int selfIndex, int health, int score)
        : GameObject("player"), selfIndex(selfIndex), health(health), score(score) {}

    virtual void OnCollisionBegin(GameObject *otherObject) {
        // std::cout << "OnCollisionBegin event occured!\n";
    }

    virtual void OnCollisionEnd(GameObject *otherObject) {
        // std::cout << "OnCollisionEnd event occured!\n";
    }

    int selfIndex;
    int health;
    int score;
};
} // namespace NCL::CSC8503