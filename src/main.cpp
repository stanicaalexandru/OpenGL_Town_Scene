// Town Scene: a small interactive 3D town rendered with legacy OpenGL and freeglut.
// Drive a car around the ring road, watch the traffic and the pedestrians, orbit the camera.

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

const float PI = 3.1415926f;
const float DEG = PI / 180.0f;

GLuint grassTex, skyTex, rockTex, reliefTex, roadTex;
GLuint barkTex, leavesTex;
GLuint buildingTex[3];

// ---------------------------------------------------------------------------
// Camera: orbits around a target point on the ground
// ---------------------------------------------------------------------------
float angleY = 0.0f;
float angleX = 0.2f;
float camDist = 55.0f;
float targetX = 0.0f;
float targetZ = 0.0f;

// ---------------------------------------------------------------------------
// Player car (arrow keys). The model is long on X: angle 0 faces +X.
// ---------------------------------------------------------------------------
float carX = 0.0f;
float carZ = 21.0f;
float carAngle = 0.0f;
float carSpeed = 0.0f;
const float CAR_RADIUS = 2.5f;

// ---------------------------------------------------------------------------
// Static scene objects. Each list is used for drawing, collisions and shadows.
// ---------------------------------------------------------------------------
struct Building { float x, z, sx, sy, sz; int tex; };
const Building BUILDINGS[] = {
    {-85, -30, 10, 7, 8, 0}, {85, -30, 10, 7, 8, 1},
    {-60, -55, 9, 6, 7, 2},  {60, -55, 9, 6, 7, 0},
    {-70, 30, 11, 7, 8, 1},  {70, 30, 11, 7, 8, 2},
    {-30, 40, 10, 6, 7, 0},  {30, 40, 10, 6, 7, 1},
    {-60, 75, 8, 6, 6, 2},   {-20, 85, 7, 5, 5, 1},
    {20, 85, 7, 5, 5, 0},    {60, 75, 8, 6, 6, 2},
    {-10, 95, 6, 4, 4, 0},   {10, 95, 6, 4, 4, 1},
};
const int NUM_BUILDINGS = sizeof(BUILDINGS) / sizeof(BUILDINGS[0]);

struct Point { float x, z; };
const Point TREES[] = { {-55, -12}, {0, -45}, {55, -12}, {-40, 25}, {40, 25}, {-25, 70}, {25, 70} };
const int NUM_TREES = sizeof(TREES) / sizeof(TREES[0]);

struct Bench { float x, z, angle; };
const Bench BENCHES[] = {
    {-85, -20, 180}, {85, -20, 180}, {-60, -50, 90}, {60, -50, -90},
    {-70, 35, 180},  {70, 35, 180},  {-30, 80, 180}, {30, 80, 180},
};
const int NUM_BENCHES = sizeof(BENCHES) / sizeof(BENCHES[0]);

const int NUM_LAMPS = 4;
const float LAMP_HEIGHT = 8.0f;
const Point LAMPS[NUM_LAMPS] = { {-50, -20}, {50, -20}, {-50, 30}, {50, 30} };

// Textured boulder in the middle of the ring
const float BOULDER_X = 15.0f, BOULDER_Z = 5.0f;

// ---------------------------------------------------------------------------
// Traffic: cars driving on an ellipse (the middle of the ring road)
// ---------------------------------------------------------------------------
const int NUM_CIRCUIT_CARS = 3;
const float CIRCUIT_A = 34.0f;
const float CIRCUIT_B = 22.0f;
float circuitAngle[NUM_CIRCUIT_CARS] = { 0.0f, 2.094f, 4.189f };
const float circuitSpeed[NUM_CIRCUIT_CARS] = { 0.008f, 0.006f, 0.010f };
const float circuitColor[NUM_CIRCUIT_CARS][3] = { {0.8f, 0.1f, 0.1f}, {0.1f, 0.2f, 0.9f}, {0.9f, 0.5f, 0.0f} };

