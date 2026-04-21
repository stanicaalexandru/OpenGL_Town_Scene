#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/freeglut.h>
#include <cmath>
#include <stdio.h>

GLuint grassTex, skyTex, rockTex, reliefTex, roadTex;
GLuint barkTex, leavesTex;
GLuint buildingTex1, buildingTex2, buildingTex3;

// === CAMERA ===
float angleY = 0.0f;   // rotatie orizontala (stanga/dreapta)
float angleX = 0.2f;   // rotatie verticala  (sus/jos)
float camDist = 55.0f;  // distanta fata de origine (zoom)
float targetX = 0.0f;   // punctul spre care priveste camera (pan inainte/inapoi/lateral)
float targetZ = 0.0f;

// === UMBRE ===
void makeShadowMatrix(float shadowMat[16], float lightPos[4], float plane[4])
{
    float dot = plane[0] * lightPos[0] + plane[1] * lightPos[1]
        + plane[2] * lightPos[2] + plane[3] * lightPos[3];

    shadowMat[0] = dot - lightPos[0] * plane[0];
    shadowMat[4] = -lightPos[0] * plane[1];
    shadowMat[8] = -lightPos[0] * plane[2];
    shadowMat[12] = -lightPos[0] * plane[3];

    shadowMat[1] = -lightPos[1] * plane[0];
    shadowMat[5] = dot - lightPos[1] * plane[1];
    shadowMat[9] = -lightPos[1] * plane[2];
    shadowMat[13] = -lightPos[1] * plane[3];

    shadowMat[2] = -lightPos[2] * plane[0];
    shadowMat[6] = -lightPos[2] * plane[1];
    shadowMat[10] = dot - lightPos[2] * plane[2];
    shadowMat[14] = -lightPos[2] * plane[3];

    shadowMat[3] = -lightPos[3] * plane[0];
    shadowMat[7] = -lightPos[3] * plane[1];
    shadowMat[11] = -lightPos[3] * plane[2];
    shadowMat[15] = dot - lightPos[3] * plane[3];
}

// loader textura
GLuint loadTexture(const char* filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 3);
    if (!data) { printf("NU gaseste: %s\n", filename); return 0; }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return texture;
}

// ===================================================================
//  GEOMETRIE SCENA
// ===================================================================

void drawGround()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, grassTex);
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);   glVertex3f(-180, 0, -220);
    glTexCoord2f(14, 0);  glVertex3f(180, 0, -220);
    glTexCoord2f(14, 14); glVertex3f(180, 0, 150);
    glTexCoord2f(0, 14);  glVertex3f(-180, 0, 150);
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawSkybox(float camX, float camY, float camZ)
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skyTex);
    glColor3f(1, 1, 1);

    float s = 110.0f, yBottom = -300.0f, yTop = 300.0f;
    glPushMatrix();
    glTranslatef(camX, camY, camZ);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, yBottom, -s); glTexCoord2f(1, 0); glVertex3f(s, yBottom, -s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, -s);  glTexCoord2f(0, 1); glVertex3f(-s, yTop, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, yBottom, s);  glTexCoord2f(1, 0); glVertex3f(s, yBottom, s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, s);  glTexCoord2f(0, 1); glVertex3f(-s, yTop, s);
    glTexCoord2f(0, 0); glVertex3f(-s, yBottom, -s);  glTexCoord2f(1, 0); glVertex3f(-s, yBottom, s);
    glTexCoord2f(1, 1); glVertex3f(-s, yTop, s);  glTexCoord2f(0, 1); glVertex3f(-s, yTop, -s);
    glTexCoord2f(0, 0); glVertex3f(s, yBottom, -s);  glTexCoord2f(1, 0); glVertex3f(s, yBottom, s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, s);  glTexCoord2f(0, 1); glVertex3f(s, yTop, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, yTop, -s);     glTexCoord2f(1, 0); glVertex3f(s, yTop, -s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, s);     glTexCoord2f(0, 1); glVertex3f(-s, yTop, s);
    glEnd();
    glPopMatrix();
    glPopAttrib();
}

