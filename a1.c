
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "graphics.h"

	/* initialize graphics library */
extern void graphicsInit(int *, char **);

	/* lighting control */
extern void setLightPosition(GLfloat, GLfloat, GLfloat);
extern GLfloat* getLightPosition();

	/* viewpoint control */
extern void setViewPosition(float, float, float);
extern void getViewPosition(float *, float *, float *);
extern void getOldViewPosition(float *, float *, float *);
extern void getViewOrientation(float *, float *, float *);

	/* add cube to display list so it will be drawn */
extern int addDisplayList(int, int, int);

	/* mob controls */
extern void createMob(int, float, float, float, float);
extern void setMobPosition(int, float, float, float, float);
extern void hideMob(int);
extern void showMob(int);

	/* player controls */
extern void createPlayer(int, float, float, float, float);
extern void setPlayerPosition(int, float, float, float, float);
extern void hidePlayer(int);
extern void showPlayer(int);

	/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
	/* flag used to indicate that the test world should be used */
extern int testWorld;
	/* list and count of polygons to be displayed, set during culling */
extern int displayList[MAX_DISPLAY_LIST][3];
extern int displayCount;
	/* flag to print out frames per second */
extern int fps;
	/* flag to indicate removal of cube the viewer is facing */
extern int dig;
	/* flag indicates the program is a client when set = 1 */
extern int netClient;
	/* flag indicates the program is a server when set = 1 */
extern int netServer; 

	/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

	/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

/********* end of extern variable declarations **************/


	/*** collisionResponse() ***/
	/* -performs collision detection and response */
	/*  sets new xyz  to position of the viewpoint after collision */
	/* -can also be used to implement gravity by updating y position of vp*/
	/* note that the world coordinates returned from getViewPosition()
	   will be the negative value of the array indices */