void circuitPosition(int i, float& x, float& z)
{
    x = CIRCUIT_A * cosf(circuitAngle[i]);
    z = CIRCUIT_B * sinf(circuitAngle[i]);
}

// Direction of travel on the ellipse (its tangent), in degrees
float circuitHeading(int i)
{
    return atan2f(CIRCUIT_B * cosf(circuitAngle[i]), -CIRCUIT_A * sinf(circuitAngle[i])) / DEG;
}

void updateCircuitCars()
{
    for (int i = 0; i < NUM_CIRCUIT_CARS; i++) {
        circuitAngle[i] += circuitSpeed[i];
        if (circuitAngle[i] > 2 * PI) circuitAngle[i] -= 2 * PI;
    }
}

// ---------------------------------------------------------------------------
// Pedestrians: walk in a random direction, change it every few seconds,
// and stay inside the inner edge of the ring road
// ---------------------------------------------------------------------------
const int NUM_PEDESTRIANS = 5;
struct Pedestrian { float x, z, angle, speed; int timer, changeAt; };
Pedestrian pedestrians[NUM_PEDESTRIANS];

void initPedestrians()
{
    const float startX[] = { -15.0f, 15.0f, 0.0f, -10.0f, 10.0f };
    const float startZ[] = { -8.0f, 8.0f, 0.0f, 10.0f, -10.0f };
    for (int i = 0; i < NUM_PEDESTRIANS; i++) {
        pedestrians[i].x = startX[i];
        pedestrians[i].z = startZ[i];
        pedestrians[i].angle = (float)(rand() % 360);
        pedestrians[i].speed = 0.05f + (rand() % 10) * 0.01f;
        pedestrians[i].timer = 0;
        pedestrians[i].changeAt = 60 + rand() % 120;
    }
}

void updatePedestrians()
{
    const float innerA = 24.0f, innerB = 13.0f;
    for (int i = 0; i < NUM_PEDESTRIANS; i++) {
        Pedestrian& p = pedestrians[i];
        if (++p.timer >= p.changeAt) {
            p.angle = (float)(rand() % 360);
            p.timer = 0;
            p.changeAt = 60 + rand() % 120;
        }
        float nx = p.x + cosf(p.angle * DEG) * p.speed;
        float nz = p.z + sinf(p.angle * DEG) * p.speed;

        // About to leave the inner ellipse: turn back towards the centre, with some randomness
        if ((nx * nx) / (innerA * innerA) + (nz * nz) / (innerB * innerB) > 1.0f) {
            p.angle = atan2f(-p.z, -p.x) / DEG + (rand() % 60 - 30);
            continue;
        }
        p.x = nx;
        p.z = nz;
    }
}

// ---------------------------------------------------------------------------
// Collisions for the player car: buildings as boxes (AABB), the rest as circles
// ---------------------------------------------------------------------------
bool insideCircle(float x, float z, float cx, float cz, float r)
{
    float dx = x - cx, dz = z - cz;
    return dx * dx + dz * dz < r * r;
}

