#include <windows.h>
#include <GL/glut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <ctime>
#include <vector>
#include <cstdio>

using namespace std;


const float PI = 3.1415926535f;

// --- Animation & Environment Variables ---
float dayNightPhase = 0.1f;
float crystalGlow = 0.0f;
float riverFlowOffset = 0.0f;
enum Weather { SUNNY, RAINY, SNOWY };
Weather currentWeather = SUNNY;
enum TimeMoment { MORNING, NOON, EVENING, NIGHT };
enum ParticleType { SPARK, SMOKE, EMBER };
const int MAX_PARTICLES = 200;
float snowCoverage = 0.0f;
float riverFreezeAmount = 0.0f;

// --- Audio ---
Mix_Music *riverSound = NULL;
Mix_Chunk *thunderSound = NULL;
Mix_Chunk *birdSound = NULL;
Mix_Chunk *rainSound = NULL;
Mix_Music *winterSound = NULL;

int birdChannel = -1;
int rainChannel = -1;
int thunderChannel = -1;

// --- Struct Definitions for Scene Elements ---
struct FallingLeaf {
    float x, y, size, speed, sway, swaySpeed, rotation, rotationSpeed;
    float r, g, b;
};
const int LEAF_COUNT = 80;
FallingLeaf leaves[LEAF_COUNT];

enum ElfState { ELF_WALKING, ELF_IDLE };
struct Elf {
    float x, y, targetX, speed;
    float r, g, b;
    ElfState state;
    float stateTimer;
    float animationPhase;
};
const int ELF_COUNT = 6;
Elf elves[ELF_COUNT];


struct Particle {
    ParticleType type;
    float x, y, vx, vy;
    float life, maxLife;
    float size;
    float r, g, b, a;
};
vector<Particle> particles;

struct Butterfly {
    float x, y, initialY;
    float speed, directionAngle;
    float flutterPhase, bobPhase;
    float r, g, b;
};
vector<Butterfly> butterflies;


enum PuddleState {
    PUDDLE_GROWING,
    PUDDLE_FULL,
    PUDDLE_SHRINKING,
    PUDDLE_FREEZING,
    PUDDLE_FROZEN,
    PUDDLE_MELTING
};

struct Puddle {
    float x, y;
    float currentRadius;
    float maxRadius;
    PuddleState state;
    float freezeProgress;
};
vector<Puddle> puddles;


struct Firefly {
    float x, y, z;
    float initialY;
    float speed;
    float glowPhase;
    float movePhaseX, movePhaseY;
};
vector<Firefly> fireflies;


struct Campfire {
    float x, y;
    float flamePhase1;
    float flamePhase2;
};
vector<Campfire> campfires;

struct Spark {
    float x, y, vx, vy, life;
    float size;
};
vector<Spark> sparks;

struct SmokePuff {
    float x, y, radius, alpha, life;
    float vx, vy;
};
vector<SmokePuff> smokePuffs;

struct Raindrop {
    float x, y, speed;
};
const int RAIN_COUNT = 300;
Raindrop raindrops[RAIN_COUNT];

struct Splash {
    float x, y, radius, maxRadius, life;
};
vector<Splash> splashes;


struct Droplet {
    float x, y, vx, vy, life;
};
vector<Droplet> droplets;

struct Star {
    float x, y, radius, alpha, twinkleSpeed, initialPhase;
};
const int STAR_COUNT = 150;
Star stars[STAR_COUNT];

struct CloudCircle {
    float x_offset, y_offset, radius, yScale;
    float r, g, b, a;
};

struct Cloud {
    float x, y, speed;
    int num_circles;
    CloudCircle circles[20];
};
const int CLOUD_COUNT = 7;
Cloud clouds[CLOUD_COUNT];

struct Bird {
    float x, y;
    float speed;
    float phase;
};
const int MAX_BIRDS = 7;
Bird birds[MAX_BIRDS];

struct FairyFox {
    float progress;
    float speed;
    float tailSway;
};
FairyFox fox;

struct Snowflake {
    float x, y, size, speed, swayPhase;
};
const int SNOW_COUNT = 1000;
Snowflake snowflakes[SNOW_COUNT];


// --- Forward Declarations ---

void initSceneElements();
void initAudio();
void updateScene(int);
void updateLeaves();
void display();
void cleanup();
void drawForegroundRiver();
void drawMoon();
void drawSunAndMoon();
void drawStars();
void drawClouds();
void initBirds();
void updateBirds();
void drawBirds();
void drawCrystal(float x, float y);
void drawHouse(float x, float y, float scale);
void drawFairyFox();
void drawFoxPath();
void drawGreatTree();
void drawUpdatedSimpleTree(float x, float y, float scale);
void drawLeaves();
void drawElves();
void drawCampfire();
void drawFlowersAndBushes();
void drawHangingLantern(float x, float y);
void drawHangingMoss(float x, float y, float scale);
void drawWishingWell(float x, float y);
void drawArcheryTrainingGround(float x, float y);
void drawArcheryTarget(float x, float y, float scale);
void drawPracticeDummy(float x, float y, float scale);
void drawArrowQuiver(float x, float y, float scale);
void drawParticles();
void drawPuddles();
void updatePuddles(float dt);
void initSnow();
void updateSnow();
void drawSnow();
void drawSnowCover();



TimeMoment getTimeMoment() {
    if (dayNightPhase >= 0.0f && dayNightPhase < 0.12f) return MORNING;
    if (dayNightPhase >= 0.12f && dayNightPhase < 0.44f) return NOON;
    if (dayNightPhase >= 0.44f && dayNightPhase < 0.5f) return EVENING;
    return NIGHT;
}

void setSceneElementColor(float baseR, float baseG, float baseB, float alpha = 1.0f) {
    TimeMoment tm = getTimeMoment();
    float r = baseR, g = baseG, b = baseB;

    if (currentWeather == RAINY) {
        r *= 0.6f; g *= 0.6f; b *= 0.7f;
    }

    if (tm == NIGHT) {
        r = r * 0.4f + 0.05f; g = g * 0.4f + 0.05f; b = b * 0.4f + 0.15f;
    } else if (tm == EVENING) {
        r = r * 0.7f + 0.3f; g = g * 0.7f + 0.2f; b = b * 0.7f + 0.05f;
    } else if (tm == MORNING) {
        r = r * 0.8f + 0.2f; g = g * 0.8f + 0.15f; b = b * 0.8f + 0.1f;
    }
    glColor4f(r, g, b, alpha);
}

// --- Drawing Primitives ---
void drawCircle(float cx, float cy, float radius, float yScale = 1.0f) {
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(cx, cy);
      for (int i = 0; i <= 30; ++i) {
          float angle = i * 2.0f * PI / 30.0f;
          glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius * yScale);
      }
    glEnd();
}

void drawPolygon(int sides, float cx, float cy, float radius, float rotation = 0.0f) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < sides; ++i) {
        float angle = i * 2.0f * PI / sides + rotation;
        glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius);
    }
    glEnd();
}

