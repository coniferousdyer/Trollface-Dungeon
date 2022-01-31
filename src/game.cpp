#include <bits/stdc++.h>
#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "game_level.h"

SpriteRenderer *Renderer;
GameObject *Player;

bool CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
    // collision x-axis?
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
                      two.Position.x + two.Size.x >= one.Position.x;
    // collision y-axis?
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
                      two.Position.y + two.Size.y >= one.Position.y;
    // collision only if on both axes
    return collisionX && collisionY;
}

void GenerateMatrix(int matrix[18][15])
{
    int WALL_LIMIT = 10;
    int COIN_LIMIT = 10;
    int RANDOM_FACTOR = 20;

    srand(time(NULL));

    for (int i = 0; i < 18; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            if (i == 0 || i == 17 || j == 0 || j == 14)
            {
                if (i == 17 && (j == 6 || j == 7 || j == 8))
                    matrix[i][j] = 0;
                else
                    matrix[i][j] = 1;
            }
            else
            {
                if (WALL_LIMIT > 0)
                {
                    matrix[i][j] = rand() % RANDOM_FACTOR;
                    if (matrix[i][j] == 2)
                        WALL_LIMIT--;
                    else if (matrix[i][j] == 3 && COIN_LIMIT > 0)
                        COIN_LIMIT--;
                    else
                        matrix[i][j] = 0;
                }
                else
                    matrix[i][j] = 0;
            }
        }
    }
}

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{
}

Game::~Game()
{
    delete Renderer;
    delete Player;
}

void Game::Init()
{
    this->score = 0;

    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
                                      static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    // set render-specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // load textures
    ResourceManager::LoadTexture("textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/amogus.png", true, "amogus");
    ResourceManager::LoadTexture("textures/coin.png", false, "coin");

    // create game matrix
    int matrix[18][15];
    GenerateMatrix(matrix);

    // write matrix to level
    std::ofstream outfile("levels/one.lvl");
    for (int i = 0; i < 18; i++)
    {
        for (int j = 0; j < 15; j++)
            outfile << matrix[i][j] << " ";

        outfile << std::endl;
    }

    // load levels
    GameLevel one;
    one.Load("levels/one.lvl", this->Width, this->Height);

    ////
    this->Levels.push_back(one);

    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("amogus"));
}

void Game::Update(float dt)
{
    // check for collisions
    this->DoCollisions();
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        // move playerboard
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0.0f && this->checkWallCollisions())
                Player->Position.x -= velocity;

            if (!this->checkWallCollisions())
                Player->Position.x += velocity;
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x && this->checkWallCollisions())
                Player->Position.x += velocity;

            if (!this->checkWallCollisions())
                Player->Position.x -= velocity;
        }
        if (this->Keys[GLFW_KEY_W])
        {
            if (Player->Position.y >= 0.0f && this->checkWallCollisions())
                Player->Position.y -= velocity;

            if (!this->checkWallCollisions())
                Player->Position.y += velocity;
        }
        if (this->Keys[GLFW_KEY_S])
        {
            if (Player->Position.y <= this->Height - Player->Size.y && this->checkWallCollisions())
                Player->Position.y += velocity;

            if (!this->checkWallCollisions())
                Player->Position.y -= velocity;
        }
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE)
    {
        // draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        // draw level
        this->Levels[this->Level].Draw(*Renderer);
        // draw player
        Player->Draw(*Renderer);
    }
}

void Game::DoCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            if (CheckCollision(*Player, box))
            {
                if (!box.IsSolid)
                {
                    if (box.IsCoin)
                        this->score++;

                    box.Destroyed = true;
                }
            }
        }
    }
}

bool Game::checkWallCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            if (CheckCollision(*Player, box))
            {
                // Only walls are solid
                if (box.IsSolid)
                {
                    return false;
                }
            }
        }
    }

    return true;
}