void collisionResponse() {

	/* your collision code goes here */
	float x_cord;
	float y_cord;
	float z_cord;
	static int fallSlowAgain = 0;

  int x, y, z;

  /* get the initial position */
  getViewPosition(&x_cord, &y_cord, &z_cord);

  x = (int)x_cord;
  y = (int)y_cord;
  z = (int)z_cord;

  x = x * -1;
  y = y * -1;
  z = z * -1;

  if( world[x][y][z] != 0 ){

    if( world[x][y+1][z] == 0){
      setViewPosition(x_cord, y_cord-1, z_cord );
    }
    getOldViewPosition(&x_cord, &y_cord, &z_cord);
    setViewPosition(x_cord, y_cord, z_cord );
  }

  /* apply gravity */
  if( world[x][y][z] == 0 && world[x][y-1][z] == 0 && y > 3 && fallSlowAgain%5 == 0){
      setViewPosition(x_cord, y_cord + 1, z_cord );
  }

	fallSlowAgain++;
}


	/*** update() ***/
	/* background process, it is called when there are no other events */
	/* -used to control animations and perform calculations while the  */
	/*  system is running */
	/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
  int i, j;
  float x_cord;
  float y_cord;
  float z_cord;
  static int fallSlow = 0;
  static int cloudx = 1, cloudz = 1;
  static int cloudx2 = 10, cloudz2 = 1;

  int x, y, z;

	/* sample animation for the test world, don't remove this code */
	/* -demo of animating mobs */
   if (testWorld) {
	/* sample of rotation and positioning of mob */
	/* coordinates for mob 0 */
      static float mob0x = 50.0, mob0y = 25.0, mob0z = 52.0;
      static float mob0ry = 0.0;
      static int increasingmob0 = 1;
	/* coordinates for mob 1 */
      static float mob1x = 50.0, mob1y = 25.0, mob1z = 52.0;
      static float mob1ry = 0.0;
      static int increasingmob1 = 1;

	/* move mob 0 and rotate */
	/* set mob 0 position */
      setMobPosition(0, mob0x, mob0y, mob0z, mob0ry);

	/* move mob 0 in the x axis */
      if (increasingmob0 == 1)
         mob0x += 0.2;
      else 
         mob0x -= 0.2;
      if (mob0x > 50) increasingmob0 = 0;
      if (mob0x < 30) increasingmob0 = 1;

	/* rotate mob 0 around the y axis */
      mob0ry += 1.0;
      if (mob0ry > 360.0) mob0ry -= 360.0;

	/* move mob 1 and rotate */
      setMobPosition(1, mob1x, mob1y, mob1z, mob1ry);

	/* move mob 1 in the z axis */
	/* when mob is moving away it is visible, when moving back it */
	/* is hidden */
      if (increasingmob1 == 1) {
         mob1z += 0.2;
         showMob(1);
      } else {
         mob1z -= 0.2;
         hideMob(1);
      }
      if (mob1z > 72) increasingmob1 = 0;
      if (mob1z < 52) increasingmob1 = 1;

	/* rotate mob 1 around the y axis */
      mob1ry += 1.0;
      if (mob1ry > 360.0) mob1ry -= 360.0;
    /* end testworld animation */
   } else {
	/* your code goes here */


     /**************************************************************************/
    /* clouds */
        
        if ( fallSlow%20 == 0 )
        {
        	/* cloud1 */
        	/* when it reaches the end of the map restart the cloud */
        	if( cloudx == 99 && cloudz == 99 ){
        	
        		world[cloudx-1][35][cloudz-1] = 0;
        		world[cloudx][35][cloudz-1] = 0;
        		world[cloudx][35][cloudz] = 0;
        		
        		cloudx = 1;
        		cloudz = 1;
        	}
        	
        	world[cloudx][35][cloudz] = 5;
        	world[cloudx+1][35][cloudz] = 5;
        	world[cloudx+1][35][cloudz+1] = 5;
        	
        	
        	world[cloudx-1][35][cloudz-1] = 0;
        	world[cloudx][35][cloudz-1] = 0;

        	cloudx++;
        	cloudz++;
        	
        	
        	/*cloud 2*/
        	/* when it reaches the end of the map restart the cloud */
        /*	if( cloudx == 99 && cloudz == 99 ){
        	
        		world[cloudx2-1][35][cloudz2-1] = 0;
        		world[cloudx2][35][cloudz2-1] = 0;
        		world[cloudx2][35][cloudz2] = 0;
        		
        		cloudx = 1;
        		cloudz = 1;
        	}
        	
        	world[cloudx2][35][cloudz2] = 5;
        	world[cloudx2+1][35][cloudz2] = 5;
        	world[cloudx2+1][35][cloudz2+1] = 5;
        	
        	
        	world[cloudx2-1][35][cloudz2-1] = 0;
        	world[cloudx2][35][cloudz2-1] = 0;

        	cloudz2++;*/

        	
        }

    /* get the initial position */
    getViewPosition(&x_cord, &y_cord, &z_cord);

    x = (int)x_cord;
    y = (int)y_cord;
    z = (int)z_cord;

    x = x * -1;
    y = y * -1;
    z = z * -1;

   // printf("%d, %d, %d\n", x, y, z);

    /* apply gravity */
    if( world[x][y][z] == 0 && world[x][y-1][z] == 0 && y > 3 && fallSlow%5 == 0 ){
      setViewPosition(x_cord, y_cord + 1, z_cord );
    }
    
    fallSlow++;

   }
}

/* Logic taken from:
   http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/
float noise( int x, int z )
{
	int dimension2 = x + z * 57;
	
	/* preform a left-shift for 13 followed by a bitwise XOR */
	dimension2 = (dimension2 << 13) ^ dimension2;
	
	/* return a floating point number between between -1.0 and 1.0 */
	return ( 1.0 - ( (dimension2 * (dimension2 * dimension2 * 15731 + 789221) 
	         + 1376312589) & 0x7fffffff) / 1073741824.0); 
}

/* Logic taken from:
   http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/
float smoothNoise( float x, float z )
{
	/* smooth the surface (corners, sides and center) */
	float corners = ( noise( x-1, z-1 ) + noise( x+1, z-1 ) + noise( x-1, z+1 ) + noise( x+1, z+1 ) ) / 16;
    float sides   = ( noise( x-1, z ) + noise( x+1, z ) + noise( x, z-1 ) + noise( x, z+1 ) ) /  8;
    float center  = noise( x, z ) / 4;
    return corners + sides + center;
}

/* Logic taken from:
   http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/
float linear_Interpolate( float a, float b, float axis )
{
	return a * (1.0 - axis) + b * axis;
}

/* Logic taken from:
   http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/
float interpolateNoise( float x, float z )
{	
	float vect1, vect2, vect3, vect4, fractional_x, fractional_z;
	float interpolate1, interpolate2;
	int x_int, z_int;
	
	/* convert floats to ints */
	x_int = (int)x;
	z_int = (int)z;
	
	/* create a fractional of y */
	fractional_x = x - x_int;
	fractional_z = z - z_int;
	
	/* smooth surfaces */
	vect1 = smoothNoise( x_int, z_int );
	vect2 = smoothNoise( x_int + 1, z_int );
	vect3 = smoothNoise( x_int, z_int + 1 );
	vect4 = smoothNoise( x_int + 1, z_int + 1 );
	
	interpolate1 = linear_Interpolate( vect1, vect2, fractional_x );
	interpolate2 = linear_Interpolate( vect3, vect4, fractional_x );
	
	return linear_Interpolate( interpolate1, interpolate2, fractional_z );
}