void drawButterflies() {

    if (currentWeather == RAINY) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& b : butterflies) {
        glPushMatrix();
        glTranslatef(b.x, b.y, 0.0f);
        glRotatef(b.directionAngle * 180.0f / PI - 90.0f, 0, 0, 1);

        float wingAngle = 45.0f + 30.0f * sinf(b.flutterPhase);

        // Wings
        glColor3f(b.r, b.g, b.b);
        glPushMatrix();
            glRotatef(wingAngle, 0, 1, 0);
            glBegin(GL_TRIANGLES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(-0.03f, 0.02f);
                glVertex2f(-0.02f, 0.04f);
            glEnd();
        glPopMatrix();
        glPushMatrix();
            glRotatef(-wingAngle, 0, 1, 0);
            glBegin(GL_TRIANGLES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(0.03f, 0.02f);
                glVertex2f(0.02f, 0.04f);
            glEnd();
        glPopMatrix();

        // Body
        glColor3f(0.1f, 0.1f, 0.1f);
        glRectf(-0.005f, -0.01f, 0.005f, 0.03f);

        glPopMatrix();
    }

    glDisable(GL_BLEND);
}



void drawFireflies() {

    if (getTimeMoment() != NIGHT || currentWeather == RAINY || currentWeather == SNOWY) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (const auto& f : fireflies) {
        float glowIntensity = 0.6f + 0.4f * sinf(f.glowPhase);
        glColor4f(1.0f, 1.0f, 0.7f, glowIntensity);

        glPushMatrix();
        glTranslatef(f.x, f.y, f.z);


        float size = 0.012f;
        glBegin(GL_QUADS);
            glVertex3f(-size/2, -size/2, 0.0f);
            glVertex3f( size/2, -size/2, 0.0f);
            glVertex3f( size/2,  size/2, 0.0f);
            glVertex3f(-size/2,  size/2, 0.0f);
        glEnd();


        glColor4f(1.0f, 1.0f, 0.7f, glowIntensity * 0.3f);
        drawCircle(0, 0, size * 2.0f);

        glPopMatrix();
    }
    glDisable(GL_BLEND);
}

// --- Drawing Functions ---


void drawWishingWell(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    float scale = 0.7f;
    glScalef(scale, scale, 1.0f);

    // Rope
    setSceneElementColor(0.6f, 0.5f, 0.3f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        glVertex2f(0.0f, 0.22f);
        glVertex2f(0.0f, -0.05f);
    glEnd();
    // Axle
    setSceneElementColor(0.35f, 0.2f, 0.1f);
    glRectf(-0.18f, 0.2f, 0.18f, 0.24f);
    // Wheel
    drawCircle(0.0f, 0.22f, 0.04f);

    // Bucket
    setSceneElementColor(0.55f, 0.35f, 0.15f);
    glBegin(GL_QUADS);
        glVertex2f(-0.03f, -0.05f);
        glVertex2f(0.03f, -0.05f);
        glVertex2f(0.04f, -0.12f);
        glVertex2f(-0.04f, -0.12f);
    glEnd();

     // --- Stone Base ---
    // Main Base
    setSceneElementColor(0.5f, 0.5f, 0.55f);
    glBegin(GL_QUADS);
        glVertex2f(-0.22f, -0.2f);
        glVertex2f(0.22f, -0.2f);
        glVertex2f(0.2f, 0.1f);
        glVertex2f(-0.2f, 0.1f);
    glEnd();
    // Stone lines
    setSceneElementColor(0.4f, 0.4f, 0.45f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex2f(-0.1f, -0.2f); glVertex2f(-0.1f, 0.1f);
        glVertex2f(0.0f, -0.2f); glVertex2f(0.0f, 0.1f);
        glVertex2f(0.1f, -0.2f); glVertex2f(0.1f, 0.1f);
    glEnd();

    // ---  Front Support Post and Rim ---
    // Front Post
    setSceneElementColor(0.4f, 0.25f, 0.15f);
    glRectf(-0.21f, 0.05f, -0.16f, 0.3f);

    // ---  Back Support Post ---
    setSceneElementColor(0.4f, 0.25f, 0.15f); // Dark wood for posts
    glRectf(0.16f, 0.05f, 0.21f, 0.3f);

    // Stone Rim
    setSceneElementColor(0.6f, 0.6f, 0.65f); // Lighter grey for front rim
    glRectf(-0.22f, 0.05f, 0.22f, 0.1f);

    // ---  Roof ---
    setSceneElementColor(0.5f, 0.3f, 0.15f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.25f, 0.3f);
        glVertex2f(0.25f, 0.3f);
        glVertex2f(0.0f, 0.4f);
    glEnd();


    if (currentWeather == SNOWY) {
        glColor4f(0.95f, 0.95f, 1.0f, 1.0f);
        glBegin(GL_TRIANGLES);
            glVertex2f(-0.26f, 0.3f);
            glVertex2f(0.26f, 0.3f);
            glVertex2f(0.0f, 0.42f);
        glEnd();
    }

    glPopMatrix();
}




void drawCampfire() {
    for (const auto& fire : campfires) {
        glPushMatrix();
        glTranslatef(fire.x, fire.y, 0.0f);

        // --- Logs ---
        setSceneElementColor(0.4f, 0.2f, 0.1f);
        glRectf(-0.06f, -0.04f, 0.06f, -0.01f);
        setSceneElementColor(0.5f, 0.3f, 0.15f);
        glRectf(-0.04f, -0.01f, 0.05f, 0.02f);


         if (currentWeather != RAINY && currentWeather != SNOWY) {

            glPushAttrib(GL_ENABLE_BIT);
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            float flickerScale = 1.0f + 0.15f * sinf(fire.flamePhase1 * 1.8f);
            float swayX = 0.005f * cosf(fire.flamePhase2);

            glPushMatrix();

            glTranslatef(0.0f, 0.01f, 0.0f);
            glScalef(1.0f, flickerScale, 1.0f);

            // Outer Flame
            glBegin(GL_POLYGON);
                glColor4f(1.0f, 0.8f, 0.2f, 0.9f); glVertex2f(0.0f, 0.01f);
                glColor4f(1.0f, 0.4f, 0.1f, 0.7f); glVertex2f(-swayX - 0.05f, 0.03f);
                glVertex2f(swayX, 0.1f);
                glVertex2f(swayX + 0.05f, 0.03f);
            glEnd();

            // Inner Core
            glBegin(GL_POLYGON);
                glColor4f(1.0f, 1.0f, 0.8f, 1.0f); glVertex2f(0.0f, 0.01f);
                glColor4f(1.0f, 0.8f, 0.2f, 0.9f); glVertex2f(-swayX * 0.5f - 0.025f, 0.02f);
                glVertex2f(swayX * 0.7f, 0.06f);
                glVertex2f(swayX * 0.5f + 0.025f, 0.02f);
            glEnd();

            glPopMatrix();
            glPopAttrib();

            // --- Draw Sparks and Smoke ---
            drawParticles();
        }
        glPopMatrix();
    }
}


void drawFlowersAndBushes() {
    // --- Draw Upgraded Bushes ---
    auto drawUpgradedBush = [](float x, float y, float scale = 1.0f) {
        glPushMatrix();
        glTranslatef(x, y, 0);
        glScalef(scale, scale, 1.0f);

        // Dark base layer
        setSceneElementColor(0.1f, 0.4f, 0.15f);
        drawCircle(0.0f, -0.03f, 0.08f);
        drawCircle(-0.04f, -0.01f, 0.06f);
        drawCircle(0.04f, -0.01f, 0.07f);

        // Mid layer
        setSceneElementColor(0.15f, 0.55f, 0.2f);
        drawCircle(0.0f, 0.0f, 0.07f);
        drawCircle(-0.03f, 0.02f, 0.05f);
        drawCircle(0.05f, 0.02f, 0.06f);

        // Highlight layer
        setSceneElementColor(0.2f, 0.7f, 0.25f);
        drawCircle(0.0f, 0.02f, 0.05f);
        drawCircle(-0.02f, 0.03f, 0.04f);
        drawCircle(0.03f, 0.03f, 0.05f);

        glPopMatrix();
    };

    drawUpgradedBush(-1.8f, -0.7f, 0.8f);
    drawUpgradedBush(-0.79f, -0.57f, 0.7f);
    drawUpgradedBush(0.5f, -0.6f, 1.0f);
    drawUpgradedBush(2.4f, -0.5f, 0.8f);
    drawUpgradedBush(2.35f, -0.72f, 0.7f);
    drawUpgradedBush(-2.4f, -0.5f, 0.9f);


    auto drawFlower = [](float x, float y, float r, float g, float b, float scale = 1.0f) {
        glPushMatrix();
        glTranslatef(x, y, 0);
        glScalef(scale, scale, 1.0f);

        // Stem
        setSceneElementColor(0.1f, 0.5f, 0.15f);
        glRectf(-0.005f, -0.05f, 0.005f, 0.0f);

        // Flower Head with sway
        glPushMatrix();


        float swayOffset = 0.01f * sinf(crystalGlow * 1.5f);
        glTranslatef(swayOffset, 0.0f, 0.0f);

        setSceneElementColor(r, g, b);
        drawPolygon(5, 0, 0, 0.02f);
        setSceneElementColor(1.0f, 1.0f, 0.3f);
        drawCircle(0, 0, 0.008f);

        glPopMatrix();

        glPopMatrix();
    };

    drawFlower(-0.5f, -0.8f, 1.0f, 0.4f, 0.4f);
    drawFlower(-0.45f, -0.83f, 1.0f, 0.4f, 0.4f, 0.8f);
    drawFlower(0.8f, -0.6f, 0.9f, 0.9f, 1.0f);
    drawFlower(-2.32f, -0.2f, 0.9f, 0.9f, 1.0f);
    drawFlower(2.2f, -0.7f, 0.6f, 0.6f, 1.0f);
    drawFlower(-2.31f, -0.28f, 0.6f, 0.6f, 1.0f);
    drawFlower(2.25f, -0.72f, 0.6f, 0.6f, 1.0f, 0.9f);
    drawFlower(-2.31f, -0.78f, 0.6f, 0.6f, 1.0f);
    drawFlower(-2.35f, -0.75f, 1.0f, 0.4f, 0.4f);
    drawFlower(-2.4f, -0.78f, 0.9f, 0.9f, 1.0f);
    drawFlower(0.7f, -0.45f, 1.0f, 0.4f, 0.4f, 0.8f);
    drawFlower(0.76f, -0.45f, 1.0f, 0.9f, 0.9f, 1.0f);
}

void drawSky() {
    TimeMoment tm = getTimeMoment();
    float r1, g1, b1, r2, g2, b2;

    if (currentWeather == RAINY) {
        r1 = 0.3f; g1 = 0.35f; b1 = 0.4f; r2 = 0.4f; g2 = 0.45f; b2 = 0.5f;
    } else {
        if (tm == MORNING) { r1 = 0.6f; g1 = 0.7f; b1 = 1.0f; r2 = 1.0f; g2 = 0.9f;  b2 = 0.8f; }
        else if (tm == NOON)    { r1 = 0.1f; g1 = 0.6f; b1 = 1.0f; r2 = 0.7f; g2 = 0.85f; b2 = 1.0f; }
        else if (tm == EVENING) { r1 = 0.9f; g1 = 0.5f; b1 = 0.2f; r2 = 1.0f; g2 = 0.8f;  b2 = 0.4f; }
        else { r1 = 0.0f; g1 = 0.0f; b1 = 0.1f; r2 = 0.1f; g2 = 0.1f;  b2 = 0.35f; }
    }

    glBegin(GL_QUADS);
      glColor3f(r1, g1, b1); glVertex2f(-2.5f, 1.5f);
      glColor3f(r1, g1, b1); glVertex2f( 2.5f, 1.5f);
      glColor3f(r2, g2, b2); glVertex2f( 2.5f, -1.5f);
      glColor3f(r2, g2, b2); glVertex2f(-2.5f, -1.5f);
    glEnd();
}

void drawMoon() {

    float moonPhase = (dayNightPhase - 0.5f) * 2.0f;


    float moonX = -2.8f + (moonPhase * 5.6f);
    float moonY = -0.2f + 1.5f * sinf(moonPhase * PI);


    if (moonY > -0.1f) {
        // Main moon body
        glColor3f(0.9f, 0.9f, 0.85f);
        drawCircle(moonX, moonY, 0.12f);

        // Craters (darker grey)
        glColor3f(0.7f, 0.7f, 0.7f);
        drawCircle(moonX - 0.025f, moonY + 0.025f, 0.02f);
        drawCircle(moonX + 0.04f, moonY - 0.015f, 0.03f);
        drawCircle(moonX + 0.015f, moonY + 0.04f, 0.015f);

        // Outer Glow
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.9f, 0.9f, 1.0f, 0.2f);
        drawCircle(moonX, moonY, 0.17f);
        glDisable(GL_BLEND);
    }
}

void drawClouds() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < CLOUD_COUNT; ++i) {
        TimeMoment tm = getTimeMoment();
        float main_r, main_g, main_b;
        float shadow_r, shadow_g, shadow_b;


        if (tm == MORNING) {
            main_r = 1.0f; main_g = 0.98f; main_b = 0.9f;
            shadow_r = 0.9f; shadow_g = 0.88f; shadow_b = 0.8f;
        } else if (tm == NOON) {
            main_r = 1.0f; main_g = 1.0f; main_b = 1.0f;
            shadow_r = 0.85f; shadow_g = 0.85f; shadow_b = 0.9f;
        } else if (tm == EVENING) {
            main_r = 1.0f; main_g = 0.9f; main_b = 0.7f;
            shadow_r = 0.9f; shadow_g = 0.75f; shadow_b = 0.6f;
        } else if (tm == NIGHT) {
            main_r = 0.5f; main_g = 0.5f; main_b = 0.6f;
            shadow_r = 0.35f; shadow_g = 0.35f; shadow_b = 0.45f;
        }

        if (currentWeather == RAINY) {
            main_r = 0.7f; main_g = 0.7f; main_b = 0.75f;
            shadow_r = 0.5f; shadow_g = 0.5f; shadow_b = 0.55f;
        }


        glColor4f(shadow_r, shadow_g, shadow_b, 1.0f);
        for (int j = 0; j < clouds[i].num_circles; ++j) {
            const CloudCircle& c = clouds[i].circles[j];
            drawCircle(clouds[i].x + c.x_offset, clouds[i].y + c.y_offset - 0.015f, c.radius, c.yScale);
        }


        glColor4f(main_r, main_g, main_b, 1.0f);
        for (int j = 0; j < clouds[i].num_circles; ++j) {
            const CloudCircle& c = clouds[i].circles[j];
            drawCircle(clouds[i].x + c.x_offset, clouds[i].y + c.y_offset, c.radius, c.yScale);
        }
    }

    glDisable(GL_BLEND);
}

void drawStars() {
    if (getTimeMoment() != NIGHT) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (int i = 0; i < STAR_COUNT; ++i) {
        glColor4f(1.0f, 1.0f, 0.9f, stars[i].alpha);
        drawCircle(stars[i].x, stars[i].y, stars[i].radius);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}


void drawSunAndMoon() {
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);

    if (dayNightPhase >= 0.0f && dayNightPhase < 0.5f) {

        float sunPhase = dayNightPhase * 2.0f;
        float sunX = -2.8f + (sunPhase * 5.6f);
        float sunY = -0.2f + 1.5f * sinf(sunPhase * PI);

        if (sunY > -0.1f) {
            float core_r = 1.0f, core_g = 0.85f, core_b = 0.5f;
            float glow_r = 1.0f, glow_g = 0.9f,  glow_b = 0.7f;

            glEnable(GL_BLEND);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            glPushMatrix();
            glTranslatef(sunX, sunY, 0.0f);

            // ---  Outer glow layer ---
            glColor4f(glow_r, glow_g, glow_b, 0.12f);
            drawCircle(0.0f, 0.0f, 0.22f);

            // ---  Middle glow layer ---
            glColor4f(glow_r, glow_g, glow_b, 0.16f);
            drawCircle(0.0f, 0.0f, 0.17f);

            // ---  Sun rays ---
            glPushMatrix();
            glRotatef(crystalGlow * 10.0f, 0, 0, 1);

            int num_rays = 8;
            glColor4f(glow_r, glow_g, glow_b, 0.20f);
            glBegin(GL_TRIANGLES);
            for (int i = 0; i < num_rays; ++i) {
                float angle = (i / (float)num_rays) * 2.0f * PI;
                float rayLength = 0.22f + 0.03f * sinf(crystalGlow * 0.8f + i);
                float baseWidth = 0.04f;

                glVertex2f(0.0f, 0.0f);
                glVertex2f(cosf(angle - baseWidth) * rayLength, sinf(angle - baseWidth) * rayLength);
                glVertex2f(cosf(angle + baseWidth) * rayLength, sinf(angle + baseWidth) * rayLength);
            }
            glEnd();
            glPopMatrix();


            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(core_r, core_g, core_b, 1.0f);
            drawCircle(0.0f, 0.0f, 0.11f);

            glPopMatrix();

            glDisable(GL_BLEND);
        }
    } else {

        drawMoon();
    }

    glPopAttrib();
}



void drawHouse(float x, float y, float scale) {

    // Roof
    setSceneElementColor(0.5f, 0.3f, 0.15f);
    glBegin(GL_TRIANGLES);
        glVertex2f(x - 0.12f * scale, y + 0.1f * scale);
        glVertex2f(x + 0.12f * scale, y + 0.1f * scale);
        glVertex2f(x, y + 0.2f * scale);
    glEnd();

    // Base
    setSceneElementColor(0.7f, 0.5f, 0.3f);
    glRectf(x - 0.1f * scale, y - 0.1f * scale, x + 0.1f * scale, y + 0.1f * scale);

    // Door
    setSceneElementColor(0.4f, 0.25f, 0.1f);
    glRectf(x - 0.04f * scale, y - 0.1f * scale, x + 0.04f * scale, y - 0.02f * scale);

    // Window
    if (getTimeMoment() == NIGHT) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.85f, 0.5f);
        drawCircle(x, y + 0.04f * scale, 0.03f * scale);
        glPopAttrib();
    } else {
        glColor3f(0.2f, 0.2f, 0.3f);
        drawCircle(x, y + 0.04f * scale, 0.03f * scale);
    }


    if (currentWeather == SNOWY) {
        glColor4f(0.95f, 0.95f, 1.0f, 1.0f);


        glBegin(GL_TRIANGLES);
            glVertex2f(x - 0.125f * scale, y + 0.1f * scale);
            glVertex2f(x + 0.125f * scale, y + 0.1f * scale);
            glVertex2f(x, y + 0.21f * scale);
        glEnd();
    }
}



void drawHangingLantern(float x, float y) {
    // Chain
    setSceneElementColor(0.2f, 0.15f, 0.1f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex2f(x, y + 0.1f);
        glVertex2f(x, y);
    glEnd();
    glLineWidth(1.0f);

    // Lantern Casing
    setSceneElementColor(0.5f, 0.4f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.02f, y);
        glVertex2f(x + 0.02f, y);
        glVertex2f(x + 0.02f, y - 0.01f);
        glVertex2f(x - 0.02f, y - 0.01f);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(x - 0.02f, y - 0.05f);
        glVertex2f(x + 0.02f, y - 0.05f);
        glVertex2f(x + 0.02f, y - 0.06f);
        glVertex2f(x - 0.02f, y - 0.06f);
    glEnd();

    // Light
    if (getTimeMoment() == NIGHT) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        // Outer Glow
        glColor4f(1.0f, 0.9f, 0.5f, 0.2f + 0.05f * sinf(crystalGlow * 2.0f));
        drawCircle(x, y - 0.03f, 0.06f);
        // Inner Core
        glColor4f(1.0f, 1.0f, 0.8f, 1.0f);
        drawCircle(x, y - 0.03f, 0.015f);
        glPopAttrib();
    } else {
        setSceneElementColor(1.0f, 1.0f, 0.8f);
        drawCircle(x, y - 0.03f, 0.015f);
    }
}


