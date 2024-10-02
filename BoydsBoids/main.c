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
GLint pauseState = 0;

// Global keyboard variables
GLint boidState = -1;

// Window Variables
GLint windowHeight = 500;
GLint windowWidth = 500;
GLint subWindowHeight = 100;
GLint distanceThreshold = 25;
GLint spawnThreshold = 45;

// Flock variables
#define FLOCK_SIZE 40
#define NUMBER_NEIGHBOURS 6
Boid currentFlock[FLOCK_SIZE];
Boid previousFlock[FLOCK_SIZE];
GLint boidSize = 1;
GLfloat flockSpeed = 0.01;
GLfloat boidDistance = 20;

// Other variables
GLfloat wallAvoidanceFactor = 0.00001;
GLfloat boidAvoidanceFactor = 0.000007;
GLfloat boidAlignmentFactor = 0.0000002;
GLfloat boidCohesionFactor = 0.0000005;

GLfloat getDistance(GLfloat x1, GLfloat x2, GLfloat y1, GLfloat y2)
{
	return (GLfloat)sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

GLfloat getRadiansFromDegrees(GLfloat degrees)
{
	return degrees * (PI / 180.0);
}

GLfloat getMagnitude(GLfloat x, GLfloat y)
{
	return (GLfloat)sqrt(x * x + y * y);
}

void normalize(Vector2 * vector)
{
	GLfloat length = sqrt(vector->x * vector->x + vector->y * vector->y);
	if (length != 0)
	{
		vector->x /= length;
		vector->y /= length;
	}
}

void applyFactor(Vector2* vector, GLfloat factor)
{
	vector->x *= factor;
	vector->y *= factor;
}

void copyCurrentFlockToPrevious()
{
	for (GLint i = 0; i < FLOCK_SIZE; i++)
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
	GLint spawnMinX = spawnThreshold;
	GLint spawnMaxX = windowWidth - spawnThreshold;
	GLint spawnMinY = subWindowHeight + spawnThreshold;
	GLint spawnMaxY = windowHeight - spawnThreshold;
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		currentFlock[i].position = (Vector2){ (rand() % (spawnMaxX - spawnMinX) + spawnMinX),
												(rand() % (spawnMaxY - spawnMinY) + spawnMinY) };

		GLfloat angle = (rand() % 360) * (PI / 180.0);
		currentFlock[i].velocity = (Vector2){ (cos(angle) * flockSpeed), (sin(angle) * flockSpeed) };

		currentFlock[i].r = 0.0;
		currentFlock[i].g = 0.0;
		currentFlock[i].b = 1.0;
	}
	copyCurrentFlockToPrevious();
}

typedef struct boidNeighbours
{
	GLfloat distance;
	GLint  index;
} boidNeighbours;

// Bubble swap for Quicksort
void swap(boidNeighbours* a, boidNeighbours* b)
{
	boidNeighbours temp = *a;
	*a = *b;
	*b = temp;
}

GLint partition(boidNeighbours arr[], GLint low, GLint high)
{
	GLfloat pivot = arr[high].distance;

	GLint i = low - 1;

	for (GLint j = low; j <= high; j++)
	{
		if (arr[j].distance < pivot)
		{
			i++;
			swap(&arr[i], &arr[j]);
		}
	}

	swap(&arr[i + 1], &arr[high]);
	return i + 1;
}

void quicksort(boidNeighbours arr[], GLint low, GLint high)
{
	if (low < high)
	{
		GLint pi = partition(arr, low, high);
		quicksort(arr, low, pi - 1);
		quicksort(arr, pi + 1, high);
	}
}

