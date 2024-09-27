/***********************************************************************************************
*	Boyd's Boids
*	
*	Authour: Boyd Pelley - B00919714
*	Email: boydp@dal.ca
* 
*	Description: A program written with OpenGL that simulates a flock of bird-oids flying across
*	the display window. Handles collisions against walls, and also handles potential collisions 
*	against other bird-oids, so they reflect off of each other.
************************************************************************************************/

#include <freeglut.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265359

typedef struct Boid
{
	GLfloat x, y;
	GLfloat direction;
	GLfloat speed;
	GLfloat r, g, b;
} Boid;

// Global mouse variables
GLint mousePressed = 0;
GLfloat mouseX, mouseY;

// Window Variables
GLint windowHeight = 500;
GLint windowWidth = 500;
GLint subWindowHeight = 100;
GLint distanceThreshold = 25;
GLint spawnThreshold = 50;

// Flock variables
#define FLOCK_SIZE 2
Boid currentFlock[FLOCK_SIZE];
Boid previousFlock[FLOCK_SIZE];
GLint boidSize = 15;

// Other variables
GLfloat smallConstant = 0.1;

GLfloat getDistance(GLfloat x1, GLfloat x2, GLfloat y1, GLfloat y2)
{
	return (GLfloat)sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

GLfloat getRadiansFromDegrees(GLfloat degrees)
{
	return degrees * (PI / 180.0);
}

void initializeBoids()
{
	GLint spawnMinX = 50;
	GLint spawnMaxX = 450;
	GLint spawnMinY = 150;
	GLint spawnMaxY = 450;
	for (int i = 0; i < FLOCK_SIZE; i++)
	{
		currentFlock[i].x = rand() % (spawnMaxX - spawnMinX) + spawnMinX;
		currentFlock[i].y = rand() % (spawnMaxY - spawnMinY) + spawnMinY;

		currentFlock[i].direction = rand() % 365;
		currentFlock[i].speed = 0.01;

		currentFlock[i].r = 0.0;
		currentFlock[i].g = 0.0;
		currentFlock[i].b = 1.0;
		printf("X: %f, Y: %f, N: %d \n", currentFlock[i].x, currentFlock[i].y, i);
	}
}

void copyCurrentFlockToPrevious()
{
	for (int i = 0; i < FLOCK_SIZE; i++)
	{
		previousFlock[i].x = currentFlock[i].x;
		previousFlock[i].y = currentFlock[i].y;
		previousFlock[i].direction = currentFlock[i].direction;
		previousFlock[i].speed = currentFlock[i].speed;
		previousFlock[i].r = currentFlock[i].r;
		previousFlock[i].g = currentFlock[i].g;
		previousFlock[i].b = currentFlock[i].b;
	}
}

GLubyte inProximityOfHorizontal(int index)
{
	if (previousFlock[index].x > windowWidth - distanceThreshold) 
		//printf("Right hit "); 
		return 0x1;
	if (previousFlock[index].x < distanceThreshold) 
		//printf("Left hit "); 
		return 0x2;
	
	return 0x0;
}

GLubyte inProximityOfVertical(int index)
{
	if (previousFlock[index].y < subWindowHeight + distanceThreshold)
		//printf("Lower hit "); 
		return 0x4;
	if (previousFlock[index].y > windowHeight - distanceThreshold) 
		//printf("Upper hit "); 
		return 0x8;
	

	return 0x0;
}

void updateDirection(int index, GLubyte proximity)
{
	GLfloat newDirection = previousFlock[index].direction;

	GLfloat inverseLeft = 1.0 / fabs(previousFlock[index].x - distanceThreshold);
	GLfloat inverseRight = 1.0 / fabs(windowWidth - previousFlock[index].x - distanceThreshold);
	GLfloat inverseBottom = 1.0 / fabs(previousFlock[index].y - subWindowHeight - distanceThreshold);
	GLfloat inverseTop = 1.0 / fabs(windowHeight - previousFlock[index].y - distanceThreshold);
	

	if (proximity & 0x1) newDirection -= inverseRight * smallConstant;
	else if (proximity & 0x2) newDirection += inverseLeft * smallConstant;


	if (proximity & 0x4) newDirection += inverseBottom * smallConstant;
	else if (proximity & 0x8) newDirection -= inverseTop * smallConstant;

	if (newDirection >= 360) newDirection -= 360;
	if (newDirection < 0) newDirection += 360;

	currentFlock[index].direction = newDirection;
}

void updateBoids()
{
	for (int i = 0; i < FLOCK_SIZE; i++)
	{
		
		GLubyte inProximity = inProximityOfHorizontal(i) | inProximityOfVertical(i);
		if (inProximity > 0)
		{
			//printf("MASK: %x\n", inProximity);
			updateDirection(i, inProximity);
		}
		else
		{

		}
		GLfloat vx = previousFlock[i].speed * cos(getRadiansFromDegrees(currentFlock[i].direction));
		GLfloat vy = previousFlock[i].speed * sin(getRadiansFromDegrees(currentFlock[i].direction));

		currentFlock[i].x += vx;
		currentFlock[i].y += vy;
	}
}

void initializeGL(void)
{
	glClearColor(0, 0, 0, 1);

	glColor3f(1.0, 0.0, 0.0);
	glPointSize(8);

	gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);
}

void drawBoids(Boid boid)
{
	glColor3f(boid.r, boid.g, boid.b);
	glBegin(GL_TRIANGLES);
	glVertex2f(boid.x, boid.y);
	glVertex2f(boid.x - boidSize / 3, boid.y - boidSize);
	glVertex2f(boid.x + boidSize / 3, boid.y - boidSize);
	glEnd();
}

void myDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);

	if (mousePressed)
	{
		glBegin(GL_POINTS);
			glVertex2f(mouseX, mouseY);
		glEnd();
	} 
	else 
	{
		for (int i = 0; i < FLOCK_SIZE; i++)
		{
			drawBoids(currentFlock[i]);
		}
	}
	glFlush();
}

void idleBoids()
{
	updateBoids();
	copyCurrentFlockToPrevious();
	glutPostRedisplay();
}

void handleClick(int button, int state, int x, int y)
{
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		exit(0);
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		mousePressed = 1;

		mouseX = (GLfloat)x / (GLfloat)windowWidth;

		mouseY = (GLfloat)windowHeight - (GLfloat)y;
		mouseY = mouseY / (GLfloat)windowHeight;

		glutPostRedisplay();
	}
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Boyd's Boids");

	initializeBoids();

	glutDisplayFunc(myDisplay);
	glutIdleFunc(idleBoids);
	glutMouseFunc(handleClick);

	initializeGL();

	glutMainLoop();
}