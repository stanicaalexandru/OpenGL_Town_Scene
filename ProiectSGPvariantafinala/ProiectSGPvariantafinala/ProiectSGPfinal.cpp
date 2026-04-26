#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/freeglut.h>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

GLuint grassTex, skyTex, rockTex, reliefTex, roadTex;
GLuint barkTex, leavesTex;
GLuint buildingTex1, buildingTex2, buildingTex3;

// === CAMERA ===
float angleY = 0.0f;
float angleX = 0.2f;
float camDist = 55.0f;
float targetX = 0.0f;
float targetZ = 0.0f;

// ===================================================================
//  C1 - MASINA CONTROLABILA
// ===================================================================
float carX = 0.0f;
float carZ = 21.0f;
float carAngle = 0.0f;
float carSpeed = 0.0f;
#define CAR_RADIUS 2.5f

// ===================================================================
//  C1 - COLIZIUNI CU CLADIRI (AABB)
// ===================================================================
struct Building { float bx, bz, hw, hd; };
#define NUM_BUILDINGS 14
Building buildings[NUM_BUILDINGS] = {
    {-85,-30,5.0f,4.0f}, { 85,-30,5.0f,4.0f},
    {-60,-55,4.5f,3.5f}, { 60,-55,4.5f,3.5f},
    {-70, 30,5.5f,4.0f}, { 70, 30,5.5f,4.0f},
    {-30, 40,5.0f,3.5f}, { 30, 40,5.0f,3.5f},
    {-60, 75,4.0f,3.0f}, {-20, 85,3.5f,2.5f},
    { 20, 85,3.5f,2.5f}, { 60, 75,4.0f,3.0f},
    {-10, 95,3.0f,2.0f}, { 10, 95,3.0f,2.0f},
};

// ===================================================================
//  C2 - MASINI PE CIRCUIT (miscare prestabilita pe elipsa)
// ===================================================================
#define NUM_CIRCUIT_CARS 3
#define CIRCUIT_A 34.0f
#define CIRCUIT_B 22.0f
float circuitAngle[NUM_CIRCUIT_CARS] = { 0.0f, 2.094f, 4.189f };
float circuitSpeed[NUM_CIRCUIT_CARS] = { 0.008f, 0.006f, 0.010f };

// ===================================================================
//  C2 - PIETONI ALEATORI
// ===================================================================
#define NUM_PEDESTRIANS 5
struct Pedestrian { float x, z, angle, speed; int timer, changeAt; };
Pedestrian pedestrians[NUM_PEDESTRIANS];

void initPedestrians()
{
    float sx[] = { -15.0f, 15.0f,  0.0f, -10.0f, 10.0f };
    float sz[] = { -8.0f,  8.0f,  0.0f,  10.0f,-10.0f };
    for (int i = 0;i < NUM_PEDESTRIANS;i++) {
        pedestrians[i].x = sx[i];
        pedestrians[i].z = sz[i];
        pedestrians[i].angle = (float)(rand() % 360);
        pedestrians[i].speed = 0.05f + (rand() % 10) * 0.01f;
        pedestrians[i].timer = 0;
        pedestrians[i].changeAt = 60 + rand() % 120;
    }
}

void updatePedestrians()
{
    for (int i = 0;i < NUM_PEDESTRIANS;i++) {
        pedestrians[i].timer++;
        if (pedestrians[i].timer >= pedestrians[i].changeAt) {
            pedestrians[i].angle = (float)(rand() % 360);
            pedestrians[i].timer = 0;
            pedestrians[i].changeAt = 60 + rand() % 120;
        }
        float rad = pedestrians[i].angle * 3.1415926f / 180.0f;
        float nx = pedestrians[i].x + cos(rad) * pedestrians[i].speed;
        float nz = pedestrians[i].z + sin(rad) * pedestrians[i].speed;

        // Limiteaza in interiorul elipsei circuitului (innerA=24, innerB=13)
        float innerA = 24.0f, innerB = 13.0f;
        if ((nx * nx) / (innerA * innerA) + (nz * nz) / (innerB * innerB) > 1.0f) {
            pedestrians[i].angle = atan2f(-pedestrians[i].z, -pedestrians[i].x) * 180.0f / 3.1415926f + (rand() % 60 - 30);
            nx = pedestrians[i].x;
            nz = pedestrians[i].z;
        }
        pedestrians[i].x = nx;
        pedestrians[i].z = nz;
    }
}

// ===================================================================
//  C1+C2 - SISTEM COMPLET DE COLIZIUNI
//  
// ===================================================================
bool checkCollisionSphere(float nx, float nz, float ox, float oz, float r)
{
    float dx = nx - ox, dz = nz - oz;
    return (dx * dx + dz * dz) < (r * r);
}

