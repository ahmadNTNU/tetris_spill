#include "TetrisWindow.h"
#include <map>
#include <random>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <unistd.h>
using namespace std;
TetrisWindow::TetrisWindow() : 
    AnimationWindow{30, 30, blockSize * tetrisGridWidth + sideBarLength, blockSize * tetrisGridHeight, "Tetris"}, 
    currentTetromino{}, 
    gridMatrix(tetrisGridHeight, std::vector<TetrominoType>(tetrisGridWidth, TetrominoType::NONE)),

    scoreBox{Point{blockSize * tetrisGridWidth, 0*sideBarLength}, sideBarLength, sideBarHeight, "Score: 0"},
    highScoreBox{Point{blockSize * tetrisGridWidth, 1*sideBarHeight}, sideBarLength, sideBarHeight, "High Score"},
    quitBtn{Point{blockSize * tetrisGridWidth ,  2*sideBarHeight}, sideBarLength, sideBarHeight,"Quit"},
    restartBtn{Point{blockSize * tetrisGridWidth ,  3*sideBarHeight}, sideBarLength, sideBarHeight,"Restart"},
    pauseBtn{Point{blockSize * tetrisGridWidth ,  4*sideBarHeight}, sideBarLength, sideBarHeight,"Pause"}


{
    generateRandomTetromino();

    //GUI elementer
    
    add(scoreBox);

    add(highScoreBox);
    ifstream ifs{"highscore.txt"};
    if (!ifs) throw runtime_error("Couldn't open highscore file highscore.txt");
    int highScore = 0;
    while(ifs >> highScore); //Gjort i tilfelle det ikke finnes en High Score fra før
    string highScoreText{"High Score: "};
    highScoreText += std::to_string(highScore);
    highScoreBox.setText(highScoreText.c_str());

    add(quitBtn);
    add(restartBtn);
    add(pauseBtn);

    quitBtn.setCallback(std::bind(&TetrisWindow::cb_quit, this));
    restartBtn.setCallback(std::bind(&TetrisWindow::cb_restart, this));
    pauseBtn.setCallback(std::bind(&TetrisWindow::cb_pause, this));
}

void TetrisWindow::handleInput() {
    if(!playing || paused) return;

    Point tetrominoPosition = currentTetromino.getPosition();

    static bool lastZKeyState = false;
    static bool lastXKeyState = false;
    static bool lastUpKeyState = false;
    static bool lastDownKeyState = false;
    static bool lastLeftKeyState = false;
    static bool lastRightKeyState = false;
    
    bool currentZKeyState = is_key_down(KeyboardKey::Z);
    bool currentXKeyState = is_key_down(KeyboardKey::X);
    bool currentUpKeyState = is_key_down(KeyboardKey::UP);
    bool currentDownKeyState = is_key_down(KeyboardKey::DOWN);
    bool currentLeftKeyState = is_key_down(KeyboardKey::LEFT);
    bool currentRightKeyState = is_key_down(KeyboardKey::RIGHT);
    
    if(currentZKeyState && !lastZKeyState) {
        //Implementert en enkel form for wall bouncing
        currentTetromino.rotateCounterClockwise();
        if(hasCrashed()) {
            //Skjekker om den kan gå en til venstre
            if(!tetrominoCrashesAt({tetrominoPosition.x - 1, tetrominoPosition.y})) {
                currentTetromino.moveLeft();
            } 
            //Skjekker om den kan gå en til høyre
            else if(!tetrominoCrashesAt({tetrominoPosition.x + 1, tetrominoPosition.y})){
                currentTetromino.moveRight();
            } 
            //Hvis ikke, opphev bevegelse
            else {
                currentTetromino.rotateClockwise(); 
            }

            //Hindre bevegelse uten wall bouncing:
            //currentTetromino.rotateCounterClockwise();
            //if(hasCrashed()) currentTetromino.rotateClockwise();
        }
    }

    if(currentUpKeyState && !lastUpKeyState) {
        currentTetromino.rotateClockwise();
        if(hasCrashed()) {
            if(!tetrominoCrashesAt({tetrominoPosition.x - 1, tetrominoPosition.y})) {
                currentTetromino.moveLeft();
            }
            else if(!tetrominoCrashesAt({tetrominoPosition.x + 1, tetrominoPosition.y})){
                currentTetromino.moveRight();
            }
            else {
                currentTetromino.rotateCounterClockwise();
            }
        }
    }

    if(currentDownKeyState && !lastDownKeyState) {
        if(!tetrominoCrashesAt({tetrominoPosition.x, tetrominoPosition.y + 1})) {
            currentTetromino.moveDown();
        }
    }

    if(currentLeftKeyState && !lastLeftKeyState) {
        if(!tetrominoCrashesAt({tetrominoPosition.x - 1, tetrominoPosition.y})) {
            currentTetromino.moveLeft();
        }
    }

    if(currentRightKeyState && !lastRightKeyState) {
        if(!tetrominoCrashesAt({tetrominoPosition.x + 1, tetrominoPosition.y})) {
            currentTetromino.moveRight();
        }
    }

    if(currentXKeyState) {
        score++;
        moveTetrominoDown();
        usleep(int(1000000.0 * 0.05));
        string scoreText{"Score: "};
        scoreText += std::to_string(score);
        scoreBox.setText(scoreText.c_str());
    }

    lastZKeyState = currentZKeyState;
    lastXKeyState = currentXKeyState;
    lastDownKeyState = currentDownKeyState;
    lastUpKeyState = currentUpKeyState;
    lastLeftKeyState = currentLeftKeyState;
    lastRightKeyState = currentRightKeyState;
}

