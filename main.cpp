#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <string>
#include <algorithm>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

struct Vec2 { float x, y; };

struct Brick {
    float x, y;
    bool visible;
    int hitsRequired;
    bool hasPowerUp;
};

struct Ball {
    Vec2 pos;
    Vec2 vel;
    float radius;
};

struct Particle {
    Vec2 pos, vel;
    float life;
};

enum GameState { MENU, PLAYING, LEVEL_COMPLETE, GAME_OVER };

int paddleWidth = 100, paddleHeight = 20;
int paddleX = WINDOW_WIDTH / 2 - paddleWidth / 2;
int paddleY = 50;

float ballSpeed = 5.0;
std::vector<Ball> balls;
std::vector<Brick> bricks;
std::vector<Particle> particles;

int rows = 5, cols = 10;
int brickWidth = 70, brickHeight = 20;
int score = 0, level = 1, lives = 3;
GameState gameState = MENU;
bool paddleExpanded = false;
int expandTimer = 0;

void resetBricks() {
    bricks.clear();
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            Brick b;
            b.x = j * (brickWidth + 5) + 30;
            b.y = WINDOW_HEIGHT - (i + 1) * (brickHeight + 5);
            b.visible = true;
            b.hitsRequired = (level >= 2 && rand() % 4 == 0) ? 2 : 1;
            b.hasPowerUp = (rand() % 10 == 0);
            bricks.push_back(b);
        }
    }
}

void spawnBall() {
    Ball b;
    b.pos = { float(WINDOW_WIDTH / 2), float(paddleY + 30) };
    b.vel = { (rand() % 2 == 0 ? -1 : 1) * ballSpeed, ballSpeed };
    b.radius = 10;
    balls.push_back(b);
}

void resetGame() {
    score = 0;
    level = 1;
    lives = 3;
    paddleExpanded = false;
    balls.clear();
    spawnBall();
    resetBricks();
}

void drawText(float x, float y, const char* str) {
    glRasterPos2f(x, y);
    while (*str) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *str++);
}

void drawRect(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float x, float y, float r) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i++) {
        float angle = i * M_PI / 180;
        glVertex2f(x + cos(angle) * r, y + sin(angle) * r);
    }
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == MENU) {
        drawText(300, 300, "Press ENTER to Start");
    } else if (gameState == GAME_OVER) {
        drawText(300, 300, "Game Over! Press R to Restart");
    } else if (gameState == LEVEL_COMPLETE) {
        drawText(280, 300, "Level Complete! Press ENTER for Next");
    } else if (gameState == PLAYING) {
        drawRect(paddleX, paddleY, paddleWidth, paddleHeight, 0.2f, 0.7f, 1);
        for (auto& b : balls) {
            glColor3f(1, 1, 1);
            drawCircle(b.pos.x, b.pos.y, b.radius);
        }
        for (auto& brick : bricks) {
            if (!brick.visible) continue;
            if (brick.hitsRequired == 2)
                drawRect(brick.x, brick.y, brickWidth, brickHeight, 1, 0.5f, 0);
            else
                drawRect(brick.x, brick.y, brickWidth, brickHeight, 0.9f, 0.2f, 0.2f);
            if (brick.hasPowerUp) drawText(brick.x + 25, brick.y + 5, "P");
        }
        for (auto& p : particles) {
            glColor3f(1, 1, 0);
            drawCircle(p.pos.x, p.pos.y, 2);
        }
        drawText(10, 570, ("Score: " + std::to_string(score)).c_str());
        drawText(700, 570, ("Lives: " + std::to_string(lives)).c_str());
        drawText(370, 570, ("Level: " + std::to_string(level)).c_str());
    }
    glutSwapBuffers();
}

void update(int) {
    if (gameState != PLAYING) goto skip;

    if (expandTimer > 0 && --expandTimer == 0) paddleWidth = 100;

    for (auto& p : particles) {
        p.pos.x += p.vel.x;
        p.pos.y += p.vel.y;
        p.life -= 0.02f;
    }
    particles.erase(remove_if(particles.begin(), particles.end(), [](Particle& p) { return p.life <= 0; }), particles.end());

    for (auto& b : balls) {
        b.pos.x += b.vel.x;
        b.pos.y += b.vel.y;

        if (b.pos.x < 0 || b.pos.x > WINDOW_WIDTH) b.vel.x *= -1;
        if (b.pos.y > WINDOW_HEIGHT) b.vel.y *= -1;
        if (b.pos.y < 0) {
            lives--;
            if (lives <= 0) gameState = GAME_OVER;
            else { balls.clear(); spawnBall(); }
            break;
        }

        if (b.pos.x > paddleX && b.pos.x < paddleX + paddleWidth && b.pos.y <= paddleY + paddleHeight) {
            b.vel.y *= -1;
        }

        for (auto& brick : bricks) {
            if (!brick.visible) continue;
            if (b.pos.x > brick.x && b.pos.x < brick.x + brickWidth &&
                b.pos.y > brick.y && b.pos.y < brick.y + brickHeight) {
                brick.hitsRequired--;
                if (brick.hitsRequired <= 0) {
                    brick.visible = false;
                    score += 10;
                    if (brick.hasPowerUp) {
                        paddleExpanded = true;
                        paddleWidth = 160;
                        expandTimer = 500;
                    }
                    for (int i = 0; i < 10; i++) {
                        Particle p = { { b.pos.x, b.pos.y }, { float(rand()%5 - 2), float(rand()%5 - 2) }, 1.0f };
                        particles.push_back(p);
                    }
                }
                b.vel.y *= -1;
                break;
            }
        }
    }

    if (std::all_of(bricks.begin(), bricks.end(), [](Brick& b) { return !b.visible; })) {
        gameState = LEVEL_COMPLETE;
        level++;
        ballSpeed += 0.5;
    }

skip:
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int, int) {
    if (key == 13) {
        if (gameState == MENU || gameState == LEVEL_COMPLETE) {
            resetBricks();
            balls.clear();
            spawnBall();
            gameState = PLAYING;
        }
    }
    if (key == 'r' || key == 'R') {
        resetGame();
        gameState = PLAYING;
    }
}

// Paddle control with mouse
void mouseMotion(int x, int y) {
    paddleX = x - paddleWidth / 2;
    if (paddleX < 0) paddleX = 0;
    if (paddleX + paddleWidth > WINDOW_WIDTH) paddleX = WINDOW_WIDTH - paddleWidth;
}

int main(int argc, char** argv) {
    srand(time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Advanced Brick Breaker");
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

    resetBricks();
    spawnBall();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion); // mouse paddle control
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
