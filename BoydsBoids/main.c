/***********************************************************************************************
*	Assignment 1 - Boyd's Boids
*	
*	Authour: Boyd Pelley - B00919714
*	Email: boydp@dal.ca
* 
*	Description: A program written with OpenGL that simulates a flock of bird-oids flying across
*	the display window. Handles collisions against walls, and applies the three main qualities
*	of Boids: Alignment, Cohesion, and Separation. Includes a pause button, boid highlighting,
*	and speed changing. 
* 
*	Note: let the program run for a few seconds to see the boids flock into each other. 
*	Depending on your system it may take longer than others.
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

// boid factors
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

// returns the magnitide (length) of a vector
GLfloat getMagnitude(GLfloat x, GLfloat y)
{
	return (GLfloat)sqrt(x * x + y * y);
}

// Normalizes a vector
void normalize(Vector2 * vector)
{
	GLfloat length = sqrt(vector->x * vector->x + vector->y * vector->y);
	if (length != 0)
	{
		vector->x /= length;
		vector->y /= length;
	}
}

// This function is used with the 4 boid factors to clean up the assignments 
void applyFactor(Vector2* vector, GLfloat factor)
{
	vector->x *= factor;
	vector->y *= factor;
}

/**
* This method copies the current flock to the previous flock
*/
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

/**
* The first method that sets all of the boids initial position, velocity, and color
*/
void initializeBoids()
{
	// Set the min and max x and y coordinates so we don't have undefined behavior if we spawn too
	// close to the walls
	GLint spawnMinX = spawnThreshold;
	GLint spawnMaxX = windowWidth - spawnThreshold;
	GLint spawnMinY = subWindowHeight + spawnThreshold;
	GLint spawnMaxY = windowHeight - spawnThreshold;
	
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		currentFlock[i].position = (Vector2){ (rand() % (spawnMaxX - spawnMinX) + spawnMinX),
												(rand() % (spawnMaxY - spawnMinY) + spawnMinY) };

		// We first find an angle and then apply cos and sin to it so we can get some x and y range
		GLfloat angle = (rand() % 360) * (PI / 180.0);
		currentFlock[i].velocity = (Vector2){ (cos(angle) * flockSpeed), (sin(angle) * flockSpeed) };

		// Set boid color to blue
		currentFlock[i].r = 0.0;
		currentFlock[i].g = 0.0;
		currentFlock[i].b = 1.0;
	}
	// Copy this to the previous flock so when we do our very first calculation we aren't calculating 
	// from null values
	copyCurrentFlockToPrevious();
}

// This struct is used to find the boids neighbours and nothing else
typedef struct boidNeighbours
{
	GLfloat distance;
	GLint index;
} boidNeighbours;

// The three methods below are standard implementations of quicksort except we pass structs rather
// than some integer/float directly
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

/**
* Using quicksort, this function will find any boids NUMBER_NEIGHBOURS (6) neighbours. This 
* function accepts 3 parameters, the current Boid, the index of the boid, and the list of 
* the boid's nearest neighbours. It first fills the struct array of boidNeighbours, then sorts
* them, copying over the indexes to the array we passed into the function.
*/
void findNearestNeighboursIndex(Boid boid, GLint index, GLint*nearestNeighboursIndexes)
{
	boidNeighbours neighbours[FLOCK_SIZE];

	// Copy over every boid to the list, if the index equals the current boid, the distance is 
	// set to some arbitrarily large value (the window width in our case), so the boid will not
	// be the first index in the sorted list (it would be index 0 because the distance would be
	// 0
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

	// Copy the first x indexes to the list we passed into the function
	for (GLint i = 0; i < NUMBER_NEIGHBOURS; i++)
	{
		nearestNeighboursIndexes[i] = neighbours[i].index;
	}
}

/**
* Takes in the boid's index and determines if it is getting close to the wall. If it is, it will
* either return 0b0001 or 0b0010. We will use the values returned from this method alongside the
* next method.
*/
GLubyte inProximityOfHorizontal(GLint index)
{
	if (previousFlock[index].position.x > windowWidth - distanceThreshold) // Right hit  
		return 0x1;
	if (previousFlock[index].position.x < distanceThreshold) // Left hit
		return 0x2;
	
	return 0x0;
}