void drawMountain3D(float x, float z, float base, float height)
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, rockTex);
    glColor3f(1, 1, 1);
    float yBase = -0.1f, half = base / 2.0f;
    float topX = x, topY = height, topZ = z;
    float x1 = x - half, z1 = z - half, x2 = x + half, z2 = z - half;
    float x3 = x + half, z3 = z + half, x4 = x - half, z4 = z + half;
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0, 0);   glVertex3f(x1, yBase, z4); glTexCoord2f(1, 0);   glVertex3f(x3, yBase, z3); glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);
    glTexCoord2f(0, 0);   glVertex3f(x2, yBase, z2); glTexCoord2f(1, 0);   glVertex3f(x4, yBase, z1); glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);
    glTexCoord2f(0, 0);   glVertex3f(x4, yBase, z1); glTexCoord2f(1, 0);   glVertex3f(x1, yBase, z4); glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);
    glTexCoord2f(0, 0);   glVertex3f(x3, yBase, z3); glTexCoord2f(1, 0);   glVertex3f(x2, yBase, z2); glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawMountains()
{
    drawMountain3D(-130, -160, 90, 80); drawMountain3D(-70, -170, 110, 95);
    drawMountain3D(0, -180, 130, 115);  drawMountain3D(75, -170, 105, 90);
    drawMountain3D(140, -160, 85, 75);
}

