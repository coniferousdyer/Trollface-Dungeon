#include <bits/stdc++.h>
#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "game_level.h"
#include "text_renderer.h"

// Global game constants
const int MAX_ENEMIES = 11;
int WALL_LIMIT = 10;
int COIN_LIMIT = 10;
int ENEMY_LIMIT = 1;
int RANDOM_FACTOR = 100;
int MOVE_FACTOR = 20;
time_t START_TIME;

// Global game variables
int NUM_COINS = 0;
int NUM_WALLS = 0;
int ENEMY_DIRECTION[MAX_ENEMIES] = {0};

// Game matrix dimensions
const int LEVEL_WIDTH = 30;
const int LEVEL_HEIGHT = 30;

// Game objects
SpriteRenderer *Renderer;
TextRenderer *Text;
GameObject *Player, *Enemies[MAX_ENEMIES];

// Game object attributes
float initialEnemySpeed = 50.0f;

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

void GenerateMatrix(int matrix[LEVEL_HEIGHT][LEVEL_WIDTH])
{
    int wallLimit = WALL_LIMIT;
    int coinLimit = COIN_LIMIT;
    int randomFactor = RANDOM_FACTOR;

    srand(time(NULL));

    // 0 - Empty, 1 - Outer Wall, 2 - Inner Wall, 3 - Coin
    for (int i = 0; i < LEVEL_HEIGHT; i++)
    {
        for (int j = 0; j < LEVEL_WIDTH; j++)
        {
            if (i == 0 || i == LEVEL_HEIGHT - 1 || j == 0 || j == LEVEL_WIDTH - 1)
            {
                if ((i == 0 || i == LEVEL_HEIGHT - 1) && (j == LEVEL_WIDTH / 2 - 1 || j == LEVEL_WIDTH / 2 || j == LEVEL_WIDTH / 2 + 1))
                    matrix[i][j] = 0;
                else
                    matrix[i][j] = 1;
            }
            else
            {
                matrix[i][j] = rand() % randomFactor;
                if (matrix[i][j] == 2 && wallLimit > 0)
                {
                    wallLimit--;
                    NUM_WALLS++;
                }
                else if (matrix[i][j] == 3 && coinLimit > 0)
                {
                    coinLimit--;
                    NUM_COINS++;
                }
                else
                    matrix[i][j] = 0;
            }
        }
    }
}

// TODO: Try finetuning this
void FindEnemyPosition(int matrix[LEVEL_HEIGHT][LEVEL_WIDTH], int &x, int &y)
{
    int randomX = rand() % LEVEL_WIDTH;
    int randomY = rand() % LEVEL_HEIGHT;

    while (matrix[randomY][randomX] != 0 || randomY == Player->Position.y || randomX == Player->Position.x)
    {
        randomX = rand() % LEVEL_WIDTH;
        randomY = rand() % LEVEL_HEIGHT;
    }

    x = randomX;
    y = randomY;
}

