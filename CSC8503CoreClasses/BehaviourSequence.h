#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourSequence : public BehaviourNodeWithChildren {
public:
    BehaviourSequence(const std::string &nodeName) : BehaviourNodeWithChildren(nodeName) {}
    ~BehaviourSequence() {}
    BehaviourState Execute(float dt) override {
        // std::cout << "Executing sequence " << name << "\n";

        if (currentState == Ongoing) {
            currentState = currentChildNode->Execute(dt);
            switch (currentState) {
            case Failure:
                currentChildNode = nullptr;
                return currentState;
            case Success:
                break;
            case Ongoing:
                return currentState;
            }
        }

        for (auto &i : childNodes) {
            if (i == currentChildNode) {
                currentChildNode = nullptr;
                continue;
            }
            currentState = i->Execute(dt);
            switch (currentState) {
            case Success:
                continue;
            case Failure:
            case Ongoing:
                currentChildNode = i;
                return currentState;
            }
        }
        return currentState;
    }
};