void drawHangingMoss(float x, float y, float scale) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Main Vine  ---
    setSceneElementColor(0.2f, 0.4f, 0.1f, 0.9f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 15; ++i) {
        float t = i / 15.0f;

        float centerX = x + 0.02f * sinf(t * 7.0f + dayNightPhase * 10.0f);
        float centerY = y - t * 0.25f * scale;


        float width = 0.01f * (1.0f - t * 0.7f);

        glVertex2f(centerX - width, centerY);
        glVertex2f(centerX + width, centerY);
    }
    glEnd();

    // --- Small Leaves attached to the vine ---
    setSceneElementColor(0.3f, 0.6f, 0.3f, 0.9f);
    for (int i = 2; i <= 10; i += 2) {
        float t = i / 15.0f;


        float centerX = x + 0.02f * sinf(t * 7.0f + dayNightPhase * 10.0f);
        float centerY = y - t * 0.25f * scale;

        glPushMatrix();
        glTranslatef(centerX, centerY, 0.0f);

        glRotatef(sinf(t * 10.0f) * 20.0f, 0, 0, 1);


        glBegin(GL_TRIANGLES);
            glVertex2f(0.0f, 0.0f);
            glVertex2f(-0.02f, -0.01f);
            glVertex2f(-0.01f, -0.02f);

            glVertex2f(0.0f, 0.0f);
            glVertex2f(0.02f, -0.01f);
            glVertex2f(0.01f, -0.02f);
        glEnd();

        glPopMatrix();
    }

    glDisable(GL_BLEND);
}

void drawFoxPath() {
    setSceneElementColor(0.4f, 0.3f, 0.2f);


    float path_y = -1.0f;
    float path_width = 0.07f;

    glBegin(GL_QUADS);
        glVertex2f(-3.5f, path_y - path_width);
        glVertex2f( 3.5f, path_y - path_width);
        glVertex2f( 3.5f, path_y + path_width);
        glVertex2f(-3.5f, path_y + path_width);
    glEnd();
}

void drawHills() {

    setSceneElementColor(0.3f, 0.4f, 0.45f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-3.5f, -0.3f); glVertex2f(-2.7f, 0.8f); glVertex2f(-1.9f, -0.3f);
        glVertex2f(-2.2f, -0.3f); glVertex2f(-1.5f, 1.0f); glVertex2f(-0.8f, -0.3f);
        glVertex2f(-1.1f, -0.3f); glVertex2f(-0.3f, 0.8f); glVertex2f(0.5f, -0.3f);
        glVertex2f(0.3f, -0.3f); glVertex2f(1.1f, 1.1f); glVertex2f(1.9f, -0.3f);
        glVertex2f(1.7f, -0.3f); glVertex2f(2.5f, 0.9f); glVertex2f(3.3f, -0.3f);
        glVertex2f(2.9f, -0.3f); glVertex2f(3.4f, 0.7f); glVertex2f(4.0f, -0.3f);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(-3.5f, -0.3f); glVertex2f(4.0f, -0.3f); glVertex2f(4.0f, -1.5f); glVertex2f(-3.5f, -1.5f);
    glEnd();


    setSceneElementColor(0.2f, 0.3f, 0.35f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-4.2f, -0.5f); glVertex2f(-3.3f, 0.6f); glVertex2f(-2.4f, -0.5f);
        glVertex2f(-2.7f, -0.5f); glVertex2f(-2.0f, 0.8f); glVertex2f(-1.3f, -0.5f);
        glVertex2f(-1.6f, -0.5f); glVertex2f(-0.8f, 0.6f); glVertex2f(0.0f, -0.5f);
        glVertex2f(-0.2f, -0.5f); glVertex2f(0.7f, 0.9f); glVertex2f(1.6f, -0.5f);
        glVertex2f(1.4f, -0.5f); glVertex2f(2.2f, 0.7f); glVertex2f(3.0f, -0.5f);
        glVertex2f(0.7f, -0.5f); glVertex2f(1.5f, 0.8f); glVertex2f(2.3f, -0.5f);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(-4.2f, -0.5f); glVertex2f(3.6f, -0.5f); glVertex2f(3.6f, -1.5f); glVertex2f(-4.2f, -1.5f);
    glEnd();

}

void drawSnowOnHills() {

    setSceneElementColor(1.0f, 1.0f, 1.0f);


    glBegin(GL_TRIANGLES);


    glVertex2f(-2.7f, 0.8f); glVertex2f(-2.8f, 0.7f); glVertex2f(-2.6f, 0.7f);
    glVertex2f(-1.5f, 1.0f); glVertex2f(-1.6f, 0.85f); glVertex2f(-1.4f, 0.85f);
    glVertex2f(-0.3f, 0.8f); glVertex2f(-0.4f, 0.7f); glVertex2f(-0.2f, 0.7f);
    glVertex2f(1.1f, 1.1f); glVertex2f(1.0f, 0.95f); glVertex2f(1.2f, 0.95f);
    glVertex2f(2.5f, 0.9f); glVertex2f(2.4f, 0.8f); glVertex2f(2.6f, 0.8f);
    glVertex2f(3.4f, 0.7f); glVertex2f(3.3f, 0.65f); glVertex2f(3.5f, 0.65f);

    glVertex2f(-3.3f, 0.6f); glVertex2f(-3.37f, 0.52f); glVertex2f(-3.23f, 0.52f);

    glVertex2f(-2.0f, 0.8f); glVertex2f(-2.07f, 0.7f); glVertex2f(-1.93f, 0.7f);

    glVertex2f(-0.8f, 0.6f); glVertex2f(-0.87f, 0.52f); glVertex2f(-0.73f, 0.52f);

    glVertex2f(0.7f, 0.9f); glVertex2f(0.63f, 0.8f); glVertex2f(0.77f, 0.8f);

    glVertex2f(2.2f, 0.7f); glVertex2f(2.13f, 0.62f); glVertex2f(2.27f, 0.62f);

    glVertex2f(1.5f, 0.8f); glVertex2f(1.43f, 0.72f); glVertex2f(1.57f, 0.72f);

    glEnd();
}


void drawRainAndSplashes() {
    if (currentWeather != RAINY) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Draw Raindrops ---
    glLineWidth(1.5f);
    glColor4f(0.8f, 0.9f, 1.0f, 0.6f);
    glBegin(GL_LINES);
    for (int i = 0; i < RAIN_COUNT; ++i) {
        glVertex2f(raindrops[i].x, raindrops[i].y);
        glVertex2f(raindrops[i].x, raindrops[i].y - 0.05f);
    }
    glEnd();

    // --- Draw Expanding Rings (Puddles) ---

    glLineWidth(2.0f);
    for (const auto& splash : splashes) {
        float alpha = splash.life * 0.8f;
        glColor4f(0.9f, 1.0f, 1.0f, alpha);

        glBegin(GL_LINE_LOOP);
        for(int i=0; i<20; ++i) {
            float angle = i * 2.0f * PI / 20.0f;
            glVertex2f(splash.x + cosf(angle) * splash.radius, splash.y + sinf(angle) * splash.radius * 0.3f);
        }
        glEnd();
    }

    // --- Draw Vertical Splashes ---
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (const auto& droplet : droplets) {
        float alpha = droplet.life * 1.5f;
        if (alpha > 1.0f) alpha = 1.0f;
        glColor4f(0.9f, 1.0f, 1.0f, alpha);
        glVertex2f(droplet.x, droplet.y);
    }
    glEnd();

    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}


void drawSimpleTree(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    // Trunk
    setSceneElementColor(0.4f, 0.25f, 0.15f);
    glRectf(-0.02f, -0.1f, 0.02f, 0.0f);

    // Foliage in layers
    setSceneElementColor(0.1f, 0.4f, 0.15f);
    drawCircle(0.0f, 0.05f, 0.1f, 0.8f);
    setSceneElementColor(0.15f, 0.55f, 0.2f);
    drawCircle(0.0f, 0.08f, 0.08f, 0.9f);
    setSceneElementColor(0.2f, 0.7f, 0.25f);
    drawCircle(0.0f, 0.1f, 0.06f, 1.0f);


    if (currentWeather == SNOWY) {
        glColor4f(0.95f, 0.95f, 1.0f, 1.0f);


        drawCircle(0.0f, 0.12f, 0.07f, 1.0f);

        drawCircle(0.0f, 0.1f, 0.09f, 0.9f);

        drawCircle(0.0f, 0.07f, 0.11f, 0.8f);
    }

    glPopMatrix();
}


void drawUpdatedSimpleTree(float x, float y, float scale) {
    // --- 1. Foliage ---

    setSceneElementColor(0.1f, 0.45f, 0.15f);
    drawCircle(x, y + 0.4f * scale, 0.25f * scale, 1.0f); // Wider base

    // Middle layer of foliage
    setSceneElementColor(0.15f, 0.55f, 0.2f);
    drawCircle(x, y + 0.45f * scale, 0.23f * scale, 1.0f);

    // Top layer of foliage (brighter, more prominent)
    setSceneElementColor(0.2f, 0.65f, 0.25f);
    drawCircle(x, y + 0.5f * scale, 0.2f * scale, 1.0f);


    if (currentWeather == SNOWY) {
        glColor4f(0.95f, 0.95f, 1.0f, 1.0f);

        drawCircle(x, y + 0.5f * scale, 0.22f * scale, 1.0f);
        drawCircle(x, y + 0.47f * scale, 0.25f * scale, 1.0f);
        drawCircle(x, y + 0.42f * scale, 0.27f * scale, 1.0f);
    }

    // ---  Integrated Trunk/Main Branch  ---
    setSceneElementColor(0.4f, 0.25f, 0.1f);
    glBegin(GL_QUAD_STRIP);
        // Base of the "trunk-branch"
        glVertex2f(x - 0.025f * scale, y);
        glVertex2f(x + 0.025f * scale, y);

        // Mid-point, slightly tapering
        glVertex2f(x - 0.015f * scale, y + 0.2f * scale);
        glVertex2f(x + 0.015f * scale, y + 0.2f * scale);

        // Top-point, tapering further
        glVertex2f(x - 0.005f * scale, y + 0.45f * scale);
        glVertex2f(x + 0.005f * scale, y + 0.45f * scale);
    glEnd();

    // Side branches extending from the main central structure
    glLineWidth(4.5f * scale);
    glBegin(GL_LINES);
        // Left side branch
        glVertex2f(x - 0.01f * scale, y + 0.38f * scale);
        glVertex2f(x - 0.15f * scale, y + 0.48f * scale);

        // Right side branch
        glVertex2f(x + 0.01f * scale, y + 0.40f * scale);
        glVertex2f(x + 0.15f * scale, y + 0.50f * scale);
    glEnd();
    glLineWidth(1.0f);
}


void drawMushroom(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    // Stem
    setSceneElementColor(0.8f, 0.8f, 0.7f);
    glRectf(-0.01f, -0.05f, 0.01f, 0.0f);

    // Cap
    if (getTimeMoment() == NIGHT) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.6f, 0.9f, 1.0f, 0.8f);
        drawCircle(0.0f, 0.0f, 0.03f, 0.5f);
        glColor4f(0.8f, 1.0f, 1.0f, 0.3f);
        drawCircle(0.0f, 0.0f, 0.05f, 0.5f);
        glPopAttrib();
    } else {
        setSceneElementColor(0.9f, 0.2f, 0.2f);
        drawCircle(0.0f, 0.0f, 0.03f, 0.5f);
    }

    glPopMatrix();
}

// A hanging lantern that glows at night
void drawLantern(float x, float y) {
    // Post
    setSceneElementColor(0.3f, 0.2f, 0.1f);
    glRectf(x - 0.01f, y - 0.2f, x + 0.01f, y);
    // Arm
    glRectf(x, y, x + 0.05f, y - 0.02f);

    // Lantern
    setSceneElementColor(0.6f, 0.5f, 0.2f);
    glRectf(x + 0.04f, y - 0.02f, x + 0.06f, y - 0.08f);

    // Light
    if (getTimeMoment() == NIGHT) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.9f, 0.5f, 1.0f);
        drawCircle(x + 0.05f, y - 0.05f, 0.02f);
        glColor4f(1.0f, 0.9f, 0.5f, 0.3f);
        drawCircle(x + 0.05f, y - 0.05f, 0.08f);
        glPopAttrib();
    } else {
        glColor3f(1.0f, 1.0f, 0.8f);
        drawCircle(x + 0.05f, y - 0.05f, 0.02f);
    }
}

// A simple wooden fence section
void drawFence(float x, float y, int sections) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    setSceneElementColor(0.4f, 0.25f, 0.15f);
    for (int i = 0; i <= sections; ++i) {
        // Post
        glRectf(i * 0.1f - 0.01f, -0.05f, i * 0.1f + 0.01f, 0.05f);
    }
    // Rails
    glRectf(0, 0.02f, sections * 0.1f, 0.04f);
    glRectf(0, -0.01f, sections * 0.1f, 0.01f);
    glPopMatrix();
}


void drawVerticalFencePost(float x, float y, float height, float width) {
    setSceneElementColor(0.55f, 0.35f, 0.15f);
    glRectf(x - width / 2, y - height / 2, x + width / 2, y + height / 2);
}


void drawHorizontalFenceRail(float x1, float y1, float x2, float y2, float thickness) {
    setSceneElementColor(0.55f, 0.35f, 0.15f);
    glRectf(x1, y1 - thickness / 2, x2, y2 + thickness / 2);
}


