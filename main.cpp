#include <iostream>
#include <pthread.h>
#include <vector>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <sys/wait.h>
#include <time.h>
#include <chrono>
#include <semaphore.h>


using namespace std;
int score = 0;
pthread_mutex_t mazeMutex;
pthread_mutex_t mutex1;
pthread_mutex_t PowerPalletMutex;
pthread_mutex_t ghostGlobalMutex;
bool pPalletGlobalBool;
bool uiThreadFinished = false;
pthread_t ui;
pthread_cond_t uiThreadFinishedCond;
pthread_mutex_t uiThreadFinishedMutex;
const int MAZE_WIDTH = 28;
const int MAZE_HEIGHT = 35;
const int TILE_SIZE = 20; // Size of each tile in pixels
char maze_Char[MAZE_HEIGHT][MAZE_WIDTH];
int maze[MAZE_HEIGHT][MAZE_WIDTH];
bool palletBool = false;
sf::Texture PacmanTextures[4]; // Array for different directions
sf::Texture dot;
sf::Texture ghost_Texture;
int direction = 0;
pthread_t powerPalletThread;
vector<sf::FloatRect> wallBounds;
pthread_mutex_t permitMutex;
sf::Texture speedPallet;
sf::Texture ChTexture;

////SEMALHORE/////////////////////
sem_t keySemaphore;
sem_t permitSemaphore;
int prevSpeeds[4] = { 0 };
///////////////
struct permits
{
    bool key = 1;
    bool permit = 1;
};
permits HousePermit;
void initializeSemaphores()
{
    sem_init(&keySemaphore, 0, 2);    // Initialize key semaphore with count 2
    sem_init(&permitSemaphore, 0, 2); // Initialize permit semaphore with count 2
}

void destroySemaphores()
{
    sem_destroy(&keySemaphore);
    sem_destroy(&permitSemaphore);
}
void getKey()
{
    sem_wait(&keySemaphore); // Decrement key semaphore count
}

void returnKey()
{
    sem_post(&keySemaphore); // Increment key semaphore count
}

void getPermit()
{
    sem_wait(&permitSemaphore); // Decrement permit semaphore count
}

void returnPermit()
{
    sem_post(&permitSemaphore); // Increment permit semaphore count
}
/////////////////////////////////////
struct Entity
{
    int xpos;
    int ypos;
    sf::Sprite sprite;
    int velocity;
    int direction = 0;
    bool reset=0;
    pthread_mutex_t ghostThread;

    void chdirection()
    {
        srand(time(0));
        int newDir = rand() % 4;
        this->direction = newDir;
    }
};
vector<Entity> ghosts(4);
struct Pacman
{
    int xpos;
    int ypos;
    // fint direction;
    sf::Sprite p;
    float velocity;
    int lives;
    sf::Clock moveclock;
    sf::Clock aniclock;
    int anistate;
    sf::Texture pac;
    Pacman() : xpos(160), ypos(140), velocity(100.0f), lives(3)
    {
        pac.loadFromFile("sprites/pacman/neutral.png");
        p.setTexture(pac);
        p.setScale(1, 1);
        anistate = 1;
    }

    void resetPacman()
    {
        xpos = 160; // Initial x position
        ypos = 140; // Initial y position
        lives--;    // Decrement lives
    }