void findNearestNeighboursIndex(Boid boid, GLint index, GLint*nearestNeighboursIndexes)
{
	GLint  sortedBoids[FLOCK_SIZE];
	boidNeighbours neighbours[FLOCK_SIZE];

	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		if (i != index)
		{
			neighbours[i].distance = getDistance(boid.position.x, currentFlock[i].position.x, boid.position.y, currentFlock[i].position.y);
			neighbours[i].index = i;
		}
		else
		{
			neighbours[i].distance = windowHeight;
			neighbours[i].index = i;
		}
	}
	quicksort(neighbours, 0, FLOCK_SIZE - 1);

	for (GLint i = 0; i < NUMBER_NEIGHBOURS; i++)
	{
		nearestNeighboursIndexes[i] = neighbours[i].index;
	}
}

GLubyte inProximityOfHorizontal(GLint index)
{
	if (previousFlock[index].position.x > windowWidth - distanceThreshold)  
		return 0x1;
	if (previousFlock[index].position.x < distanceThreshold) 
		return 0x2;
	
	return 0x0;
}

GLubyte inProximityOfVertical(GLint index)
{
	if (previousFlock[index].position.y < subWindowHeight + distanceThreshold)
		return 0x4;
	if (previousFlock[index].position.y > windowHeight - distanceThreshold) 
		return 0x8;
	
	return 0x0;
}

void avoidWalls(GLint index, GLubyte proximity)
{
	Vector2 newVelocity = previousFlock[index].velocity;
	
	if (proximity & 0x1) // Right hit
	{
		newVelocity.x += (1.0 / (previousFlock[index].position.x - windowWidth)) * wallAvoidanceFactor;
	}
	else if (proximity & 0x2) // Left hit
	{
		newVelocity.x += (1.0 / previousFlock[index].position.x) * wallAvoidanceFactor;
	}

	if (proximity & 0x4) // Bottom hit
	{
		newVelocity.y += ((1.0 / previousFlock[index].position.y) * wallAvoidanceFactor);
	}
	else if (proximity & 0x8) // Top hit
	{
		newVelocity.y += ((1.0 / (previousFlock[index].position.y - windowHeight)) * wallAvoidanceFactor);
	}

	currentFlock[index].velocity = newVelocity;

}

void handleBoidRules(GLint i, GLint* nearestNeighbours)
{
	Vector2 alignment = { 0, 0 };
	Vector2 cohesion = { 0, 0 };
	Vector2 separation = { 0, 0 };

	for (int j = 0; j < NUMBER_NEIGHBOURS; j++)
	{
		GLint neighbour = nearestNeighbours[j];

		alignment.x += previousFlock[neighbour].velocity.x;
		alignment.y += previousFlock[neighbour].velocity.y;

		cohesion.x += previousFlock[neighbour].position.x;
		cohesion.y += previousFlock[neighbour].position.y;

		GLfloat distance = getDistance(previousFlock[i].position.x, previousFlock[neighbour].position.x,
			previousFlock[i].position.y, previousFlock[neighbour].position.y);

		if (distance < boidDistance)
		{
			Vector2 directionAway =
			{
				previousFlock[i].position.x - previousFlock[neighbour].position.x,
				previousFlock[i].position.y - previousFlock[neighbour].position.y
			};

			normalize(&directionAway);

			directionAway.x *= (1.0 / distance) * boidAvoidanceFactor;
			directionAway.y *= (1.0 / distance) * boidAvoidanceFactor;

			separation.x += directionAway.x;
			separation.y += directionAway.y;
		}
	}

	alignment.x /= NUMBER_NEIGHBOURS;
	alignment.y /= NUMBER_NEIGHBOURS;

	alignment.x -= previousFlock[i].velocity.x;
	alignment.y -= previousFlock[i].velocity.y;

	normalize(&alignment);
	applyFactor(&alignment, boidAlignmentFactor);

	currentFlock[i].velocity.x += alignment.x;
	currentFlock[i].velocity.y += alignment.y;

	cohesion.x /= NUMBER_NEIGHBOURS;
	cohesion.y /= NUMBER_NEIGHBOURS;

	cohesion.x -= previousFlock[i].position.x;
	cohesion.y -= previousFlock[i].position.y;

	normalize(&cohesion);
	applyFactor(&cohesion, boidCohesionFactor);

	currentFlock[i].velocity.x += cohesion.x;
	currentFlock[i].velocity.y += cohesion.y;

	currentFlock[i].velocity.x += separation.x;
	currentFlock[i].velocity.y += separation.y;

	GLfloat currentSpeed = getMagnitude(currentFlock[i].velocity.x, currentFlock[i].velocity.y);

	if (currentSpeed > flockSpeed)
	{
		currentFlock[i].velocity.x = (currentFlock[i].velocity.x / currentSpeed) * flockSpeed;
		currentFlock[i].velocity.y = (currentFlock[i].velocity.y / currentSpeed) * flockSpeed;
	}

}