void Game::ResetGame()
{
    if (this->Level != 2)
    {
        // Reset game variables
        this->score = 0;
        NUM_COINS = 0;
        NUM_WALLS = 0;
        MOVE_FACTOR = 20;
        for (int i = 0; i < MAX_ENEMIES; i++)
            ENEMY_DIRECTION[i] = 0;

        for (int i = 0; i < ENEMY_LIMIT - 5; i++)
            delete Enemies[i];

        // Increase difficulty
        ENEMY_LIMIT += 5;
        WALL_LIMIT += 5;
        COIN_LIMIT += 5;

        // Reset game matrix
        int matrix[LEVEL_HEIGHT][LEVEL_WIDTH];
        GenerateMatrix(matrix);

        std::string file = "";
        if (this->Level == 0)
            file = "levels/two.lvl";
        else if (this->Level == 1)
            file = "levels/three.lvl";

        // write matrix to level
        std::ofstream outfile(file);
        for (int i = 0; i < LEVEL_HEIGHT; i++)
        {
            for (int j = 0; j < LEVEL_WIDTH; j++)
                outfile << matrix[i][j] << " ";

            outfile << std::endl;
        }

        GameLevel lvl;
        lvl.Load(file.c_str(), this->Width, this->Height);
        this->Levels.push_back(lvl);

        // Reset player position
        Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, 0.0f);

        // Generating new enemies
        glm::vec2 enemySize = glm::vec2(this->Levels[this->Level + 1].TileWidth, this->Levels[this->Level + 1].TileHeight);
        for (int i = 0; i < ENEMY_LIMIT; i++)
        {
            // Find random enemy spawn position
            int x, y;
            FindEnemyPosition(matrix, x, y);
            glm::vec2 enemyPos = glm::vec2(x * enemySize.x, y * enemySize.y);

            // Create enemies
            Enemies[i] = new GameObject(enemyPos, enemySize, ResourceManager::GetTexture("enemy"));
            Enemies[i]->IsEnemy = true;
        }
    }
    else
        this->State = GAME_WIN;
}

void Game::MoveEnemy(GameObject *enemy, int index, float dt)
{
    if (this->State == GAME_ACTIVE)
    {
        float velocity = initialEnemySpeed * dt;

        if (MOVE_FACTOR == 0)
        {
            // Choosing random direction for enemy movement
            ENEMY_DIRECTION[index] = rand() % 4;
            MOVE_FACTOR = 20;
        }
        else
            MOVE_FACTOR--;

        // move enemy
        if (ENEMY_DIRECTION[index] == 0)
        {
            if (enemy->Position.x >= 0.0f && this->checkWallCollisions(enemy))
                enemy->Position.x -= velocity;

            if (!this->checkWallCollisions(enemy))
                enemy->Position.x += velocity;
        }
        if (ENEMY_DIRECTION[index] == 1)
        {
            if (enemy->Position.x <= this->Width - enemy->Size.x && this->checkWallCollisions(enemy))
                enemy->Position.x += velocity;

            if (!this->checkWallCollisions(enemy))
                enemy->Position.x -= velocity;
        }
        if (ENEMY_DIRECTION[index] == 2)
        {
            if (enemy->Position.y >= 0.0f && this->checkWallCollisions(enemy))
                enemy->Position.y -= velocity;

            if (!this->checkWallCollisions(enemy))
                enemy->Position.y += velocity;
        }
        if (ENEMY_DIRECTION[index] == 3)
        {
            if (enemy->Position.y <= this->Height - enemy->Size.y && this->checkWallCollisions(enemy))
                enemy->Position.y += velocity;

            if (!this->checkWallCollisions(enemy))
                enemy->Position.y -= velocity;
        }
    }
}

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{
}

Game::~Game()
{
    delete Player;
    for (int i = 0; i < ENEMY_LIMIT; i++)
        delete Enemies[i];
}

void Game::Init()
{
    srand(time(NULL));

    this->score = 0;
    this->Level = 0;

    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
                                      static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);

    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("fonts/OCRAEXT.TTF", 24);

    // set render-specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // load textures
    ResourceManager::LoadTexture("textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/amogus.png", true, "amogus");
    ResourceManager::LoadTexture("textures/coin.png", false, "coin");
    ResourceManager::LoadTexture("textures/trollface.png", true, "enemy");
    ResourceManager::LoadTexture("textures/game_over.jpg", false, "game_over");
    ResourceManager::LoadTexture("textures/game_won.jpeg", false, "game_won");

    // create game matrix
    int matrix[LEVEL_HEIGHT][LEVEL_WIDTH];
    GenerateMatrix(matrix);

    // write matrix to level
    std::ofstream outfile("levels/one.lvl");
    for (int i = 0; i < LEVEL_HEIGHT; i++)
    {
        for (int j = 0; j < LEVEL_WIDTH; j++)
            outfile << matrix[i][j] << " ";

        outfile << std::endl;
    }

    // load levels
    GameLevel one;
    one.Load("levels/one.lvl", this->Width, this->Height);
    this->Levels.push_back(one);

    // Create player
    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, 0.0f);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("amogus"));
    Player->IsEnemy = false;

    glm::vec2 enemySize = glm::vec2(this->Levels[this->Level].TileWidth, this->Levels[this->Level].TileHeight);
    for (int i = 0; i < ENEMY_LIMIT; i++)
    {
        // Find random enemy spawn position
        int x, y;
        FindEnemyPosition(matrix, x, y);
        glm::vec2 enemyPos = glm::vec2(x * enemySize.x, y * enemySize.y);

        // Create enemy
        Enemies[i] = new GameObject(enemyPos, enemySize, ResourceManager::GetTexture("enemy"));
        Enemies[i]->IsEnemy = true;
    }

    START_TIME = time(NULL);
}