    void move(float dt)
    {
        animation();

        if (moveclock.getElapsedTime().asSeconds() >= 0.1)

        {
            int newX = xpos;
            int newY = ypos;
            // if ((xpos < 560 && xpos > 0 && ypos < 700 && ypos > 0))
            //{

            if (direction == 0)
                newX += TILE_SIZE;
            else if (direction == 1)
                newY -= TILE_SIZE;
            else if (direction == 2)
                newY += TILE_SIZE;
            else if (direction == 3)
                newX -= TILE_SIZE;

            if (maze_Char[newY / TILE_SIZE][newX / TILE_SIZE] != 'X')
            {
                xpos = newX;
                ypos = newY;
            }
            else if (newX < 0)
            {
                xpos = 560;
            }
            else if (newX >= 560)
            {
                xpos = 0;
            }
            //}
            moveclock.restart();
        }
    }
    void animation()
    {
        if (direction == 4)
        {
            aniclock.restart();
            return;
        }

        if (aniclock.getElapsedTime().asSeconds() > 0.1)
        {
            aniclock.restart();

            string directionstr = "sprites/pacman/";
            switch (direction)
            {
            case 1:
                directionstr += "up";
                break;
            case 0:
                directionstr += "right";
                break;
            case 2:
                directionstr += "down";
                break;
            case 3:
                directionstr += "left";
                break;
            }
            switch (anistate)
            {
            case 1:
                anistate = 2;
                directionstr += "_2";
                break;
            case 2:
                anistate = 1;
                directionstr += "_1";
                break;
            }

            directionstr += ".png";

            pac.loadFromFile(directionstr);
            this->p.setTexture(pac);
            p.setScale(1, 1);
        }
    }
    void changeDirection(int newDirection)
    {
        direction = newDirection;
    }
};
Pacman pacman;void loadMaze()
{
    ifstream file("maze.txt");

    char cell;
    int rows = 0;
    int cols = 0;
    while (file.get(cell))
    {
        if (cell == ' ')
            continue;
        if (cell == '\n')
        {
            ++rows;
            cols = 0;
        }
        else
        {
            maze_Char[rows][cols] = cell;
            ++cols;
        }
    }
    ++rows;
    file.close();

    int speedPelletsSpawned = 0;
    srand(time(nullptr)); // Seed the random number generator

    while (speedPelletsSpawned < 2)
    {
        int randRow = (rand() % MAZE_HEIGHT)+2;
        int randCol = rand() % MAZE_WIDTH;

        if (maze_Char[randRow][randCol] == ' ' || maze_Char[randRow][randCol] == '.')
        {
            maze_Char[randRow][randCol] = ',';
            speedPelletsSpawned++;
        }
    }
}

