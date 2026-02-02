#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>

using namespace sf;
using namespace std;

class Particle {
public:
    Vector2f pos;
    Vector2f vel;
    float life;
    float maxLife;
    Color col;
    bool active;

    Particle() : pos(0, 0), vel(0, 0), life(0), maxLife(0), col(Color::White), active(false) {}
};

class ParticleSystem {
private:
    static const int MAX_PARTICLES = 200;
    Particle particles[MAX_PARTICLES];
    int randomSeed;

public:
    ParticleSystem() : randomSeed(static_cast<int>(time(nullptr))) {
        for (auto& p : particles) p.active = false;
    }

    int simpleRandom() {
        randomSeed = (randomSeed * 1103515245 + 12345) & 0x7fffffff;
        return randomSeed;
    }

    void emit(const Vector2f& origin, const Color& color, int count = 20) {
        for (int i = 0; i < MAX_PARTICLES && count > 0; i++) {
            if (!particles[i].active) {
                float vx = (simpleRandom() % 101 - 50) / 2.0f;
                float vy = (simpleRandom() % 101 - 50) / 2.0f;
                float life = 0.5f + (simpleRandom() % 100) / 100.0f;

                particles[i].pos = origin;
                particles[i].vel = { vx, vy };
                particles[i].life = life;
                particles[i].maxLife = life;
                particles[i].col = color;
                particles[i].active = true;

                count--;
            }
        }
    }

    void update(float dt) {
        for (auto& p : particles) {
            if (!p.active) continue;
            p.pos += p.vel * dt;
            p.life -= dt;
            if (p.life <= 0) {
                p.active = false;
            }
            else {
                p.col.a = static_cast<Uint8>(255 * (p.life / p.maxLife));
            }
        }
    }

    void draw(RenderWindow& win) {
        CircleShape ps(2);
        ps.setOrigin(2, 2);
        for (auto& p : particles) {
            if (!p.active) continue;
            ps.setPosition(p.pos);
            ps.setFillColor(p.col);
            win.draw(ps);
        }
    }
};

class Food {
private:
    CircleShape shape;
    Vector2f position;
    float pulseTime;

public:
    Food() : pulseTime(0) {
        shape.setRadius(10);
        shape.setOrigin(10, 10);
        regenerate();
    }

    void regenerate() {
        position.x = static_cast<float>((rand() % 40) * 20);
        position.y = static_cast<float>((rand() % 30) * 20);
        shape.setPosition(position);
        shape.setFillColor(Color::Red);
    }

    void update(float dt) {
        pulseTime += dt * 5;
        float alpha = 150 + 105 * sin(pulseTime);
        shape.setFillColor(Color(255, 50, 50, static_cast<Uint8>(alpha)));
    }

    void draw(RenderWindow& win) const {
        win.draw(shape);
    }

    Vector2f getPosition() const { return position; }
    FloatRect getBounds() const { return shape.getGlobalBounds(); }
};

class Snake {
private:
    static const int MAX_SIZE = 200;
    RectangleShape segmentShapes[MAX_SIZE];
    Vector2f segmentPrevPos[MAX_SIZE];
    Vector2f segmentTargetPos[MAX_SIZE];
    int size;
    Vector2f direction;
    bool hasEaten;

public:
    Snake() : size(3), direction(20.f, 0.f), hasEaten(false) {
        for (int i = 0; i < size; i++) {
            segmentShapes[i].setSize({ 18, 18 });
            segmentShapes[i].setOrigin(9, 9);
            Vector2f pos(400 - i * 20.f, 300.f);
            segmentShapes[i].setPosition(pos);
            segmentPrevPos[i] = pos;
            segmentTargetPos[i] = pos;
        }
        updateColors();
    }

    void updateColors() {
        for (int i = 1; i < size; i++) {
            Color c(0, 200, 0, static_cast<Uint8>(255 - i * 2));
            segmentShapes[i].setSize({ 18, 18 });
            segmentShapes[i].setOrigin(9, 9);
            segmentShapes[i].setFillColor(c);
        }
    }