bool checkCollision(float x, float z)
{
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        const Building& b = BUILDINGS[i];
        if (fabsf(x - b.x) < b.sx / 2 + CAR_RADIUS && fabsf(z - b.z) < b.sz / 2 + CAR_RADIUS) return true;
    }
    for (int i = 0; i < NUM_PEDESTRIANS; i++)
        if (insideCircle(x, z, pedestrians[i].x, pedestrians[i].z, CAR_RADIUS + 0.5f)) return true;
    for (int i = 0; i < NUM_CIRCUIT_CARS; i++) {
        float cx, cz;
        circuitPosition(i, cx, cz);
        if (insideCircle(x, z, cx, cz, CAR_RADIUS + 2.0f)) return true;
    }
    return insideCircle(x, z, BOULDER_X, BOULDER_Z, CAR_RADIUS + 5.5f);
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
GLuint loadTexture(const char* name)
{
    char path[256];
    snprintf(path, sizeof(path), "assets/textures/%s", name);
    int w, h, channels;
    unsigned char* data = stbi_load(path, &w, &h, &channels, 3);
    if (!data) {
        fprintf(stderr, "Texture not found: %s (run the program from the project folder)\n", path);
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // rows of RGB images are not always a multiple of 4 bytes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return tex;
}

// Textured quad: corners in order, texture repeated (tu, tv) times
void texturedQuad(const float a[3], const float b[3], const float c[3], const float d[3], float tu = 1, float tv = 1)
{
    glTexCoord2f(0, 0);   glVertex3fv(a);
    glTexCoord2f(tu, 0);  glVertex3fv(b);
    glTexCoord2f(tu, tv); glVertex3fv(c);
    glTexCoord2f(0, tv);  glVertex3fv(d);
}

// Textured walls of an axis-aligned box centred at the origin (no top or bottom)
void texturedWalls(float hx, float height, float hz, float tu, float tv)
{
    const float c[4][2] = { {-hx, hz}, {hx, hz}, {hx, -hz}, {-hx, -hz} };
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++) {
        const float* p = c[i];
        const float* q = c[(i + 1) % 4];
        const float a[] = { p[0], 0, p[1] }, b[] = { q[0], 0, q[1] };
        const float bt[] = { q[0], height, q[1] }, at[] = { p[0], height, p[1] };
        texturedQuad(a, b, bt, at, tu, tv);
    }
    glEnd();
}

// ---------------------------------------------------------------------------
// Terrain, sky and road
// ---------------------------------------------------------------------------
void drawGround()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, grassTex);
    glColor3f(1, 1, 1);
    const float a[] = { -180, 0, -220 }, b[] = { 180, 0, -220 }, c[] = { 180, 0, 150 }, d[] = { -180, 0, 150 };
    glBegin(GL_QUADS);
    texturedQuad(a, b, c, d, 14, 14);
    glEnd();
    glEnable(GL_LIGHTING);
}

// Sky box that follows the camera, drawn first without depth so everything else covers it
void drawSkybox(float cx, float cy, float cz)
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skyTex);
    glColor3f(1, 1, 1);

    const float s = 110, bottom = -300, top = 300;
    const float bl[] = { -s, bottom, -s }, br[] = { s, bottom, -s }, tr[] = { s, top, -s }, tl[] = { -s, top, -s };
    const float fbl[] = { -s, bottom, s }, fbr[] = { s, bottom, s }, ftr[] = { s, top, s }, ftl[] = { -s, top, s };
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glBegin(GL_QUADS);
    texturedQuad(bl, br, tr, tl);      // back
    texturedQuad(fbl, fbr, ftr, ftl);  // front
    texturedQuad(bl, fbl, ftl, tl);    // left
    texturedQuad(br, fbr, ftr, tr);    // right
    texturedQuad(tl, tr, ftr, ftl);    // top
    glEnd();
    glPopMatrix();
    glPopAttrib();
}

