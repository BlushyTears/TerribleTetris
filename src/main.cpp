#include "raylib.h"
#include "resource_dir.h"	

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <memory>
#include <random>
#include <string> 
#include <chrono>

const int screenWidth = 400;
const int screenHeight = 1200;
const int squareSize = (screenWidth + screenHeight) / 20;

// How long in milliseconds to update the game (not const since game usually speeds up over time)
int tickrate = 200;

using namespace std;
using namespace std::chrono;

const Color gridColor = {255, 255, 255, 150};

struct Offset {
    float x;
    float y;
};

struct Shape {
    float origin_x;
    float origin_y;
    int blockSize = 50;
    Color color;
    std::vector<Offset> offsets;

    Shape(float x, float y, Color c)
        : origin_x(x), origin_y(y), color(c) {
    }

    Shape(const Shape& other)
        : origin_x(other.origin_x), origin_y(other.origin_y),
        blockSize(other.blockSize), color(other.color),
        offsets(other.offsets) {
    }

    void DesignShape(const Offset& o) {
        offsets.push_back(o);
    }
};


chrono::high_resolution_clock* globalStart;
std::unique_ptr<Shape> currentShape;

std::vector<std::unique_ptr<Shape>> dormantShapes;

int index = 0;
void generateNewShape(std::vector<Shape*>& DB_Shapes);

/*
    Requirements:
    - Find a way to draw shapes using blocks  dynamically 
    - Make them have gravity by tweaking the existing gravity tick function
    - Create a rotation function for left and right
    - Make sure they can't go outside of screen
    - Create a selector which randomly selects a new shape when pressing c Or when it has fallen to the ground
    - Create a grid which hosts all the dormant shapes
    - Create a collision checker for stacking shapes
*/

std::vector<Offset> FirstShape = { Offset{0 , 0}, Offset{1 , 0}, Offset{1 , -1}, Offset{1 , -2}, Offset{2 , -2} };
std::vector<Offset> SecondShape = { Offset{0 , 0}, Offset{1 , 0}, Offset{1 , -1}, Offset{0 , -1} };
std::vector<Offset> ThirdShape = { Offset{0 , 0}, Offset{0 , -1}, Offset{0 , -2}, Offset{0 , -3}, Offset{0 , -4} };
std::vector<Offset> FourthShape = { Offset{0 , 0}, Offset{1 , 0}, Offset{1 , -1}, Offset{1 , -2}, Offset{0 , -2} };

void ProcessTick() {
    currentShape->origin_y += currentShape->blockSize;

    if (!(currentShape->origin_x > screenWidth - currentShape->blockSize * 2)) {
        if (IsKeyDown(KEY_D)) currentShape->origin_x += currentShape->blockSize;
    }
    if (!(currentShape->origin_x < 0 + currentShape->blockSize)) {
        if (IsKeyDown(KEY_A)) currentShape->origin_x -= currentShape->blockSize;
    }
}

// Note: Draw dormant shapes and checking for collision should happen in the same for loop probably
// Furthermore, loop should go backwards, since later shapes are more likely to collide with currentShape
void checkCollision(std::vector<Shape*>& DB_Shapes) {
    for (int i = 0; i < currentShape->offsets.size(); i++) {
        if (currentShape->origin_y + currentShape->offsets[i].y * currentShape->blockSize + currentShape->blockSize > 1150) {
            generateNewShape(DB_Shapes);
            return;
        }
    }

    for (int i = 0; i < dormantShapes.size(); i++) {
        Shape tempShape = *dormantShapes[i];
        for (int j = 0; j < tempShape.offsets.size(); j++) {
            for (int k = 0; k < currentShape->offsets.size(); k++) {
                if (currentShape->origin_x + currentShape->offsets[k].x * currentShape->blockSize == tempShape.origin_x + tempShape.offsets[j].x * currentShape->blockSize &&
                    currentShape->origin_y + currentShape->offsets[k].y * currentShape->blockSize + currentShape->blockSize == tempShape.origin_y + tempShape.offsets[j].y * currentShape->blockSize) {
                    generateNewShape(DB_Shapes);
                    return;
                }
            }
        }
    }
}

void StepProcess() {
    static auto lastStepTime = std::chrono::steady_clock::now();

    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastStepTime);

    // Note: We do A and D movement twice (duplicate code) since we want to be able to hold 
    // Down the keys and press it to give it tetris-style movement
    if (elapsed.count() >= tickrate) {
        ProcessTick();
        lastStepTime = currentTime;
    }
}

int getRandomNumberInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