bool checkCollision(float nx, float nz)
{
    // Coliziune cu cladiri (AABB)
    for (int i = 0;i < NUM_BUILDINGS;i++) {
        if (fabs(nx - buildings[i].bx) < buildings[i].hw + CAR_RADIUS &&
            fabs(nz - buildings[i].bz) < buildings[i].hd + CAR_RADIUS)
            return true;
    }
    // Coliziune cu pietonii
    for (int i = 0;i < NUM_PEDESTRIANS;i++) {
        if (checkCollisionSphere(nx, nz, pedestrians[i].x, pedestrians[i].z, CAR_RADIUS + 0.5f))
            return true;
    }
    // Coliziune cu masinile de pe circuit
    for (int i = 0;i < NUM_CIRCUIT_CARS;i++) {
        float cx = CIRCUIT_A * cos(circuitAngle[i]);
        float cz = CIRCUIT_B * sin(circuitAngle[i]);
        if (checkCollisionSphere(nx, nz, cx, cz, CAR_RADIUS + 2.0f))
            return true;
    }
    // Coliziune cu bolovanul din centru (pozitie 15,5 raza ~6)
    if (checkCollisionSphere(nx, nz, 15.0f, 5.0f, CAR_RADIUS + 5.5f))
        return true;
    return false;
}

// ===================================================================
//  SHADOW MATRIX
// ===================================================================
void makeShadowMatrix(float shadowMat[16], float lightPos[4], float plane[4])
{
    float dot = plane[0] * lightPos[0] + plane[1] * lightPos[1] + plane[2] * lightPos[2] + plane[3] * lightPos[3];
    shadowMat[0] = dot - lightPos[0] * plane[0]; shadowMat[4] = -lightPos[0] * plane[1];
    shadowMat[8] = -lightPos[0] * plane[2];    shadowMat[12] = -lightPos[0] * plane[3];
    shadowMat[1] = -lightPos[1] * plane[0];    shadowMat[5] = dot - lightPos[1] * plane[1];
    shadowMat[9] = -lightPos[1] * plane[2];    shadowMat[13] = -lightPos[1] * plane[3];
    shadowMat[2] = -lightPos[2] * plane[0];    shadowMat[6] = -lightPos[2] * plane[1];
    shadowMat[10] = dot - lightPos[2] * plane[2]; shadowMat[14] = -lightPos[2] * plane[3];
    shadowMat[3] = -lightPos[3] * plane[0];    shadowMat[7] = -lightPos[3] * plane[1];
    shadowMat[11] = -lightPos[3] * plane[2];    shadowMat[15] = dot - lightPos[3] * plane[3];
}

GLuint loadTexture(const char* filename)
{
    int w, h, ch;
    unsigned char* data = stbi_load(filename, &w, &h, &ch, 3);
    if (!data) { printf("NU gaseste: %s\n", filename);return 0; }
    GLuint tex;
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data); return tex;
}

// ===================================================================
//  GEOMETRIE SCENA
// ===================================================================
void drawGround()
{
    glDisable(GL_LIGHTING); glBindTexture(GL_TEXTURE_2D, grassTex); glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);glVertex3f(-180, 0, -220); glTexCoord2f(14, 0);glVertex3f(180, 0, -220);
    glTexCoord2f(14, 14);glVertex3f(180, 0, 150); glTexCoord2f(0, 14);glVertex3f(-180, 0, 150);
    glEnd(); glEnable(GL_LIGHTING);
}

void drawSkybox(float cx, float cy, float cz)
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skyTex);glColor3f(1, 1, 1);
    float s = 110, yB = -300, yT = 300;
    glPushMatrix();glTranslatef(cx, cy, cz);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);glVertex3f(-s, yB, -s);glTexCoord2f(1, 0);glVertex3f(s, yB, -s);glTexCoord2f(1, 1);glVertex3f(s, yT, -s);glTexCoord2f(0, 1);glVertex3f(-s, yT, -s);
    glTexCoord2f(0, 0);glVertex3f(-s, yB, s);glTexCoord2f(1, 0);glVertex3f(s, yB, s);glTexCoord2f(1, 1);glVertex3f(s, yT, s);glTexCoord2f(0, 1);glVertex3f(-s, yT, s);
    glTexCoord2f(0, 0);glVertex3f(-s, yB, -s);glTexCoord2f(1, 0);glVertex3f(-s, yB, s);glTexCoord2f(1, 1);glVertex3f(-s, yT, s);glTexCoord2f(0, 1);glVertex3f(-s, yT, -s);
    glTexCoord2f(0, 0);glVertex3f(s, yB, -s);glTexCoord2f(1, 0);glVertex3f(s, yB, s);glTexCoord2f(1, 1);glVertex3f(s, yT, s);glTexCoord2f(0, 1);glVertex3f(s, yT, -s);
    glTexCoord2f(0, 0);glVertex3f(-s, yT, -s);glTexCoord2f(1, 0);glVertex3f(s, yT, -s);glTexCoord2f(1, 1);glVertex3f(s, yT, s);glTexCoord2f(0, 1);glVertex3f(-s, yT, s);
    glEnd();glPopMatrix();glPopAttrib();
}

