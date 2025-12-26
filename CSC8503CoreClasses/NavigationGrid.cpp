#include "NavigationGrid.h"
#include "Assets.h"

#include <fstream>
#include <sstream>
#include <cassert>

using namespace NCL;
using namespace CSC8503;

const int LEFT_NODE = 0;
const int RIGHT_NODE = 1;
const int TOP_NODE = 2;
const int BOTTOM_NODE = 3;

Grid::Grid(const std::string &filename, int nodeSize) : nodeSize(nodeSize) {
    assert(nodeSize == 1);
    // load grid
    std::string gridStr;
    bool ok = Assets::ReadTextFile(filename, gridStr);
    if (!ok) {
        throw std::runtime_error("failed to load grid file");
    }

    int colNums = 0;
    std::stringstream ss(gridStr);
    {
        std::vector<std::vector<char>> &gameGrid = const_cast<std::vector<std::vector<char>> &>(data);
        std::string line;
        while (std::getline(ss, line)) {
            colNums = std::max(colNums, static_cast<int>(line.size()));
            std::vector<char> v;
            v.insert(v.begin(), line.begin(), line.end());

            gameGrid.push_back(v);
        }

        for (auto &line : gameGrid) {
            int remain = colNums - line.size();
            for (int k = 0; k < remain; ++k) {
                line.push_back(FLOOR_NODE);
            }
        }
    }
    width = data[0].size();
    height = data.size();
}

Vector3 Grid::GetGridCoord(Vector2i pos, float y) const {
    // return Vector3(pos.x, y, pos.y) -
    //        Vector3{static_cast<float>(data[0].size()) / 2.0f, 0, static_cast<float>(data.size()) / 2.0f};
    return Vector3{static_cast<float>(pos.x), y, static_cast<float>(pos.y)};
}

std::optional<char> NCL::CSC8503::Grid::GetTile(Vector2i pos) const {
    std::optional<char> ret;
    if (pos.y >= 0 && pos.y < height) {
        if (pos.x >= 0 && pos.x < width) {
            return data[pos.y][pos.x];
        }
    }
    return std::nullopt;
}

NavigationGrid::NavigationGrid() {
    nodeSize = 0;
    gridWidth = 0;
    gridHeight = 0;
    allNodes = nullptr;
}

NavigationGrid::NavigationGrid(const std::string &filename) : NavigationGrid() {
    std::ifstream infile(Assets::DATADIR + filename);

    infile >> nodeSize;
    infile >> gridWidth;
    infile >> gridHeight;

    allNodes = new GridNode[gridWidth * gridHeight];

    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            GridNode &n = allNodes[(gridWidth * y) + x];
            char type = 0;
            infile >> type;
            n.type = type;
            n.position = Vector3((float)(x * nodeSize), 0, (float)(y * nodeSize));
        }
    }

    // now to build the connectivity between the nodes
    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            GridNode &n = allNodes[(gridWidth * y) + x];

            if (y > 0) { // get the above node
                n.connected[0] = &allNodes[(gridWidth * (y - 1)) + x];
            }
            if (y < gridHeight - 1) { // get the below node
                n.connected[1] = &allNodes[(gridWidth * (y + 1)) + x];
            }
            if (x > 0) { // get left node
                n.connected[2] = &allNodes[(gridWidth * (y)) + (x - 1)];
            }
            if (x < gridWidth - 1) { // get right node
                n.connected[3] = &allNodes[(gridWidth * (y)) + (x + 1)];
            }
            for (int i = 0; i < 4; ++i) {
                if (n.connected[i]) {
                    if (n.connected[i]->type == '.') {
                        n.costs[i] = 1;
                    }
                    if (n.connected[i]->type == WALL_NODE) {
                        n.connected[i] = nullptr; // actually a wall, disconnect!
                    }
                }
            }
        }
    }
}

NavigationGrid::NavigationGrid(const Grid &grid) : nodeSize(grid.nodeSize) {

    gridWidth = grid.data[0].size();
    gridHeight = grid.data.size();

    allNodes = new GridNode[gridWidth * gridHeight];

    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            GridNode &n = allNodes[(gridWidth * y) + x];
            char type = grid.data[y][x];
            n.type = type;
            n.position = grid.GetGridCoord({x, y}, 0);
        }
    }

    // now to build the connectivity between the nodes
    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            GridNode &n = allNodes[(gridWidth * y) + x];

            if (y > 0) { // get the above node
                n.connected[0] = &allNodes[(gridWidth * (y - 1)) + x];
            }
            if (y < gridHeight - 1) { // get the below node
                n.connected[1] = &allNodes[(gridWidth * (y + 1)) + x];
            }
            if (x > 0) { // get left node
                n.connected[2] = &allNodes[(gridWidth * (y)) + (x - 1)];
            }
            if (x < gridWidth - 1) { // get right node
                n.connected[3] = &allNodes[(gridWidth * (y)) + (x + 1)];
            }
            for (int i = 0; i < 4; ++i) {
                if (n.connected[i]) {
                    if (n.connected[i]->type == FLOOR_NODE) {
                        n.costs[i] = 1;
                    }
                    if (n.connected[i]->type == WALL_NODE) {
                        n.connected[i] = nullptr; // actually a wall, disconnect!
                    }
                }
            }
        }
    }
}

NavigationGrid::~NavigationGrid() {
    delete[] allNodes;
}