unsigned char getRandomNumberUCHAR(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

void CreateShape(std::vector<Offset> tempShape, std::vector<Shape*>* DB_Shapes) {
    Color color = CLITERAL(Color) { getRandomNumberUCHAR(120, 255), getRandomNumberUCHAR(120, 255), getRandomNumberUCHAR(120, 255), getRandomNumberUCHAR(255, 255) };
    Shape* shape = new Shape(screenWidth / 2.0f, screenHeight / 12.0f, color);

    for (int i = 0; i < tempShape.size(); i++) {
        shape->DesignShape(tempShape[i]);
    }

    std::unique_ptr<Shape> currentShape;
    DB_Shapes->push_back(shape);
    index++;
}

// first draw colored boxes then black outlines
void DrawShape(std::vector<Shape*>& DB_Shapes) {
    for (int j = 0; j < currentShape->offsets.size(); j++) {
        DrawRectangle(currentShape->origin_x + (currentShape->blockSize * currentShape->offsets[j].x),
                currentShape->origin_y + (currentShape->blockSize * currentShape->offsets[j].y),
                currentShape->blockSize, currentShape->blockSize, currentShape->color);
        DrawRectangleLines(currentShape->origin_x + (currentShape->blockSize * currentShape->offsets[j].x),
            currentShape->origin_y + (currentShape->blockSize * currentShape->offsets[j].y),
            currentShape->blockSize, currentShape->blockSize, BLACK);
    }

    for (int i = 0; i < dormantShapes.size(); i++) {
        Shape tempShape = *dormantShapes[i];
        for (int j = 0; j < tempShape.offsets.size(); j++) {
            DrawRectangle(tempShape.origin_x + (tempShape.blockSize * tempShape.offsets[j].x),
                tempShape.origin_y + (tempShape.blockSize * tempShape.offsets[j].y),
                tempShape.blockSize, tempShape.blockSize, tempShape.color);
            DrawRectangleLines(tempShape.origin_x + (tempShape.blockSize * tempShape.offsets[j].x),
                tempShape.origin_y + (tempShape.blockSize * tempShape.offsets[j].y),
                tempShape.blockSize, tempShape.blockSize, BLACK);
        }
    }
}

void generateNewShape(std::vector<Shape*>& DB_Shapes) {
    dormantShapes.push_back(std::make_unique<Shape>(*currentShape));

    int temp = index;
    while (temp == index) {
        temp = getRandomNumberInt(0, DB_Shapes.size() - 1);
    }
    index = temp;
    currentShape = std::make_unique<Shape>(*DB_Shapes[index]);
    currentShape->origin_y = screenHeight / 12;
}

void swapShape(std::vector<Shape*>& DB_Shapes) {
    if (IsKeyPressed(KEY_Z)) {
        if (index <= 0) {
            index = DB_Shapes.size() - 1;
        }
        else {
            index--;
        }
        currentShape = std::make_unique<Shape>(*DB_Shapes[index]);
        currentShape->origin_y = screenHeight / 12;
    }
    if (IsKeyPressed(KEY_X)) {
        if (index >= DB_Shapes.size() - 1) {
            index = 0;
        }
        else {
            index++;
        }
        currentShape = std::make_unique<Shape>(*DB_Shapes[index]);
        currentShape->origin_y = screenHeight / 12;
    }
    if (IsKeyPressed(KEY_C)) {
        generateNewShape(DB_Shapes);
    }
}

void drawGrid() {
    for (int i = 1; i < screenHeight / currentShape->blockSize; i++) {
        float x = static_cast<float>(i * currentShape->blockSize);
        DrawLineV(Vector2{ 0, x }, Vector2{ screenWidth, x }, { 255,255,255, 120 });
    }
    for (int j = 1; j < screenWidth / currentShape->blockSize; j++) {
        float y = static_cast<float>(j * currentShape->blockSize);
        DrawLineV(Vector2{ y, 0 }, Vector2{ y, screenHeight }, { 255,255,255, 120 });
    }
}

void moveShape() {
    // Rotate left
    if (IsKeyPressed(KEY_Q)) {
        for (int j = 0; j < currentShape->offsets.size(); j++) {
            float temp = currentShape->offsets[j].x;
            currentShape->offsets[j].x = currentShape->offsets[j].y;
            currentShape->offsets[j].y = -temp;
        }
    }
    // Rotate right
    if (IsKeyPressed(KEY_E)) {
        for (int j = 0; j < currentShape->offsets.size(); j++) {
            float temp = currentShape->offsets[j].x;
            currentShape->offsets[j].x = -currentShape->offsets[j].y;
            currentShape->offsets[j].y = temp;
        }
    }

    if (!(currentShape->origin_x > screenWidth - currentShape->blockSize * 2)) {
        if (IsKeyPressed(KEY_D)) currentShape->origin_x += currentShape->blockSize;
    }
    if (!(currentShape->origin_x < 0 + currentShape->blockSize)) {
        if (IsKeyPressed(KEY_A)) currentShape->origin_x -= currentShape->blockSize;
    }
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Terrible Tetris");
    SetTargetFPS(60);
    std::vector<Shape*> DB_Shapes;
    CreateShape(FirstShape, &DB_Shapes);
    CreateShape(SecondShape, &DB_Shapes);
    CreateShape(ThirdShape, &DB_Shapes);
    CreateShape(FourthShape, &DB_Shapes);

    currentShape = std::make_unique<Shape>(*DB_Shapes[0]);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, RAYWHITE);
        DrawShape(DB_Shapes);
        swapShape(DB_Shapes);
        drawGrid();
        moveShape();
        checkCollision(DB_Shapes);
        StepProcess();
        EndDrawing();
    }

    for (auto shape : DB_Shapes) {
        delete shape;
    }

    CloseWindow();
    return 0;
}