// Four-sided pyramid standing on the ground
void drawMountain(float x, float z, float base, float height)
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, rockTex);
    glColor3f(1, 1, 1);
    const float y = -0.1f, h = base / 2;
    const float corners[4][3] = { {x - h, y, z + h}, {x + h, y, z + h}, {x + h, y, z - h}, {x - h, y, z - h} };
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 4; i++) {
        glTexCoord2f(0, 0);    glVertex3fv(corners[i]);
        glTexCoord2f(1, 0);    glVertex3fv(corners[(i + 1) % 4]);
        glTexCoord2f(0.5f, 1); glVertex3f(x, height, z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawMountains()
{
    drawMountain(-130, -160, 90, 80);
    drawMountain(-70, -170, 110, 95);
    drawMountain(0, -180, 130, 115);
    drawMountain(75, -170, 105, 90);
    drawMountain(140, -160, 85, 75);
}

void drawBoulder()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, reliefTex);
    glColor3f(1, 1, 1);
    glPushMatrix();
    glTranslatef(BOULDER_X, 2, BOULDER_Z);
    glScalef(6, 3, 6);
    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluSphere(q, 1, 40, 40);
    gluDeleteQuadric(q);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

// Ring road: a textured strip between two ellipses, with a dashed yellow centre line
void drawRoad()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, roadTex);
    glColor3f(1, 1, 1);
    const float outerA = 40, outerB = 28, innerA = 28, innerB = 16;
    const int segments = 100;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float t = 2 * PI * i / segments;
        glTexCoord2f(i / 5.0f, 0); glVertex3f(outerA * cosf(t), 0.02f, outerB * sinf(t));
        glTexCoord2f(i / 5.0f, 1); glVertex3f(innerA * cosf(t), 0.02f, innerB * sinf(t));
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 0);
    const int dashes = 48;
    glBegin(GL_LINES);
    for (int i = 0; i < dashes; i += 2) {
        float t1 = 2 * PI * i / dashes, t2 = 2 * PI * (i + 1) / dashes;
        glVertex3f(CIRCUIT_A * cosf(t1), 0.06f, CIRCUIT_B * sinf(t1));
        glVertex3f(CIRCUIT_A * cosf(t2), 0.06f, CIRCUIT_B * sinf(t2));
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

// ---------------------------------------------------------------------------
// Objects. The "shape" functions draw geometry only, so the same shape is used
// for the coloured object and for its shadow.
// ---------------------------------------------------------------------------
void box(float x, float y, float z, float sx, float sy, float sz)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1);
    glPopMatrix();
}

void benchShape(const Bench& b, bool coloured)
{
    glPushMatrix();
    glTranslatef(b.x, 0, b.z);
    glRotatef(b.angle, 0, 1, 0);
    if (coloured) glColor3f(0.2f, 0.2f, 0.2f);
    box(-1.5f, 0.6f, 0, 0.2f, 1.2f, 0.2f);
    box(1.5f, 0.6f, 0, 0.2f, 1.2f, 0.2f);
    if (coloured) glColor3f(0.6f, 0.3f, 0.1f);
    box(0, 1.2f, 0, 3.5f, 0.2f, 1);
    box(0, 1.9f, -0.4f, 3.5f, 1.2f, 0.2f);
    glPopMatrix();
}