/* Logic taken from:
   http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/
float perlinNoise( float x, float z )
{
	int i;
	float total = 0.0;
	float persistence = 0.25;
	float frequency, amplitude;
	int numOfOctaves = 30 - 1;
	
	for( i = 0; i < numOfOctaves; i++ )
	{
		frequency = pow(2,i);
		amplitude = pow(persistence,i);

		total = total + interpolateNoise(x * frequency, z * frequency) * amplitude;
	}
	
	return total;
}

int main(int argc, char** argv)
{
int i, j, k, l;
	/* initialize the graphics system */
   graphicsInit(&argc, argv);

	/* the first part of this if statement builds a sample */
	/* world which will be used for testing */
	/* DO NOT remove this code. */
	/* Put your code in the else statment below */
	/* The testworld is only guaranteed to work with a world of
		with dimensions of 100,50,100. */
   if (testWorld == 1) {
	/* initialize world to empty */
      for(i=0; i<WORLDX; i++)
         for(j=0; j<WORLDY; j++)
            for(k=0; k<WORLDZ; k++)
               world[i][j][k] = 0;
	
	/* some sample objects */
	/* build a red platform */
      for(i=0; i<WORLDX; i++) {
         for(j=0; j<WORLDZ; j++) {
            world[i][24][j] = 3;
         }
      }
	/* create some green and blue cubes */
      world[50][25][50] = 1;
      world[49][25][50] = 1;
      world[49][26][50] = 1;
      world[52][25][52] = 2;
      world[52][26][52] = 2;

	/* blue box shows xy bounds of the world */
      for(i=0; i<WORLDX-1; i++) {
         world[i][25][0] = 2;
         world[i][25][WORLDZ-1] = 2;
      }
      for(i=0; i<WORLDZ-1; i++) {
         world[0][25][i] = 2;
         world[WORLDX-1][25][i] = 2;
      }

	/* create two sample mobs */
	/* these are animated in the update() function */
      createMob(0, 50.0, 25.0, 52.0, 0.0);
      createMob(1, 50.0, 25.0, 52.0, 0.0);

	/* create sample player */
      createPlayer(0, 52.0, 27.0, 52.0, 0.0);

   } else {
	/* TODO: Make Noise function
	   TODO: Make Interpolated Noise function
	   TODO: Create World
	*/
	
	float randomNoise;
	
	/* Initialize the world as empty */
      for(i = 0; i < WORLDX; i++)
      {
         for(j = 0; j < WORLDY; j++)
         {
            for(k = 0; k < WORLDZ; k++)
            {
               world[i][j][k] = 0;
            }
         } 
      } 
    /* Create the ground */
    for(i = 0; i < WORLDX; i++) {
         for(j = 0; j < WORLDZ; j++) {
         //	for(k = 0; k < 5; k++) {
         		/* make sure ground is only 5 cubes deep */
         		world[i][3][j] = 8;
         //	}
         }
    }
               
	  /* Create world using Perlin noise algo */
      for(i = 0; i < WORLDX; i++) {
         for(j = 0; j < WORLDZ; j++) {
         
         	randomNoise = perlinNoise(i,j); // random number for testing
         	
         //	printf("perlin Noise results: %lf\n", (randomNoise+1) * 25);

          int intepolateInt = (int) ((randomNoise+1) * 7);

          //printf("int: %d\n", intepolateInt);

          world[i][intepolateInt][j] = 1;
         			
     			/* fill in the gaps below the cube */
          if(intepolateInt > 3){
     		     for( l = 3; l < intepolateInt; l++ )
     			    {
     				     world[i][l][j] = 1;
     			    }
          }
        }
      }
      

      //createMob(0, 50.0, 25.0, 52.0, 0.0);
      
      /* create sample player */
      //createPlayer(0, 52.0, 27.0, 52.0, 0.0);
               
	
	/* your code to build the world goes here */

   }


	/* starts the graphics processing loop */
	/* code after this will not run until the program exits */
   glutMainLoop();
   return 0; 
}