    void move() {
        for (int i = 0; i < size; i++)
            segmentPrevPos[i] = segmentShapes[i].getPosition();

        if (!hasEaten) {
            for (int i = size - 1; i > 0; i--)
                segmentTargetPos[i] = segmentPrevPos[i - 1];
        }
        else {
            if (size < MAX_SIZE) {
                segmentShapes[size] = segmentShapes[size - 1];
                segmentPrevPos[size] = segmentPrevPos[size - 1];
                segmentTargetPos[size] = segmentTargetPos[size - 1];
                size++;
            }
            hasEaten = false;
        }
        segmentTargetPos[0] = segmentPrevPos[0] + direction;
    }

    void update(float lerpFactor) {
        lerpFactor = fmaxf(0.f, fminf(lerpFactor, 1.f));
        for (int i = 0; i < size; i++) {
            Vector2f newPos = segmentPrevPos[i] +
                (segmentTargetPos[i] - segmentPrevPos[i]) * lerpFactor;
            segmentShapes[i].setPosition(newPos);
        }
    }

    void setDirection(Vector2f newDir) {
        if (size > 1) {
            Vector2f curDir = segmentTargetPos[0] - segmentTargetPos[1];
            if (newDir.x == -curDir.x && newDir.y == -curDir.y) return;
        }
        direction = newDir;
    }

    void grow() {
        hasEaten = true;
    }

    bool checkFoodCollision(const Food& food) const {
        Vector2f headPos = segmentShapes[0].getPosition();
        FloatRect headBounds(headPos.x - 11, headPos.y - 11, 22, 22);
        return headBounds.intersects(food.getBounds());
    }

    bool checkSelfCollision() const {
        Vector2f headPos = segmentTargetPos[0];
        for (int i = 4; i < size; i++)
            if (segmentTargetPos[i] == headPos) return true;
        return false;
    }

    bool checkWallCollision() const {
        Vector2f headPos = segmentTargetPos[0];
        return (headPos.x < 0 || headPos.x >= 800 || headPos.y < 0 || headPos.y >= 600);
    }

    void draw(RenderWindow& win) const {
        for (int i = 1; i < size; i++)
            win.draw(segmentShapes[i]);

        if (size > 0) {
            ConvexShape triangle;
            triangle.setPointCount(3);
            triangle.setPoint(0, { 0, -11 });
            triangle.setPoint(1, { 11, 11 });
            triangle.setPoint(2, { -11, 11 });
            triangle.setFillColor(Color::Green);
            float angle = atan2(direction.y, direction.x) * 180.f / 3.14159265f + 90.f;
            triangle.setPosition(segmentShapes[0].getPosition());
            triangle.setRotation(angle);
            win.draw(triangle);
        }
    }

    void reset() {
        size = 3; direction = { 20.f, 0.f }; hasEaten = false;
        for (int i = 0; i < size; i++) {
            Vector2f pos(400 - i * 20.f, 300.f);
            segmentShapes[i].setPosition(pos);
            segmentPrevPos[i] = pos;
            segmentTargetPos[i] = pos;
        }
        updateColors();
    }

    Vector2f getHeadPosition() const { return segmentTargetPos[0]; }
};

static const std::map<char, std::vector<std::string>> BLOCK_LETTERS = {
    { 'A', { " ### ", "#   #", "#####", "#   #", "#   #" } },
    { 'E', { "#####", "#    ", "###  ", "#    ", "#####" } },
    { 'G', { " ####", "#    ", "# ###", "#   #", " ####" } },
    { 'K', { "#   #", "#  # ", "###  ", "#  # ", "#   #" } },
    { 'M', { "#   #", "## ##", "# # #", "#   #", "#   #" } },
    { 'N', { "#   #", "##  #", "# # #", "#  ##", "#   #" } },
    { 'P', { "#### ", "#   #", "#### ", "#    ", "#    " } },
    { 'R', { "#### ", "#   #", "#### ", "#  # ", "#   #" } },
    { 'S', { " ####", "#    ", " ####", "    #", "#### " } },
    { 'T', { "#####", "  #  ", "  #  ", "  #  ", "  #  " } },
    { ' ', { "", "", "", "", "" } }
};