void drawVegetableField(float x, float y, float width, float height) {
    glPushMatrix();
    glTranslatef(x, y, 0.f);

    //  Draw the brown earth patch
    setSceneElementColor(0.4f, 0.25f, 0.1f);
    glRectf(-width / 2, -height / 2, width / 2, height / 2);

    //  Draw the vegetables in neat rows ON the patch
    int numRows = 4;
    int plantsPerRow = 10;
    float rowSpacing = height / numRows;
    float plantSpacing = width / plantsPerRow;

    for (int i = 0; i < numRows; ++i) {
        float rowY = -height / 2 + rowSpacing * (i + 0.5f);
        for (int j = 0; j < plantsPerRow; ++j) {
            float plantX = -width / 2 + plantSpacing * (j + 0.5f);

            if ((i + j) % 2 == 0) { // Carrot
                setSceneElementColor(0.2f, 0.6f, 0.2f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(plantX, rowY + 0.02f);
                    glVertex2f(plantX - 0.01f, rowY);
                    glVertex2f(plantX + 0.01f, rowY);
                glEnd();
                setSceneElementColor(0.9f, 0.5f, 0.1f);
                glBegin(GL_TRIANGLES);
                    glVertex2f(plantX, rowY);
                    glVertex2f(plantX - 0.005f, rowY - 0.01f);
                    glVertex2f(plantX + 0.005f, rowY - 0.01f);
                glEnd();
            } else { // Cabbage
                setSceneElementColor(0.4f, 0.7f, 0.4f);
                drawCircle(plantX, rowY, 0.015f);
            }
        }
    }

    //  Draw Fences on the back and sides
    // Back Fence (Horizontal)
    drawFence(-width / 2, height / 2, (int)(width / 0.1f));

    // --- Side Fences with Corrected Height ---
    setSceneElementColor(0.4f, 0.25f, 0.15f); // Fence wood color
    float postWidth = 0.01f;


    // Left Side Fence
    glRectf(-width/2 - postWidth, -height/2, -width/2 + postWidth, height/2);
    // Right Side Fence
    glRectf(width/2 - postWidth, -height/2, width/2 + postWidth, height/2);


     // ---  Snow to the field in winter ---
    if (currentWeather == SNOWY) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.95f, 0.95f, 1.0f, 0.85f); // Snow color

        //  Snow on the dirt patch
        glRectf(-width / 2, -height / 2, width / 2, height / 2);

        //  Snow on the vegetables
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Opaque white for plant tops
        for (int i = 0; i < numRows; ++i) {
            float rowY = -height / 2 + rowSpacing * (i + 0.5f);
            for (int j = 0; j < plantsPerRow; ++j) {
                float plantX = -width / 2 + plantSpacing * (j + 0.5f);
                // A small mound of snow on each plant
                drawCircle(plantX, rowY + 0.01f, 0.018f, 0.7f);
            }
        }
        glDisable(GL_BLEND);
    }

    glPopMatrix();

}


void drawArcheryTarget(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    // Stand
    setSceneElementColor(0.4f, 0.25f, 0.1f);
    glRectf(-0.02f, -0.1f, 0.02f, 0.15f); // Vertical post
    glRectf(-0.05f, -0.1f, 0.05f, -0.08f); // Base

    // Target
    setSceneElementColor(1.0f, 1.0f, 0.8f); drawCircle(0.0f, 0.0f, 0.1f);
    setSceneElementColor(0.0f, 0.0f, 0.0f); drawCircle(0.0f, 0.0f, 0.08f);
    setSceneElementColor(0.0f, 0.5f, 1.0f); drawCircle(0.0f, 0.0f, 0.06f);
    setSceneElementColor(1.0f, 0.0f, 0.0f); drawCircle(0.0f, 0.0f, 0.04f);
    setSceneElementColor(1.0f, 0.9f, 0.0f); drawCircle(0.0f, 0.0f, 0.02f);
    glPopMatrix();
}

void drawPracticeDummy(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    setSceneElementColor(0.5f, 0.35f, 0.15f); // Woody brown
    drawCircle(0.0f, 0.12f, 0.05f); // Head
    glRectf(-0.06f, -0.15f, 0.06f, 0.1f); // Body
    glRectf(-0.1f, 0.0f, 0.1f, 0.04f); // Arms
    glPopMatrix();
}

void drawArrowQuiver(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    // Quiver Barrel
    setSceneElementColor(0.6f, 0.4f, 0.2f);
    glRectf(-0.05f, -0.08f, 0.05f, 0.05f);
    // Arrows
    setSceneElementColor(0.8f, 0.6f, 0.4f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        glVertex2f(-0.02f, 0.05f); glVertex2f(-0.02f, 0.12f);
        glVertex2f(0.00f, 0.05f);  glVertex2f(0.00f, 0.13f);
        glVertex2f(0.02f, 0.05f);  glVertex2f(0.02f, 0.11f);
    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();
}

void drawArcheryTrainingGround(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);

    // ---  Training Field Ground (Shrunk horizontally) ---
    float fieldWidth = 0.45f;
    setSceneElementColor(0.5f, 0.4f, 0.25f);
    glRectf(-fieldWidth, 0.0f, fieldWidth, -0.25f);

    // ---  Fences (Adjusted to new size) ---
    // Back Fence
    drawFence(-fieldWidth, 0.05f, (int)(fieldWidth * 2 / 0.1f));

    // Left Side Fence
    glPushMatrix();
    glTranslatef(-fieldWidth, 0.0f, 0.0f);
    setSceneElementColor(0.4f, 0.25f, 0.15f);
    glRectf(-0.01f, -0.25f, 0.01f, 0.05f);
    glRectf(-0.01f, -0.2f, 0.01f, -0.18f);
    glRectf(-0.01f, -0.1f, 0.01f, -0.08f);
    glPopMatrix();

    // Right Side Fence
    glPushMatrix();
    glTranslatef(fieldWidth, 0.0f, 0.0f);
    setSceneElementColor(0.4f, 0.25f, 0.15f);
    glRectf(-0.01f, -0.25f, 0.01f, 0.05f);
    glRectf(-0.01f, -0.2f, 0.01f, -0.18f);
    glRectf(-0.01f, -0.1f, 0.01f, -0.08f);
    glPopMatrix();

    float itemScale = 0.4f;

    auto drawSingleTarget = [&](float targetX, float targetY, float size) {
        glPushMatrix();
        glTranslatef(targetX, targetY, 0.0f);
        glScalef(size, size, 1.0f);
        setSceneElementColor(0.4f, 0.25f, 0.1f);
        glRectf(-0.02f, -0.1f, 0.02f, 0.15f);
        glRectf(-0.05f, -0.1f, 0.05f, -0.08f);
        setSceneElementColor(1.0f, 1.0f, 0.8f); drawCircle(0.0f, 0.0f, 0.1f);
        setSceneElementColor(0.0f, 0.0f, 0.0f); drawCircle(0.0f, 0.0f, 0.08f);
        setSceneElementColor(0.0f, 0.5f, 1.0f); drawCircle(0.0f, 0.0f, 0.06f);
        setSceneElementColor(1.0f, 0.0f, 0.0f); drawCircle(0.0f, 0.0f, 0.04f);
        setSceneElementColor(1.0f, 0.9f, 0.0f); drawCircle(0.0f, 0.0f, 0.02f);
        glPopMatrix();
    };

    auto drawSingleQuiver = [&](float quiverX, float quiverY, float size) {
        glPushMatrix();
        glTranslatef(quiverX, quiverY, 0.0f);
        glScalef(size, size, 1.0f);
        setSceneElementColor(0.6f, 0.4f, 0.2f);
        glBegin(GL_POLYGON);
            glVertex2f(-0.06f, -0.12f); glVertex2f(0.06f, -0.12f);
            glVertex2f(0.04f, 0.08f); glVertex2f(-0.04f, 0.08f);
        glEnd();
        setSceneElementColor(0.8f, 0.6f, 0.4f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
            glVertex2f(-0.02f, 0.08f); glVertex2f(-0.02f, 0.15f);
            glVertex2f(0.00f, 0.08f); glVertex2f(0.00f, 0.16f);
            glVertex2f(0.02f, 0.08f); glVertex2f(0.02f, 0.14f);
        glEnd();
        glLineWidth(1.0f);
        glPopMatrix();
    };

    auto drawSingleDummy = [&](float dummyX, float dummyY, float size) {
        glPushMatrix();
        glTranslatef(dummyX, dummyY, 0.0f);
        glScalef(size, size, 1.0f);
        setSceneElementColor(0.5f, 0.35f, 0.15f);
        drawCircle(0.0f, 0.12f, 0.05f);
        glRectf(-0.06f, -0.15f, 0.06f, 0.1f);
        glRectf(-0.1f, 0.0f, 0.1f, 0.04f);
        glPopMatrix();
    };


    float baseGroundY = -0.18f;

    drawSingleTarget(-0.28f, baseGroundY + 0.15f, itemScale * 0.9f);
    drawSingleTarget(-0.1f, baseGroundY + 0.15f, itemScale);
    drawSingleTarget(0.1f, baseGroundY + 0.15f, itemScale);
    drawSingleTarget(0.28f, baseGroundY + 0.15f, itemScale * 0.9f);

    drawSingleDummy(-0.18f, baseGroundY + 0.1f, itemScale);
    drawSingleDummy(0.18f, baseGroundY + 0.1f, itemScale);

    drawSingleQuiver(-0.28f, baseGroundY, itemScale);
    drawSingleQuiver(-0.1f, baseGroundY, itemScale * 0.9f);
    drawSingleQuiver(0.1f, baseGroundY, itemScale * 0.9f);
    drawSingleQuiver(0.28f, baseGroundY, itemScale);

     // ---   Snow during winter ---
    if (currentWeather == SNOWY) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.95f, 0.95f, 1.0f, 0.9f); // Snow color


        glRectf(-0.45f, 0.0f, 0.45f, -0.25f);


        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        // Snow on targets
        drawCircle(-0.3f, baseGroundY + 0.05f * itemScale, 0.1f * itemScale);
        drawCircle(0.3f, baseGroundY + 0.05f * itemScale * 0.9f, 0.1f * itemScale * 0.9f);
        // Snow on dummies
        drawCircle(-0.1f, baseGroundY + 0.1f + 0.15f * itemScale * 0.9f, 0.05f * itemScale * 0.9f); // Head
        glRectf(-0.1f - 0.1f * itemScale * 0.9f, baseGroundY + 0.1f + 0.03f * itemScale * 0.9f, -0.1f + 0.1f * itemScale * 0.9f, baseGroundY + 0.1f + 0.05f * itemScale * 0.9f); // Shoulders
        drawCircle(0.1f, baseGroundY + 0.1f + 0.15f * itemScale, 0.05f * itemScale); // Head
        glRectf(0.1f - 0.1f * itemScale, baseGroundY + 0.1f + 0.03f * itemScale, 0.1f + 0.1f * itemScale, baseGroundY + 0.1f + 0.05f * itemScale); // Shoulders
        // Snow on quivers
        glRectf(-0.2f - 0.05f * itemScale, baseGroundY + 0.15f + 0.05f, -0.2f + 0.05f * itemScale, baseGroundY + 0.15f + 0.06f);
        glRectf(0.2f - 0.05f * itemScale, baseGroundY + 0.15f + 0.05f, 0.2f + 0.05f * itemScale, baseGroundY + 0.15f + 0.06f);


        glDisable(GL_BLEND);
    }


    glPopMatrix();
}


void drawVillageDetails() {

    drawMushroom(-0.6f, -0.7f, 1.0f);
    drawMushroom(0.78f, -0.68f, 0.8f);
    drawMushroom(0.84f, -0.69f, 0.7f);
    drawMushroom(-2.41f, -0.3f, 0.7f);
    drawMushroom(-2.43f, -0.22f, 0.7f);
    drawMushroom(2.3f, -0.22f, 0.7f);
    drawMushroom(2.4f, -0.22f, 0.7f);
    drawMushroom(2.35f, -0.18f, 0.7f);
    drawMushroom(0.45f, -0.45f, 0.7f);
    drawMushroom(0.37f, -0.42f, 0.7f);
    drawMushroom(0.43f, -0.39f, 0.7f);
    drawMushroom(-0.68f, -0.7f, 0.9f);
    drawMushroom(-0.7f, -0.45f, 0.75f);
    drawMushroom(-0.77f, -0.45f, 0.75f);
    drawMushroom(1.5f, -0.45f, 0.7f);
    drawMushroom(1.56f, -0.42f, 0.7f);
    drawMushroom(1.6f, -0.47f, 0.7f);


    drawSnowCover();

    drawVegetableField(-1.8f, -0.31f, 0.9f, 0.2f);
    drawVegetableField(-0.8f, -0.31f, 0.9f, 0.2f);

    drawUpdatedSimpleTree(-2.0f, -0.14f, 0.53f);
    drawUpdatedSimpleTree(-2.3f, -0.14f, 0.6f);
    drawUpdatedSimpleTree(-1.7f, -0.14f, 0.55f);
    drawUpdatedSimpleTree(-1.4f, -0.14f, 0.52f);
    drawUpdatedSimpleTree(-1.1f, -0.14f, 0.55f);
    drawUpdatedSimpleTree(-0.78f, -0.14f, 0.6f);
    drawUpdatedSimpleTree(1.7f, -0.13f, 0.56f);
    drawUpdatedSimpleTree(0.8f, -0.12f, 0.55f);
    drawUpdatedSimpleTree(1.1f, -0.12f, 0.56f);
    drawUpdatedSimpleTree(1.4f, -0.12f, 0.57f);
    drawUpdatedSimpleTree(2.0f, -0.13f, 0.56f);
    drawUpdatedSimpleTree(2.3f, -0.13f, 0.55f);
    drawUpdatedSimpleTree(2.6f, -0.13f, 0.56f);

    drawArcheryTrainingGround(1.0f, -0.13f);
    drawPuddles();
    drawWishingWell(1.8f, -0.2f);


    drawSimpleTree(-2.2f, -0.5f, 1.5f);
    drawSimpleTree(1.25f, -0.5f, 1.7f);
    drawSimpleTree(2.2f, -0.48f, 1.5f);




    // --- Draw lanterns ---
    drawLantern(1.75f, -0.55f);
    drawLantern(0.7f, -0.55f);

    drawLantern(-2.3f, -0.55f);
    drawLantern(-1.3f, -0.55f);

    // --- Draw a fence section ---
    drawFence(1.2f, -0.7f, 5);

}