void drawRelief()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, reliefTex);
    glColor3f(1, 1, 1);
    glPushMatrix(); glTranslatef(15, 2, 5); glScalef(6, 3, 6);
    GLUquadric* q = gluNewQuadric(); gluQuadricTexture(q, GL_TRUE); gluSphere(q, 1, 40, 40); gluDeleteQuadric(q);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawRoad()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, roadTex);
    glColor3f(1, 1, 1);
    float outerA = 40, outerB = 28, innerA = 28, innerB = 16; int seg = 100;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0;i <= seg;i++) {
        float t = 2 * 3.1415926f * i / seg;
        glTexCoord2f(i / 5.0f, 0); glVertex3f(outerA * cos(t), 0.02f, outerB * sin(t));
        glTexCoord2f(i / 5.0f, 1); glVertex3f(innerA * cos(t), 0.02f, innerB * sin(t));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawRoadLines()
{
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 0);
    float midA = 34, midB = 22; int seg = 48;
    for (int i = 0;i < seg;i += 2) {
        float t1 = 2 * 3.1415926f * i / seg, t2 = 2 * 3.1415926f * (i + 1) / seg;
        glBegin(GL_LINES);
        glVertex3f(midA * cos(t1), 0.06f, midB * sin(t1));
        glVertex3f(midA * cos(t2), 0.06f, midB * sin(t2));
        glEnd();
    }
    glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

void drawBench(float x, float z, float angle)
{
    glPushMatrix(); glTranslatef(x, 0, z); glRotatef(angle, 0, 1, 0);
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix(); glTranslatef(-1.5f, 0.6f, 0); glScalef(0.2f, 1.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, 0.6f, 0); glScalef(0.2f, 1.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glColor3f(0.6f, 0.3f, 0.1f);
    glPushMatrix(); glTranslatef(0, 1.2f, 0);     glScalef(3.5f, 0.2f, 1.0f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 1.9f, -0.4f); glScalef(3.5f, 1.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glPopMatrix();
}

void drawCar(float x, float z, float angle)
{
    glPushMatrix(); glTranslatef(x, 0.8f, z); glRotatef(angle, 0, 1, 0);
    glColor3f(0.8f, 0, 0);
    glPushMatrix(); glScalef(3, 0.8f, 1.8f); glutSolidCube(1); glPopMatrix();
    glColor3f(0.9f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(0.2f, 0.7f, 0); glScalef(1.8f, 0.7f, 1.5f); glutSolidCube(1); glPopMatrix();
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(-1, -0.3f, 0.9f); glutSolidTorus(0.1, 0.25, 10, 20); glPopMatrix();
    glPushMatrix(); glTranslatef(1, -0.3f, 0.9f); glutSolidTorus(0.1, 0.25, 10, 20); glPopMatrix();
    glPushMatrix(); glTranslatef(-1, -0.3f, -0.9f); glutSolidTorus(0.1, 0.25, 10, 20); glPopMatrix();
    glPushMatrix(); glTranslatef(1, -0.3f, -0.9f); glutSolidTorus(0.1, 0.25, 10, 20); glPopMatrix();
    glPopMatrix();
}

void drawTexturedTrunk()
{
    glBindTexture(GL_TEXTURE_2D, barkTex); glColor3f(1, 1, 1);
    float w = 0.4f, h = 4.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-w, 0, w);  glTexCoord2f(1, 0); glVertex3f(w, 0, w);
    glTexCoord2f(1, 1); glVertex3f(w, h, w);   glTexCoord2f(0, 1); glVertex3f(-w, h, w);
    glTexCoord2f(0, 0); glVertex3f(w, 0, -w);  glTexCoord2f(1, 0); glVertex3f(-w, 0, -w);
    glTexCoord2f(1, 1); glVertex3f(-w, h, -w); glTexCoord2f(0, 1); glVertex3f(w, h, -w);
    glTexCoord2f(0, 0); glVertex3f(-w, 0, -w); glTexCoord2f(1, 0); glVertex3f(-w, 0, w);
    glTexCoord2f(1, 1); glVertex3f(-w, h, w);  glTexCoord2f(0, 1); glVertex3f(-w, h, -w);
    glTexCoord2f(0, 0); glVertex3f(w, 0, w);   glTexCoord2f(1, 0); glVertex3f(w, 0, -w);
    glTexCoord2f(1, 1); glVertex3f(w, h, -w);  glTexCoord2f(0, 1); glVertex3f(w, h, w);
    glEnd();
}

void drawTexturedCrown()
{
    glBindTexture(GL_TEXTURE_2D, leavesTex); glColor3f(1, 1, 1);
    GLUquadric* q = gluNewQuadric(); gluQuadricTexture(q, GL_TRUE); gluSphere(q, 2.0, 30, 30); gluDeleteQuadric(q);
}

void drawTree(float x, float z)
{
    glDisable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);
    glPushMatrix(); glTranslatef(x, 0, z);
    drawTexturedTrunk();
    glPushMatrix(); glTranslatef(0, 5.2f, 0); drawTexturedCrown(); glPopMatrix();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawTexturedBuilding(float x, float z, float sx, float sy, float sz, GLuint tex)
{
    glDisable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex); glColor3f(1, 1, 1);
    float hx = sx / 2, hy = sy, hz = sz / 2, tx2 = 2, ty = 3;
    glPushMatrix(); glTranslatef(x, 0, z);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);   glVertex3f(-hx, 0, hz);   glTexCoord2f(tx2, 0);  glVertex3f(hx, 0, hz);
    glTexCoord2f(tx2, ty);glVertex3f(hx, hy, hz);   glTexCoord2f(0, ty);   glVertex3f(-hx, hy, hz);
    glTexCoord2f(0, 0);   glVertex3f(hx, 0, -hz);   glTexCoord2f(tx2, 0);  glVertex3f(-hx, 0, -hz);
    glTexCoord2f(tx2, ty);glVertex3f(-hx, hy, -hz); glTexCoord2f(0, ty);   glVertex3f(hx, hy, -hz);
    glTexCoord2f(0, 0);   glVertex3f(-hx, 0, -hz);  glTexCoord2f(tx2, 0);  glVertex3f(-hx, 0, hz);
    glTexCoord2f(tx2, ty);glVertex3f(-hx, hy, hz);  glTexCoord2f(0, ty);   glVertex3f(-hx, hy, -hz);
    glTexCoord2f(0, 0);   glVertex3f(hx, 0, hz);    glTexCoord2f(tx2, 0);  glVertex3f(hx, 0, -hz);
    glTexCoord2f(tx2, ty);glVertex3f(hx, hy, -hz);  glTexCoord2f(0, ty);   glVertex3f(hx, hy, hz);
    glTexCoord2f(0, 0);   glVertex3f(-hx, hy, -hz); glTexCoord2f(tx2, 0);  glVertex3f(hx, hy, -hz);
    glTexCoord2f(tx2, tx2);glVertex3f(hx, hy, hz);  glTexCoord2f(0, tx2);  glVertex3f(-hx, hy, hz);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawRoof(float x, float z, float sx, float sy, float sz)
{
    glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);
    glColor3f(0.6f, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(x, sy + 0.6f, z); glScalef(sx + 0.5f, 1.0f, sz + 0.5f); glutSolidCube(1); glPopMatrix();
    glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

void drawNiceBuilding(float x, float z, float sx, float sy, float sz, GLuint tex)
{
    drawTexturedBuilding(x, z, sx, sy, sz, tex);
    drawRoof(x, z, sx, sy, sz);
}

// ===================================================================
//  STALPI DE ILUMINAT  (P3)
// ===================================================================
#define NUM_LAMPS 4
static float lampPos[NUM_LAMPS][3] = {
    {-50, 8, -20}, { 50, 8, -20},
    {-50, 8,  30}, { 50, 8,  30}
};

void drawLampPost(float px, float pz)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.3f, 0.3f, 0.35f);
    glPushMatrix(); glTranslatef(px, 4.0f, pz); glScalef(0.3f, 8.0f, 0.3f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(px + 1.0f, 8.2f, pz); glScalef(2.0f, 0.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glColor3f(1.0f, 0.95f, 0.5f);
    glPushMatrix(); glTranslatef(px + 2.0f, 8.2f, pz); glutSolidSphere(0.35f, 16, 16); glPopMatrix();
    glEnable(GL_TEXTURE_2D);
}

void drawAllLampPosts()
{
    for (int i = 0;i < NUM_LAMPS;i++) drawLampPost(lampPos[i][0], lampPos[i][2]);
}

// ===================================================================
//  OBIECTE STATICE
// ===================================================================
void drawStaticObjects()
{
    drawTree(-55, -12); drawTree(0, -45); drawTree(55, -12);
    drawNiceBuilding(-85, -30, 10, 7, 8, buildingTex1); drawNiceBuilding(85, -30, 10, 7, 8, buildingTex2);
    drawNiceBuilding(-60, -55, 9, 6, 7, buildingTex3);  drawNiceBuilding(60, -55, 9, 6, 7, buildingTex1);
    drawTree(-40, 25); drawTree(40, 25);
    drawNiceBuilding(-70, 30, 11, 7, 8, buildingTex2);  drawNiceBuilding(70, 30, 11, 7, 8, buildingTex3);
    drawNiceBuilding(-30, 40, 10, 6, 7, buildingTex1);  drawNiceBuilding(30, 40, 10, 6, 7, buildingTex2);
    drawTree(-25, 70); drawTree(25, 70);
    drawNiceBuilding(-60, 75, 8, 6, 6, buildingTex3);   drawNiceBuilding(-20, 85, 7, 5, 5, buildingTex2);
    drawNiceBuilding(20, 85, 7, 5, 5, buildingTex1);    drawNiceBuilding(60, 75, 8, 6, 6, buildingTex3);
    drawNiceBuilding(-10, 95, 6, 4, 4, buildingTex1);   drawNiceBuilding(10, 95, 6, 4, 4, buildingTex2);
    drawBench(-85, -20, 180); drawBench(85, -20, 180);
    drawBench(-60, -50, 90);  drawBench(60, -50, -90);
    drawBench(-70, 35, 180);  drawBench(70, 35, 180);
    drawBench(-30, 80, 180);  drawBench(30, 80, 180);
}

//  CERC DE LUMINA PE SOL  (P3)

void drawLightCircleOnGround(float px, float pz)
{
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float cx = px + 2.0f, cz = pz, radius = 10.0f; int steps = 64;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.9f, 0.3f, 0.55f); glVertex3f(cx, 0.03f, cz);
    glColor4f(1.0f, 0.85f, 0.1f, 0.0f);
    for (int i = 0;i <= steps;i++) {
        float t = 2.0f * 3.1415926f * i / steps;
        glVertex3f(cx + radius * cos(t), 0.03f, cz + radius * sin(t));
    }
    glEnd();
    glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

void drawAllLightCircles()
{
    for (int i = 0;i < NUM_LAMPS;i++) drawLightCircleOnGround(lampPos[i][0], lampPos[i][2]);
}

// Cerc de umbra (intunecat) sub baza fiecarui stalp – umbre multiple
void drawShadowCircleUnderLamp(float px, float pz)
{
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cx = px, cz = pz;
    float radius = 3.5f;
    int   steps = 48;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);   // centru: negru semi-transparent
    glVertex3f(cx, 0.05f, cz);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);    // margine: transparent
    for (int i = 0;i <= steps;i++) {
        float t = 2.0f * 3.1415926f * i / steps;
        glVertex3f(cx + radius * cos(t), 0.05f, cz + radius * sin(t));
    }
    glEnd();

    glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

void drawAllShadowCircles()
{
    for (int i = 0;i < NUM_LAMPS;i++)
        drawShadowCircleUnderLamp(lampPos[i][0], lampPos[i][2]);
}

//  UMBRE PROIECTATE PE SOL  (P3)

#define SHADOW_COLOR glColor4f(0,0,0,0.4f)

void shadowBuilding(float x, float z, float sx, float sy, float sz)
{
    SHADOW_COLOR;
    glPushMatrix(); glTranslatef(x, sy / 2.0f, z); glScalef(sx, sy, sz); glutSolidCube(1); glPopMatrix();
    SHADOW_COLOR;
    glPushMatrix(); glTranslatef(x, sy + 0.5f, z); glScalef(sx + 0.5f, 1.0f, sz + 0.5f); glutSolidCube(1); glPopMatrix();
}

void shadowBench(float x, float z, float angle)
{
    SHADOW_COLOR;
    glPushMatrix();
    glTranslatef(x, 0, z); glRotatef(angle, 0, 1, 0);
    glPushMatrix(); glTranslatef(0, 1.2f, 0);     glScalef(3.5f, 0.2f, 1.0f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 1.9f, -0.4f); glScalef(3.5f, 1.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.5f, 0.6f, 0); glScalef(0.2f, 1.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(1.5f, 0.6f, 0); glScalef(0.2f, 1.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    glPopMatrix();
}

void drawSceneForShadow()
{
    // Masina
    SHADOW_COLOR;
    glPushMatrix(); glTranslatef(0, 0.8f, 21.0f);
    glPushMatrix(); glScalef(3.0f, 0.8f, 1.8f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0.2f, 0.7f, 0); glScalef(1.8f, 0.7f, 1.5f); glutSolidCube(1); glPopMatrix();
    glPopMatrix();

    // Copaci (trunchi + coroana sferic)
    float treesX[] = { -55, 0, 55,-40, 40,-25, 25 };
    float treesZ[] = { -12,-45,-12, 25, 25, 70, 70 };
    for (int i = 0;i < 7;i++) {
        SHADOW_COLOR;
        glPushMatrix(); glTranslatef(treesX[i], 2.0f, treesZ[i]); glScalef(0.8f, 4.0f, 0.8f); glutSolidCube(1); glPopMatrix();
        SHADOW_COLOR;
        glPushMatrix(); glTranslatef(treesX[i], 5.2f, treesZ[i]); glutSolidSphere(2.0f, 12, 12); glPopMatrix();
    }

    // Stalpi de iluminat
    for (int i = 0;i < NUM_LAMPS;i++) {
        SHADOW_COLOR;
        glPushMatrix(); glTranslatef(lampPos[i][0], 4.0f, lampPos[i][2]); glScalef(0.3f, 8.0f, 0.3f); glutSolidCube(1); glPopMatrix();
        SHADOW_COLOR;
        glPushMatrix(); glTranslatef(lampPos[i][0] + 1.0f, 8.2f, lampPos[i][2]); glScalef(2.0f, 0.2f, 0.2f); glutSolidCube(1); glPopMatrix();
    }

    // Cladiri
    shadowBuilding(-85, -30, 10, 7, 8); shadowBuilding(85, -30, 10, 7, 8);
    shadowBuilding(-60, -55, 9, 6, 7); shadowBuilding(60, -55, 9, 6, 7);
    shadowBuilding(-70, 30, 11, 7, 8); shadowBuilding(70, 30, 11, 7, 8);
    shadowBuilding(-30, 40, 10, 6, 7); shadowBuilding(30, 40, 10, 6, 7);
    shadowBuilding(-60, 75, 8, 6, 6); shadowBuilding(-20, 85, 7, 5, 5);
    shadowBuilding(20, 85, 7, 5, 5); shadowBuilding(60, 75, 8, 6, 6);
    shadowBuilding(-10, 95, 6, 4, 4); shadowBuilding(10, 95, 6, 4, 4);

    // Banci
    shadowBench(-85, -20, 180); shadowBench(85, -20, 180);
    shadowBench(-60, -50, 90); shadowBench(60, -50, -90);
    shadowBench(-70, 35, 180); shadowBench(70, 35, 180);
    shadowBench(-30, 80, 180); shadowBench(30, 80, 180);

    // Relief (sfera)
    SHADOW_COLOR;
    glPushMatrix(); glTranslatef(15, 2, 5); glScalef(6, 3, 6); glutSolidSphere(1.0f, 20, 20); glPopMatrix();
}

// isSun=1 -> umbra soarelui (negru, mai opaca)
// isSun=0 -> umbra stalp (galbuie, mai transparenta)
void castShadow(float lx, float ly, float lz, float lw, int isSun = 1)
{
    float light[4] = { lx,ly,lz,lw }, plane[4] = { 0,1,0,0 }, shadowMat[16];
    makeShadowMatrix(shadowMat, light, plane);
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (isSun)
        glColor4f(0.0f, 0.0f, 0.0f, 0.40f);   // umbra soare: negru, opacitate 40%
    else
        glColor4f(0.05f, 0.03f, 0.0f, 0.22f);   // umbra stalp: maro inchis, opacitate 22%
    glPushMatrix();
    glMultMatrixf(shadowMat);
    glTranslatef(0, isSun ? 0.02f : 0.04f, 0); // stalpii usor deasupra umbrei soarelui
    drawSceneForShadow();
    glPopMatrix();
    glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

// ===================================================================
//  DISPLAY
// ===================================================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera orbit in jurul punctului (targetX, 0, targetZ)
    float camX = targetX + camDist * cos(angleX) * sin(angleY);
    float camY = camDist * sin(angleX);
    float camZ = targetZ + camDist * cos(angleX) * cos(angleY);

    gluLookAt(camX, camY, camZ,
        targetX, 0.0f, targetZ,
        0.0f, 1.0f, 0.0f);

    // Lumina principala (soare)
    GLfloat sunPos[] = { 80, 120, 60, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);

    // Spotlights stalpi
    for (int i = 0;i < NUM_LAMPS;i++) {
        GLenum light = GL_LIGHT1 + i;
        GLfloat pos[] = { lampPos[i][0] + 2.0f, lampPos[i][1], lampPos[i][2], 1.0f };
        GLfloat diff[] = { 1.0f, 0.92f, 0.45f, 1.0f };
        GLfloat amb[] = { 0.0f, 0.0f,  0.0f,  1.0f };
        GLfloat spotDir[] = { 0.0f,-1.0f,  0.0f };
        glLightfv(light, GL_POSITION, pos);
        glLightfv(light, GL_DIFFUSE, diff);
        glLightfv(light, GL_AMBIENT, amb);
        glLightfv(light, GL_SPOT_DIRECTION, spotDir);
        glLightf(light, GL_SPOT_CUTOFF, 45.0f);
        glLightf(light, GL_SPOT_EXPONENT, 8.0f);
        glLightf(light, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(light, GL_LINEAR_ATTENUATION, 0.05f);
        glLightf(light, GL_QUADRATIC_ATTENUATION, 0.005f);
        glEnable(light);
    }

    drawSkybox(camX, camY, camZ);
    drawMountains();
    drawGround();
    drawAllLightCircles();
    drawAllShadowCircles();
    drawRoad();
    drawRoadLines();
    drawCar(0.0f, 21.0f, 0.0f);
    drawRelief();
    drawStaticObjects();
    drawAllLampPosts();
    // Umbra soarelui
    castShadow(sunPos[0], sunPos[1], sunPos[2], 1.0f);

    glutSwapBuffers();
}

//  RESHAPE

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)w / h, 1, 500);
    glMatrixMode(GL_MODELVIEW);
}


//  TASTATURA  (P3)
void keyboard(unsigned char key, int x, int y)
{
    float speed = 3.0f;

    // Rotatie orizontala
    if (key == 'a' || key == 'A') angleY -= 0.05f;
    if (key == 'd' || key == 'D') angleY += 0.05f;

    // Rotatie verticala
    if (key == 'w' || key == 'W') { angleX += 0.05f; if (angleX > 1.4f) angleX = 1.4f; }
    if (key == 's' || key == 'S') { angleX -= 0.05f; if (angleX < 0.05f) angleX = 0.05f; }

    // Miscare inainte / inapoi in directia in care priveste camera (plan XZ)
    // Directia "inainte" = vectorul de la camera spre target, proiectat pe XZ
    if (key == 'q' || key == 'Q') {
        targetX -= speed * sin(angleY);
        targetZ -= speed * cos(angleY);
    }
    if (key == 'e' || key == 'E') {
        targetX += speed * sin(angleY);
        targetZ += speed * cos(angleY);
    }

    // Zoom
    if (key == '+' || key == '=') { camDist -= 3.0f; if (camDist < 5.0f)  camDist = 5.0f; }
    if (key == '-') { camDist += 3.0f; if (camDist > 200.0f) camDist = 200.0f; }

    if (key == 27) exit(0);
    glutPostRedisplay();
}

// Mouse wheel zoom
void mouseWheel(int button, int dir, int x, int y)
{
    if (dir > 0) camDist -= 3.0f; else camDist += 3.0f;
    if (camDist < 5.0f)   camDist = 5.0f;
    if (camDist > 200.0f) camDist = 200.0f;
    glutPostRedisplay();
}

//  MAIN

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 600);
    glutCreateWindow("Scena P3 - Camera, Lumina, Umbre, Stalpi");

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);

    GLfloat ambient[] = { 0.15f,0.15f,0.2f,1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    GLfloat sunDiff[] = { 0.6f,0.6f,0.55f,1.0f }, sunSpec[] = { 0.3f,0.3f,0.3f,1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);

    GLfloat matAmb[] = { 0.3f,0.3f,0.3f,1.0f }, matDiff[] = { 0.8f,0.8f,0.8f,1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    grassTex = loadTexture("grass.bmp");
    skyTex = loadTexture("sky.bmp");
    rockTex = loadTexture("rock.bmp");
    reliefTex = loadTexture("relief.bmp");
    roadTex = loadTexture("road.bmp");
    barkTex = loadTexture("bark.bmp");
    leavesTex = loadTexture("leaves.bmp");
    buildingTex1 = loadTexture("building1.bmp");
    buildingTex2 = loadTexture("building2.bmp");
    buildingTex3 = loadTexture("building3.bmp");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseWheelFunc(mouseWheel);

    glutMainLoop();
    return 0;
}