bool NavigationGrid::FindPath(const Vector3 &from, const Vector3 &to, NavigationPath &outPath) {
    // need to work out which node 'from' sits in, and 'to' sits in
    int fromX = ((int)from.x / nodeSize);
    int fromZ = ((int)from.z / nodeSize);

    int toX = ((int)to.x / nodeSize);
    int toZ = ((int)to.z / nodeSize);

    if (fromX < 0 || fromX > gridWidth - 1 || fromZ < 0 || fromZ > gridHeight - 1) {
        return false; // outside of map region!
    }

    if (toX < 0 || toX > gridWidth - 1 || toZ < 0 || toZ > gridHeight - 1) {
        return false; // outside of map region!
    }

    GridNode *startNode = &allNodes[(fromZ * gridWidth) + fromX];
    GridNode *endNode = &allNodes[(toZ * gridWidth) + toX];

    std::vector<GridNode *> openList;
    std::vector<GridNode *> closedList;

    openList.push_back(startNode);

    startNode->f = 0;
    startNode->g = 0;
    startNode->parent = nullptr;

    GridNode *currentBestNode = nullptr;

    while (!openList.empty()) {
        currentBestNode = RemoveBestNode(openList);

        if (currentBestNode == endNode) { // we've found the path!
            GridNode *node = endNode;
            while (node != nullptr) {
                outPath.PushWaypoint(node->position);
                node = node->parent;
            }
            return true;
        } else {
            for (int i = 0; i < 4; ++i) {
                GridNode *neighbour = currentBestNode->connected[i];
                if (!neighbour) { // might not be connected...
                    continue;
                }
                bool inClosed = NodeInList(neighbour, closedList);
                if (inClosed) {
                    continue; // already discarded this neighbour...
                }

                float h = Heuristic(neighbour, endNode);
                float g = currentBestNode->g + currentBestNode->costs[i];
                float f = h + g;

                bool inOpen = NodeInList(neighbour, openList);

                if (!inOpen) { // first time we've seen this neighbour
                    openList.emplace_back(neighbour);
                }
                if (!inOpen || f < neighbour->f) { // might be a better route to this neighbour
                    neighbour->parent = currentBestNode;
                    neighbour->f = f;
                    neighbour->g = g;
                }
            }
            closedList.emplace_back(currentBestNode);
        }
    }
    return false; // open list emptied out with no path!
}

bool NavigationGrid::NodeInList(GridNode *n, std::vector<GridNode *> &list) const {
    std::vector<GridNode *>::iterator i = std::find(list.begin(), list.end(), n);
    return i == list.end() ? false : true;
}

GridNode *NavigationGrid::RemoveBestNode(std::vector<GridNode *> &list) const {
    std::vector<GridNode *>::iterator bestI = list.begin();

    GridNode *bestNode = *list.begin();

    for (auto i = list.begin(); i != list.end(); ++i) {
        if ((*i)->f < bestNode->f) {
            bestNode = (*i);
            bestI = i;
        }
    }
    list.erase(bestI);

    return bestNode;
}

float NavigationGrid::Heuristic(GridNode *hNode, GridNode *endNode) const {
    return Vector::Length(hNode->position - endNode->position);
}

std::vector<Brick> NCL::CSC8503::CombineToBricks(const std::vector<std::vector<char>> &grid, char emptyNode) {

    int rowNums = grid.size();
    if (grid.empty()) {
        return {};
    }
    int colNums = grid[0].size();

    auto CalcStarWidth = [](const std::vector<std::vector<char>> &grid, char node, int iStart, int jStart, int rowNums,
                            int colNums) {
        for (int j = jStart; j < colNums; ++j) {
            if (grid[iStart][j] != node) {
                return j - jStart;
            }
        }
        return colNums - jStart;
    };

    auto CalcStarHeight = [](const std::vector<std::vector<char>> &grid, char node, int iStart, int jStart, int rowNums,
                             int colNums) {
        for (int i = iStart; i < rowNums; ++i) {
            if (grid[i][jStart] != node) {
                return i - iStart;
            }
        }
        return rowNums - iStart;
    };

    auto CalcBrick = [&CalcStarWidth, &CalcStarHeight](const std::vector<std::vector<char>> &grid, char node,
                                                       int iStart, int jStart, int rowNums, int colNums) {
        Brick brick;
        brick.c = node;
        brick.start = {jStart, iStart};

        int width = CalcStarWidth(grid, node, iStart, jStart, rowNums, colNums);
        int height = CalcStarHeight(grid, node, iStart, jStart, rowNums, colNums);
        int i = iStart, j = jStart;
        if (width > height) {
            for (i = iStart + 1; i < iStart + height; ++i) {
                bool fullOfColumn = true;
                for (j = jStart; j < jStart + width; ++j) {
                    if (grid[i][j] != node) {
                        fullOfColumn = false;
                        break;
                    }
                }
                if (!fullOfColumn) {
                    break;
                }
            }
            brick.end = {jStart + width - 1, i - 1};
        } else {
            for (j = jStart + 1; j < jStart + width; ++j) {
                bool fullOfRaw = true;
                for (i = iStart; i < iStart + height; ++i) {
                    if (grid[i][j] != node) {
                        fullOfRaw = false;
                        break;
                    }
                }
                if (!fullOfRaw) {
                    break;
                }
            }
            brick.end = {j - 1, iStart + height - 1};
        }

        return brick;
    };

    auto gridTemp = grid;
    std::vector<Brick> bricks;
    for (int i = 0; i < rowNums; ++i) {
        for (int j = 0; j < colNums; ++j) {
            char c = gridTemp[i][j];
            if (c == emptyNode) {
                continue;
            }

            Brick brick = CalcBrick(gridTemp, c, i, j, rowNums, colNums);
            bricks.push_back(brick);
            for (int x = brick.start.x; x <= brick.end.x; ++x) {
                for (int y = brick.start.y; y <= brick.end.y; ++y) {
                    gridTemp[y][x] = emptyNode;
                }
            }
        }
    }
    return bricks;
}
