/* Simple 2D physics for circles using ASCII graphics
	-original NCurses code from "Game Programming in C with the Ncurses Library"
	 https://www.viget.com/articles/game-programming-in-c-with-the-ncurses-library/
	 and from "NCURSES Programming HOWTO"
	 http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
	-Physics code and algorithms from "How to Create a Custom 2D Physics
	 Engine: The Basics and Impulse Resolution"
	 https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331
*/


#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<unistd.h>
#include<ncurses.h>

	// used to slow curses animation
#define DELAY 50000

	// number of balls
#define POPSIZE 50
	// ball radius, all circles have the same radius
#define RADIUS 1.0
	// indicate if balls collide or not
#define COLLIDE 1
#define NOCOLLIDE 0
	// restitution controls how bounce the objects will be
#define RESTITUTION 0.5
	// object mass
#define MASS 1.0

	// maximum screen size, both height and width
#define SCREENSIZE 100

	// ball location (x,y,z) and velocity (vx,vy,vz) in ballArray[][]
#define BX 0
#define BY 1
#define VX 2
#define VY 3

	// maximum screen dimensions
int max_y = 0, max_x = 0;

	// location and velocity of ball
float ballArray[POPSIZE][4];
	// change in velocity is stored for each ball (x,y,z)
float ballUpdate[POPSIZE][2];

void initBalls() {
int i;
	// calculate initial random locations for each ball, scaled based on the screen size
   for(i=0; i<POPSIZE; i++) {
      ballArray[i][BX] = (float) (random() % SCREENSIZE);
      ballArray[i][BY] = (float) (random() % SCREENSIZE);
      ballArray[i][VX] =  (float) ((random() % 5) - 2);
      ballArray[i][VY] = (float) ((random() % 5) - 2);
      // printf("%lf, %lf, %lf, %lf\n", ballArray[i][BX], ballArray[i][BY], ballArray[i][VX], ballArray[i][VY]);

      ballUpdate[i][BX] = 0.0;
      ballUpdate[i][BY] = 0.0;
   }
}

int drawBalls() {
int c, i;
float multx, multy;

	// update screen maximum size
   getmaxyx(stdscr, max_y, max_x);

	// used to scale position of balls based on screen size
   multx = (float)max_x / SCREENSIZE;
   multy = (float)max_y / SCREENSIZE;

   clear();

	// display balls
   for (i=0; i<POPSIZE; i++) {
      mvprintw((int)(ballArray[i][BX]*multy), (int)(ballArray[i][BY]*multx), "o");
   }

   refresh();

   usleep(DELAY);

	// read keyboard and exit if 'q' pressed
   c = getch();
   if (c == 'q') return(1);
   else return(0);
}

	// determine if two balls in ballArray collide
	// calcualte distance between the two balls and compare to the
	//	sum of the radii
	// use balli and ballj to identify elements in ballArray[]
int ballCollision(int balli, int ballj) {
float distance;
float radiiSum;

		// Pythagorean distance
	distance = sqrtf(powf((ballArray[balli][BX]-ballArray[ballj][BX]),2)
		 + powf((ballArray[balli][BY]-ballArray[ballj][BY]),2));
	radiiSum = RADIUS + RADIUS;
		// if the sum of the two radii is less than the distance
		// between the balls then they collide, otherwise they
		// do not collide
	if (distance < radiiSum) return(COLLIDE);
	else return(NOCOLLIDE);
}

	// calculate the dot product between two vectors
float dotProduct(float x1, float y1, float x2, float y2) {
   return(x1 * x2 + y1 * y2);
}

	// calculate effects of collision between ballArray[i][] and
	// ballArray[j][] where i and j are the parameters to the function