void drawMountain3D(float x, float z, float base, float height)
{
    glDisable(GL_LIGHTING);glBindTexture(GL_TEXTURE_2D, rockTex);glColor3f(1, 1, 1);
    float yBase = -0.1f, half = base / 2, tx = x, ty = height, tz = z;
    float x1 = x - half, z1 = z - half, x2 = x + half, z2 = z - half, x3 = x + half, z3 = z + half, x4 = x - half, z4 = z + half;
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0, 0);glVertex3f(x1, yBase, z4);glTexCoord2f(1, 0);glVertex3f(x3, yBase, z3);glTexCoord2f(.5, 1);glVertex3f(tx, ty, tz);
    glTexCoord2f(0, 0);glVertex3f(x2, yBase, z2);glTexCoord2f(1, 0);glVertex3f(x4, yBase, z1);glTexCoord2f(.5, 1);glVertex3f(tx, ty, tz);
    glTexCoord2f(0, 0);glVertex3f(x4, yBase, z1);glTexCoord2f(1, 0);glVertex3f(x1, yBase, z4);glTexCoord2f(.5, 1);glVertex3f(tx, ty, tz);
    glTexCoord2f(0, 0);glVertex3f(x3, yBase, z3);glTexCoord2f(1, 0);glVertex3f(x2, yBase, z2);glTexCoord2f(.5, 1);glVertex3f(tx, ty, tz);
    glEnd();glEnable(GL_LIGHTING);
}

void drawMountains() {
    drawMountain3D(-130, -160, 90, 80);drawMountain3D(-70, -170, 110, 95);
    drawMountain3D(0, -180, 130, 115);drawMountain3D(75, -170, 105, 90);drawMountain3D(140, -160, 85, 75);
}

void drawRelief() {
    glDisable(GL_LIGHTING);glBindTexture(GL_TEXTURE_2D, reliefTex);glColor3f(1, 1, 1);
    glPushMatrix();glTranslatef(15, 2, 5);glScalef(6, 3, 6);
    GLUquadric* q = gluNewQuadric();gluQuadricTexture(q, GL_TRUE);gluSphere(q, 1, 40, 40);gluDeleteQuadric(q);
    glPopMatrix();glEnable(GL_LIGHTING);
}

void drawRoad() {
    glDisable(GL_LIGHTING);glBindTexture(GL_TEXTURE_2D, roadTex);glColor3f(1, 1, 1);
    float oA = 40, oB = 28, iA = 28, iB = 16;int seg = 100;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0;i <= seg;i++) {
        float t = 2 * 3.1415926f * i / seg;
        glTexCoord2f(i / 5.0f, 0);glVertex3f(oA * cos(t), .02f, oB * sin(t));
        glTexCoord2f(i / 5.0f, 1);glVertex3f(iA * cos(t), .02f, iB * sin(t));
    }
    glEnd();glEnable(GL_LIGHTING);
}

void drawRoadLines() {
    glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glColor3f(1, 1, 0);
    float mA = 34, mB = 22;int seg = 48;
    for (int i = 0;i < seg;i += 2) {
        float t1 = 2 * 3.1415926f * i / seg, t2 = 2 * 3.1415926f * (i + 1) / seg;
        glBegin(GL_LINES);glVertex3f(mA * cos(t1), .06f, mB * sin(t1));glVertex3f(mA * cos(t2), .06f, mB * sin(t2));glEnd();
    }
    glEnable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}

void drawBench(float x, float z, float a) {
    glPushMatrix();glTranslatef(x, 0, z);glRotatef(a, 0, 1, 0);
    glColor3f(.2f, .2f, .2f);
    glPushMatrix();glTranslatef(-1.5f, .6f, 0);glScalef(.2f, 1.2f, .2f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(1.5f, .6f, 0);glScalef(.2f, 1.2f, .2f);glutSolidCube(1);glPopMatrix();
    glColor3f(.6f, .3f, .1f);
    glPushMatrix();glTranslatef(0, 1.2f, 0);glScalef(3.5f, .2f, 1);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0, 1.9f, -.4f);glScalef(3.5f, 1.2f, .2f);glutSolidCube(1);glPopMatrix();
    glPopMatrix();
}