/**
* Similar to the method above, this one returns a value whether boid i is close to the top or 
* bottom of the screen. We have two separate methods as we can be close to corners. Either 
* return 0b0100 or 0b1000 accordingly.
*/
GLubyte inProximityOfVertical(GLint index)
{
	if (previousFlock[index].position.y < subWindowHeight + distanceThreshold) // Bottom hit
		return 0x4;
	if (previousFlock[index].position.y > windowHeight - distanceThreshold) // Top hit
		return 0x8;
	
	return 0x0;
}

/**
* This method will steer the boid away if it within proximity of a wall(s). It takes in two 
* parameters, the boid's index, and a proximity value. The proximity value is the value 
* determined in the two methods above, we can use bitwise operations to determine if the 
* boid is close to one or two walls (example: proximity of 0x9 or 0b1001 means the boid is
* in the top right corner and we can handle accordingly).
* We set the velocity by determining how close the boid is to the wall, the closer they are,
* the more it pushes the boid away
*/
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

/**
* This method is the method that handles the three main ideas of boids: Alignment, Cohesion and
* Separation. For each boid's neighbours, we adjust the values based on a few conditions: we set
* the alignment by the average velocity of the boids, the cohesion based on the average position
* and the separation based on whether we are too close to any boid.
* 
* The inspiration for this method was derived from ideas that I learned from the Boids wikipedia
* page, specifically the images "Rules applied in simple Boids", which can be found here:
* https://en.wikipedia.org/wiki/Boids
*/
void handleBoidRules(GLint i, GLint* nearestNeighbours)
{
	Vector2 alignment = { 0, 0 };
	Vector2 cohesion = { 0, 0 };
	Vector2 separation = { 0, 0 };

	for (int j = 0; j < NUMBER_NEIGHBOURS; j++)
	{
		// Calculate the neighbour index and store in a variable so we don't have to write 
		// nearestNeighbour[j] as an array index
		GLint neighbour = nearestNeighbours[j];

		// Add each boid's velocity to the alignment vector
		alignment.x += previousFlock[neighbour].velocity.x;
		alignment.y += previousFlock[neighbour].velocity.y;

		// Add each boid's position to the cohesion vector
		cohesion.x += previousFlock[neighbour].position.x;
		cohesion.y += previousFlock[neighbour].position.y;

		// Find the boid's distance to its neighbour to figure out if we need to increase the separation 
		// varaible or not
		GLfloat distance = getDistance(previousFlock[i].position.x, previousFlock[neighbour].position.x,
			previousFlock[i].position.y, previousFlock[neighbour].position.y);

		if (distance < boidDistance)
		{
			// Create a direction that points back at the boid
			Vector2 directionAway =
			{
				previousFlock[i].position.x - previousFlock[neighbour].position.x,
				previousFlock[i].position.y - previousFlock[neighbour].position.y
			};

			// Normalize to make its magnitude 1
			normalize(&directionAway);

			// Similar to the method with the method that determines if a boid is too close to a wall
			// we push a boid away from its neighbour by finding the inverse direction
			directionAway.x *= (1.0 / distance) * boidAvoidanceFactor;
			directionAway.y *= (1.0 / distance) * boidAvoidanceFactor;

			// Add the direction away vector to the separation vector (
			separation.x += directionAway.x;
			separation.y += directionAway.y;
		}
	}

	// Take the average alignment vector
	alignment.x /= NUMBER_NEIGHBOURS;
	alignment.y /= NUMBER_NEIGHBOURS;

	// Subtract our own velocity because we want to be more like our neighbours, we are adding this
	// velocity to the boid's own velocity
	alignment.x -= previousFlock[i].velocity.x;
	alignment.y -= previousFlock[i].velocity.y;

	// Make the velocity have a magnitude of one and apply the factor
	normalize(&alignment);
	applyFactor(&alignment, boidAlignmentFactor);

	// Take the average cohesion
	cohesion.x /= NUMBER_NEIGHBOURS;
	cohesion.y /= NUMBER_NEIGHBOURS;

	// For the same reason as alignnment, we are adding this to our own position, so we must remove
	// our own position
	cohesion.x -= previousFlock[i].position.x;
	cohesion.y -= previousFlock[i].position.y;

	// Normalize and apply our factor
	normalize(&cohesion);
	applyFactor(&cohesion, boidCohesionFactor);

	// Apply the three factors to the boids velocity
	currentFlock[i].velocity.x += alignment.x;
	currentFlock[i].velocity.y += alignment.y;

	currentFlock[i].velocity.x += cohesion.x;
	currentFlock[i].velocity.y += cohesion.y;

	currentFlock[i].velocity.x += separation.x;
	currentFlock[i].velocity.y += separation.y;

	// We find the current speed and slow the boids down if they are travelling too fast. I had
	// issues with my boids progressively getting faster and faster without this block of code
	GLfloat currentSpeed = getMagnitude(currentFlock[i].velocity.x, currentFlock[i].velocity.y);

	if (currentSpeed > flockSpeed)
	{
		currentFlock[i].velocity.x = (currentFlock[i].velocity.x / currentSpeed) * flockSpeed;
		currentFlock[i].velocity.y = (currentFlock[i].velocity.y / currentSpeed) * flockSpeed;
	}

}