//////////////////////////////////////////// MAZE DRAWING FUNCTION //////////////////
void drawMaze(sf::RenderWindow &window)
{
    sf::RectangleShape wall(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    sf::RectangleShape glow(sf::Vector2f(TILE_SIZE + 10, TILE_SIZE + 10)); // Larger rectangle for glow
    sf::Sprite pellets(dot);
    sf::Texture S;
    S.loadFromFile("pacman-art/other/strawberry.png");
    speedPallet.loadFromFile("pacman-art/other/apple.png");
    sf::Sprite strawberry(S);
    sf::Sprite speedApple(speedPallet);
    pellets.setScale(1.5, 1.5);
    for (int i = 0; i < MAZE_HEIGHT; ++i)
    {
        for (int j = 0; j < MAZE_WIDTH; ++j)
        {
            if (maze_Char[i][j] == 'X')
            { 
                glow.setPosition(j * TILE_SIZE - 5, i * TILE_SIZE - 5);
                glow.setFillColor(sf::Color(0, 0, 255, 50)); // Blue color with some transparency
                window.draw(glow);                           // Draw the glow rectangle first
                // Set position and color for original wall rectangle
                wall.setFillColor(sf::Color::Blue);
                wall.setPosition(j * TILE_SIZE, (i * TILE_SIZE));
                wallBounds.push_back((wall.getGlobalBounds()));
                window.draw(wall);
            }
            else if (maze_Char[i][j] == '+')
            { // Pellet
                strawberry.setPosition(j * TILE_SIZE, i * TILE_SIZE);
                window.draw(strawberry);
            }
            else if (maze_Char[i][j] == '.')
            { // Pellet
                pellets.setPosition(j * TILE_SIZE, i * TILE_SIZE);
                window.draw(pellets);
            }
            else if (maze_Char[i][j] == ',')
            { // Pellet
                speedApple.setPosition(j * TILE_SIZE, i * TILE_SIZE);
                window.draw(speedApple);
            }
        }
    }
}

void initializeGhosts()
{
    // Set velocities for each ghost
    ghosts[0].velocity = 150;
    ghosts[1].velocity = 150;
    ghosts[2].velocity = 150;
    ghosts[3].velocity = 150;

    for (int i = 0; i < ghosts.size(); ++i)
    {
        ghosts[i].xpos = 13 * TILE_SIZE;
        ghosts[i].ypos = 17 * TILE_SIZE;
        ghosts[i].sprite.setTexture(ghost_Texture);
        ghosts[i].sprite.setScale(1.2, 1.2);
    }

    for (int i = 0; i < ghosts.size(); ++i)
    {
        ghosts[i].sprite.setPosition(ghosts[i].xpos, ghosts[i].ypos);
    }
}

/////////////////////////////////////Ghost Movement Thread FunctiON /////////////////
void *ghostMovement(void *arg)
{
    bool prevwrap = 0;

    Entity *ghost = static_cast<Entity *>(arg);

    sf::Clock clock;
    sf::Time lastUpdateTime = clock.getElapsedTime();
    bool exithomeAllowed = true;
    bool exitHome = false;
    bool hasKey = false;
    bool hasPermit = false;

    ////////////////////SEMAPHORE IMPLEMENTATION FOR 3rd SCENARIO ////////////
    if (!exitHome)
    {
        getKey();
        getPermit();
        exithomeAllowed = true;
    }
    sf::Clock moveclock;
    while (true)
    {
        sf::Time now = clock.getElapsedTime();
        sf::Time deltaTime = now - lastUpdateTime;
        lastUpdateTime = now;
        float dt = deltaTime.asSeconds();
        if (exithomeAllowed)
        {
            if (!exitHome)
            {
                // Move the ghost upward until it exits the ghost house
                ghost->ypos -= TILE_SIZE;
                if (ghost->ypos <= 260) // Adjust this value as per your ghost house position
                {
                    exitHome = true;
                }
            }
            else
            {
                if(ghost->reset)
                {
                    exitHome=0;
                    ghost->reset=0;
                    getKey();
                    getPermit();
                    continue;
                }
                returnKey();
                returnPermit();
                
                bool left = false, right = false, up = false, down = false;
                int newY = ghost->ypos;
                int newX = ghost->xpos;
                pthread_mutex_lock(&ghostGlobalMutex);  ////// Synchronization scenario 1//////////              
                if (maze_Char[newY / TILE_SIZE][(newX - TILE_SIZE) / TILE_SIZE] != 'X')
                {
                    left = true;
                }
                if (maze_Char[newY / TILE_SIZE][(newX + TILE_SIZE) / TILE_SIZE] != 'X')
                {
                    right = true;
                }
                if (maze_Char[(newY + TILE_SIZE) / TILE_SIZE][newX / TILE_SIZE] != 'X' && maze_Char[(newY + TILE_SIZE) / TILE_SIZE][newX / TILE_SIZE] != '=')
                {
                    down = true;
                }
                if (maze_Char[(newY - TILE_SIZE) / TILE_SIZE][newX / TILE_SIZE] != 'X')
                {
                    up = true;
                }
                pthread_mutex_unlock(&ghostGlobalMutex);
                bool exit = true;
                while (exit)
                {
                    int dir = rand() % 40;
                    if (dir < 10)
                    {
                        if (ghost->direction != 3 && right)
                        {
                            ghost->xpos += TILE_SIZE;
                            ghost->direction = 0;
                            break;
                        }
                        else if (ghost->xpos + TILE_SIZE > 540 && direction != 3)
                        {
                            ghost->xpos = 0;
                            ghost->direction = 0;
                            break;
                        }
                    }
                    else if (dir > 9 && dir < 20)
                    {
                        if (ghost->direction != 0 && left)
                        {
                            ghost->xpos -= TILE_SIZE;
                            ghost->direction = 3;
                            break;
                        }
                        else if (ghost->xpos - TILE_SIZE < 0 && direction != 0)
                        {
                            ghost->xpos = 540;
                            ghost->direction = 3;
                            break;
                        }
                    }
                    else if (dir > 19 && dir < 29)
                    {
                        if (ghost->direction != 1 && down)
                        {
                            ghost->ypos += TILE_SIZE;
                            ghost->direction = 2;
                            break;
                        }
                    }
                    else if (dir > 29 && dir < 39)
                    {
                        if (ghost->direction != 2 && up)
                        {
                            ghost->ypos -= TILE_SIZE;
                            ghost->direction = 1;
                            break;
                        }
                    }
                }
                if (maze_Char[ghost->ypos/TILE_SIZE][ghost->xpos/TILE_SIZE] == ','&&ghost->velocity!=115)
                {
                    maze_Char[ghost->ypos/TILE_SIZE][ghost->xpos/TILE_SIZE] = ' ';
                    ghost->velocity = 115;
                    ChTexture.loadFromFile("pacman-art/ghosts/pinky.png");
                    ghost->sprite.setTexture(ChTexture);
                }
                ghost->sprite.setPosition(ghost->xpos, ghost->ypos);

                // Sleep to control ghost movement speed
            } // Adjust the sleep duration as needed
        }
        sf::sleep(sf::milliseconds(ghost->velocity));
    }
}


/////////////////////////////////USER INTERFACE THREAD FUNCTION ////////////// 
void *UserInterface(void *args)
{
    int windowWidth = MAZE_WIDTH * TILE_SIZE;
    int windowHeight = MAZE_HEIGHT * TILE_SIZE;
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Menu");
    sf::Font font;
    font.loadFromFile("pacfont-good.ttf"); // Load a font
    sf::Text PacmanText;
    PacmanText.setFont(font);                   // Set the font
    PacmanText.setCharacterSize(40);            // Set the character size
    PacmanText.setFillColor(sf::Color::Yellow); // Set the color
    PacmanText.setPosition(130, 200);           // Set the position
    string pstring = "  PACMAN\n\n    MENU\n\n  Press S\n  to Start";
    PacmanText.setString(pstring);
    window.draw(PacmanText);
    window.display();
    sf::Event event;
    while (window.isOpen())
    {
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                switch (event.key.code)
                {
                case sf::Keyboard::S:
                    uiThreadFinished = true;
                    pthread_cond_signal(&uiThreadFinishedCond);
                    pthread_exit(0);
                    break;
                }
            }
        }
    }
    uiThreadFinished = true;
    pthread_cond_signal(&uiThreadFinishedCond);
    pthread_exit(0);
}
////////////////////////////////////////////////////////////////////////////////////