void TetrisWindow::run() {
    unsigned int framesSinceLastTetronimoMove = 0;
    const unsigned int framesPerTetronimoMove = 20;

    while(!should_close()) {
        if(!paused) {
            framesSinceLastTetronimoMove++;
            if(framesSinceLastTetronimoMove >= framesPerTetronimoMove) {
                framesSinceLastTetronimoMove = 0;
                moveTetrominoDown();
            }
            handleInput();
        }
        
        draw_rectangle({0, 0}, blockSize * tetrisGridWidth, blockSize * tetrisGridHeight, TDT4102::Color::gray);
        drawGridMatrix();
        drawCurrentTetromino();

        if(paused) {
            draw_rectangle({23, 33}, 170, 30);
            draw_text({40, 33}, "Game Paused");
        }

        if(!playing) {
            draw_rectangle({23, 33}, 200, 30, Color::yellow);
            draw_text({40, 33}, "Game has ended");
        }

        next_frame();
    }
}

void TetrisWindow::drawGridMatrix() {
    for(int row = 0; row < tetrisGridHeight; row++) {
        for(int column = 0; column < tetrisGridWidth; column++) {
            TetrominoType field = gridMatrix.at(row).at(column);
            if(field != TetrominoType::NONE) {
                draw_rectangle(Point{column * blockSize, row * blockSize}, blockSize, blockSize, tetrominoColorMap.at(field), Color::black);
            }
        }
    }
}

void TetrisWindow::drawCurrentTetromino() {
    Point topLeftCorner = currentTetromino.getPosition();
    int size = currentTetromino.getMatrixSize();

    for(int row = 0; row < size; row++) {
        for(int column = 0; column < size; column++) {
            TetrominoType field = currentTetromino.getBlock(row, column);
            if(field != TetrominoType::NONE) {
                draw_rectangle(Point{(topLeftCorner.x + column) * blockSize, (topLeftCorner.y + row) * blockSize}, blockSize, blockSize, tetrominoColorMap.at(field), Color::black);
            }
        }
    }
}

void TetrisWindow::generateRandomTetromino() {
    random_device rd;	
    default_random_engine generator(rd());
    int randomNumber = generator() % 7; //Antallet tetrominoer
    currentTetromino = Tetromino{startPoint, static_cast<TetrominoType>(randomNumber)};
}