void drawGroundPatches() {
    float ground_upper_y = -0.1f;

    setSceneElementColor(0.2f, 0.6f, 0.25f);
    glRectf(-2.5f, ground_upper_y, 2.5f, -1.5f);

    setSceneElementColor(0.3f, 0.75f, 0.3f);
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(0.0f, -1.5f);
      glVertex2f(-2.5f, ground_upper_y - 0.2f);
      glVertex2f(-1.5f, ground_upper_y - 0.1f);
      glVertex2f(-0.8f, ground_upper_y - 0.05f);
      glVertex2f(0.0f, ground_upper_y - 0.1f);
      glVertex2f(0.8f, ground_upper_y - 0.0f);
      glVertex2f(1.5f, ground_upper_y - 0.05f);
      glVertex2f(2.5f, ground_upper_y - 0.15f);
      glVertex2f(2.5f, -1.5f);
    glEnd();
}

void drawForegroundRiver() {
    float river_top_y = -1.1f;
    float river_bottom_y = -1.5f;

    // ---  Define Water and Ice Colors ---
    float water_r = 0.05f, water_g = 0.3f, water_b = 0.6f;
    float ice_r = 0.9f, ice_g = 0.95f, ice_b = 1.0f;


    float r = water_r + (ice_r - water_r) * riverFreezeAmount;
    float g = water_g + (ice_g - water_g) * riverFreezeAmount;
    float b = water_b + (ice_b - water_b) * riverFreezeAmount;

    // ---  Draw the Main River Body ---
    setSceneElementColor(r, g, b);
    glRectf(-2.5f, river_top_y, 2.5f, river_bottom_y);

    // ---  Draw Animated Waves (which fade when frozen) ---
    float waveAlpha = 0.6f * (1.0f - riverFreezeAmount);

    if (waveAlpha > 0.01f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(2.0f);
        glColor4f(0.6f, 0.85f, 1.0f, waveAlpha);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 100; ++i) {
            float x = -2.5f + (i / 100.0f) * 5.0f;
            float waveY = river_top_y + 0.02f * sinf(x * 3.0f + riverFlowOffset * 1.5f) + 0.01f * cosf(x * 1.5f + riverFlowOffset);
            glVertex2f(x, waveY + 0.005f);
        }
        glEnd();
        glLineWidth(1.0f);
        glDisable(GL_BLEND);
    }
}

void drawTreeHouse(float x, float y) {
    setSceneElementColor(0.6f, 0.4f, 0.25f); glRectf(x - 0.08f, y - 0.06f, x + 0.08f, y + 0.06f);
    setSceneElementColor(0.5f, 0.3f, 0.15f);
    glBegin(GL_TRIANGLES); glVertex2f(x - 0.1f, y + 0.06f); glVertex2f(x + 0.1f, y + 0.06f); glVertex2f(x, y + 0.14f); glEnd();
    if (getTimeMoment() == NIGHT) {
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.85f, 0.5f);
        drawCircle(x, y + 0.01f, 0.03f);
        glPopAttrib();
    } else {
        glColor3f(0.2f, 0.2f, 0.3f);
        drawCircle(x, y + 0.01f, 0.03f);
    }
    setSceneElementColor(0.4f, 0.3f, 0.2f);
    glLineWidth(2.0f); glBegin(GL_LINES);
    glVertex2f(x - 0.03f, y - 0.06f); glVertex2f(x - 0.03f, y - 0.3f); glVertex2f(x + 0.03f, y - 0.06f); glVertex2f(x + 0.03f, y - 0.3f);
    for(float i = 0; i < 8; ++i) { float rY = y - 0.09f - i * 0.03f; glVertex2f(x - 0.03f, rY); glVertex2f(x + 0.03f, rY); }
    glEnd(); glLineWidth(1.0f);
}



void drawGreatTree() {
    float y_offset = -0.1f;

    // --- 1. Trunk and Main Branches ---
    setSceneElementColor(0.45f, 0.3f, 0.15f);
    glBegin(GL_POLYGON);
        glVertex2f(-0.18f, -0.6f + y_offset);
        glVertex2f(-0.28f, -0.45f + y_offset);
        glVertex2f(-0.12f, 0.0f + y_offset);
        glVertex2f(-0.08f, 0.4f + y_offset);
        glVertex2f(0.0f, 0.5f + y_offset);
        glVertex2f(0.08f, 0.4f + y_offset);
        glVertex2f(0.12f, 0.0f + y_offset);
        glVertex2f(0.28f, -0.45f + y_offset);
        glVertex2f(0.18f, -0.6f + y_offset);
    glEnd();
    setSceneElementColor(0.55f, 0.4f, 0.25f);
    glBegin(GL_QUADS);
        glVertex2f(-0.05f, -0.55f + y_offset); glVertex2f(0.05f, -0.55f + y_offset);
        glVertex2f(0.05f, 0.45f + y_offset); glVertex2f(-0.05f, 0.45f + y_offset);
    glEnd();
    setSceneElementColor(0.35f, 0.2f, 0.1f);
    glLineWidth(8.0f);
    glBegin(GL_LINES);
        glVertex2f(0.0f, 0.5f + y_offset); glVertex2f(-0.25f, 0.7f + y_offset);
        glVertex2f(0.0f, 0.5f + y_offset); glVertex2f(0.25f, 0.7f + y_offset);
        glVertex2f(-0.1f, 0.3f + y_offset); glVertex2f(-0.4f, 0.45f + y_offset);
        glVertex2f(0.1f, 0.3f + y_offset); glVertex2f(0.4f, 0.45f + y_offset);
    glEnd();
    glLineWidth(1.0f);

    // --- 2. Foliage ---
    setSceneElementColor(0.05f, 0.3f, 0.1f);
    drawCircle(0.0f, 0.3f + y_offset, 0.7f, 0.5f);
    drawCircle(-0.5f, 0.4f + y_offset, 0.3f, 0.7f);
    drawCircle(0.5f, 0.4f + y_offset, 0.3f, 0.7f);
    drawCircle(-0.2f, -0.05f + y_offset, 0.4f, 0.4f);
    drawCircle(0.2f, -0.05f + y_offset, 0.4f, 0.4f);
    drawCircle(0.0f, 0.7f + y_offset, 0.35f, 0.8f);
    drawCircle(-0.65f, 0.4f + y_offset, 0.3f, 0.7f);
    drawCircle(0.65f, 0.4f + y_offset, 0.3f, 0.7f);

    setSceneElementColor(0.1f, 0.5f, 0.2f);
    drawCircle(0.0f, 0.15f + y_offset, 0.65f, 0.6f);
    drawCircle(-0.45f, 0.25f + y_offset, 0.35f, 0.8f);
    drawCircle(0.45f, 0.25f + y_offset, 0.35f, 0.8f);
    drawCircle(-0.25f, 0.6f + y_offset, 0.3f, 0.9f);
    drawCircle(0.25f, 0.6f + y_offset, 0.3f, 0.9f);
    drawCircle(0.0f, 0.8f + y_offset, 0.4f, 0.9f);

    setSceneElementColor(0.2f, 0.7f, 0.3f);
    drawCircle(0.0f, 0.9f + y_offset, 0.2f, 0.9f);
    drawCircle(-0.25f, 0.78f + y_offset, 0.2f, 0.8f);
    drawCircle(0.25f, 0.78f + y_offset, 0.2f, 0.8f);
    drawCircle(-0.5f, 0.55f + y_offset, 0.22f, 0.7f);
    drawCircle(0.5f, 0.55f + y_offset, 0.22f, 0.7f);
    drawCircle(-0.1f, 0.3f + y_offset, 0.25f, 0.6f);
    drawCircle(0.1f, 0.3f + y_offset, 0.25f, 0.6f);

    // --- 3. Minor Branches & Details ---
    setSceneElementColor(0.3f, 0.15f, 0.05f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
        glVertex2f(-0.2f, 0.5f + y_offset); glVertex2f(-0.45f, 0.65f + y_offset);
        glVertex2f(-0.3f, 0.1f + y_offset); glVertex2f(-0.5f, 0.0f + y_offset);
        glVertex2f(0.2f, 0.5f + y_offset); glVertex2f(0.45f, 0.65f + y_offset);
        glVertex2f(0.3f, 0.1f + y_offset); glVertex2f(0.5f, 0.0f + y_offset);
    glEnd();
    glLineWidth(1.0f);

    // --- Hanging Moss and Lanterns ---
    drawHangingMoss(-0.5f, 0.2f + y_offset, 1.0f);
    drawHangingMoss(0.1f, 0.5f + y_offset, 0.8f);
    drawHangingMoss(0.45f, 0.25f + y_offset, 1.2f);
    drawHangingMoss(-0.0f, -0.2f + y_offset, 0.8f);
    drawHangingMoss(-0.3f, 0.7f + y_offset, 0.9f);
    drawHangingMoss(0.3f, 0.7f + y_offset, 0.9f);

    drawHangingLantern(-0.5f, 0.4f + y_offset);
    drawHangingLantern(0.5f, 0.4f + y_offset);
    drawHangingLantern(-0.3f, 0.0f + y_offset);
    drawHangingLantern(0.3f, 0.0f + y_offset);

    // --- Tree Houses ---
    drawTreeHouse(-0.4f, 0.45f + y_offset);
    drawTreeHouse(0.4f, 0.45f + y_offset);
    drawTreeHouse(-0.2f, 0.05f + y_offset);
    drawTreeHouse(0.2f, 0.05f + y_offset);

    // --- Snow to the Great Tree in Winter ---
    if (currentWeather == SNOWY) {
        glColor4f(0.95f, 0.95f, 1.0f, 1.0f);

        // Snow on foliage
        drawCircle(0.0f, 0.95f + y_offset, 0.25f, 0.9f); // Topmost
        drawCircle(-0.25f, 0.82f + y_offset, 0.2f, 0.8f);
        drawCircle(0.25f, 0.82f + y_offset, 0.2f, 0.8f);
        drawCircle(0.0f, 0.85f + y_offset, 0.4f, 0.9f);
        drawCircle(-0.5f, 0.6f + y_offset, 0.22f, 0.7f);
        drawCircle(0.5f, 0.6f + y_offset, 0.22f, 0.7f);
        drawCircle(0.0f, 0.5f + y_offset, 0.7f, 0.5f);
        drawCircle(-0.45f, 0.3f + y_offset, 0.35f, 0.8f);
        drawCircle(0.45f, 0.3f + y_offset, 0.35f, 0.8f);
    }
}

void drawPuddles() {
    if (puddles.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& p : puddles) {
        // Water and Frozen colors
        float water_r = 0.15f, water_g = 0.3f, water_b = 0.5f;
        float frozen_r = 0.8f, frozen_g = 0.9f, frozen_b = 1.0f;

        // Interpolate color based on how frozen it is
        float r = water_r + (frozen_r - water_r) * p.freezeProgress;
        float g = water_g + (frozen_g - water_g) * p.freezeProgress;
        float b = water_b + (frozen_b - water_b) * p.freezeProgress;

        // Frozen puddles are less transparent
        float alpha = 0.6f + 0.2f * p.freezeProgress;

        glColor4f(r, g, b, alpha);
        drawCircle(p.x, p.y, p.currentRadius, 0.4f);

        //  a frosty edge when it's freezing/frozen
        if (p.freezeProgress > 0.1f) {
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f * p.freezeProgress);
            glBegin(GL_LINE_LOOP);
            for(int i=0; i<30; ++i) {
                float angle = i * 2.0f * PI / 30.0f;
                glVertex2f(p.x + cosf(angle) * p.currentRadius, p.y + sinf(angle) * p.currentRadius * 0.4f);
            }
            glEnd();
        }
    }

    glDisable(GL_BLEND);
}

void drawSnow() {
    if (currentWeather != SNOWY) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Snowflakes are white and semi-transparent
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

    for (int i = 0; i < SNOW_COUNT; ++i) {
        drawCircle(snowflakes[i].x, snowflakes[i].y, snowflakes[i].size);
    }

    glDisable(GL_BLEND);
}