// C1: masina controlabila , C2: masini AI colorate
// Modelul e lung pe X (scale 3,0.8,1.8)
// Directia de mers: nx=carX+cos(rad)*speed, nz=carZ+sin(rad)*speed
void drawCar(float x, float z, float angle, float r, float g, float b) {
    glPushMatrix();glTranslatef(x, .8f, z);glRotatef(angle, 0, 1, 0);
    glColor3f(r, g, b);
    glPushMatrix();glScalef(3, .8f, 1.8f);glutSolidCube(1);glPopMatrix();
    glColor3f(r * .9f, g * .9f, b * .9f);
    glPushMatrix();glTranslatef(.2f, .7f, 0);glScalef(1.8f, .7f, 1.5f);glutSolidCube(1);glPopMatrix();
    glColor3f(.1f, .1f, .1f);
    glPushMatrix();glTranslatef(-1, -.3f, .9f);glutSolidTorus(.1, .25, 10, 20);glPopMatrix();
    glPushMatrix();glTranslatef(1, -.3f, .9f);glutSolidTorus(.1, .25, 10, 20);glPopMatrix();
    glPushMatrix();glTranslatef(-1, -.3f, -.9f);glutSolidTorus(.1, .25, 10, 20);glPopMatrix();
    glPushMatrix();glTranslatef(1, -.3f, -.9f);glutSolidTorus(.1, .25, 10, 20);glPopMatrix();
    glPopMatrix();
}

void drawPedestrian(float x, float z, float angle) {
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();glTranslatef(x, 0, z);glRotatef(angle, 0, 1, 0);
    glColor3f(.2f, .4f, .8f);
    glPushMatrix();glTranslatef(0, 1.0f, 0);glScalef(.4f, 1.2f, .3f);glutSolidCube(1);glPopMatrix();
    glColor3f(.9f, .75f, .6f);
    glPushMatrix();glTranslatef(0, 1.85f, 0);glutSolidSphere(.22f, 10, 10);glPopMatrix();
    glColor3f(.1f, .1f, .3f);
    glPushMatrix();glTranslatef(-.12f, .3f, 0);glScalef(.15f, .6f, .2f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(.12f, .3f, 0);glScalef(.15f, .6f, .2f);glutSolidCube(1);glPopMatrix();
    glPopMatrix();glEnable(GL_TEXTURE_2D);
}

void drawTexturedTrunk() {
    glBindTexture(GL_TEXTURE_2D, barkTex);glColor3f(1, 1, 1);float w = .4f, h = 4;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);glVertex3f(-w, 0, w);glTexCoord2f(1, 0);glVertex3f(w, 0, w);glTexCoord2f(1, 1);glVertex3f(w, h, w);glTexCoord2f(0, 1);glVertex3f(-w, h, w);
    glTexCoord2f(0, 0);glVertex3f(w, 0, -w);glTexCoord2f(1, 0);glVertex3f(-w, 0, -w);glTexCoord2f(1, 1);glVertex3f(-w, h, -w);glTexCoord2f(0, 1);glVertex3f(w, h, -w);
    glTexCoord2f(0, 0);glVertex3f(-w, 0, -w);glTexCoord2f(1, 0);glVertex3f(-w, 0, w);glTexCoord2f(1, 1);glVertex3f(-w, h, w);glTexCoord2f(0, 1);glVertex3f(-w, h, -w);
    glTexCoord2f(0, 0);glVertex3f(w, 0, w);glTexCoord2f(1, 0);glVertex3f(w, 0, -w);glTexCoord2f(1, 1);glVertex3f(w, h, -w);glTexCoord2f(0, 1);glVertex3f(w, h, w);
    glEnd();
}
void drawTexturedCrown() {
    glBindTexture(GL_TEXTURE_2D, leavesTex);glColor3f(1, 1, 1);
    GLUquadric* q = gluNewQuadric();gluQuadricTexture(q, GL_TRUE);gluSphere(q, 2, 30, 30);gluDeleteQuadric(q);
}
void drawTree(float x, float z) {
    glDisable(GL_LIGHTING);glEnable(GL_TEXTURE_2D);
    glPushMatrix();glTranslatef(x, 0, z);drawTexturedTrunk();
    glPushMatrix();glTranslatef(0, 5.2f, 0);drawTexturedCrown();glPopMatrix();
    glPopMatrix();glEnable(GL_LIGHTING);
}