// Method to set the boids color to blue
void setAllBoidsColourBlue()
{
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		currentFlock[i].r = 0.0f;
		currentFlock[i].g = 0.0f;
		currentFlock[i].b = 1.0f;
	}
}

// Sets the boid to red, sets its nearest neighbours to green
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

/**
* This method is what is used in the idle loop, we set every boid to blue every loop, then loop
* through the list of boids, finding each boid's neighbours. If boidState is between 1 and 9, then
* we color the boids according to the handleBoidState variable. If a boid is too close to a wall
* then we avoid walls, and we handle the three boid factors otherwise
*/
void updateBoids()
{
	setAllBoidsColourBlue();
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		GLint nearestNeighbours[NUMBER_NEIGHBOURS];
		findNearestNeighboursIndex(currentFlock[i], i, nearestNeighbours);

		if (i == boidState)
		{
			handleBoidState(boidState, nearestNeighbours);
		}
		
		// We use a bitwise or here so we can combine the two return values (i.e. 0x1 and 0x8 
		// turns into 0x9 or 0b1001)
		GLubyte inProximity = inProximityOfHorizontal(i) | inProximityOfVertical(i);
		if (inProximity > 0)
		{
			avoidWalls(i, inProximity);
		}
		else
		{
			handleBoidRules(i, nearestNeighbours);
		}
		// This is what makes the boids move
		currentFlock[i].position.x += currentFlock[i].velocity.x;
		currentFlock[i].position.y += currentFlock[i].velocity.y;
	}
}

// Set the background to black
void initializeGL(void)
{
	glClearColor(0, 0, 0, 1);

	gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);
}

/**
* This method draws boids based on their angle so they are pointing in the direction they are 
* facing, as well as scales. It first calculates the arctangent based on the x and y velocity,
* we translate our boid, rotate based on the angle, color, scale, then draw.
*/
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

// Draws the text "boids" in the button on the UI
void drawText(GLint offset, char * string)
{
	if (pauseState) glColor3f(0.5f, 1.0f, 0.7f);
	else glColor3f(0.4f, 0.8f, 0.6f);

	glRasterPos2d(225 + offset, 45);

	for (GLint i = 0; i < strlen(string); i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, string[i]);
	}
}