void setAllBoidsColour()
{
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		currentFlock[i].r = 0.0f;
		currentFlock[i].g = 0.0f;
		currentFlock[i].b = 1.0f;
	}
}

void handleBoidState(GLint i, GLint* nearestNeighbours)
{
	currentFlock[i].r = 1.0f;
	currentFlock[i].g = 0.0f;
	currentFlock[i].b = 0.0f;

	for (GLint i = 0; i < NUMBER_NEIGHBOURS; i++)
	{
		GLint neighbour = nearestNeighbours[i];
		currentFlock[neighbour].r = 0.0f;
		currentFlock[neighbour].g = 1.0f;
		currentFlock[neighbour].b = 0.0f;
	}
}

void updateBoids()
{
	setAllBoidsColour();
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		GLint nearestNeighbours[NUMBER_NEIGHBOURS];
		findNearestNeighboursIndex(currentFlock[i], i, nearestNeighbours);

		if (i == boidState)
		{
			handleBoidState(boidState, nearestNeighbours);
		}
		
		GLubyte inProximity = inProximityOfHorizontal(i) | inProximityOfVertical(i);
		if (inProximity > 0)
		{
			avoidWalls(i, inProximity);
		}
		else
		{
			handleBoidRules(i, nearestNeighbours);
		}
		currentFlock[i].position.x += currentFlock[i].velocity.x;
		currentFlock[i].position.y += currentFlock[i].velocity.y;
	}
}

void initializeGL(void)
{
	glClearColor(0, 0, 0, 1);

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
	glVertex2f(8 * boidSize, 0);
	glVertex2f(-3 * boidSize, 3 * boidSize);
	glVertex2f(-3 * boidSize, -3 * boidSize);
	glEnd();

	glPopMatrix();
}

void drawUI()
{
	glColor3f(0.5f, 0.9f, 0.7f);
	glBegin(GL_POLYGON);
		glVertex2d(0, 0);
		glVertex2d(0, subWindowHeight);
		glVertex2d(windowWidth, subWindowHeight);
		glVertex2d(windowWidth, 0);
	glEnd();

	glColor3f(0.5f, 1.0f, 0.7f);
	glBegin(GL_POLYGON);
		glVertex2d(0, subWindowHeight - 25);
		glVertex2d(0, subWindowHeight - 20);
		glVertex2d(windowWidth, subWindowHeight - 20);
		glVertex2d(windowWidth, subWindowHeight - 25);
	glEnd();

	GLint midXSubWindow = windowWidth / 2;
	GLint midYSubWindow = subWindowHeight / 2;
	GLint buttonWidth = 75;
	GLint buttonHeight = 15;
	GLint offset = 2;

	if (pauseState)
	{
		glColor3f(0.5f, 1.0f, 0.7f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth - offset, midYSubWindow - buttonHeight + offset);
			glVertex2d(midXSubWindow - buttonWidth - offset, midYSubWindow + buttonHeight + offset);
			glVertex2d(midXSubWindow + buttonWidth - offset, midYSubWindow + buttonHeight + offset);
			glVertex2d(midXSubWindow + buttonWidth - offset, midYSubWindow - buttonHeight + offset);
		glEnd();

		glColor3f(0.4f, 0.8f, 0.6f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow - buttonHeight);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow - buttonHeight);
		glEnd();
	}
	else
	{
		glColor3f(0.3f, 0.7f, 0.5f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth + offset, midYSubWindow - buttonHeight - offset);
			glVertex2d(midXSubWindow - buttonWidth + offset, midYSubWindow + buttonHeight - offset);
			glVertex2d(midXSubWindow + buttonWidth + offset, midYSubWindow + buttonHeight - offset);
			glVertex2d(midXSubWindow + buttonWidth + offset, midYSubWindow - buttonHeight - offset);
		glEnd();

		glColor3f(0.5f, 1.0f, 0.7f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow - buttonHeight);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow - buttonHeight);
		glEnd();
	}
}

void myDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);

	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		drawBoids(currentFlock[i]);
	}

	drawUI();

	if (mousePressed)
	{
		glColor3f(1.0, 0.0, 0.0);
		glPointSize(8);
		glBegin(GL_POINTS);
		glVertex2f(mouseX, mouseY);
		glEnd();
	}

	glFlush();
}

void idleBoids()
{
	if (pauseState == 0)
	{
		updateBoids();
		copyCurrentFlockToPrevious();
		glutPostRedisplay();
	}
}

void handleClick(GLint button, GLint state, GLint x, GLint y)
{
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		exit(0);
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		mousePressed = 1;

		mouseX = (GLfloat)x;
		mouseY = (GLfloat)windowHeight - (GLfloat)y;

		printf("mouseX: %f, mouseY: %f\n", mouseX, mouseY);

		if (mouseX >= 175 && mouseX <= 325 && mouseY >= 35 && mouseY <= 65)
		{
			if (pauseState == 0) pauseState = 1;
			else if (pauseState == 1) pauseState = 0;
		}

		glutPostRedisplay();
	}
}

void handleSpecialKeyboard(unsigned char key, GLint x, GLint y)
{
	if (key == GLUT_KEY_PAGE_UP || key == GLUT_KEY_UP)
	{
		flockSpeed += 0.0001;
		printf("Speed: %f\n", flockSpeed);
		fflush(stdout);
	}
	else if (key == GLUT_KEY_PAGE_DOWN || key == GLUT_KEY_UP)
	{
		flockSpeed -= 0.0001;
		printf("Speed: %f\n", flockSpeed);
		fflush(stdout);
	}
}

void handleKeyboard(unsigned char key, GLint x, GLint y)
{
	if (key >= '1' && key <= '9')
	{
		boidState = key - 1 - '0';
	}
	else if (key == '0')
	{
		boidState = -1 - '0';
	}
	else if (key == 'Q' || key == 'q')
	{
		exit(0);
	}
	else if (key == 'P' || key == 'p')
	{
		if (pauseState == 0) pauseState = 1;
		else if (pauseState == 1) pauseState = 0;
	}
}

void initialPrintStatement()
{
	printf("\n\n");
	printf("Scene Controls\n");
	printf("-----------------\n");
	printf("Page Up   : faster\n");
	printf("Page Down : slower\n");
	printf("[1-9]     : highlight boid and its neighbours\n");
	printf("0         : turn off highlighting\n");
	printf("q         : quit\n");
	printf("\nNote: may need to use FN keys to use Page Up and Page Down on Laptops\n\n");
}

GLint main(GLint argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Boyd's Boids");

	initializeBoids();

	initialPrintStatement();

	glutDisplayFunc(myDisplay);
	glutKeyboardFunc(handleKeyboard);
	glutSpecialFunc(handleSpecialKeyboard);
	glutIdleFunc(idleBoids);
	glutMouseFunc(handleClick);

	initializeGL();

	glutMainLoop();
}