void drawTexturedBuilding(float x, float z, float sx, float sy, float sz, GLuint tex) {
    glDisable(GL_LIGHTING);glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D, tex);glColor3f(1, 1, 1);
    float hx = sx / 2, hy = sy, hz = sz / 2, t2 = 2, ty = 3;
    glPushMatrix();glTranslatef(x, 0, z);glBegin(GL_QUADS);
    glTexCoord2f(0, 0);glVertex3f(-hx, 0, hz);glTexCoord2f(t2, 0);glVertex3f(hx, 0, hz);glTexCoord2f(t2, ty);glVertex3f(hx, hy, hz);glTexCoord2f(0, ty);glVertex3f(-hx, hy, hz);
    glTexCoord2f(0, 0);glVertex3f(hx, 0, -hz);glTexCoord2f(t2, 0);glVertex3f(-hx, 0, -hz);glTexCoord2f(t2, ty);glVertex3f(-hx, hy, -hz);glTexCoord2f(0, ty);glVertex3f(hx, hy, -hz);
    glTexCoord2f(0, 0);glVertex3f(-hx, 0, -hz);glTexCoord2f(t2, 0);glVertex3f(-hx, 0, hz);glTexCoord2f(t2, ty);glVertex3f(-hx, hy, hz);glTexCoord2f(0, ty);glVertex3f(-hx, hy, -hz);
    glTexCoord2f(0, 0);glVertex3f(hx, 0, hz);glTexCoord2f(t2, 0);glVertex3f(hx, 0, -hz);glTexCoord2f(t2, ty);glVertex3f(hx, hy, -hz);glTexCoord2f(0, ty);glVertex3f(hx, hy, hz);
    glTexCoord2f(0, 0);glVertex3f(-hx, hy, -hz);glTexCoord2f(t2, 0);glVertex3f(hx, hy, -hz);glTexCoord2f(t2, t2);glVertex3f(hx, hy, hz);glTexCoord2f(0, t2);glVertex3f(-hx, hy, hz);
    glEnd();glPopMatrix();glEnable(GL_LIGHTING);
}
void drawRoof(float x, float z, float sx, float sy, float sz) {
    glDisable(GL_TEXTURE_2D);glDisable(GL_LIGHTING);glColor3f(.6f, .1f, .1f);
    glPushMatrix();glTranslatef(x, sy + .6f, z);glScalef(sx + .5f, 1, sz + .5f);glutSolidCube(1);glPopMatrix();
    glEnable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}
void drawNiceBuilding(float x, float z, float sx, float sy, float sz, GLuint tex) {
    drawTexturedBuilding(x, z, sx, sy, sz, tex);drawRoof(x, z, sx, sy, sz);
}