void drawCrystal(float x, float y) {
    // --- Magical Glow and Effects (ONLY AT NIGHT) ---
    if (getTimeMoment() == NIGHT) {
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for glows

        //  The Volumetric Glow and Aura
        float auraPulse = 0.6f + 0.4f * sinf(crystalGlow * 0.7f);
        float aura_r = 0.4f + 0.1f * sinf(crystalGlow * 0.5f);
        float aura_g = 0.7f + 0.1f * sinf(crystalGlow * 0.6f + PI / 2);
        float aura_b = 0.9f + 0.1f * sinf(crystalGlow * 0.4f + PI);
        glColor4f(aura_r, aura_g, aura_b, 0.06f * auraPulse);
        drawCircle(x, y, 0.35f, 1.3f);

        float midPulse = 0.7f + 0.3f * sinf(crystalGlow * 1.2f + PI / 4);
        float mid_r = 0.5f + 0.2f * sinf(crystalGlow * 0.8f);
        float mid_g = 0.8f + 0.2f * sinf(crystalGlow * 0.9f + PI / 3);
        float mid_b = 1.0f;
        glColor4f(mid_r, mid_g, mid_b, 0.12f * midPulse);
        drawCircle(x, y, 0.22f, 1.2f);

        float corePulse = 0.8f + 0.2f * sinf(crystalGlow * 1.8f);
        float core_r = 0.7f + 0.3f * sinf(crystalGlow * 1.5f);
        float core_g = 0.9f + 0.1f * sinf(crystalGlow * 1.6f + PI / 6);
        float core_b = 1.0f;
        glColor4f(core_r, core_g, core_b, 0.25f * corePulse);
        drawCircle(x, y, 0.15f, 1.1f);

        // Animated Light Rays
        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glRotatef(crystalGlow * 25.0f, 0, 0, 1);
        glLineWidth(2.0f);
        for (int i = 0; i < 6; ++i) {
            float angle = i * 60.0f * (PI / 180.0f);
            float length = 0.15f + 0.04f * sinf(crystalGlow * 1.2f + i * 0.5f);
            glColor4f(0.8f, 0.95f, 1.0f, 0.2f * corePulse);
            glBegin(GL_LINES);
                glVertex2f(0,0);
                glVertex2f(cosf(angle) * length, sinf(angle) * length * 2.0f);
            glEnd();
        }
        glLineWidth(1.0f);
        glPopMatrix();


        glPopAttrib();
    }


    // ---  The Physical Crystal (Always visible) ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_POLYGON);
        float color_top_r = 0.3f, color_top_g = 0.6f, color_top_b = 0.9f;
        float color_bottom_r = 0.5f, color_bottom_g = 0.2f, color_bottom_b = 0.7f;
        setSceneElementColor(color_top_r, color_top_g, color_top_b);
        glVertex2f(x, y + 0.15f);
        setSceneElementColor(0.4f, 0.5f, 0.8f);
        glVertex2f(x + 0.08f, y + 0.05f);
        setSceneElementColor(color_bottom_r, color_bottom_g, color_bottom_b);
        glVertex2f(x + 0.05f, y - 0.12f);
        glVertex2f(x - 0.05f, y - 0.12f);
        setSceneElementColor(0.4f, 0.5f, 0.8f);
        glVertex2f(x - 0.08f, y + 0.05f);
    glEnd();

    // Internal reflections
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glBegin(GL_POLYGON);
        glVertex2f(x, y + 0.12f);
        glVertex2f(x + 0.02f, y + 0.08f);
        glVertex2f(x + 0.01f, y - 0.08f);
        glVertex2f(x - 0.01f, y - 0.08f);
        glVertex2f(x - 0.02f, y + 0.08f);
    glEnd();

    glDisable(GL_BLEND);

    // Crystal Shards at the Base
    glBegin(GL_TRIANGLES);
        setSceneElementColor(0.4f, 0.5f, 0.8f); glVertex2f(x - 0.04f, y - 0.1f);
        setSceneElementColor(0.3f, 0.4f, 0.6f); glVertex2f(x - 0.12f, y - 0.15f);
        setSceneElementColor(0.4f, 0.5f, 0.8f); glVertex2f(x - 0.08f, y - 0.08f);

        setSceneElementColor(0.4f, 0.5f, 0.8f); glVertex2f(x + 0.04f, y - 0.1f);
        setSceneElementColor(0.3f, 0.4f, 0.6f); glVertex2f(x + 0.12f, y - 0.15f);
        setSceneElementColor(0.4f, 0.5f, 0.8f); glVertex2f(x + 0.08f, y - 0.08f);
    glEnd();
}

void drawLeaves() {
    if (currentWeather == SNOWY) return;

    glEnable(GL_BLEND);
    for (int i = 0; i < LEAF_COUNT; ++i) {
        setSceneElementColor(leaves[i].r, leaves[i].g, leaves[i].b, 0.85f);
        glPushMatrix();
        glTranslatef(leaves[i].x, leaves[i].y, 0.0f); glRotatef(leaves[i].rotation, 0,0,1);
        glScalef(leaves[i].size * 0.015f, leaves[i].size * 0.015f, 1.0f);
        glBegin(GL_QUADS); glVertex2f(0,1); glVertex2f(-0.5,0); glVertex2f(0,-1); glVertex2f(0.5,0); glEnd();
        glPopMatrix();
    }
    glDisable(GL_BLEND);
}

void drawElves() {
    for(int i = 0; i < ELF_COUNT; ++i) {
        glPushMatrix();
        glTranslatef(elves[i].x, elves[i].y, 0.0f);

        // Flip direction based on movement
        if (elves[i].x < elves[i].targetX) {
            glScalef(1.0f, 1.0f, 1.0f);
        } else {
            glScalef(-1.0f, 1.0f, 1.0f);
        }

        // Animation calculations
        float legAngle = 0.0f;
        float armAngle = 0.0f;
        if (elves[i].state == ELF_WALKING) {
            legAngle = 20.0f * sinf(elves[i].animationPhase);
            armAngle = 15.0f * sinf(elves[i].animationPhase);
        }

        // --- Draw Legs ---
        setSceneElementColor(0.2f, 0.15f, 0.1f); // Dark pants
        glPushMatrix(); // Leg 1
            glTranslatef(-0.01f, -0.04f, 0.0f);
            glRotatef(legAngle, 1, 0, 0);
            glRectf(-0.005f, 0, 0.005f, -0.04f);
        glPopMatrix();
        glPushMatrix(); // Leg 2
            glTranslatef(0.01f, -0.04f, 0.0f);
            glRotatef(-legAngle, 1, 0, 0);
            glRectf(-0.005f, 0, 0.005f, -0.04f);
        glPopMatrix();

        // --- Draw Arms ---
        setSceneElementColor(0.95f, 0.85f, 0.75f); // Skin color
        glPushMatrix(); // Arm 1 (back)
            glTranslatef(-0.018f, -0.01f, 0.0f);
            glRotatef(-armAngle, 1, 0, 0);
            glRectf(-0.005f, 0, 0.005f, -0.035f);
        glPopMatrix();
        glPushMatrix(); // Arm 2 (front)
            glTranslatef(0.018f, -0.01f, 0.0f);
            glRotatef(armAngle, 1, 0, 0);
            glRectf(-0.005f, 0, 0.005f, -0.035f);
        glPopMatrix();

        // --- Draw Torso ---
        setSceneElementColor(elves[i].r, elves[i].g, elves[i].b); // Tunic color
        glRectf(-0.02f, -0.04f, 0.02f, 0.0f);

        // --- Draw Head ---
        setSceneElementColor(0.95f, 0.85f, 0.75f); // Skin color
        drawCircle(0, 0.015f, 0.015f);

        // Pointy Ears
        glBegin(GL_TRIANGLES);
            glVertex2f(-0.01f, 0.025f); glVertex2f(-0.025f, 0.04f); glVertex2f(-0.015f, 0.045f);
            glVertex2f(0.01f, 0.025f); glVertex2f(0.025f, 0.04f); glVertex2f(0.015f, 0.045f);
        glEnd();

        // Hair
        setSceneElementColor(0.9f, 0.9f, 0.3f); // Blonde hair
        drawCircle(0, 0.025f, 0.016f);

        glPopMatrix();
    }
}

