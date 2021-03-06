#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "game_level.h"

enum GameState
{
    GAME_ACTIVE,
    GAME_WIN,
    GAME_OVER
};
// Initial size of the player character
const glm::vec2 PLAYER_SIZE(20.0f, 20.0f);
// Initial velocity of the player character
const float PLAYER_VELOCITY(200.0f);

class Game
{
public:
    GameState State;
    bool light;
    glm::vec2 playerPos;
    unsigned int score;
    bool Keys[1024];
    unsigned int Width, Height;
    std::vector<GameLevel> Levels;
    unsigned int Level;
    Game(unsigned int width, unsigned int height);
    ~Game();

    void Init();
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();

    void DoCollisions();
    void MoveEnemy(GameObject *enemy, int i, float dt);
    bool checkWallCollisions(GameObject *obj);
    void ResetGame();
};

#endif