// === STALPI (P3) ===
#define NUM_LAMPS 4
static float lampPos[NUM_LAMPS][3] = { {-50,8,-20},{50,8,-20},{-50,8,30},{50,8,30} };
void drawLampPost(float px, float pz) {
    glDisable(GL_TEXTURE_2D);glColor3f(.3f, .3f, .35f);
    glPushMatrix();glTranslatef(px, 4, pz);glScalef(.3f, 8, .3f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(px + 1, 8.2f, pz);glScalef(2, .2f, .2f);glutSolidCube(1);glPopMatrix();
    glColor3f(1, .95f, .5f);glPushMatrix();glTranslatef(px + 2, 8.2f, pz);glutSolidSphere(.35f, 16, 16);glPopMatrix();
    glEnable(GL_TEXTURE_2D);
}
void drawAllLampPosts() { for (int i = 0;i < NUM_LAMPS;i++)drawLampPost(lampPos[i][0], lampPos[i][2]); }

void drawStaticObjects() {
    drawTree(-55, -12);drawTree(0, -45);drawTree(55, -12);
    drawNiceBuilding(-85, -30, 10, 7, 8, buildingTex1);drawNiceBuilding(85, -30, 10, 7, 8, buildingTex2);
    drawNiceBuilding(-60, -55, 9, 6, 7, buildingTex3);drawNiceBuilding(60, -55, 9, 6, 7, buildingTex1);
    drawTree(-40, 25);drawTree(40, 25);
    drawNiceBuilding(-70, 30, 11, 7, 8, buildingTex2);drawNiceBuilding(70, 30, 11, 7, 8, buildingTex3);
    drawNiceBuilding(-30, 40, 10, 6, 7, buildingTex1);drawNiceBuilding(30, 40, 10, 6, 7, buildingTex2);
    drawTree(-25, 70);drawTree(25, 70);
    drawNiceBuilding(-60, 75, 8, 6, 6, buildingTex3);drawNiceBuilding(-20, 85, 7, 5, 5, buildingTex2);
    drawNiceBuilding(20, 85, 7, 5, 5, buildingTex1);drawNiceBuilding(60, 75, 8, 6, 6, buildingTex3);
    drawNiceBuilding(-10, 95, 6, 4, 4, buildingTex1);drawNiceBuilding(10, 95, 6, 4, 4, buildingTex2);
    drawBench(-85, -20, 180);drawBench(85, -20, 180);drawBench(-60, -50, 90);drawBench(60, -50, -90);
    drawBench(-70, 35, 180);drawBench(70, 35, 180);drawBench(-30, 80, 180);drawBench(30, 80, 180);
}

// === LUMINA/UMBRA PE SOL (P3) ===
void drawLightCircleOnGround(float px, float pz) {
    glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float cx = px + 2, cz = pz, r = 10;int s = 64;
    glBegin(GL_TRIANGLE_FAN);glColor4f(1, .9f, .3f, .55f);glVertex3f(cx, .03f, cz);glColor4f(1, .85f, .1f, 0);
    for (int i = 0;i <= s;i++) { float t = 2 * 3.1415926f * i / s;glVertex3f(cx + r * cos(t), .03f, cz + r * sin(t)); }
    glEnd();glDisable(GL_BLEND);glEnable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}
void drawAllLightCircles() { for (int i = 0;i < NUM_LAMPS;i++)drawLightCircleOnGround(lampPos[i][0], lampPos[i][2]); }

void drawShadowCircleUnderLamp(float px, float pz) {
    glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float cx = px, cz = pz, r = 3.5f;int s = 48;
    glBegin(GL_TRIANGLE_FAN);glColor4f(0, 0, 0, .45f);glVertex3f(cx, .05f, cz);glColor4f(0, 0, 0, 0);
    for (int i = 0;i <= s;i++) { float t = 2 * 3.1415926f * i / s;glVertex3f(cx + r * cos(t), .05f, cz + r * sin(t)); }
    glEnd();glDisable(GL_BLEND);glEnable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}
void drawAllShadowCircles() { for (int i = 0;i < NUM_LAMPS;i++)drawShadowCircleUnderLamp(lampPos[i][0], lampPos[i][2]); }

// === UMBRE GEOMETRICE (P3) ===
#define SC glColor4f(0,0,0,0.4f)
void shadowBuilding(float x, float z, float sx, float sy, float sz) {
    SC;glPushMatrix();glTranslatef(x, sy / 2, z);glScalef(sx, sy, sz);glutSolidCube(1);glPopMatrix();
    SC;glPushMatrix();glTranslatef(x, sy + .5f, z);glScalef(sx + .5f, 1, sz + .5f);glutSolidCube(1);glPopMatrix();
}
void shadowBench(float x, float z, float a) {
    SC;glPushMatrix();glTranslatef(x, 0, z);glRotatef(a, 0, 1, 0);
    glPushMatrix();glTranslatef(0, 1.2f, 0);glScalef(3.5f, .2f, 1);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(0, 1.9f, -.4f);glScalef(3.5f, 1.2f, .2f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(-1.5f, .6f, 0);glScalef(.2f, 1.2f, .2f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(1.5f, .6f, 0);glScalef(.2f, 1.2f, .2f);glutSolidCube(1);glPopMatrix();
    glPopMatrix();
}

void drawSceneForShadow() {
    // Masina controlabila
    SC;glPushMatrix();glTranslatef(carX, .8f, carZ);glRotatef(carAngle, 0, 1, 0);
    glPushMatrix();glScalef(3, .8f, 1.8f);glutSolidCube(1);glPopMatrix();
    glPushMatrix();glTranslatef(.2f, .7f, 0);glScalef(1.8f, .7f, 1.5f);glutSolidCube(1);glPopMatrix();
    glPopMatrix();
    // Masini circuit
    for (int i = 0;i < NUM_CIRCUIT_CARS;i++) {
        SC;float cx = CIRCUIT_A * cos(circuitAngle[i]), cz = CIRCUIT_B * sin(circuitAngle[i]);
        glPushMatrix();glTranslatef(cx, .8f, cz);glPushMatrix();glScalef(3, .8f, 1.8f);glutSolidCube(1);glPopMatrix();glPopMatrix();
    }
    // Pietoni
    for (int i = 0;i < NUM_PEDESTRIANS;i++) {
        SC;glPushMatrix();glTranslatef(pedestrians[i].x, 1, pedestrians[i].z);glScalef(.4f, 2, .3f);glutSolidCube(1);glPopMatrix();
    }
    // Copaci
    float tx[] = { -55,0,55,-40,40,-25,25 }, tz[] = { -12,-45,-12,25,25,70,70 };
    for (int i = 0;i < 7;i++) {
        SC;glPushMatrix();glTranslatef(tx[i], 2, tz[i]);glScalef(.8f, 4, .8f);glutSolidCube(1);glPopMatrix();
        SC;glPushMatrix();glTranslatef(tx[i], 5.2f, tz[i]);glutSolidSphere(2, 12, 12);glPopMatrix();
    }
    // Stalpi
    for (int i = 0;i < NUM_LAMPS;i++) {
        SC;glPushMatrix();glTranslatef(lampPos[i][0], 4, lampPos[i][2]);glScalef(.3f, 8, .3f);glutSolidCube(1);glPopMatrix();
        SC;glPushMatrix();glTranslatef(lampPos[i][0] + 1, 8.2f, lampPos[i][2]);glScalef(2, .2f, .2f);glutSolidCube(1);glPopMatrix();
    }
    // Cladiri
    shadowBuilding(-85, -30, 10, 7, 8);shadowBuilding(85, -30, 10, 7, 8);shadowBuilding(-60, -55, 9, 6, 7);shadowBuilding(60, -55, 9, 6, 7);
    shadowBuilding(-70, 30, 11, 7, 8);shadowBuilding(70, 30, 11, 7, 8);shadowBuilding(-30, 40, 10, 6, 7);shadowBuilding(30, 40, 10, 6, 7);
    shadowBuilding(-60, 75, 8, 6, 6);shadowBuilding(-20, 85, 7, 5, 5);shadowBuilding(20, 85, 7, 5, 5);shadowBuilding(60, 75, 8, 6, 6);
    shadowBuilding(-10, 95, 6, 4, 4);shadowBuilding(10, 95, 6, 4, 4);
    shadowBench(-85, -20, 180);shadowBench(85, -20, 180);shadowBench(-60, -50, 90);shadowBench(60, -50, -90);
    shadowBench(-70, 35, 180);shadowBench(70, 35, 180);shadowBench(-30, 80, 180);shadowBench(30, 80, 180);
    SC;glPushMatrix();glTranslatef(15, 2, 5);glScalef(6, 3, 6);glutSolidSphere(1, 20, 20);glPopMatrix();
}

void castShadow(float lx, float ly, float lz, float lw) {
    float light[4] = { lx,ly,lz,lw }, plane[4] = { 0,1,0,0 }, sm[16];
    makeShadowMatrix(sm, light, plane);
    glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);glColor4f(0, 0, 0, .4f);
    glPushMatrix();glMultMatrixf(sm);glTranslatef(0, .02f, 0);drawSceneForShadow();glPopMatrix();
    glDisable(GL_BLEND);glEnable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}

// ===================================================================
//  C2 - UPDATE MASINI CIRCUIT
// ===================================================================
void updateCircuitCars() {
    for (int i = 0;i < NUM_CIRCUIT_CARS;i++) {
        circuitAngle[i] += circuitSpeed[i];
        if (circuitAngle[i] > 2 * 3.1415926f) circuitAngle[i] -= 2 * 3.1415926f;
    }
}

// ===================================================================
//  DISPLAY
// ===================================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);glLoadIdentity();
    float camX = targetX + camDist * cos(angleX) * sin(angleY);
    float camY = camDist * sin(angleX);
    float camZ = targetZ + camDist * cos(angleX) * cos(angleY);
    gluLookAt(camX, camY, camZ, targetX, 0, targetZ, 0, 1, 0);

    GLfloat sunPos[] = { 80,120,60,1 };glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    for (int i = 0;i < NUM_LAMPS;i++) {
        GLenum l = GL_LIGHT1 + i;
        GLfloat pos[] = { lampPos[i][0] + 2,lampPos[i][1],lampPos[i][2],1 };
        GLfloat diff[] = { 1,.92f,.45f,1 };GLfloat amb[] = { 0,0,0,1 };GLfloat sd[] = { 0,-1,0 };
        glLightfv(l, GL_POSITION, pos);glLightfv(l, GL_DIFFUSE, diff);glLightfv(l, GL_AMBIENT, amb);
        glLightfv(l, GL_SPOT_DIRECTION, sd);glLightf(l, GL_SPOT_CUTOFF, 45);glLightf(l, GL_SPOT_EXPONENT, 8);
        glLightf(l, GL_CONSTANT_ATTENUATION, .5f);glLightf(l, GL_LINEAR_ATTENUATION, .05f);glLightf(l, GL_QUADRATIC_ATTENUATION, .005f);
        glEnable(l);
    }

    drawSkybox(camX, camY, camZ);drawMountains();drawGround();
    drawAllLightCircles();drawAllShadowCircles();
    drawRoad();drawRoadLines();

    // C1 - Masina controlabila (VERDE)
    drawCar(carX, carZ, carAngle, 0.1f, 0.8f, 0.1f);

    // C2 - Masini pe circuit
    float cc[NUM_CIRCUIT_CARS][3] = { {.8f,.1f,.1f},{.1f,.2f,.9f},{.9f,.5f,0} };
    for (int i = 0;i < NUM_CIRCUIT_CARS;i++) {
        float cx = CIRCUIT_A * cos(circuitAngle[i]), cz = CIRCUIT_B * sin(circuitAngle[i]);
        // Tangenta la elipsa -> directia vizuala
        float dir = atan2f(CIRCUIT_B * cos(circuitAngle[i]), -CIRCUIT_A * sin(circuitAngle[i])) * 180 / 3.1415926f;
        drawCar(cx, cz, dir, cc[i][0], cc[i][1], cc[i][2]);
    }

    // C2 - Pietoni aleatori
    for (int i = 0;i < NUM_PEDESTRIANS;i++)
        drawPedestrian(pedestrians[i].x, pedestrians[i].z, pedestrians[i].angle);

    drawRelief();drawStaticObjects();drawAllLampPosts();
    castShadow(sunPos[0], sunPos[1], sunPos[2], 1);
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);glMatrixMode(GL_PROJECTION);glLoadIdentity();
    gluPerspective(60, (float)w / h, 1, 500);glMatrixMode(GL_MODELVIEW);
}