void drawCar(float x, float z, float angle, const float color[3])
{
    glPushMatrix();
    glTranslatef(x, 0.8f, z);
    glRotatef(angle, 0, 1, 0);
    glColor3f(color[0], color[1], color[2]);
    box(0, 0, 0, 3, 0.8f, 1.8f);
    glColor3f(color[0] * 0.9f, color[1] * 0.9f, color[2] * 0.9f);
    box(0.2f, 0.7f, 0, 1.8f, 0.7f, 1.5f);
    glColor3f(0.1f, 0.1f, 0.1f);
    const float wheels[4][2] = { {-1, 0.9f}, {1, 0.9f}, {-1, -0.9f}, {1, -0.9f} };
    for (const auto& w : wheels) {
        glPushMatrix();
        glTranslatef(w[0], -0.3f, w[1]);
        glutSolidTorus(0.1, 0.25, 10, 20);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawPedestrian(const Pedestrian& p)
{
    glPushMatrix();
    glTranslatef(p.x, 0, p.z);
    glRotatef(p.angle, 0, 1, 0);
    glColor3f(0.2f, 0.4f, 0.8f);
    box(0, 1.0f, 0, 0.4f, 1.2f, 0.3f);
    glColor3f(0.9f, 0.75f, 0.6f);
    glPushMatrix();
    glTranslatef(0, 1.85f, 0);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();
    glColor3f(0.1f, 0.1f, 0.3f);
    box(-0.12f, 0.3f, 0, 0.15f, 0.6f, 0.2f);
    box(0.12f, 0.3f, 0, 0.15f, 0.6f, 0.2f);
    glPopMatrix();
}

void drawTree(const Point& t)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    glPushMatrix();
    glTranslatef(t.x, 0, t.z);
    glBindTexture(GL_TEXTURE_2D, barkTex);
    texturedWalls(0.4f, 4, 0.4f, 1, 1);

    glBindTexture(GL_TEXTURE_2D, leavesTex);
    glTranslatef(0, 5.2f, 0);
    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluSphere(q, 2, 30, 30);
    gluDeleteQuadric(q);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawBuilding(const Building& b)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, buildingTex[b.tex]);
    glColor3f(1, 1, 1);
    glPushMatrix();
    glTranslatef(b.x, 0, b.z);
    texturedWalls(b.sx / 2, b.sy, b.sz / 2, 2, 3);
    glPopMatrix();

    // Flat red roof, slightly larger than the walls
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.6f, 0.1f, 0.1f);
    box(b.x, b.sy + 0.6f, b.z, b.sx + 0.5f, 1, b.sz + 0.5f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void lampShape(const Point& l)
{
    box(l.x, LAMP_HEIGHT / 2, l.z, 0.3f, LAMP_HEIGHT, 0.3f);
    box(l.x + 1, LAMP_HEIGHT + 0.2f, l.z, 2, 0.2f, 0.2f);
}

void drawLampPost(const Point& l)
{
    glColor3f(0.3f, 0.3f, 0.35f);
    lampShape(l);
    glColor3f(1, 0.95f, 0.5f);
    glPushMatrix();
    glTranslatef(l.x + 2, LAMP_HEIGHT + 0.2f, l.z);
    glutSolidSphere(0.35f, 16, 16);
    glPopMatrix();
}

// Soft circle on the ground: a colour in the centre fading to transparent at the edge
void groundGlow(float cx, float cz, float radius, float y, const float centre[4], const float edge[4])
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const int segments = 64;
    glBegin(GL_TRIANGLE_FAN);
    glColor4fv(centre);
    glVertex3f(cx, y, cz);
    glColor4fv(edge);
    for (int i = 0; i <= segments; i++) {
        float t = 2 * PI * i / segments;
        glVertex3f(cx + radius * cosf(t), y, cz + radius * sinf(t));
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

// Warm pool of light under each street lamp and a small contact shadow at its base
void drawLampLightOnGround()
{
    const float light[] = { 1, 0.9f, 0.3f, 0.55f }, lightEdge[] = { 1, 0.85f, 0.1f, 0 };
    const float dark[] = { 0, 0, 0, 0.45f }, darkEdge[] = { 0, 0, 0, 0 };
    for (int i = 0; i < NUM_LAMPS; i++) groundGlow(LAMPS[i].x + 2, LAMPS[i].z, 10, 0.03f, light, lightEdge);
    for (int i = 0; i < NUM_LAMPS; i++) groundGlow(LAMPS[i].x, LAMPS[i].z, 3.5f, 0.05f, dark, darkEdge);
}

// ---------------------------------------------------------------------------
// Planar shadows: simplified geometry of the scene, flattened onto the ground
// (y = 0) by a projection matrix built from the sun position
// ---------------------------------------------------------------------------
void makeShadowMatrix(float m[16], const float light[4], const float plane[4])
{
    float dot = plane[0] * light[0] + plane[1] * light[1] + plane[2] * light[2] + plane[3] * light[3];
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            m[col * 4 + row] = (row == col ? dot : 0) - light[row] * plane[col];
}

void drawShadowGeometry()
{
    glPushMatrix();
    glTranslatef(carX, 0.8f, carZ);
    glRotatef(carAngle, 0, 1, 0);
    box(0, 0, 0, 3, 0.8f, 1.8f);
    box(0.2f, 0.7f, 0, 1.8f, 0.7f, 1.5f);
    glPopMatrix();

    for (int i = 0; i < NUM_CIRCUIT_CARS; i++) {
        float cx, cz;
        circuitPosition(i, cx, cz);
        box(cx, 0.8f, cz, 3, 0.8f, 1.8f);
    }
    for (int i = 0; i < NUM_PEDESTRIANS; i++) box(pedestrians[i].x, 1, pedestrians[i].z, 0.4f, 2, 0.3f);
    for (int i = 0; i < NUM_TREES; i++) {
        box(TREES[i].x, 2, TREES[i].z, 0.8f, 4, 0.8f);
        glPushMatrix();
        glTranslatef(TREES[i].x, 5.2f, TREES[i].z);
        glutSolidSphere(2, 12, 12);
        glPopMatrix();
    }
    for (int i = 0; i < NUM_LAMPS; i++) lampShape(LAMPS[i]);
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        const Building& b = BUILDINGS[i];
        box(b.x, b.sy / 2, b.z, b.sx, b.sy, b.sz);
        box(b.x, b.sy + 0.5f, b.z, b.sx + 0.5f, 1, b.sz + 0.5f);
    }
    for (int i = 0; i < NUM_BENCHES; i++) benchShape(BENCHES[i], false);

    glPushMatrix();
    glTranslatef(BOULDER_X, 2, BOULDER_Z);
    glScalef(6, 3, 6);
    glutSolidSphere(1, 20, 20);
    glPopMatrix();
}

void drawShadows(const float sun[4])
{
    const float ground[4] = { 0, 1, 0, 0 };
    float m[16];
    makeShadowMatrix(m, sun, ground);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0, 0.5f);
    // Lift the flattened geometry just above the ground (and pull it towards the camera
    // in depth), otherwise it fights with the ground for visibility
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2, -2);
    glPushMatrix();
    glTranslatef(0, 0.04f, 0);
    glMultMatrixf(m);
    drawShadowGeometry();
    glPopMatrix();
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
void setupStreetLights()
{
    const GLfloat diffuse[] = { 1, 0.92f, 0.45f, 1 }, ambient[] = { 0, 0, 0, 1 }, down[] = { 0, -1, 0 };
    for (int i = 0; i < NUM_LAMPS; i++) {
        GLenum light = GL_LIGHT1 + i;
        const GLfloat pos[] = { LAMPS[i].x + 2, LAMP_HEIGHT, LAMPS[i].z, 1 };
        glLightfv(light, GL_POSITION, pos);
        glLightfv(light, GL_DIFFUSE, diffuse);
        glLightfv(light, GL_AMBIENT, ambient);
        glLightfv(light, GL_SPOT_DIRECTION, down);
        glLightf(light, GL_SPOT_CUTOFF, 45);
        glLightf(light, GL_SPOT_EXPONENT, 8);
        glLightf(light, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(light, GL_LINEAR_ATTENUATION, 0.05f);
        glLightf(light, GL_QUADRATIC_ATTENUATION, 0.005f);
        glEnable(light);
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    float camX = targetX + camDist * cosf(angleX) * sinf(angleY);
    float camY = camDist * sinf(angleX);
    float camZ = targetZ + camDist * cosf(angleX) * cosf(angleY);
    gluLookAt(camX, camY, camZ, targetX, 0, targetZ, 0, 1, 0);

    const GLfloat sun[] = { 110, 70, 80, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, sun);
    setupStreetLights();

    drawSkybox(camX, camY, camZ);
    drawMountains();
    drawGround();
    drawLampLightOnGround();
    drawRoad();
    drawBoulder();
    for (int i = 0; i < NUM_TREES; i++) drawTree(TREES[i]);
    for (int i = 0; i < NUM_BUILDINGS; i++) drawBuilding(BUILDINGS[i]);

    // Untextured objects
    glDisable(GL_TEXTURE_2D);
    const float playerColor[] = { 0.1f, 0.8f, 0.1f };
    drawCar(carX, carZ, carAngle, playerColor);
    for (int i = 0; i < NUM_CIRCUIT_CARS; i++) {
        float cx, cz;
        circuitPosition(i, cx, cz);
        drawCar(cx, cz, circuitHeading(i), circuitColor[i]);
    }
    for (int i = 0; i < NUM_PEDESTRIANS; i++) drawPedestrian(pedestrians[i]);
    for (int i = 0; i < NUM_BENCHES; i++) benchShape(BENCHES[i], true);
    for (int i = 0; i < NUM_LAMPS; i++) drawLampPost(LAMPS[i]);
    glEnable(GL_TEXTURE_2D);

    drawShadows(sun);
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)w / (h > 0 ? h : 1), 1, 500);
    glMatrixMode(GL_MODELVIEW);
}