void drawFairyFox() {
    //  Stop drawing the fox if it is raining
    if (currentWeather == RAINY) {
        return;
    }

    // --- Path and Position Calculation ---
    float path_y = -0.95f;
    float x = -3.5f + fox.progress * 7.0f;
    float y = path_y + 0.04f;

    glPushMatrix();
    glTranslatef(x, y, 0.0f);

    glScalef(0.65f, 0.65f, 1.0f);

    // --- Leg Animation Calculation ---
    float walkCycle = fox.progress * 150.0f;
    float legOffset1 = 0.015f * sinf(walkCycle);
    float legOffset2 = 0.015f * sinf(walkCycle + PI);

    // --- 1. Draw the Tail FIRST ---
    glPushMatrix();
    glTranslatef(-0.12f, 0.06f, 0.0f);
    glRotatef(sinf(fox.tailSway) * 20.0f, 0,0,1);

    setSceneElementColor(0.8f, 0.45f, 0.15f);
    glPushMatrix();
    glTranslatef(-0.06f, 0.0f, 0.0f);
    glScalef(0.07f, 0.05f, 1.0f);
    drawCircle(0, 0, 1.0f);
    glPopMatrix();

    setSceneElementColor(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(-0.12f, 0.0f, 0.0f);
    glScalef(0.035f, 0.025f, 1.0f);
    drawCircle(0, 0, 1.0f);
    glPopMatrix();

    glPopMatrix();

    // --- 2. Draw Legs BEFORE the body ---
    setSceneElementColor(0.5f, 0.25f, 0.05f);
    glBegin(GL_QUADS);
        glVertex2f(-0.03f, 0); glVertex2f(-0.01f, 0);
        glVertex2f(-0.01f, -0.08f + legOffset1); glVertex2f(-0.03f, -0.08f + legOffset1);
        glVertex2f(0.08f, 0); glVertex2f(0.10f, 0);
        glVertex2f(0.10f, -0.08f + legOffset2); glVertex2f(0.08f, -0.08f + legOffset2);
    glEnd();

    setSceneElementColor(0.6f, 0.35f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(-0.08f, 0); glVertex2f(-0.06f, 0);
        glVertex2f(-0.06f, -0.08f + legOffset2); glVertex2f(-0.08f, -0.08f + legOffset2);
        glVertex2f(0.03f, 0); glVertex2f(0.05f, 0);
        glVertex2f(0.05f, -0.08f + legOffset1); glVertex2f(0.03f, -0.08f + legOffset1);
    glEnd();

    // --- 3. Draw Body and Head LAST ---
    setSceneElementColor(0.8f, 0.45f, 0.15f);
    glPushMatrix(); glTranslatef(0, 0.05f, 0.0f); glScalef(0.12f, 0.08f, 1.0f); drawCircle(0, 0, 1.0f); glPopMatrix();

    setSceneElementColor(0.8f, 0.45f, 0.15f);
    drawCircle(0.12f, 0.13f, 0.04f, 0.8f);

    setSceneElementColor(0.2f, 0.1f, 0.0f); drawCircle(0.15f, 0.13f, 0.008f);

    setSceneElementColor(0.1f, 0.1f, 0.1f);
    drawCircle(0.13f, 0.145f, 0.005f);

    setSceneElementColor(0.8f, 0.45f, 0.15f);
    glBegin(GL_TRIANGLES);
        glVertex2f(0.10f, 0.17f); glVertex2f(0.13f, 0.17f); glVertex2f(0.115f, 0.20f);
    glEnd();

    setSceneElementColor(0.95f, 0.95f, 0.95f);
    glBegin(GL_TRIANGLES);
        glVertex2f(0.11f, 0.17f); glVertex2f(0.125f, 0.17f); glVertex2f(0.118f, 0.19f);
    glEnd();

    glPopMatrix();
}


void drawSnowCover() {

    if (snowCoverage <= 0.0f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use a slightly blueish-white for the snow, with transparency based on coverage
    glColor4f(0.95f, 0.95f, 1.0f, snowCoverage * 0.9f);

    // --- Draw a large rectangle to cover the entire ground area ---

    glRectf(-2.5f, -0.1f, 2.5f, -1.1f);

    // Add a few extra circles to make the snow drifts look more natural and less flat
     if (snowCoverage > 0.5f) {
        glColor4f(0.95f, 0.95f, 1.0f, (snowCoverage - 0.5f) * 0.5f);
        drawCircle(-1.5f, -0.7f, 0.5f, 0.4f);
        drawCircle(0.0f, -0.8f, 0.7f, 0.4f);
        drawCircle(1.8f, -0.75f, 0.6f, 0.5f);
    }

    glDisable(GL_BLEND);
}


void drawBirds() {
    TimeMoment tm = getTimeMoment();
    if (tm == NIGHT || currentWeather == RAINY || currentWeather == SNOWY) {
        return;
    }

    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(2.0f);

    for (int i = 0; i < MAX_BIRDS; i++) {
        float wingAngle = 0.02f * sinf(birds[i].phase);

        glBegin(GL_LINE_STRIP);
          glVertex2f(birds[i].x - 0.02f, birds[i].y + wingAngle);
          glVertex2f(birds[i].x, birds[i].y);
          glVertex2f(birds[i].x + 0.02f, birds[i].y + wingAngle);
        glEnd();
    }
    glLineWidth(1.0f);
}

// --- Initialization Functions ---

void initRain() {
    for (int i = 0; i < RAIN_COUNT; ++i) {
        raindrops[i].x = -2.5f + (rand() / (float)RAND_MAX) * 5.0f;
        raindrops[i].y = 1.5f + (rand() / (float)RAND_MAX) * 2.0f;
        raindrops[i].speed = 0.02f + (rand() / (float)RAND_MAX) * 0.02f;
    }
}
void initButterflies() {
    for (int i = 0; i < 7; ++i) { // Create 7 butterflies
        Butterfly b;
        b.x = -2.0f + (rand() / (float)RAND_MAX) * 4.0f;
        b.y = -0.8f + (rand() / (float)RAND_MAX) * 0.4f;
        b.initialY = b.y;
        b.speed = 0.001f + (rand() / (float)RAND_MAX) * 0.002f;
        b.directionAngle = (rand() / (float)RAND_MAX) * 2.0f * PI;
        b.flutterPhase = (rand() / (float)RAND_MAX) * PI;
        b.bobPhase = (rand() / (float)RAND_MAX) * PI;

        int colorType = rand() % 3;
        if (colorType == 0) { b.r = 1.0f; b.g = 0.8f; b.b = 0.2f; } // Yellow
        else if (colorType == 1) { b.r = 0.5f; b.g = 0.7f; b.b = 1.0f; } // Blue
        else { b.r = 1.0f; b.g = 0.6f; b.b = 0.8f; } // Pink

        butterflies.push_back(b);
    }
}

void initFireflies() {
    fireflies.clear();
    for (int i = 0; i < 50; ++i) {
        Firefly f;

        f.x = -2.5f + (rand() / (float)RAND_MAX) * 5.0f;
        // Confine them to the ground and lower tree area
        f.y = -0.8f + (rand() / (float)RAND_MAX) * 0.6f;
        f.z = -0.5f + (rand() / (float)RAND_MAX) * 1.0f;

        f.initialY = f.y;
        f.speed = 0.0005f + (rand() / (float)RAND_MAX) * 0.001f;
        f.glowPhase = (rand() / (float)RAND_MAX) * PI;
        f.movePhaseX = (rand() / (float)RAND_MAX) * PI * 2;
        f.movePhaseY = (rand() / (float)RAND_MAX) * PI * 2;
        fireflies.push_back(f);
    }
}


void initLeaves() {
    float y_offset = -0.1f;
    for (int i = 0; i < LEAF_COUNT; ++i) {
        leaves[i].x = -0.6f + (rand() / (float)RAND_MAX) * 1.2f;
        leaves[i].y = (0.0f + y_offset) + (rand() / (float)RAND_MAX) * 0.8f;
        leaves[i].size = 0.8f + (rand() / (float)RAND_MAX);
        leaves[i].speed = 0.001f + (rand() / (float)RAND_MAX) * 0.001f;
        leaves[i].sway = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        leaves[i].swaySpeed = 0.01f + (rand() / (float)RAND_MAX) * 0.02f;
        leaves[i].rotation = (rand() / (float)RAND_MAX) * 360.0f;
        leaves[i].rotationSpeed = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float green_base = 0.4f + (rand() / (float)RAND_MAX) * 0.4f;
        float red_comp = 0.1f + (rand() / (float)RAND_MAX) * 0.15f;
        float blue_comp = 0.2f + (rand() / (float)RAND_MAX) * 0.1f;
        leaves[i].r = red_comp; leaves[i].g = green_base; leaves[i].b = blue_comp;
    }
}

void initSnow() {
    for (int i = 0; i < SNOW_COUNT; ++i) {
        snowflakes[i].x = -3.0f + (rand() / (float)RAND_MAX) * 6.0f;
        snowflakes[i].y = -1.5f + (rand() / (float)RAND_MAX) * 3.0f; // Scatter them all over
        snowflakes[i].size = 0.003f + (rand() / (float)RAND_MAX) * 0.005f;
        snowflakes[i].speed = 0.001f + (rand() / (float)RAND_MAX) * 0.001f;
        snowflakes[i].swayPhase = (rand() / (float)RAND_MAX) * PI * 2.0f;
    }
}

void initElves() {
    for(int i = 0; i < ELF_COUNT; ++i) {
        elves[i].x = -1.8f + (rand() / (float)RAND_MAX) * 3.6f;
        elves[i].y = -0.7f;
        elves[i].targetX = elves[i].x;
        elves[i].speed = 0.001f + (rand() / (float)RAND_MAX) * 0.001f;
        elves[i].state = ELF_IDLE;
        elves[i].stateTimer = 2.0f + (rand() / (float)RAND_MAX) * 3.0f; // Idle for 2-5 seconds
        elves[i].animationPhase = 0.0f;

        if (i % 3 == 0) { elves[i].r = 0.8f; elves[i].g = 0.1f; elves[i].b = 0.2f; } // Red tunic
        else if (i % 3 == 1) { elves[i].r = 0.1f; elves[i].g = 0.2f; elves[i].b = 0.8f; } // Blue tunic
        else { elves[i].r = 0.1f; elves[i].g = 0.6f; elves[i].b = 0.3f; } // Green tunic
    }
}

void initStars() {
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i].x = -2.5f + (rand() / (float)RAND_MAX) * 5.0f;
        stars[i].y = (rand() / (float)RAND_MAX) * 1.5f;
        stars[i].radius = 0.002f + (rand() / (float)RAND_MAX) * 0.004f;
        stars[i].twinkleSpeed = 0.5f + (rand() / (float)RAND_MAX) * 1.5f;
        stars[i].initialPhase = (rand() / (float)RAND_MAX) * PI * 2.0f;
        stars[i].alpha = 0.0f;
    }
}

void initBirds() {
    for (int i = 0; i < MAX_BIRDS; i++) {
        birds[i].x = -3.0f - (static_cast<float>(rand()) / RAND_MAX) * 5.0f;
        birds[i].y = 0.8f + (static_cast<float>(rand()) / RAND_MAX) * 0.6f;
        birds[i].speed = 0.006f + (static_cast<float>(rand()) / RAND_MAX) * 0.004f;
        birds[i].phase = (static_cast<float>(rand()) / RAND_MAX) * 3.14159f;
    }
}


void initClouds() {
    float section_width = 8.0f / CLOUD_COUNT;

    for (int i = 0; i < CLOUD_COUNT; ++i) {
        // --- Basic Cloud Properties ---
        clouds[i].x = -4.0f + i * section_width + ((rand() / (float)RAND_MAX) - 0.5f) * section_width;
        clouds[i].y = 0.6f + (rand() / (float)RAND_MAX) * 0.7f;
        clouds[i].speed = (0.001f + (rand() / (float)RAND_MAX) * 0.0015f) * 2.0f;


        clouds[i].circles[0].radius = (0.12f + (rand() / (float)RAND_MAX) * 0.03f);
        clouds[i].circles[0].yScale = 0.5f;


        int num_puffs = 5 + rand() % 6;
        clouds[i].num_circles = num_puffs + 1;

        for (int j = 1; j <= num_puffs; j++) {
            CloudCircle& c = clouds[i].circles[j];

            c.x_offset = ( (rand() / (float)RAND_MAX) - 0.5f ) * clouds[i].circles[0].radius * 1.8f;
            c.y_offset = ( (rand() / (float)RAND_MAX) * 0.5f ) * clouds[i].circles[0].radius;

            c.radius = clouds[i].circles[0].radius * (0.4f + (rand() / (float)RAND_MAX) * 0.5f);
            c.yScale = 1.0f;
        }
    }
}


void initSceneElements() {
    srand(static_cast<unsigned int>(time(nullptr)));
    initLeaves();
    initElves();
    initStars();
    initClouds();
    initBirds();
    initRain();
    initButterflies();
    initFireflies();
    initSnow();

    // Initialize campfire
    campfires.push_back({-1.5f, -0.6f});
    if (!campfires.empty()) {
        campfires[0].flamePhase1 = 0.0f;
        campfires[0].flamePhase2 = PI / 2.0f;
    }

    // Initialize Fox
    fox.progress = 0.0f;
    fox.speed = 0.05f;
    fox.tailSway = 0.0f;

    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
}
// --- Update Functions ---

void updateElves() {
    TimeMoment tm = getTimeMoment();
    if (tm == NIGHT || currentWeather == RAINY || currentWeather == SNOWY) return;

    for(int i = 0; i < ELF_COUNT; ++i) {
        elves[i].stateTimer -= 0.016f; // Decrease timer

        if (elves[i].stateTimer <= 0) {
            if (elves[i].state == ELF_IDLE) {
                // Switch to walking
                elves[i].state = ELF_WALKING;
                elves[i].stateTimer = 5.0f + (rand() / (float)RAND_MAX) * 5.0f; // Walk for 5-10 seconds
                // Pick a new target
                elves[i].targetX = -1.8f + (rand() / (float)RAND_MAX) * 3.6f;
            } else {
                // Switch to idle
                elves[i].state = ELF_IDLE;
                elves[i].stateTimer = 2.0f + (rand() / (float)RAND_MAX) * 3.0f; // Idle for 2-5 seconds
            }
        }

        if (elves[i].state == ELF_WALKING) {
            // Move towards target
            if (elves[i].x < elves[i].targetX) {
                elves[i].x += elves[i].speed;
            } else {
                 elves[i].x -= elves[i].speed;
            }
            elves[i].animationPhase += 0.2f; // Update animation phase
        }
    }
}

void updateButterflies() {

    if (currentWeather == RAINY || currentWeather == SNOWY) {
        if (!butterflies.empty()) {
            butterflies.clear();
        }
        return;
    }


    if (butterflies.empty()) {
        initButterflies();
    }

    for (auto& b : butterflies) {
        b.x += cosf(b.directionAngle) * b.speed;
        // Bobbing motion
        b.y = b.initialY + 0.05f * sinf(b.bobPhase);

        b.flutterPhase += 0.3f;
        b.bobPhase += 0.05f;

        // Occasionally change direction
        if (rand() % 100 == 0) {
            b.directionAngle += ((rand() / (float)RAND_MAX) - 0.5f);
        }

        // Wrap around screen edges
        if (b.x > 2.7f) b.x = -2.7f;
        if (b.x < -2.7f) b.x = 2.7f;
    }
}



void updateAudio() {

    Mix_HaltMusic();
    if (birdChannel != -1) Mix_HaltChannel(birdChannel);
    if (rainChannel != -1) Mix_HaltChannel(rainChannel);
    if (thunderChannel != -1) Mix_HaltChannel(thunderChannel);


    switch (currentWeather) {
        case RAINY:
            if (rainSound) rainChannel = Mix_PlayChannel(-1, rainSound, -1);
            if (thunderSound) thunderChannel = Mix_PlayChannel(-1, thunderSound, -1);
            break;

        case SUNNY:
            if (riverSound) Mix_PlayMusic(riverSound, -1);
            if (birdSound) birdChannel = Mix_PlayChannel(-1, birdSound, -1);
            break;

        case SNOWY:

            if (winterSound) Mix_PlayMusic(winterSound, -1);
            break;
    }
}

void updateFireflies() {

    if (getTimeMoment() != NIGHT || currentWeather == RAINY || currentWeather == SNOWY) {
        if (!fireflies.empty()) {
            fireflies.clear();
        }
        return;
    }


    if (fireflies.empty()) {
        initFireflies();
    }

    for (auto& f : fireflies) {
        f.movePhaseX += 0.03f * f.speed * 100;
        f.movePhaseY += 0.02f * f.speed * 100;

        f.x += cosf(f.movePhaseX) * 0.001f;
        f.y += sinf(f.movePhaseY) * 0.0005f;

        f.glowPhase += 0.1f;

        // Wrap around screen edges
        if (f.x > 2.7f) f.x = -2.7f;
        if (f.x < -2.7f) f.x = 2.7f;
        if (f.y > 0.0f) f.y = -0.8f; // Reset if they fly too high
        if (f.y < -0.9f) f.y = -0.8f;
    }
}

void updateRainAndSplashes() {
    if (currentWeather != RAINY) {
        splashes.clear();
        droplets.clear();
        return;
    }

    // Update raindrops
    for (int i = 0; i < RAIN_COUNT; ++i) {
        raindrops[i].y -= raindrops[i].speed;


        if (raindrops[i].y < -1.1f) {
            // Create the expanding ring (puddle)
            splashes.push_back({raindrops[i].x, raindrops[i].y, 0.0f, 0.1f, 1.0f});


            for (int j = 0; j < 5; ++j) {
                float angle = (rand() / (float)RAND_MAX) * PI; // Upward arc
                float speed = 0.01f + (rand() / (float)RAND_MAX) * 0.02f;
                droplets.push_back({
                    raindrops[i].x,
                    raindrops[i].y,
                    cosf(angle) * speed * 0.5f, // Horizontal velocity
                    sinf(angle) * speed,       // Vertical velocity
                    0.5f + (rand() / (float)RAND_MAX) * 0.5f // Lifetime
                });
            }

            // Reset the raindrop
            raindrops[i].y = 1.5f;
            raindrops[i].x = -2.5f + (rand() / (float)RAND_MAX) * 5.0f;
        } else if (raindrops[i].y < -1.5f) {
            raindrops[i].y = 1.5f;
            raindrops[i].x = -2.5f + (rand() / (float)RAND_MAX) * 5.0f;
        }
    }

    // Update expanding ring splashes
    for (size_t i = 0; i < splashes.size(); ) {
        splashes[i].life -= 0.05f;
        splashes[i].radius += 0.005f;
        if (splashes[i].life <= 0) {
            splashes.erase(splashes.begin() + i);
        } else {
            i++;
        }
    }


    float gravity = 0.08f;
    for (size_t i = 0; i < droplets.size(); ) {
        droplets[i].life -= 0.02f;
        droplets[i].vy -= gravity * 0.016f; // Apply gravity
        droplets[i].x += droplets[i].vx;
        droplets[i].y += droplets[i].vy;
        if (droplets[i].life <= 0) {
            droplets.erase(droplets.begin() + i);
        } else {
            i++;
        }
    }
}


void updatePuddles(float dt) {

    if (currentWeather == RAINY && (rand() % 150 == 0) && puddles.size() < 15) {
        for (int attempt = 0; attempt < 10; ++attempt) {
            float candidateX = -2.0f + (rand() / (float)RAND_MAX) * 4.0f;
            float candidateY = -0.8f + (rand() / (float)RAND_MAX) * 0.7f;
            float candidateMaxRadius = 0.1f + (rand() / (float)RAND_MAX) * 0.2f;

            bool overlaps = false;
            for (const auto& p : puddles) {
                float dx = p.x - candidateX;
                float dy = p.y - candidateY;
                float distance = sqrt(dx*dx + dy*dy);
                if (distance < p.maxRadius + candidateMaxRadius) {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps) {
                puddles.push_back({candidateX, candidateY, 0.0f, candidateMaxRadius, PUDDLE_GROWING, 0.0f});
                break;
            }
        }
    }


    for (size_t i = 0; i < puddles.size(); ) {
        bool shouldBeRemoved = false;

        if (currentWeather == RAINY) {

            if (puddles[i].state == PUDDLE_FROZEN) puddles[i].state = PUDDLE_MELTING;

            if (puddles[i].state == PUDDLE_GROWING) {
                if (puddles[i].currentRadius < puddles[i].maxRadius) {
                    puddles[i].currentRadius += 0.0005f;
                } else {
                    puddles[i].state = PUDDLE_FULL;
                }
            }
            if (puddles[i].state == PUDDLE_MELTING) {
                puddles[i].freezeProgress -= dt * 2.0f; // Melts over half a second
                if (puddles[i].freezeProgress <= 0) {
                    puddles[i].freezeProgress = 0;
                    puddles[i].state = PUDDLE_FULL; // Becomes a full water puddle
                }
            }
        }
        else if (currentWeather == SNOWY) {

            if (puddles[i].state == PUDDLE_GROWING || puddles[i].state == PUDDLE_FULL) {
                puddles[i].state = PUDDLE_FREEZING;
            }

            if (puddles[i].state == PUDDLE_FREEZING) {
                puddles[i].freezeProgress += dt * 2.0f; // Freezes over half a second
                if (puddles[i].freezeProgress >= 1.0f) {
                    puddles[i].freezeProgress = 1.0f;
                    puddles[i].state = PUDDLE_FROZEN;
                }
            }
        }
        else { // Sunny or other weather

            if (puddles[i].state == PUDDLE_FROZEN) puddles[i].state = PUDDLE_MELTING;
            if (puddles[i].state == PUDDLE_GROWING || puddles[i].state == PUDDLE_FULL) puddles[i].state = PUDDLE_SHRINKING;

            if (puddles[i].state == PUDDLE_MELTING) {
                puddles[i].freezeProgress -= dt * 2.0f;
                if (puddles[i].freezeProgress <= 0) {
                    puddles[i].freezeProgress = 0;
                    puddles[i].state = PUDDLE_SHRINKING; // Start shrinking after melting
                }
            }
            if (puddles[i].state == PUDDLE_SHRINKING) {
                puddles[i].currentRadius -= 0.0005f;
                if (puddles[i].currentRadius <= 0) {
                    shouldBeRemoved = true;
                }
            }
        }

        if (shouldBeRemoved) {
            puddles.erase(puddles.begin() + i);
        } else {
            i++;
        }
    }
}


void updateStars() {
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i].alpha = 0.5f + 0.5f * sinf(stars[i].initialPhase + crystalGlow * stars[i].twinkleSpeed);
    }
}