void Game::Update(float dt)
{
    // check for collisions
    this->DoCollisions();

    for (int i = 0; i < ENEMY_LIMIT; i++)
        this->MoveEnemy(Enemies[i], i, dt);
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        // move player
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0.0f && this->checkWallCollisions(Player))
                Player->Position.x -= velocity;

            if (!this->checkWallCollisions(Player))
                Player->Position.x += velocity;
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x && this->checkWallCollisions(Player))
                Player->Position.x += velocity;

            if (!this->checkWallCollisions(Player))
                Player->Position.x -= velocity;
        }
        if (this->Keys[GLFW_KEY_W])
        {
            if (Player->Position.y >= 0.0f && this->checkWallCollisions(Player))
                Player->Position.y -= velocity;

            if (!this->checkWallCollisions(Player))
                Player->Position.y += velocity;
        }
        if (this->Keys[GLFW_KEY_S])
        {
            if (this->checkWallCollisions(Player))
            {
                if (this->score >= NUM_COINS)
                    Player->Position.y += velocity;
                else if (Player->Position.y <= this->Height - Player->Size.y)
                    Player->Position.y += velocity;
            }

            if (!this->checkWallCollisions(Player))
                Player->Position.y -= velocity;
        }

        if (this->score >= NUM_COINS && Player->Position.y >= this->Height - Player->Size.y)
        {
            this->ResetGame();
            this->Level++;
        }
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE)
    {
        // draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height));
        // draw level
        this->Levels[this->Level].Draw(*Renderer);
        // draw player
        Player->Draw(*Renderer);
        // draw enemies
        for (int i = 0; i < ENEMY_LIMIT; i++)
            Enemies[i]->Draw(*Renderer);

        int coinsLeft = NUM_COINS - this->score;
        std::stringstream ss, ss2;
        ss << coinsLeft;
        Text->RenderText("Coins left:" + ss.str(), 5.0f, 5.0f, 1.0f);
        ss2 << time(NULL) - START_TIME;
        Text->RenderText("Seconds since start: " + ss2.str(), 5.0f, 25.0f, 1.0f);
    }
    else if (this->State == GAME_OVER)
        Renderer->DrawSprite(ResourceManager::GetTexture("game_over"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height));
    else if (this->State == GAME_WIN)
        Renderer->DrawSprite(ResourceManager::GetTexture("game_won"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height));
}

void Game::DoCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            // Player collision
            if (CheckCollision(*Player, box))
                if (!box.IsSolid)
                {
                    if (box.IsCoin)
                        this->score++;

                    box.Destroyed = true;
                }

            // Player-enemy collision
            for (int i = 0; i < ENEMY_LIMIT; i++)
                if (CheckCollision(*Player, *Enemies[i]))
                    this->State = GAME_OVER;
        }
    }
}

bool Game::checkWallCollisions(GameObject *obj)
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
        if (!box.Destroyed)
            if (CheckCollision(*obj, box))
                // Only walls are solid
                if (box.IsSolid)
                    return false;
                else if (obj->IsEnemy)
                    return false;

    return true;
}