class Game {
private:
    RenderWindow window;
    Snake snake;
    Food food;
    ParticleSystem particles;
    Clock moveClock;
    Clock frameClock;
    float gameSpeed;
    int score;
    int level;
    bool gameOver;

    Texture backgroundTexture;
    Sprite backgroundSprite;

public:
    Game()
        : window(VideoMode(800, 600), "Mighty Snake Game"),
        gameSpeed(200.f), score(0), level(1), gameOver(false)
    {
        window.setVerticalSyncEnabled(true);
        moveClock.restart();
        frameClock.restart();
        srand(static_cast<unsigned>(time(nullptr)));

        if (backgroundTexture.loadFromFile("background.jpg")) {
            backgroundSprite.setTexture(backgroundTexture);
            float texW = (float)backgroundTexture.getSize().x;
            float texH = (float)backgroundTexture.getSize().y;
            backgroundSprite.setScale(800.f / texW, 600.f / texH);
            backgroundSprite.setPosition(0.f, 0.f);
        }
    }

    void drawBlockText(const std::string& text, float yOffset, int blockSize) {
        vector<const vector<string>*> letters;
        for (char c : text) {
            char up = (char)toupper(c);
            auto it = BLOCK_LETTERS.find(up);
            letters.push_back(it != BLOCK_LETTERS.end() ? &it->second : &BLOCK_LETTERS.at(' '));
        }

        int totalCols = 0;
        for (auto ptr : letters) {
            totalCols += (ptr->empty() || (*ptr)[0].empty()) ? 3 : 6;
        }
        if (!letters.empty()) {
            const auto& last = *letters.back();
            if (!last.empty() && !last[0].empty()) totalCols -= 1;
        }

        float xOffset = (800.f - totalCols * blockSize) / 2.f;
        int cursor = 0;

        for (auto ptr : letters) {
            const auto& pattern = *ptr;
            bool isSpace = pattern.empty() || pattern[0].empty();
            if (isSpace) {
                cursor += 3;
            }
            else {
                for (int row = 0; row < 5; ++row) {
                    for (int col = 0; col < 5; ++col) {
                        if (pattern[row][col] == '#') {
                            RectangleShape block({ (float)(blockSize - 1), (float)(blockSize - 1) });
                            block.setPosition(
                                xOffset + (cursor + col) * blockSize,
                                yOffset + row * blockSize
                            );
                            block.setFillColor(Color(255, 50, 50));
                            window.draw(block);
                        }
                    }
                }
                cursor += 6;
            }
        }
    }

    void showStartMenu() {
        while (window.isOpen()) {
            Event e;
            while (window.pollEvent(e)) {
                if (e.type == Event::Closed) {
                    window.close();
                    return;
                }
                if (e.type == Event::KeyPressed) {
                    if (e.key.code == Keyboard::Enter)
                        return;
                    if (e.key.code == Keyboard::Escape) {
                        window.close();
                        return;
                    }
                }
            }
            window.clear(Color::Black);
            drawBlockText("SNAKE", 150.f, 12);
            drawBlockText("PRESS ENTER", 300.f, 10);
            window.display();
        }
    }