bool collisionWithGhost(Pacman &pacman, Entity &ghost)
{
    // Calculate the bounding boxes for Pac-Man and the ghost
    sf::FloatRect pacmanBounds = pacman.p.getGlobalBounds();
    sf::FloatRect ghostBounds = ghost.sprite.getGlobalBounds();
    if (pacmanBounds.intersects(ghostBounds))
    {
        return true;
    }

    return false;
}

bool allPelletsEaten() {
    for (int i = 0; i < MAZE_HEIGHT; ++i) {
        for (int j = 0; j < MAZE_WIDTH; ++j) {
            if (maze_Char[i][j] == '.' || maze_Char[i][j] == '+') {
                // If there is any pellet remaining, return false
                return false;
            }
        }
    }
    // If no pellets are found, return true
    return true;
}


/////////////////////////////////// GAME ENGINE THREAD ///////////////////
void *game_Engine(void *args)
{
    pthread_create(&ui, NULL, &UserInterface, NULL);
    pthread_mutex_lock(&uiThreadFinishedMutex);
    while (!uiThreadFinished)
    {
        pthread_cond_wait(&uiThreadFinishedCond, &uiThreadFinishedMutex);
    }
    pthread_mutex_unlock(&uiThreadFinishedMutex);
    sf::Texture livesTex;
    livesTex.loadFromFile("pacman-art/pacman-left/1.png");
    sf::Sprite livesSprite;
    livesSprite.setTexture(livesTex);
    livesSprite.setScale(1.2, 1.2);
    loadMaze();
    bool pauseBool = false;
    dot.loadFromFile("pacman-art/other/dot.png");
    ghost_Texture.loadFromFile("pacman-art/ghosts/blinky.png");
    sf::Font font;
    font.loadFromFile("pacfont-good.ttf"); // Load a font
    sf::Text scoreText;
    scoreText.setFont(font);                   // Set the font
    scoreText.setCharacterSize(24);            // Set the character size
    scoreText.setFillColor(sf::Color::Yellow); // Set the color
    scoreText.setPosition(20, 10);             // Set the position
    sf::Font font2;
    font2.loadFromFile("Arial.ttf");
    sf::Text scoreT;
    scoreT.setFont(font2);                   // Set the font
    scoreT.setCharacterSize(24);            // Set the character size
    scoreT.setFillColor(sf::Color::Yellow); // Set the color
    scoreT.setPosition(140, 8);     
    int counter = 0;
    int windowWidth = MAZE_WIDTH * TILE_SIZE;
    int windowHeight = MAZE_HEIGHT * TILE_SIZE;
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Pac-Man Maze");
    sf::Clock clock;
    sf::Time lastUpdateTime = clock.getElapsedTime();
    sf::Time plastUpdateTime = clock.getElapsedTime();

    initializeGhosts();
    pthread_t ghostThreads[4];
    pthread_mutex_init(&ghostGlobalMutex, NULL);
    for (int i = 0; i < 4; ++i)
    {
        pthread_create(&ghostThreads[i], nullptr, ghostMovement, &ghosts[i]);
    }
    while (window.isOpen())
    {
        //////////////////////////////////////////////
        sf::Time now = clock.getElapsedTime();
        sf::Time deltaTime = now - lastUpdateTime;
        lastUpdateTime = now;
        float dt = deltaTime.asSeconds();
        //////////////////////////////////////////////
        sf::Time pnow = clock.getElapsedTime();
        sf::Time pdeltaTime = pnow - plastUpdateTime;
        float pdt = pdeltaTime.asSeconds();
        /////////////////////////////////////////////

        for (int i = 0; i < 4; ++i)
        {
            if (collisionWithGhost(pacman, ghosts[i]) )
            {    if(pPalletGlobalBool==0)
                pacman.resetPacman();
                else
                {
                    ghosts[i].reset=1;
                    ghosts[i].xpos=13*TILE_SIZE;
                    ghosts[i].ypos=(16*TILE_SIZE);
                } // Reset Pac-Man if collision occurs
            }
        }
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                switch (event.key.code)
                {
                case sf::Keyboard::W:
                    pacman.changeDirection(1); // Up
                    break;
                case sf::Keyboard::S:
                    pacman.changeDirection(2); // Down
                    break;
                case sf::Keyboard::A:
                    pacman.changeDirection(3); // Left
                    break;
                case sf::Keyboard::D:
                    pacman.changeDirection(0); // Right
                    break;
                case sf::Keyboard::P:
                    pauseBool = !pauseBool;
                    break;
                }
            }
        }
        int newY = pacman.ypos;
        int newX = pacman.xpos;
        pacman.move(dt);
        pthread_mutex_lock(&mutex1);  ////////////////////// SYNCHRONIZATION SCENARIO 2///////
        if (maze_Char[newY / TILE_SIZE][newX / TILE_SIZE] == '.')
        {
            score++;
            maze_Char[newY / TILE_SIZE][newX / TILE_SIZE] = ' ';
        }

        if (!pPalletGlobalBool)
        {
            plastUpdateTime = clock.getElapsedTime();
            if (maze_Char[newY / TILE_SIZE][newX / TILE_SIZE] == '+')
            {
                maze_Char[newY / TILE_SIZE][newX / TILE_SIZE] = ' ';
                score++;
                pPalletGlobalBool = true;
                for (int i = 0; i < 4; i++)
                {
                    prevSpeeds[i] = ghosts[i].velocity;
                    ghost_Texture.loadFromFile("pacman-art/ghosts/blue_ghost.png");
                    ghosts[i].sprite.setTexture(ghost_Texture);
                    ghosts[i].sprite.setScale(1.2, 1.2);
                    ghosts[i].velocity = 400;
                }
                palletBool = true;
            }
        }
        pthread_mutex_unlock(&mutex1);        
        if (pdt >= 5)
        {
            for (int i = 0; i < 4; i++)
            {
                ghost_Texture.loadFromFile("pacman-art/ghosts/blinky.png");
                ghosts[i].sprite.setTexture(ghost_Texture);
                ghosts[i].sprite.setScale(1.2, 1.2);
                ghosts[i].velocity = 150;
            }
            pPalletGlobalBool = false;
        }
        string scoreTT = to_string(score);
        string scoreString = "Score: ";
        scoreText.setString(scoreString);
        scoreT.setString(scoreTT);
        window.clear();
        drawMaze(window);
        pacman.p.setPosition(pacman.xpos, pacman.ypos);
        for (int i = 0; i < 4; ++i)
        {
            ghosts[i].sprite.setPosition(ghosts[i].xpos, ghosts[i].ypos);
            window.draw(ghosts[i].sprite);
        }
        for (int i = 0; i < pacman.lives; i++)
        {
            livesSprite.setPosition(400 + i * 25, 15);
            window.draw(livesSprite);
        }
        window.draw(pacman.p);
        window.draw(scoreText);
        window.draw(scoreT);
        window.display();
        if(allPelletsEaten() || pacman.lives == 0)
        {
            window.close();
            sf::RenderWindow window2(sf::VideoMode(windowWidth, windowHeight), "GAME OVER");
            sf::Font font;
            font.loadFromFile("pacfont-good.ttf"); // Load a font
            sf::Text PacmanText;
            PacmanText.setFont(font);                   // Set the font
            PacmanText.setCharacterSize(40);            // Set the character size
            PacmanText.setFillColor(sf::Color::Yellow); // Set the color
            PacmanText.setPosition(130, 200);           // Set the position
            string pstring = "GAME OVER \n\n   PRESS E \n   TO EXIT";
            PacmanText.setString(pstring);
            sf::Event event2;
            while(window2.isOpen())
            {
                window2.draw(PacmanText);
                window2.display();
                while (window2.pollEvent(event2))
                {
                    if (event2.type == sf::Event::Closed)
                        window2.close();
                    if (event2.type == sf::Event::KeyPressed)
                    {
                        switch (event2.key.code)
                        {
                        case sf::Keyboard::E:
                            pthread_exit(0);
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < ghosts.size(); ++i)
    {
        pthread_cancel(ghostThreads[i]);
    }
    pthread_exit(0);
}

int main()
{
    initializeSemaphores();
    pthread_t game_E;
    pthread_mutex_init(&mutex1, NULL);
    pthread_create(&game_E, NULL, &game_Engine, NULL);
    // game_Engine(NULL);
    pthread_exit(0);
}