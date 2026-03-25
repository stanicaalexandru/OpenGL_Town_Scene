#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/freeglut.h>
#include <cmath>
#include <stdio.h>

GLuint grassTex, skyTex, rockTex, reliefTex, roadTex;
GLuint barkTex, leavesTex;
GLuint buildingTex1, buildingTex2, buildingTex3;
float angleY = 0;

// loader textura
GLuint loadTexture(const char* filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 3);

    if (!data) {
        printf("NU gaseste: %s\n", filename);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return texture;
}

// teren
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
// cerul
void drawSkybox(float camX, float camY, float camZ)
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, skyTex);
    glColor3f(1, 1, 1);

    float s = 110.0f;
    float yBottom = -300.0f;
    float yTop = 300.0f;

    glPushMatrix();
    glTranslatef(camX, camY, camZ);

    // spate
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, yBottom, -s);
    glTexCoord2f(1, 0); glVertex3f(s, yBottom, -s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, yTop, -s);
    glEnd();

    // fata
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, yBottom, s);
    glTexCoord2f(1, 0); glVertex3f(s, yBottom, s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, s);
    glTexCoord2f(0, 1); glVertex3f(-s, yTop, s);
    glEnd();

    // stanga
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, yBottom, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, yBottom, s);
    glTexCoord2f(1, 1); glVertex3f(-s, yTop, s);
    glTexCoord2f(0, 1); glVertex3f(-s, yTop, -s);
    glEnd();

    // dreapta
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, yBottom, -s);
    glTexCoord2f(1, 0); glVertex3f(s, yBottom, s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, s);
    glTexCoord2f(0, 1); glVertex3f(s, yTop, -s);
    glEnd();

    // sus
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, yTop, -s);
    glTexCoord2f(1, 0); glVertex3f(s, yTop, -s);
    glTexCoord2f(1, 1); glVertex3f(s, yTop, s);
    glTexCoord2f(0, 1); glVertex3f(-s, yTop, s);
    glEnd();

    glPopMatrix();

    glPopAttrib();
}

//Muntele 3D
void drawMountain3D(float x, float z, float base, float height)
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, rockTex);
    glColor3f(1, 1, 1);

    float yBase = -0.1f;   // putin infipt in iarba ca sa nu dea pe sus
    float half = base / 2.0f;

    // varf
    float topX = x;
    float topY = height;
    float topZ = z;

    // colturi baza
    float x1 = x - half, z1 = z - half;
    float x2 = x + half, z2 = z - half;
    float x3 = x + half, z3 = z + half;
    float x4 = x - half, z4 = z + half;

    glBegin(GL_TRIANGLES);

    // fata
    glTexCoord2f(0, 0);   glVertex3f(x1, yBase, z4);
    glTexCoord2f(1, 0);   glVertex3f(x3, yBase, z3);
    glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);

    // spate
    glTexCoord2f(0, 0);   glVertex3f(x2, yBase, z2);
    glTexCoord2f(1, 0);   glVertex3f(x4, yBase, z1);
    glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);

    // stanga
    glTexCoord2f(0, 0);   glVertex3f(x4, yBase, z1);
    glTexCoord2f(1, 0);   glVertex3f(x1, yBase, z4);
    glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);

    // dreapta
    glTexCoord2f(0, 0);   glVertex3f(x3, yBase, z3);
    glTexCoord2f(1, 0);   glVertex3f(x2, yBase, z2);
    glTexCoord2f(0.5, 1); glVertex3f(topX, topY, topZ);

    glEnd();

    glEnable(GL_LIGHTING);
}

// munti
void drawMountains()
{
    // lant muntos mai lat, care acopera fundalul
    drawMountain3D(-130.0f, -160.0f, 90.0f, 80.0f);
    drawMountain3D(-70.0f, -170.0f, 110.0f, 95.0f);
    drawMountain3D(0.0f, -180.0f, 130.0f, 115.0f);
    drawMountain3D(75.0f, -170.0f, 105.0f, 90.0f);
    drawMountain3D(140.0f, -160.0f, 85.0f, 75.0f);
}

