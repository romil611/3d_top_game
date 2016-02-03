#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <stdio.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "imageloader.h"
#include "vec3f.h"
using namespace std;

#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)

class Target
{
public:
	float tarx, tarz, TARR1, TARR2, TARR3;
	Target()
	{
		TARR1 = 2.5f;
		TARR3 = 3.5f;
		TARR2 = 3.0f;
	} 
};

class Top
{
	public:
		float latitude_X, latitude_Z, lattuVX, lattuVZ, lath1, Latitude_Y;
		float latr1, latr2, lath2, latn1, latn2;
		float VelMod;
		int score,angle;
		Top()
		{
			latn1 = 0.1;
			latn2 = 0.3;
			latitude_X = 0.0f;
			latitude_Z = 127.0f;
			lattuVX = 0.0f;
			lattuVZ = 0.0f;
			lath1 = 2.5f;
			latr1 = 1.2f;
			latr2 = 1.0f;
			lath2 = 2.0f;
			VelMod = 0.0; 
			score = 0;
			angle = 0;
		}
};

//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
	private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) {
			w = w2;
			l = l2;
			
			hs = new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
			}
			
			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}
			
			computedNormals = false;
		}
		
		~Terrain() {
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
			}
			delete[] hs;
			
			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}
		
		int width() {
			return w;
		}
		
		int length() {
			return l;
		}
		
		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];
					
					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for(int y = 0; y < image->height; y++) {
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}
	
	delete image;
	t->computeNormals();
	return t;
}

Top TOP;
Target MARKSPOT;
Terrain* ter;

Vec3f g(0.0, -1.0, 0.0);
float dott;
Vec3f gt;

int NetSpeed = 1;
int score =11;
float ang = 0.0f;
float ang2 = -30.0f;
float ang3 =  0.0f;
float ang4 = 0.0f;
float friction = 0.999f;
bool wasMoving = false;


int detectCollision4()
{
	if(pow((TOP.latitude_X - MARKSPOT.tarx), 2) < 5 && pow((TOP.latitude_Z - MARKSPOT.tarz), 2) < 5){
		score+=10;
		return 1;
	}
	return 0;
}

void cleanup() {
	delete ter;
}

void writeOnBoard(float x, float y, float z,std::string s, float r, float g, float b) {
	int len, i;
	glPushMatrix();
	glColor3f(r,g,b);
	glRasterPos3f(x,y,z);
	len = s.size();
	const char *ss = s.c_str();
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ss[i]);
	}
	glPopMatrix();
}

int detectBoundary4()
{
	if(TOP.latitude_X >= 127 ||  TOP.latitude_Z <= 0 || TOP.latitude_X <=0){
		score-=1;
		if(score<0)
			exit(0);
		return 1;
	}
	return 0;
}

void reset(){
		TOP.latitude_X = 64.0f;
		TOP.latitude_Z = 127.0f;
		TOP.lattuVX = 0.0f;
		TOP.lattuVZ = 0.0f;
		TOP.angle = 0;
		NetSpeed = 1;
		wasMoving=false;
		MARKSPOT.tarx = rand()%128;
		MARKSPOT.tarz = rand()%128;
		if(MARKSPOT.tarz >= 50)
			MARKSPOT.tarz -= 40;
}


int hasStopped(){
	if(TOP.lattuVX <= 0.09f && TOP.lattuVZ <=0.09f && wasMoving==true){
		score-=1;
		if(score<0)
			exit(0);
		return 1;
	}
	return 0;
}

void update(int value) {
	
	glutPostRedisplay();
	ang4+=10;

	if(detectCollision4() || detectBoundary4() || hasStopped())
		reset();
	TOP.latitude_X+=TOP.lattuVX;
	TOP.latitude_Z-=TOP.lattuVZ;
	TOP.lattuVX=TOP.lattuVX*friction;
	TOP.lattuVZ=TOP.lattuVZ*friction;
	glutTimerFunc(25, update, 0);
}


