#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourSelector : public BehaviourNodeWithChildren {
public:
    BehaviourSelector(const std::string &nodeName) : BehaviourNodeWithChildren(nodeName) {}
    ~BehaviourSelector() {}
    BehaviourState Execute(float dt) override {
        // std::cout << "Executing selector " << name << "\n";
        if (currentState == Ongoing) {
            currentState = currentChildNode->Execute(dt);
            switch (currentState) {
            case Failure:
                break;
            case Success:
                currentChildNode = nullptr;
                return currentState;
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
            case Failure:
                continue;
            case Success:
            case Ongoing:
                currentChildNode = i;
                return currentState;
            }
        }
        return currentState;
    }
};