/**
* Draws the "button" and the block at the bottom of the screen, If the pauseState is on, then we
* change the shading to give the impression that the pause button is clicked.
*/
void drawUI()
{
	// Draw the bottom subwindow
	glColor3f(0.5f, 0.9f, 0.7f);
	glBegin(GL_POLYGON);
		glVertex2d(0, 0);
		glVertex2d(0, subWindowHeight);
		glVertex2d(windowWidth, subWindowHeight);
		glVertex2d(windowWidth, 0);
	glEnd();

	// Draw the little bar near the top of the subwindow
	glColor3f(0.5f, 1.0f, 0.7f);
	glBegin(GL_POLYGON);
		glVertex2d(0, subWindowHeight - 25);
		glVertex2d(0, subWindowHeight - 20);
		glVertex2d(windowWidth, subWindowHeight - 20);
		glVertex2d(windowWidth, subWindowHeight - 25);
	glEnd();

	// Finds the center of the subwindow
	GLint midXSubWindow = windowWidth / 2;
	GLint midYSubWindow = subWindowHeight / 2;
	GLint buttonWidth = 75;
	GLint buttonHeight = 15;
	GLint offset = 2;

	// Pause is clicked
	if (pauseState)
	{
		// Draws the shading on the top left 
		glColor3f(0.5f, 1.0f, 0.7f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth - offset, midYSubWindow - buttonHeight + offset);
			glVertex2d(midXSubWindow - buttonWidth - offset, midYSubWindow + buttonHeight + offset);
			glVertex2d(midXSubWindow + buttonWidth - offset, midYSubWindow + buttonHeight + offset);
			glVertex2d(midXSubWindow + buttonWidth - offset, midYSubWindow - buttonHeight + offset);
		glEnd();

		// Draw the button
		glColor3f(0.4f, 0.8f, 0.6f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow - buttonHeight);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow - buttonHeight);
		glEnd();

		// Draws the boid text according to the offset to give the appearance it's on the button
		drawText(offset, "boids");
	}
	else
	{
		// Draw the highlight in the bottom right corner
		glColor3f(0.3f, 0.7f, 0.5f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth + offset, midYSubWindow - buttonHeight - offset);
			glVertex2d(midXSubWindow - buttonWidth + offset, midYSubWindow + buttonHeight - offset);
			glVertex2d(midXSubWindow + buttonWidth + offset, midYSubWindow + buttonHeight - offset);
			glVertex2d(midXSubWindow + buttonWidth + offset, midYSubWindow - buttonHeight - offset);
		glEnd();

		// Draw the button
		glColor3f(0.5f, 1.0f, 0.7f);
		glBegin(GL_POLYGON);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow - buttonHeight);
			glVertex2d(midXSubWindow - buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow + buttonHeight);
			glVertex2d(midXSubWindow + buttonWidth, midYSubWindow - buttonHeight);
		glEnd();

		// Draw the text in the center of the button
		drawText(0, "boids");
	}
}

// Method that puts together all of the drawing methods into one. Start with drawing the boids, 
// then draw the UI overtop, then handle mouse clicking
void myDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw each boid
	for (GLint i = 0; i < FLOCK_SIZE; i++)
	{
		drawBoids(currentFlock[i]);
	}

	drawUI();

	// If there is a click, draw a red dot where the click appeared and don't move the red dot until
	// they click somwhere else
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

// The loop where the boids are updated. Only "turns on" when the pause state is 0
void idleBoids()
{
	if (pauseState == 0)
	{
		updateBoids();
		copyCurrentFlockToPrevious();
		glutPostRedisplay();
	}
}

// Handle mouse click, if right click close the program, otherwise set the mouseX and
// mouseY values based on the position of the mouse
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

		// If the mouse position lies within these coordinates, then we are over the pause button
		if (mouseX >= 175 && mouseX <= 325 && mouseY >= 35 && mouseY <= 65)
		{
			if (pauseState == 0) pauseState = 1;
			else if (pauseState == 1) pauseState = 0;
		}

		glutPostRedisplay();
	}
}

// This method handles the special keys for page up and page down
void handleSpecialKeyboard(unsigned char key, GLint x, GLint y)
{
	if (key == GLUT_KEY_PAGE_UP || key == GLUT_KEY_UP)
	{
		flockSpeed += 0.001;
		printf("Speed: %f\n", flockSpeed);
	}
	else if (key == GLUT_KEY_PAGE_DOWN || key == GLUT_KEY_UP)
	{
		flockSpeed -= 0.001;
		printf("Speed: %f\n", flockSpeed);
	}
}

// Handles the other keys, 1-9 set the boid state and draws the boids as the different colors,
// 0 sets it back to standard boid drawing, and q quits the program
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
}

// Put print statement into a separate method to clean things up
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

/**
* The main method that glues all of the other methods together. It tells glut which functions
* are which, initializes OpenGL, and starts the main loop
*/
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