// ===================================================================
//  TIMER - animatie + fizica masina
// ===================================================================
void timer(int v) {
    updateCircuitCars();
    updatePedestrians();

    // Frecare: masina incetineste natural
    if (carSpeed > 0) { carSpeed -= .008f;if (carSpeed < 0)carSpeed = 0; }
    if (carSpeed < 0) { carSpeed += .008f;if (carSpeed > 0)carSpeed = 0; }

    // Miscare continua
    if (carSpeed != 0) {
        float rad = carAngle * 3.1415926f / 180.0f;
        // Modelul lung pe X: fata pe +X la angle=0
        // dx=cos(angle), dz=sin(angle)
        float nx = carX + cos(rad) * carSpeed;
        float nz = carZ + sin(rad) * carSpeed;
        if (!checkCollision(nx, nz)) { carX = nx;carZ = nz; }
        else carSpeed = 0;
        if (carX < -170)carX = -170;if (carX > 170)carX = 170;
        if (carZ < -210)carZ = -210;if (carZ > 140)carZ = 140;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ===================================================================
//  TASTATURA
// ===================================================================
void keyboard(unsigned char key, int x, int y) {
    float sp = 3;
    if (key == 'a' || key == 'A') angleY -= .05f;
    if (key == 'd' || key == 'D') angleY += .05f;
    if (key == 'w' || key == 'W') { angleX += .05f;if (angleX > 1.4f)angleX = 1.4f; }
    if (key == 's' || key == 'S') { angleX -= .05f;if (angleX < .05f)angleX = .05f; }
    if (key == 'q' || key == 'Q') { targetX -= sp * sin(angleY);targetZ -= sp * cos(angleY); }
    if (key == 'e' || key == 'E') { targetX += sp * sin(angleY);targetZ += sp * cos(angleY); }
    if (key == '+' || key == '=') { camDist -= 3;if (camDist < 5)camDist = 5; }
    if (key == '-') { camDist += 3;if (camDist > 200)camDist = 200; }
    if (key == 27)exit(0);
    glutPostRedisplay();
}

// C1: Sageti = control masina
void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT)  carAngle -= 4.0f;
    if (key == GLUT_KEY_RIGHT) carAngle += 4.0f;
    if (key == GLUT_KEY_UP) { carSpeed += .05f; if (carSpeed > .5f)carSpeed = .5f; }
    if (key == GLUT_KEY_DOWN) { carSpeed -= .08f; if (carSpeed < -.2f)carSpeed = -.2f; }
    glutPostRedisplay();
}