// About 60 updates per second: traffic, pedestrians and the player car (friction and collisions)
void timer(int)
{
    updateCircuitCars();
    updatePedestrians();

    const float friction = 0.008f;
    if (carSpeed > 0) carSpeed = fmaxf(0, carSpeed - friction);
    if (carSpeed < 0) carSpeed = fminf(0, carSpeed + friction);

    if (carSpeed != 0) {
        float nx = carX + cosf(carAngle * DEG) * carSpeed;
        float nz = carZ + sinf(carAngle * DEG) * carSpeed;
        if (checkCollision(nx, nz)) {
            carSpeed = 0;
        } else {
            carX = fminf(170, fmaxf(-170, nx));
            carZ = fminf(140, fmaxf(-210, nz));
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void zoom(float delta) { camDist = fminf(200, fmaxf(5, camDist + delta)); }

void keyboard(unsigned char key, int, int)
{
    const float step = 3;
    switch (key) {
    case 'a': case 'A': angleY -= 0.05f; break;
    case 'd': case 'D': angleY += 0.05f; break;
    case 'w': case 'W': angleX = fminf(1.4f, angleX + 0.05f); break;
    case 's': case 'S': angleX = fmaxf(0.05f, angleX - 0.05f); break;
    case 'q': case 'Q': targetX -= step * sinf(angleY); targetZ -= step * cosf(angleY); break;
    case 'e': case 'E': targetX += step * sinf(angleY); targetZ += step * cosf(angleY); break;
    case '+': case '=': zoom(-3); break;
    case '-': zoom(3); break;
    case 27: exit(0);
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    switch (key) {
    case GLUT_KEY_LEFT: carAngle -= 4.0f; break;
    case GLUT_KEY_RIGHT: carAngle += 4.0f; break;
    case GLUT_KEY_UP: carSpeed = fminf(0.5f, carSpeed + 0.05f); break;
    case GLUT_KEY_DOWN: carSpeed = fmaxf(-0.2f, carSpeed - 0.08f); break;
    }
    glutPostRedisplay();
}

void mouseWheel(int, int direction, int, int)
{
    zoom(direction > 0 ? -3.0f : 3.0f);
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    srand((unsigned)time(nullptr));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Town Scene");

    glClearColor(0.1f, 0.1f, 0.15f, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);

    const GLfloat ambient[] = { 0.15f, 0.15f, 0.2f, 1 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    const GLfloat sunDiffuse[] = { 0.6f, 0.6f, 0.55f, 1 }, sunSpecular[] = { 0.3f, 0.3f, 0.3f, 1 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);
    const GLfloat matAmbient[] = { 0.3f, 0.3f, 0.3f, 1 }, matDiffuse[] = { 0.8f, 0.8f, 0.8f, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    grassTex = loadTexture("grass.jpg");
    skyTex = loadTexture("sky.jpg");
    rockTex = loadTexture("rock.jpg");
    reliefTex = loadTexture("relief.jpg");
    roadTex = loadTexture("road.jpg");
    barkTex = loadTexture("bark.jpg");
    leavesTex = loadTexture("leaves.jpg");
    buildingTex[0] = loadTexture("building1.jpg");
    buildingTex[1] = loadTexture("building2.jpg");
    buildingTex[2] = loadTexture("building3.jpg");

    initPedestrians();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseWheelFunc(mouseWheel);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