// relief
void drawRelief()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, reliefTex);
    glColor3f(1, 1, 1);

    glPushMatrix();
    glTranslatef(15, 2, 5);
    glScalef(6, 3, 6);

    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluSphere(q, 1, 40, 40);
    gluDeleteQuadric(q);

    glPopMatrix();

    glEnable(GL_LIGHTING);
}

// circuit stradal oval
void drawRoad()
{
    glDisable(GL_LIGHTING);
    glBindTexture(GL_TEXTURE_2D, roadTex);
    glColor3f(1, 1, 1);

    float outerA = 40.0f;
    float outerB = 28.0f;
    float innerA = 28.0f;
    float innerB = 16.0f;

    int segments = 100;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++)
    {
        float t = 2.0f * 3.1415926f * i / segments;
        float ct = cos(t);
        float st = sin(t);

        glTexCoord2f(i / 5.0f, 0.0f);
        glVertex3f(outerA * ct, 0.02f, outerB * st);

        glTexCoord2f(i / 5.0f, 1.0f);
        glVertex3f(innerA * ct, 0.02f, innerB * st);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

//Banca desenata
void drawBench(float x, float z, float angle)
{
    glPushMatrix();
    glTranslatef(x, 0, z);
    glRotatef(angle, 0, 1, 0);

    // picioare
    glColor3f(0.2f, 0.2f, 0.2f);

    glPushMatrix();
    glTranslatef(-1.5f, 0.6f, 0.0f);
    glScalef(0.2f, 1.2f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.5f, 0.6f, 0.0f);
    glScalef(0.2f, 1.2f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // sezut
    glColor3f(0.6f, 0.3f, 0.1f);

    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glScalef(3.5f, 0.2f, 1.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // spatar
    glPushMatrix();
    glTranslatef(0.0f, 1.9f, -0.4f);
    glScalef(3.5f, 1.2f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}


//Masina de pe drum
void drawCar(float x, float z, float angle)
{
    glPushMatrix();
    glTranslatef(x, 0.8f, z);
    glRotatef(angle, 0, 1, 0);

    // caroserie jos
    glColor3f(0.8f, 0.0f, 0.0f);
    glPushMatrix();
    glScalef(3.0f, 0.8f, 1.8f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // caroserie sus
    glColor3f(0.9f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.2f, 0.7f, 0.0f);
    glScalef(1.8f, 0.7f, 1.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // roti
    glColor3f(0.1f, 0.1f, 0.1f);

    glPushMatrix();
    glTranslatef(-1.0f, -0.3f, 0.9f);
    glutSolidTorus(0.1, 0.25, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, -0.3f, 0.9f);
    glutSolidTorus(0.1, 0.25, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.0f, -0.3f, -0.9f);
    glutSolidTorus(0.1, 0.25, 10, 20);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, -0.3f, -0.9f);
    glutSolidTorus(0.1, 0.25, 10, 20);
    glPopMatrix();

    glPopMatrix();
}


//Marcaj pe mijloc
void drawRoadLines()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 0.0f);

    float midA = 34.0f;
    float midB = 22.0f;
    int segments = 48;

    for (int i = 0; i < segments; i += 2)
    {
        float t1 = 2.0f * 3.1415926f * i / segments;
        float t2 = 2.0f * 3.1415926f * (i + 1) / segments;

        glBegin(GL_LINES);
        glVertex3f(midA * cos(t1), 0.06f, midB * sin(t1));
        glVertex3f(midA * cos(t2), 0.06f, midB * sin(t2));
        glEnd();
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

//trunchi texturat
void drawTexturedTrunk()
{
    glBindTexture(GL_TEXTURE_2D, barkTex);
    glColor3f(1, 1, 1);

    float w = 0.4f;
    float h = 4.0f;

    glBegin(GL_QUADS);

    // fata
    glTexCoord2f(0, 0); glVertex3f(-w, 0, w);
    glTexCoord2f(1, 0); glVertex3f(w, 0, w);
    glTexCoord2f(1, 1); glVertex3f(w, h, w);
    glTexCoord2f(0, 1); glVertex3f(-w, h, w);

    // spate
    glTexCoord2f(0, 0); glVertex3f(w, 0, -w);
    glTexCoord2f(1, 0); glVertex3f(-w, 0, -w);
    glTexCoord2f(1, 1); glVertex3f(-w, h, -w);
    glTexCoord2f(0, 1); glVertex3f(w, h, -w);

    // stanga
    glTexCoord2f(0, 0); glVertex3f(-w, 0, -w);
    glTexCoord2f(1, 0); glVertex3f(-w, 0, w);
    glTexCoord2f(1, 1); glVertex3f(-w, h, w);
    glTexCoord2f(0, 1); glVertex3f(-w, h, -w);

    // dreapta
    glTexCoord2f(0, 0); glVertex3f(w, 0, w);
    glTexCoord2f(1, 0); glVertex3f(w, 0, -w);
    glTexCoord2f(1, 1); glVertex3f(w, h, -w);
    glTexCoord2f(0, 1); glVertex3f(w, h, w);

    glEnd();
}


//coroana texturata
void drawTexturedCrown()
{
    glBindTexture(GL_TEXTURE_2D, leavesTex);
    glColor3f(1, 1, 1);

    GLUquadric* q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluSphere(q, 2.0, 30, 30);
    gluDeleteQuadric(q);
}


// copac
void drawTree(float x, float z)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPushMatrix();
    glTranslatef(x, 0, z);

    // trunchi
    glPushMatrix();
    drawTexturedTrunk();
    glPopMatrix();

    // coroana
    glPushMatrix();
    glTranslatef(0, 5.2f, 0);
    drawTexturedCrown();
    glPopMatrix();

    glPopMatrix();

    glEnable(GL_LIGHTING);
}



// cladire
void drawBuilding(float x, float z, float sx, float sy, float sz)
{
    glPushMatrix();
    glTranslatef(x, sy / 2.0f, z);
    glScalef(sx, sy, sz);
    glColor3f(0.7f, 0.7f, 0.75f);
    glColor3f(0.8f, 0.7f, 0.6f);
    glutSolidCube(1.0);
    glPopMatrix();
}


//Cladirea texturata
void drawTexturedBuilding(float x, float z, float sx, float sy, float sz, GLuint tex)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor3f(1, 1, 1);

    float hx = sx / 2.0f;
    float hy = sy;
    float hz = sz / 2.0f;

    float tx = 2.0f;
    float ty = 3.0f;

    glPushMatrix();
    glTranslatef(x, 0, z);

    glBegin(GL_QUADS);

    // fata
    glTexCoord2f(0, 0);  glVertex3f(-hx, 0, hz);
    glTexCoord2f(tx, 0); glVertex3f(hx, 0, hz);
    glTexCoord2f(tx, ty);glVertex3f(hx, hy, hz);
    glTexCoord2f(0, ty); glVertex3f(-hx, hy, hz);

    // spate
    glTexCoord2f(0, 0);  glVertex3f(hx, 0, -hz);
    glTexCoord2f(tx, 0); glVertex3f(-hx, 0, -hz);
    glTexCoord2f(tx, ty);glVertex3f(-hx, hy, -hz);
    glTexCoord2f(0, ty); glVertex3f(hx, hy, -hz);

    // stanga
    glTexCoord2f(0, 0);  glVertex3f(-hx, 0, -hz);
    glTexCoord2f(tx, 0); glVertex3f(-hx, 0, hz);
    glTexCoord2f(tx, ty);glVertex3f(-hx, hy, hz);
    glTexCoord2f(0, ty); glVertex3f(-hx, hy, -hz);

    // dreapta
    glTexCoord2f(0, 0);  glVertex3f(hx, 0, hz);
    glTexCoord2f(tx, 0); glVertex3f(hx, 0, -hz);
    glTexCoord2f(tx, ty);glVertex3f(hx, hy, -hz);
    glTexCoord2f(0, ty); glVertex3f(hx, hy, hz);

    // sus
    glTexCoord2f(0, 0);  glVertex3f(-hx, hy, -hz);
    glTexCoord2f(tx, 0); glVertex3f(hx, hy, -hz);
    glTexCoord2f(tx, tx);glVertex3f(hx, hy, hz);
    glTexCoord2f(0, tx); glVertex3f(-hx, hy, hz);

    glEnd();

    glPopMatrix();

    glEnable(GL_LIGHTING);
}

//Acoperisul
void drawRoof(float x, float z, float sx, float sy, float sz)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(0.6f, 0.1f, 0.1f);

    glPushMatrix();
    glTranslatef(x, sy + 0.6f, z);
    glScalef(sx + 0.5f, 1.0f, sz + 0.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

//Functia pt cladire, formata din cladirea texturata si acoperis)
void drawNiceBuilding(float x, float z, float sx, float sy, float sz, GLuint tex)
{
    drawTexturedBuilding(x, z, sx, sy, sz, tex);
    drawRoof(x, z, sx, sy, sz);
}


// obiectele statice(pomi, banci, case, institutii)
void drawStaticObjects()
{
    // POMI (apropiati - mai mari)
    drawTree(-55, -12);
    drawTree(0, -45);
    drawTree(55, -12);

    // CLADIRI APROPIATE (MARI)
   // mai mici si mai naturale langa drum
    drawNiceBuilding(-85, -30, 10, 7, 8, buildingTex1);
    drawNiceBuilding(85, -30, 10, 7, 8, buildingTex2);

    drawNiceBuilding(-60, -55, 9, 6, 7, buildingTex3);
    drawNiceBuilding(60, -55, 9, 6, 7, buildingTex1);
  
    // ZONA DE MIJLOC (MEDIi)
    drawTree(-40, 25);
    drawTree(40, 25);

    drawNiceBuilding(-70, 30, 11, 7, 8, buildingTex2);
    drawNiceBuilding(70, 30, 11, 7, 8, buildingTex3);

    drawNiceBuilding(-30, 40, 10, 6, 7, buildingTex1);
    drawNiceBuilding(30, 40, 10, 6, 7, buildingTex2);

    // FUNDAL (MICI)
    drawTree(-25, 70);
    drawTree(25, 70);

    drawNiceBuilding(-60, 75, 8, 6, 6, buildingTex3);
    drawNiceBuilding(-20, 85, 7, 5, 5, buildingTex2);

    drawNiceBuilding(20, 85, 7, 5, 5, buildingTex1);
    drawNiceBuilding(60, 75, 8, 6, 6, buildingTex3);

    // EXTRA fundal(foarte mici)
    drawNiceBuilding(-10, 95, 6, 4, 4, buildingTex1);
    drawNiceBuilding(10, 95, 6, 4, 4, buildingTex2);

    // bancute langa cladiri din fata
    drawBench(-85, -20, 180);
    drawBench(85, -20, 180);

    // bancute langa cladiri laterale
    drawBench(-60, -50, 90);
    drawBench(60, -50, -90);

    // bancute zona mijloc
    drawBench(-70, 35, 180);
    drawBench(70, 35, 180);

    // bancute fundal
    drawBench(-30, 80, 180);
    drawBench(30, 80, 180);
}

// display
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float camX = 55.0f * sin(angleY);
    float camY = 22.0f;
    float camZ = 55.0f * cos(angleY);

    gluLookAt(camX, camY, camZ,
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f);

    drawSkybox(camX, 22.0f, camZ);
    drawMountains();
    drawGround();
    drawRoad();
    drawRoadLines();
    drawCar(0.0f, 21.0f, 0.0f);
    drawRelief();
    drawStaticObjects();

    glutSwapBuffers();
}

// reshape
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)w / h, 1, 200);

    glMatrixMode(GL_MODELVIEW);
}

// tastatura
void keyboard(unsigned char key, int x, int y)
{
    if (key == 'a') angleY -= 0.05f;
    if (key == 'd') angleY += 0.05f;
    glutPostRedisplay();
}

// main
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 600);
    glutCreateWindow("Scena in cub - laborator");
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 0,50,50,1 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

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

    glutMainLoop();
    return 0;
}