void handleKeypress(unsigned char key, int x, int y) {
	switch (key) {
		case 27: //Escape key
			cleanup();
			exit(0);
		case 'a':
			ang2 = -30.0;
			ang += 1.0f;
			if (ang > 360)
				ang -= 360;
			break;
		case 'd':
			ang2 = -30.0;
			ang-=1.0f;
			if(ang < 0)
				ang +=360;
			break;
		case 'w':
			ang2 = -90.0f;
			break;
		case 's':
			ang2 = 90.0f;
			break;
		case 'x':
		if(TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f){
			if(TOP.angle!=-90)
				TOP.angle-=5;
		}
			break;
		case 'z':
		if(TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f){
			if(TOP.angle!=90)
				TOP.angle += 5;
		}
			break;
		case ' ':
		if(TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f){
			TOP.VelMod = NetSpeed*0.1;
			TOP.lattuVZ = TOP.VelMod*cos(DEG2RAD(-TOP.angle));
			TOP.lattuVX = TOP.VelMod*sin(DEG2RAD(-TOP.angle));
			wasMoving =true;
		}
			break;
		case 'r':
		if(TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f){
			MARKSPOT.tarx = rand()%128;
			MARKSPOT.tarz = rand()%128;
			if(MARKSPOT.tarz >= 50)
				MARKSPOT.tarz -= 40;
		}
			break;
		default:
			break;
	}
}

void HandleSpecKeys4(int key, int x, int y)
{
	if (key == GLUT_KEY_UP )
    {
    	if(NetSpeed!=10 && TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f)
    	NetSpeed++;
    }
    else if(key == GLUT_KEY_DOWN && TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f)
    {
    	if(NetSpeed!=0) 
    	NetSpeed--;
    }
    else if(key == GLUT_KEY_LEFT && TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f)
    {
    	if(TOP.latitude_X!=0.0)
    		TOP.latitude_X-=1.0;
    }
    else if(key == GLUT_KEY_RIGHT && TOP.lattuVZ<=0.001f && TOP.lattuVX<=0.001f)
    {
    	if(TOP.latitude_X!=127.0)
    		TOP.latitude_X+=1.0;
    }
}




void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}


void Target()
{
	glPushMatrix();
		glTranslatef(MARKSPOT.tarx, ter->getHeight(MARKSPOT.tarx, MARKSPOT.tarz)+MARKSPOT.TARR1+1, MARKSPOT.tarz);
  		glColor3f(0.0, 1.0, 1.0);
  		glutWireTorus(1, MARKSPOT.TARR2, 10, 10);
  		glColor3f(1.0, 0.0, 1.0);
  		glutWireTorus(1, MARKSPOT.TARR1, 10, 10);
  		glColor3f(1.0, 0.0, 0.0);
  		glutWireSphere(MARKSPOT.TARR1-1, 32, 32);
  	glPopMatrix();
}


void drawCube(float z, int i)
{
	glPushMatrix();
		glTranslatef(135.0, 5.0, z-4.0 );
		glColor3f((i*28.4)/256.0, ((9-i)*28.4)/256.0, 0.0);
		glRotatef(1.5*ang4, 1.0, 1.0, 1.0);
		glPushMatrix();
			glutWireTorus( 0.54, 3, 5, 10);
		glPopMatrix();
		glPushMatrix();
			glRotatef(60,0.0,1.0,0.0);
			glutWireTorus( 0.54, 3, 5, 10);
		glPopMatrix();
		glPushMatrix();
			glRotatef(120,0.0,1.0,0.0);
			glutWireTorus( 0.54, 3, 5, 10);
		glPopMatrix();
		glPushMatrix();
			glRotatef(90,1.0,0.0,0.0);
			glutWireTorus( 0.54, 3, 5, 10);
		glPopMatrix();
	glPopMatrix();
}

void drawDirectionVector(){
	glPushMatrix();
		glTranslatef(-10,ter->getHeight(0,100),100);
		GLUquadricObj *quadObj = gluNewQuadric();
		glRotatef(180,1.0,0.0,0.0);
		glRotatef(-TOP.angle,0.0,1.0,0.0);
		gluCylinder(quadObj,0.55,0.55,7,150,150);
		glPushMatrix();
			glTranslatef(0.0,0.0,7.0);
			glRotatef(-135,0.0,1.0,0.0);
			GLUquadricObj *quadObj1 = gluNewQuadric();
			gluCylinder(quadObj1,0.41,0.41,3.5,150,150);
		glPopMatrix();
		glPushMatrix();
			glTranslatef(0.0,0.0,7.0);
			glRotatef(135,0.0,1.0,0.0);
			GLUquadricObj *quadObj2 = gluNewQuadric();
			gluCylinder(quadObj2,0.41,0.41,3.5,150,150);
		glPopMatrix();
	glPopMatrix();

}

