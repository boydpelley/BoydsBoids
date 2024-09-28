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

typedef struct Vector2
{
	GLfloat x, y;
}Vector2;

typedef struct Boid
{
	Vector2 position;
	Vector2 velocity;
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
#define FLOCK_SIZE 20
Boid currentFlock[FLOCK_SIZE];
Boid previousFlock[FLOCK_SIZE];
GLint boidSize = 1;
GLfloat flockSpeed = 0.1;

// Other variables
GLfloat smallConstant = 0.01;

GLfloat getDistance(GLfloat x1, GLfloat x2, GLfloat y1, GLfloat y2)
{
	return (GLfloat)sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

GLfloat getRadiansFromDegrees(GLfloat degrees)
{
	return degrees * (PI / 180.0);
}

void copyCurrentFlockToPrevious()
{
	for (int i = 0; i < FLOCK_SIZE; i++)
	{
		previousFlock[i].position = currentFlock[i].position;
		previousFlock[i].velocity = currentFlock[i].velocity;
		previousFlock[i].r = currentFlock[i].r;
		previousFlock[i].g = currentFlock[i].g;
		previousFlock[i].b = currentFlock[i].b;
	}
}

void initializeBoids()
{
	GLint spawnMinX = 50;
	GLint spawnMaxX = 450;
	GLint spawnMinY = 150;
	GLint spawnMaxY = 450;
	for (int i = 0; i < FLOCK_SIZE; i++)
	{
		currentFlock[i].position = (Vector2){ (rand() % (spawnMaxX - spawnMinX) + spawnMinX),
												(rand() % (spawnMaxY - spawnMinY) + spawnMinY) };

		GLfloat angle = (rand() % 360) * (PI / 180.0);
		printf("angle: %f \n", angle);
		currentFlock[i].velocity = (Vector2){ (cos(angle) * flockSpeed), (sin(angle) * flockSpeed) };

		currentFlock[i].r = 0.0;
		currentFlock[i].g = 0.0;
		currentFlock[i].b = 1.0;
		printf("X: %f, Y: %f, N: %d \n", currentFlock[i].position.x, currentFlock[i].position.y, i);
		printf("VX: %f, VY: %f, N: %d \n", currentFlock[i].velocity.x, currentFlock[i].velocity.y, i);
	}
	copyCurrentFlockToPrevious();
}

GLubyte inProximityOfHorizontal(int index)
{
	if (previousFlock[index].position.x > windowWidth - distanceThreshold) 
		//printf("Right hit "); 
		return 0x1;
	if (previousFlock[index].position.x < distanceThreshold) 
		//printf("Left hit "); 
		return 0x2;
	
	return 0x0;
}

GLubyte inProximityOfVertical(int index)
{
	if (previousFlock[index].position.y < subWindowHeight + distanceThreshold)
		//printf("Lower hit "); 
		return 0x4;
	if (previousFlock[index].position.y > windowHeight - distanceThreshold) 
		//printf("Upper hit "); 
		return 0x8;
	

	return 0x0;
}



//
//GENERAL PSEUDOCODE FOR ALGORITHM
// if this.position.position.x < proximity
//		new velocity = new vector((1/this.position.position.x) * constant, 0);
// Follow for other vectors
//
void updateDirection(int index, GLubyte proximity)
{
	Vector2 newVelocity = previousFlock[index].velocity;
	
	if (proximity & 0x1) // Right hit
	{
		newVelocity.x += (1.0 / (previousFlock[index].position.x - windowWidth)) * smallConstant;
	}
	else if (proximity & 0x2) // Left hit
	{
		newVelocity.x += (1.0 / previousFlock[index].position.x) * smallConstant;
	}


	if (proximity & 0x4) // Bottom hit
	{
		newVelocity.y += ((1.0 / previousFlock[index].position.y) * smallConstant);
	}
	else if (proximity & 0x8) // Top hit
	{
		newVelocity.y += ((1.0 / (previousFlock[index].position.y - windowHeight)) * smallConstant);
	}
	printf("VX: %f, VY: %f \n", newVelocity.x, newVelocity.y);
	currentFlock[index].velocity = newVelocity;

}

void updateBoids()
{
	for (int i = 0; i < FLOCK_SIZE; i++)
	{
		
		GLubyte inProximity = inProximityOfHorizontal(i) | inProximityOfVertical(i);
		if (inProximity > 0)
		{
			updateDirection(i, inProximity);
		}
		else
		{

		}
		currentFlock[i].position.x += currentFlock[i].velocity.x;
		currentFlock[i].position.y += currentFlock[i].velocity.y;
		if (currentFlock[i].position.x < 0)
			currentFlock[i].position.x = 0;
		if (currentFlock[i].position.x > windowWidth)
			currentFlock[i].position.x = windowWidth;
		if (currentFlock[i].position.y < 0)
			currentFlock[i].position.y = 0;
		if (currentFlock[i].position.y > windowHeight)
			currentFlock[i].position.y = windowHeight;
		printf("X: %f, Y: %f \n", currentFlock[0].position.x, currentFlock[0].position.y);
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
	GLfloat angleRads = atan2f(boid.velocity.y, boid.velocity.x);

	glPushMatrix();

	glTranslatef(boid.position.x, boid.position.y, 0.0f);
	glRotatef(angleRads * (180.0 / PI), 0.0f, 0.0f, 1.0f);

	glColor3f(boid.r, boid.g, boid.b);
	glBegin(GL_TRIANGLES);
	glVertex2f(10 * boidSize, 0);
	glVertex2f(-4 * boidSize, 4 * boidSize);
	glVertex2f(-4 * boidSize, -4 * boidSize);
	glEnd();

	glPopMatrix();
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