void updateClouds() {
   for (int i = 0; i < CLOUD_COUNT; ++i) {
       clouds[i].x += clouds[i].speed;
        if (clouds[i].x > 4.0f) {
           clouds[i].x = -4.0f;
       }
    }
 }

void updateBirds() {
    for (int i = 0; i < MAX_BIRDS; i++) {
        birds[i].x += birds[i].speed;
        birds[i].phase += 0.2f + birds[i].speed * 10.0f;
        if (birds[i].x > 3.0f) {
            birds[i].x = -3.0f;
            birds[i].y = 0.8f + (static_cast<float>(rand()) / RAND_MAX) * 0.6f;
            birds[i].speed = 0.006f + (static_cast<float>(rand()) / RAND_MAX) * 0.004f;
        }
    }
}

void updateLeaves() {
    if (currentWeather == SNOWY) return;

    float y_offset = -0.1f;
    for (int i = 0; i < LEAF_COUNT; ++i) {
        leaves[i].y -= leaves[i].speed;
        leaves[i].x += sinf(leaves[i].sway) * 0.001f;
        leaves[i].sway += leaves[i].swaySpeed;
        leaves[i].rotation += leaves[i].rotationSpeed;
        if (leaves[i].y < -1.5f) {
            leaves[i].x = -0.6f + (rand() / (float)RAND_MAX) * 1.2f;
            leaves[i].y = (0.0f + y_offset) + (rand() / (float)RAND_MAX) * 0.8f;
        }
    }
}

void updateSnow() {
    if (currentWeather != SNOWY) return;

    for (int i = 0; i < SNOW_COUNT; ++i) {
        snowflakes[i].y -= snowflakes[i].speed;
        // Add a gentle horizontal sway
        snowflakes[i].x += sinf(snowflakes[i].swayPhase + snowflakes[i].y * 2.0f) * 0.001f;

        // Reset snowflake to the top when it falls off-screen
        if (snowflakes[i].y < -1.5f) {
            snowflakes[i].y = 1.5f;
            snowflakes[i].x = -3.0f + (rand() / (float)RAND_MAX) * 6.0f;
        }
    }
}

void drawParticles() {
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glPointSize(3.0f);

    // Draw Sparks and Embers (Points)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for glows
    glBegin(GL_POINTS);
    for (const auto& p : particles) {
        float alpha = p.life / p.maxLife;
        if (p.type == SPARK || p.type == EMBER) {
            glColor4f(p.r, p.g, p.b, alpha);
            glVertex2f(p.x, p.y);
        }
    }
    glEnd();

    // Draw Smoke (Circles)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard transparency for smoke
    for (const auto& p : particles) {
        if (p.type == SMOKE) {
            float alpha = p.life / p.maxLife;
            // Smoke fades out as it gets higher
            alpha *= (1.0f - (p.y - (-0.6f)) / 0.5f); // Fade based on height above ground
            if (alpha < 0) alpha = 0;

            glColor4f(p.r, p.g, p.b, alpha * p.a);
            drawCircle(p.x, p.y, p.size);
        }
    }

    glPopAttrib();
}

void updateParticles(float dt) {
    // --- Emit New Particles ---

     if (currentWeather != RAINY && currentWeather != SNOWY) {
        if (!campfires.empty() && particles.size() < MAX_PARTICLES) {
            const auto& fire = campfires[0];

            // Sparks
            if (rand() % 4 == 0) {
                particles.push_back(Particle{SPARK, fire.x, fire.y + 0.05f, (rand()%100-50)/8000.f, 0.002f, 0.2f, 0.2f, 0.f, 1.0f, 0.8f, 0.2f, 1.f});
            }
            // Embers
            if (rand() % 15 == 0) {
                particles.push_back(Particle{EMBER, fire.x, fire.y, (rand()%100-50)/3000.f, 0.002f, 0.4f, 0.4f, 0.f, 1.0f, 0.4f, 0.0f, 1.f});
            }
            // Smoke
            if (rand() % 5 == 0) {
                float initialSmokeRadius = 0.01f;
                float initialSmokeVY = 0.003f + (rand() / (float)RAND_MAX) * 0.002f;
                particles.push_back(Particle{SMOKE, fire.x + (-0.01f + (rand() / (float)RAND_MAX) * 0.02f), fire.y + 0.08f,
                                             (-0.0005f + (rand() / (float)RAND_MAX) * 0.001f), initialSmokeVY,
                                             2.0f + (rand() / (float)RAND_MAX) * 1.0f, 3.0f, initialSmokeRadius,
                                             0.8f, 0.8f, 0.8f, 0.4f});
            }
        }
    }


    for (size_t i = 0; i < particles.size(); ) {
        particles[i].life -= dt;
        if (particles[i].life <= 0) {
            particles.erase(particles.begin() + i);
            continue;
        }

        if (particles[i].type == SPARK) {
            particles[i].vy -= 0.0003f;
        } else if (particles[i].type == EMBER) {
            particles[i].vy -= 0.00005f;
            particles[i].vx *= 0.98f;
        } else if (particles[i].type == SMOKE) {
            particles[i].size += dt * 0.05f;
            particles[i].vy *= 1.005f;
            particles[i].vx *= 0.99f;
        }

        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        i++;
    }
}

void updateScene(int) {
    dayNightPhase += 0.0002f;
    if (dayNightPhase > 1.0f) dayNightPhase = 0.0f;

    crystalGlow += 0.05f;


    if (!campfires.empty()) {
        campfires[0].flamePhase1 += 0.1f;
        campfires[0].flamePhase2 += 0.07f;


        // ---  Update Snow Coverage based on Weather ---
    if (currentWeather == SNOWY) {
        if (snowCoverage < 1.0f) {
            snowCoverage += 0.0015f; // Snow slowly accumulates
        }
    } else if (currentWeather == RAINY) {
        if (snowCoverage > 0.0f) {
            snowCoverage -= 0.002f; // Rain melts snow faster
            // Chance to turn melting snow into a puddle
            if (rand() % 100 == 0) {
                 puddles.push_back({-2.0f + (rand() / (float)RAND_MAX) * 4.0f, -0.8f + (rand() / (float)RAND_MAX) * 0.7f, 0.0f, 0.1f + (rand() / (float)RAND_MAX) * 0.1f, PUDDLE_GROWING, 0.0f});
            }
        }
    } else { // Sunny
        if (snowCoverage > 0.0f) {
            snowCoverage -= 0.003f; // Sun melts snow slowly
        }
    }
    // Clamp the value between 0 and 1
    if (snowCoverage < 0.0f) snowCoverage = 0.0f;
    if (snowCoverage > 1.0f) snowCoverage = 1.0f;


       if (currentWeather == RAINY || currentWeather == SNOWY) {
            GLfloat lightOff[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOff);
            glLightfv(GL_LIGHT1, GL_SPECULAR, lightOff);
        } else {
            float lightStrength = 0.8f + 0.2f * sinf(campfires[0].flamePhase1);
            if (getTimeMoment() == NOON || getTimeMoment() == MORNING) {
                lightStrength *= 0.5f;
            }

        GLfloat lightPos[] = { campfires[0].x, campfires[0].y, -1.0f, 1.0f }; // Position light at the fire
        GLfloat lightColor[] = { 1.0f * lightStrength, 0.6f * lightStrength, 0.2f * lightStrength, 1.0f }; // Warm, flickering color

        glLightfv(GL_LIGHT1, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor);
        glLightfv(GL_LIGHT1, GL_SPECULAR, lightColor);

        glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1.0f);
      }

    }

   // ---  Update River Freeze/Melt ---
    if (currentWeather == SNOWY) {
        if (riverFreezeAmount < 1.0f) riverFreezeAmount += 0.0025f;
    } else {
        if (riverFreezeAmount > 0.0f) riverFreezeAmount -= 0.0025f;
    }
    if (riverFreezeAmount > 1.0f) riverFreezeAmount = 1.0f;
    if (riverFreezeAmount < 0.0f) riverFreezeAmount = 0.0f;

    // --- River Flow Speed now depends on how frozen it is ---
    float flowSpeedMultiplier = 1.0f - riverFreezeAmount;
    if (currentWeather == RAINY) {
        riverFlowOffset -= 0.06f * flowSpeedMultiplier;
    } else {
        riverFlowOffset -= 0.02f * flowSpeedMultiplier;
    }

    if (currentWeather != RAINY) {
        fox.progress += fox.speed * 0.016f;
        if (fox.progress > 1.0f) {
            fox.progress = 0.0f;
        }
        fox.tailSway += 0.08f;
    }

    updateLeaves();
     if (currentWeather == SUNNY) {
        updateElves();
    }
    updateStars();
    updateClouds();
    updateBirds();
    updateRainAndSplashes();
    updateParticles(0.016f);
    updateButterflies();
    updateFireflies();
    updatePuddles(0.016f);
    updateSnow();

    glutPostRedisplay();
    glutTimerFunc(16, updateScene, 0);
}


// --- Main GLUT and Program Functions ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT);


    // Draw skybox elements first
    drawSky();


    if (currentWeather != RAINY && currentWeather != SNOWY) {
        drawStars();
        drawSunAndMoon();
    }
    if (currentWeather != SNOWY) {
        drawBirds();
    }

    drawClouds();

    // Draw all ground-level and foreground elements
    drawHills();
     if (currentWeather == SNOWY) {
         drawSnowOnHills();
    }
    drawGroundPatches();
    drawFoxPath();
    drawFlowersAndBushes();
    drawVillageDetails();
    drawCampfire();

    drawFairyFox();

    drawCrystal(-0.5f, -0.5f);
    drawGreatTree();
    drawLeaves();


    drawHouse(-2.0f, -0.6f, 1.0f);
    drawHouse(-1.0f, -0.6f, 1.2f);
    drawHouse(1.0f, -0.6f, 1.2f);
    drawHouse(2.0f, -0.6f, 1.0f);

    drawButterflies();
    drawFireflies();
    drawRainAndSplashes();
    drawSnow();
    drawParticles();
    drawForegroundRiver();


    if (currentWeather == SUNNY) {
        drawElves();
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'r': case 'R':
            currentWeather = RAINY;
            printf("Weather: Rainy\n");
            break;
        case 's': case 'S':
            currentWeather = SUNNY;
            printf("Weather: Sunny\n");
            break;
        case 'w': case 'W':
            currentWeather = SNOWY;
            printf("Weather: Snowy\n");
            break;
        case 27: // ESC key
            cleanup();
            exit(0);
            break;
    }
    updateAudio();
}
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (h == 0) h = 1;

    gluOrtho2D(-2.5f, 2.5f, -1.5f, 1.5f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void initAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer Error: %s\n", Mix_GetError());
        return;
    }


    riverSound = Mix_LoadMUS("E:/AIUB/AIUB/Semester - 8/Computer design/Final/New folder/river.mp3");
    if (!riverSound) printf("Load Error: river.mp3 - %s\n", Mix_GetError());


    winterSound = Mix_LoadMUS("E:/AIUB/AIUB/Semester - 8/Computer design/Final/New folder/winter.mp3");
    if (!winterSound) printf("Load Error: winter.mp3 - %s\n", Mix_GetError());

    thunderSound = Mix_LoadWAV("E:/AIUB/AIUB/Semester - 8/Computer design/Final/New folder/thunder.wav");
    if (!thunderSound) printf("Load Error: thunder.wav - %s\n", Mix_GetError());

    birdSound = Mix_LoadWAV("E:/AIUB/AIUB/Semester - 8/Computer design/Final/New folder/bird.mp3");
    if (!birdSound) printf("Load Error: bird.mp3 - %s\n", Mix_GetError());

    rainSound = Mix_LoadWAV("E:/AIUB/AIUB/Semester - 8/Computer design/Final/New folder/rain.mp3");
    if (!rainSound) printf("Load Error: rain.mp3 - %s\n", Mix_GetError());
}

void cleanup() {
    if (rainSound) Mix_FreeChunk(rainSound);
    if (birdSound) Mix_FreeChunk(birdSound);
    if (thunderSound) Mix_FreeChunk(thunderSound);
    if (riverSound) Mix_FreeMusic(riverSound);
    if (winterSound) Mix_FreeMusic(winterSound);

    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    printf("Audio cleaned up.\n");
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1920, 1080);
    glutCreateWindow("Elven Village");

    initSceneElements();
    initAudio();

    atexit(cleanup);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, updateScene, 0);

    updateAudio();

    glutMainLoop();
    return 0;
}