void TetrisWindow::moveTetrominoDown() {
    Point tetrominoPosition = currentTetromino.getPosition();
    if(tetrominoCrashesAt({tetrominoPosition.x, tetrominoPosition.y + 1})) {
        fastenTetromino();
        removeFullRows();
        generateRandomTetromino();
        if(hasCrashed()) {
            lostGame();
        }
    } else {
        currentTetromino.moveDown();
    }
}

void TetrisWindow::fastenTetromino() {
    Point tetrominoPosition = currentTetromino.getPosition();
    for(int row = 0; row < currentTetromino.getMatrixSize(); ++row) {
        for(int column = 0; column < currentTetromino.getMatrixSize(); ++column) {
            if(currentTetromino.blockExist(row, column)) {
                gridMatrix.at(tetrominoPosition.y + row).at(tetrominoPosition.x + column) = currentTetromino.getBlock(row, column);
            }
        }
    }
    currentTetromino = Tetromino{}; //Ikke nødvendig, men pent slik at blokkene ikke finnes to plasser samtidig
}

bool TetrisWindow::tetrominoCrashesAt(Point point) {
    int tetrominoMatrixSize = currentTetromino.getMatrixSize();
    for(int row = 0; row < tetrominoMatrixSize; ++row) {
        for(int column = 0; column < tetrominoMatrixSize; ++column) {
            if(currentTetromino.blockExist(row,column)) {
                if(point.x + column >= tetrisGridWidth 
                    || point.x + column < 0
                    || point.y + row >= tetrisGridHeight
                    || gridMatrix[point.y + row][point.x + column] != TetrominoType::NONE) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool TetrisWindow::hasCrashed() {
    Point tetrominoPosition = currentTetromino.getPosition();
    return tetrominoCrashesAt(tetrominoPosition);
}


void TetrisWindow::removeFullRows() {
    int linesCleared = 0;
    for(int row = 0; row < tetrisGridHeight; ++row) {
        int numberOfBlocks = 0;
        for(int column = 0; column < tetrisGridWidth; ++column) {
            if(gridMatrix[row][column] != TetrominoType::NONE) {
                numberOfBlocks++; //Teller opp om vi har fulle rader
            }
        }
        if(numberOfBlocks == tetrisGridWidth) {
            linesCleared++;

            for(int row_over = row; row_over > 0; --row_over) {
                for(int column = 0; column < tetrisGridWidth; ++column) {
                    //Oppdaterer posisjonen til blokkene over fjernet rad
                    gridMatrix[row_over][column] = gridMatrix[row_over-1][column]; 
                }
            }
        }
    }

    switch(linesCleared){
        case 1:
           score += 40;
           break; 
        case 2:
           score += 100;
           break;
        case 3  :
           score += 300;
           break;
        case 4  :
           score += 1200;
           break;
        default: 
            break;
    }
    string scoreText{"Score: "};
    scoreText += std::to_string(score);
    scoreBox.setText(scoreText.c_str());
}

void TetrisWindow::lostGame(){
    playing = false;

    currentTetromino = Tetromino{};

    //Oppdaterer highscore
    ifstream ifs{"highscore.txt"};
	if (!ifs) throw runtime_error("Couldn't open highscore file highscore.txt");

    int highScore = 0;
    while(ifs >> highScore);
    highScore = std::max<int>(highScore, score);
    string scoreText{"Score: "};
    scoreText += std::to_string(highScore);
    highScoreBox.setText(scoreText.c_str());

    ofstream ofs{"highscore.txt"};
    if (!ofs) throw runtime_error("Couldn't open highscore file highscore.txt");
    
    ofs << highScore;
}

void TetrisWindow::restartGame(){
    if (playing) lostGame();

    playing = true;
    paused = false;
    score = 0;
    string scoreText{"Score: "};
    scoreText += std::to_string(score);
    scoreBox.setText(scoreText.c_str());

    // Reset grid matrix
    for(int row = 0; row < tetrisGridHeight; row++) {
        for(int column = 0; column < tetrisGridWidth; column++) {
            gridMatrix.at(row).at(column) = TetrominoType::NONE;
        }
    }

    generateRandomTetromino();
}

void TetrisWindow::pauseGame() {
    paused = !paused;
}