    void handleInput() {
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) {
                window.close();
            }
            if (e.type == Event::KeyPressed) {
                if (gameOver) {
                    if (e.key.code == Keyboard::R) restart();
                    if (e.key.code == Keyboard::Escape) window.close();
                }
                else {
                    switch (e.key.code) {
                    case Keyboard::W:
                    case Keyboard::Up:
                        snake.setDirection({ 0, -20 }); break;
                    case Keyboard::S:
                    case Keyboard::Down:
                        snake.setDirection({ 0, 20 }); break;
                    case Keyboard::A:
                    case Keyboard::Left:
                        snake.setDirection({ -20, 0 }); break;
                    case Keyboard::D:
                    case Keyboard::Right:
                        snake.setDirection({ 20, 0 }); break;
                    case Keyboard::Escape:
                        window.close(); break;
                    default:
                        break;
                    }
                }
            }
        }
    }

    void update() {
        float dt = frameClock.restart().asSeconds();
        float elapsedMs = moveClock.getElapsedTime().asMilliseconds();
        float lerpFactor = elapsedMs / gameSpeed;

        snake.update(lerpFactor);
        food.update(dt);
        particles.update(dt);

        if (elapsedMs >= gameSpeed && !gameOver) {
            snake.move();
            if (snake.checkWallCollision() || snake.checkSelfCollision()) {
                gameOver = true;
                particles.emit(snake.getHeadPosition(), Color::Red, 50);
            }
            else if (snake.checkFoodCollision(food)) {
                snake.grow();
                particles.emit(food.getPosition(), Color::Yellow, 30);
                food.regenerate();
                score += 10 * level;
                if (score % 100 == 0) {
                    level++;
                    gameSpeed = max(50.f, gameSpeed - 15.f);
                }
            }
            moveClock.restart();
        }
    }

    void drawGameOver() {
        const int blockSize = 10;
        const Color color(255, 50, 50);

        vector<vector<string>> letters = {
            { " ####", "#    ", "# ###", "#   #", " ####" },
            { " ### ", "#   #", "#####", "#   #", "#   #" },
            { "#   #", "## ##", "# # #", "#   #", "#   #" },
            { "#####", "#    ", "#### ", "#    ", "#####" },
            {},
            { " ### ", "#   #", "#   #", "#   #", " ### " },
            { "#   #", "#   #", "#   #", " # # ", "  #  " },
            { "#####", "#    ", "#### ", "#    ", "#####" },
            { "#### ", "#   #", "#### ", "#  # ", "#   #" }
        };

        int totalWidth = 0;
        for (auto& letter : letters)
            totalWidth += !letter.empty() ? 6 : 3;
        totalWidth -= 1;

        Vector2f offset((800 - totalWidth * blockSize) / 2.f,
            (600 - 5 * blockSize) / 2.f);

        int xCursor = 0;
        for (auto& letter : letters) {
            for (int y = 0; y < letter.size(); ++y) {
                for (int x = 0; x < letter[y].size(); ++x) {
                    if (letter[y][x] == '#') {
                        RectangleShape block({ (float)(blockSize - 1), (float)(blockSize - 1) });
                        block.setPosition(offset + Vector2f((xCursor + x) * blockSize, y * blockSize));
                        block.setFillColor(color);
                        window.draw(block);
                    }
                }
            }
            xCursor += !letter.empty() ? 6 : 3;
        }
    }

    void render() {
        window.clear();
        if (backgroundSprite.getTexture()) {
            window.draw(backgroundSprite);
        }

        VertexArray grid(Lines);
        for (int x = 20; x < 800; x += 20) {
            grid.append(Vertex({ (float)x, 0.f }, Color(255, 255, 255, 20)));
            grid.append(Vertex({ (float)x, 600.f }, Color(255, 255, 255, 20)));
        }
        for (int y = 20; y < 600; y += 20) {
            grid.append(Vertex({ 0.f, (float)y }, Color(255, 255, 255, 20)));
            grid.append(Vertex({ 800.f, (float)y }, Color(255, 255, 255, 20)));
        }
        window.draw(grid);

        food.draw(window);
        snake.draw(window);
        particles.draw(window);

        if (gameOver) {
            drawGameOver();
        }

        window.display();
    }

    void restart() {
        snake.reset();
        food.regenerate();
        gameSpeed = 200.f;
        score = 0;
        level = 1;
        gameOver = false;
        moveClock.restart();
        frameClock.restart();
    }

    void run() {
        showStartMenu();
        if (!window.isOpen())
            return;
        while (window.isOpen()) {
            handleInput();
            update();
            render();
        }
    }
};

int main()
{
    Game game;
    game.run();
    
    return 0;
}