void mouseWheel(int b, int dir, int x, int y) {
    if (dir > 0)camDist -= 3;else camDist += 3;
    if (camDist < 5)camDist = 5;if (camDist > 200)camDist = 200;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 600);
    glutCreateWindow("Scena Examen - C1+C2");

    glClearColor(.1f, .1f, .15f, 1);
    glEnable(GL_DEPTH_TEST);glEnable(GL_TEXTURE_2D);glEnable(GL_NORMALIZE);
    GLfloat amb[] = { .15f,.15f,.2f,1 };glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);
    GLfloat sd[] = { .6f,.6f,.55f,1 }, ss[] = { .3f,.3f,.3f,1 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sd);glLightfv(GL_LIGHT0, GL_SPECULAR, ss);
    GLfloat ma[] = { .3f,.3f,.3f,1 }, md[] = { .8f,.8f,.8f,1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ma);glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, md);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);glEnable(GL_COLOR_MATERIAL);

    grassTex = loadTexture("grass.bmp");skyTex = loadTexture("sky.bmp");
    rockTex = loadTexture("rock.bmp");reliefTex = loadTexture("relief.bmp");
    roadTex = loadTexture("road.bmp");barkTex = loadTexture("bark.bmp");
    leavesTex = loadTexture("leaves.bmp");buildingTex1 = loadTexture("building1.bmp");
    buildingTex2 = loadTexture("building2.bmp");buildingTex3 = loadTexture("building3.bmp");

    initPedestrians();
    glutDisplayFunc(display);glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);glutSpecialFunc(specialKeys);glutMouseWheelFunc(mouseWheel);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();return 0;
}