void drawLattu(Vec3f nor)
{
	glPushMatrix();
		
			glTranslatef(TOP.latitude_X, ter->getHeight(TOP.latitude_X,TOP.latitude_Z)+0.3, TOP.latitude_Z);
			glRotatef(90.0, nor[0], nor[1], nor[2]);
			glRotatef(ang4, 0.0, 1.0, 0.0);
			glPushMatrix();
			glColor3f(0.20, 0.20, 0.40);
			glTranslatef(0.0,2.7,0.0);	
			glRotatef(90,1.0,0.0,0.0);
			glutSolidCone(3,2.7,50,50);
			glPopMatrix();
			glPushMatrix();
			glColor3f(0.30, 0.30, 0.50);
			glTranslatef(0.0,2.7,0.0);
			glRotatef(90,1.0,0.0,0.0);
			GLUquadricObj *quadObj = gluNewQuadric();
			gluCylinder(quadObj,3,3,0.3,150,150);
			glutWireTorus( 0.54, 3, 7, 20);
			glPopMatrix();
			glPushMatrix();
			glColor3f(0.40, 0.40, 0.60);
			glTranslatef(0.0,3.0,0.0);
			glRotatef(270,1.0,0.0,0.0);
			GLUquadricObj *quadObj1 = gluNewQuadric();
			gluCylinder(quadObj1,3,0.2,2,150,150);
			glPopMatrix();
			glPushMatrix();
			glColor3f(0.50, 0.50, 0.50);
			glTranslatef(0.0,5,0.0);
			glRotatef(270,1.0,0.0,0.0);
			GLUquadricObj *quadObj2 = gluNewQuadric();
			gluCylinder(quadObj2,0.2,0.70,1.4,5,150);

			glPopMatrix();
			glPopMatrix();
}

void drawScoreboard() {
	std::string string23 = std::to_string(score);
	string23 = "Score : " + string23;
	writeOnBoard(0,0,-12, string23,0.340,0.870,0.820);
}

void drawScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -7.8f);
    glRotatef(-ang2, 1.0f, 0.0f, 0.0f);
	glRotatef(-ang, 0.0f, 1.0f, 0.0f);
	
	GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
	
	GLfloat lightColor0[] = {0.6f, 1.0f, 0.6f, 1.0f};
	GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	
	float scale = 6.0f / max(ter->width() - 1, ter->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float)(ter->width() - 1) / 2,
				 0.0f,
				 -(float)(ter->length() - 1) / 2);
	
	glColor3f(0.3, 0.4, 0.3);
	for(int z = 0; z < ter->length() - 1; z++)
	{
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for(int x = 0; x < ter->width(); x++) {
			Vec3f normal = ter->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, ter->getHeight(x, z), z);
			normal = ter->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, ter->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}
	Vec3f nor = ter->getNormal(TOP.latitude_X, TOP.latitude_Z);
	dott = nor.dot(g);
	gt = g + nor*dott;
	drawLattu(nor);
	for (int i = 0; i < NetSpeed; ++i)
	{
		drawCube(127.0 - i*10.0, i);
	}
	Target();
	drawScoreboard();
	drawDirectionVector();
	glutSwapBuffers();
}


int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	int windowWidth = glutGet(GLUT_SCREEN_WIDTH);
    int windowHeight = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	
	glutCreateWindow("Game");
	initRendering();
	MARKSPOT.tarx = rand()%128;
	MARKSPOT.tarz = rand()%128;
	if(MARKSPOT.tarz >= 64)
		MARKSPOT.tarz -= 50;
	ter = loadTerrain("map.bmp", 20);
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutSpecialFunc(HandleSpecKeys4);
	glutReshapeFunc(handleResize);

	glutTimerFunc(25, update, 0);
	
	glutMainLoop();
	return 0;
}
