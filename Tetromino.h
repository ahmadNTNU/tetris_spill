#pragma once
#include "Point.h"
#include <vector>
enum class TetrominoType{J, L, T, S, Z, O, I, NONE}; 
using namespace TDT4102;
using namespace std;
class Tetromino {
public:
    static constexpr int blockSize {30};

    Tetromino(Point startingPoint, TetrominoType tetType);
    Tetromino() : matrixSize{0}, topLeftCorner{0,0} {} 
    
    void rotateClockwise();
    void rotateCounterClockwise();
    void moveDown();
    void moveLeft();
    void moveRight();

    bool blockExist(int row, int column) const {
        return blockMatrix.at(row).at(column) != TetrominoType::NONE;
    }

    TetrominoType getBlock(int row, int column) const {
        return blockMatrix.at(row).at(column);
    }

    int getMatrixSize() const {
        return matrixSize;
    }

    Point getPosition() const {
        return topLeftCorner;
    }
    
    ~Tetromino() {}

private: 

    int matrixSize;
    Point topLeftCorner;
    vector<vector<TetrominoType>> blockMatrix;
};