void resolveCollision(int i, int j) {
float rvx, rvy;
float nx, ny;
float distance;
float vnormal;
float impulse;
float ix, iy;

	// calculate relative velocity
  rvx = ballArray[j][VX] - ballArray[i][VX];
  rvy = ballArray[j][VY] - ballArray[i][VY];

	// calculate collision normal
  nx = ballArray[j][BX] - ballArray[i][BX];
  ny = ballArray[j][BY] - ballArray[i][BY];

	// Pythagorean distance
  distance = sqrtf(powf((ballArray[j][BX]-ballArray[i][BX]),2)
		 + powf((ballArray[j][BY]-ballArray[i][BY]),2));
  if (distance == 0) return;

  nx = nx / distance;
  ny = ny / distance;

	// Calculate relative velocity in terms of the normal direction
  vnormal = dotProduct(rvx, rvy, nx, ny);

	// Do not resolve if velocities are separating
  if(vnormal > 0)
    return;

	// Calculate impulse scalar
  impulse = -(1 + RESTITUTION) * vnormal;
  impulse /= ((1 / MASS) + (1 / MASS));

	// Apply impulse
  ix = impulse * nx;
  iy = impulse * ny;
  ballUpdate[i][BX] = ballArray[i][VX] - ((1/MASS) * impulse);
  ballUpdate[i][BY] = ballArray[i][VY] - ((1/MASS) * impulse);
  ballUpdate[j][BX] = ballArray[j][VX] + ((1/MASS) * impulse);
  ballUpdate[j][BY] = ballArray[j][VY] + ((1/MASS) * impulse);

}


void moveBalls() {
int i,j;

	// update velocity of balls based upon collisions
	// compare all balls to all other circles using two loops
   for (i=0; i<POPSIZE; i++) {
      for (j=i+1; j<POPSIZE; j++) {
         if (ballCollision(i, j) == COLLIDE){
            resolveCollision(i, j);
         }
      }
   }


  // printf("-----------------------------------\n");
	// move balls by calculating updating velocity and position
   for (i=0; i<POPSIZE; i++) {
    // printf("%lf, %lf\n", ballUpdate[i][BX], ballUpdate[i][BY]);

	// update velocity for each ball
      if (ballUpdate[i][BX] != 0.0)
         ballArray[i][VX] = ballUpdate[i][BX];
      if (ballUpdate[i][BY] != 0.0)
         ballArray[i][VY] = ballUpdate[i][BY];

	// enforce maximum velocity of 2.0 in each axis
	// done to make it easier to see collisions
      if (ballArray[i][VX] > 2.0) ballArray[i][VX] = 2.0;
      if (ballArray[i][VY] > 2.0) ballArray[i][VY] = 2.0;

	// update position for each ball
      ballArray[i][BX] += ballArray[i][VX];
      ballArray[i][BY] += ballArray[i][VY];

	// if ball moves off the screen then reverse velocity so it bounces
	// back onto the screen, and move it onto the screen
      if (ballArray[i][BX] > (SCREENSIZE-1)) {
         ballArray[i][VX] *= -1.0;
         ballArray[i][BX] = SCREENSIZE - 1.5;
      }
      if (ballArray[i][BX] < 0.0) {
         ballArray[i][VX] *= -1.0;
         ballArray[i][BX] = 0.5;
      }
      if (ballArray[i][BY] > (SCREENSIZE-1)) {
         ballArray[i][VY] *= -1.0;
         ballArray[i][BY] = SCREENSIZE - 1.5;
      }
      if (ballArray[i][BY] < 0.0) {
         ballArray[i][VY] *= -1.0;
         ballArray[i][BY] = 0.5;
      }

   }
}

int main(int argc, char *argv[]) {
int i, count;

	// initialize curses
   initscr();
   noecho();
   cbreak();
   timeout(0);
   curs_set(FALSE);
  // Global var `stdscr` is created by the call to `initscr()`
   getmaxyx(stdscr, max_y, max_x);

	// place balls in initial position
   initBalls();

	// draw and move balls using ncurses
  // int x = 0;
  //  while(x < 100) {
   while(1) {
      if (drawBalls() == 1) break;
      moveBalls();
      // x++;

   }


	// shut down curses
   endwin();

}
