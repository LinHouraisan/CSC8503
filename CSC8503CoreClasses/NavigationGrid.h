#pragma once
#include "NavigationMap.h"
#include <string>
namespace NCL {
namespace CSC8503 {

const char WALL_NODE = '*';
const char FLOOR_NODE = ' ';

class Grid {
public:
    Grid(const std::string &filename, int nodeSize = 1);

    Vector3 GetGridCoord(Vector2i pos, float y) const;

    std::optional<char> GetTile(Vector2i pos) const;

    int width;
    int height;
    int nodeSize;
    const std::vector<std::vector<char>> data;
};

struct Brick {
    char c;
    Vector2i start, end;
    Vector2i GetCenter() const {
        return (end + start) / 2;
    }
};

std::vector<Brick> CombineToBricks(const std::vector<std::vector<char>> &grid, char emptyNode);

struct GridNode {
    GridNode *parent;

    GridNode *connected[4];
    int costs[4];

    Vector3 position;

    float f;
    float g;

    int type;

    GridNode() {
        for (int i = 0; i < 4; ++i) {
            connected[i] = nullptr;
            costs[i] = 0;
        }
        f = 0;
        g = 0;
        type = 0;
        parent = nullptr;
    }
    ~GridNode() {}
};

class NavigationGrid : public NavigationMap {
public:
    NavigationGrid();
    NavigationGrid(const std::string &filename);
    NavigationGrid(const Grid &grid);
    ~NavigationGrid();

    bool FindPath(const Vector3 &from, const Vector3 &to, NavigationPath &outPath) override;

protected:
    bool NodeInList(GridNode *n, std::vector<GridNode *> &list) const;
    GridNode *RemoveBestNode(std::vector<GridNode *> &list) const;
    float Heuristic(GridNode *hNode, GridNode *endNode) const;
    int nodeSize;
    int gridWidth;
    int gridHeight;

    GridNode *allNodes;
};
} // namespace CSC8503
} // namespace NCL
