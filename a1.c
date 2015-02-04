
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "graphics.h"

	/* mouse function called by GLUT when a button is pressed or released */
void mouse(int, int, int, int);

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

   float x_cord;
   float y_cord;
   float z_cord;
   static int fallSlowAgain = 0;

   int x, y, z;

   /* get the initial position */
   getViewPosition(&x_cord, &y_cord, &z_cord);

   /*convert the cordinates to positive integers */
   x = (int)x_cord * -1;
   y = (int)y_cord * -1;
   z = (int)z_cord * -1;

   /* ensure we are on the map */
   if ( x < 2 || x > 98 || y < 4 || y > 47 || z < 2 || z > 98 )
   {
        getOldViewPosition(&x_cord, &y_cord, &z_cord);
        setViewPosition(x_cord, y_cord, z_cord );
   }

   /* allow for climbing 1 block if needed */
   if( world[x][y][z] != 0 )
   {
      if( world[x][y+1][z] == 0 )
      {
          setViewPosition(x_cord, y_cord - 0.5, z_cord );
      }
      else
      {
          getOldViewPosition(&x_cord, &y_cord, &z_cord);
          setViewPosition(x_cord, y_cord, z_cord );
      }
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

  int i, j, k, x, y, z;
  float *la;
  float x_cord;
  float y_cord;
  float z_cord;
  static int fallSlow = 0;
  static int cloudx = 1, cloudz = 1;
  static int cloudx2 = 10, cloudz2 = 1;
  static int cloudx3 = 98, cloudz3 = 30;


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
   } 
   else 
   {

      /********************** CLOUDS *******************************************/
      if ( fallSlow%5 == 0 ) 
      {
         /* cloud1 */
         /* when it reaches the end of the map restart the cloud */
         if( cloudx == 99 && cloudz == 99 )
         {
            world[cloudx-1][48][cloudz-1] = 0;
            world[cloudx][48][cloudz-1] = 0;
            world[cloudx][48][cloudz] = 0;
            
            cloudx = 1;
            cloudz = 1;
         }
         
         /* create cloud at new position */
         world[cloudx][48][cloudz] = 5;
         world[cloudx+1][48][cloudz] = 5;
         world[cloudx+1][48][cloudz+1] = 5;
         
         /* erase cloud from old position */
         world[cloudx-1][48][cloudz-1] = 0;
         world[cloudx][48][cloudz-1] = 0;

         cloudx++;
         cloudz++;
         
         
         /*cloud 2*/
         /* when it reaches the end of the map restart the cloud */
         if( cloudz2 == 99 && cloudx2 == 99 )
         {
            world[cloudx2][48][cloudz2-1] = 0;
            
            cloudz2 = 1;
         }
         
         /* create cloud at new position */
         world[cloudx2][48][cloudz2] = 5;
         world[cloudx2][48][cloudz2+1] = 5;
         
         /* erase cloud from old position */
         world[cloudx2][48][cloudz2-1] = 0;

         cloudz2++; 

          /*cloud 3*/
          /* when it reaches the end of the map restart the cloud */
          if( cloudx3 == 1 )
          {
          
            world[cloudx3+1][48][cloudz3] = 0;
            world[cloudx3][48][cloudz3] = 0;
            
            cloudx3 = 98;
          }
          
          /* create cloud at new position */
          world[cloudx3][48][cloudz3] = 5;
          world[cloudx3-1][48][cloudz3] = 5;
          
          /* erase cloud from old position */
          world[cloudx3+1][48][cloudz3] = 0;

          cloudx3--; 
      }

      /*********** GRAVITY AND COLISION DETECTION *******************/

      /* get the initial position */
      getViewPosition(&x_cord, &y_cord, &z_cord);

      /* convert the cordinates to positive integers */
      x = (int)x_cord * -1;
      y = (int)y_cord * -1;
      z = (int)z_cord * -1;

      /* apply gravity */
      if( world[x][y][z] == 0 && world[x][y-1][z] == 0 && y > 3 && fallSlow%5 == 0){
         setViewPosition(x_cord, y_cord + 1, z_cord );
      }

      fallSlow++;

   }
}


	/* called by GLUT when a mouse button is pressed or released */
	/* -button indicates which button was pressed or released */
	/* -state indicates a button down or button up event */
	/* -x,y are the screen coordinates when the mouse is pressed or */
	/*  released */ 
void mouse(int button, int state, int x, int y) {

   if (button == GLUT_LEFT_BUTTON)
      printf("left button - ");
   else if (button == GLUT_MIDDLE_BUTTON)
      printf("middle button - ");
   else
      printf("right button - ");

   if (state == GLUT_UP)
      printf("up - ");
   else
      printf("down - ");

   printf("%d %d\n", x, y);
}

/**** PERLIN NOISE FUNCTONS ****/
/* Algoritm taken from: http://lodev.org/cgtutor/randomnoise.html#3D_Random_Noise */
#define noiseWidth 30  
#define noiseHeight 30
#define noiseDepth 30

double noise[noiseWidth][noiseHeight][noiseDepth]; //the noise array

void generateNoise()
{
    int x, y, z;

    for( x = 0; x < noiseWidth; x++)
    for( y = 0; y < noiseHeight; y++)
    for( z = 0; z < noiseDepth; z++)
    {
        noise[x][y][z] = (rand() % 32768) / 32768.0;
    }
}

double smoothNoise(double x, double y, double z)
{ 
    //get fractional part of x and y
    double fractX = x - (int)x;
    double fractY = y - (int)y;
    double fractZ = z - (int)z;   
  
    //wrap around
    int x1 = ((int)x + noiseWidth) % noiseWidth;
    int y1 = ((int)y + noiseHeight) % noiseHeight;
    int z1 = ((int)z + noiseDepth) % noiseDepth;
  
    //neighbor values
    int x2 = (x1 + noiseWidth - 1) % noiseWidth;
    int y2 = (y1 + noiseHeight - 1) % noiseHeight;
    int z2 = (z1 + noiseDepth - 1) % noiseDepth;

    //smooth the noise with bilinear interpolation
    double value = 0.0;
    value += fractX       * fractY       * fractZ       * noise[x1][y1][z1];
    value += fractX       * (1 - fractY) * fractZ       * noise[x1][y2][z1];
    value += (1 - fractX) * fractY       * fractZ       * noise[x2][y1][z1];
    value += (1 - fractX) * (1 - fractY) * fractZ       * noise[x2][y2][z1];

    value += fractX       * fractY       * (1 - fractZ) * noise[x1][y1][z2];
    value += fractX       * (1 - fractY) * (1 - fractZ) * noise[x1][y2][z2];
    value += (1 - fractX) * fractY       * (1 - fractZ) * noise[x2][y1][z2];
    value += (1 - fractX) * (1 - fractY) * (1 - fractZ) * noise[x2][y2][z2];

    return value;
}

double turbulence(double x, double y, double z, double size)
{
    double value = 0.0, initialSize = size;
   
    while(size >= 1)
    {
        value += smoothNoise(x / size, y / size, z / size) * size;
        size /= 2.0;
    }
   
    return(128.0 * value / initialSize);
}
/********* End of Perlin Noise Functions *****************/



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

  } 
  /* code for world */
  else
  {
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
               world[i][3][j] = 8;
         }
      }

      /* create some random noise */
      generateNoise();
    
      uint noiseReturn;
      double t;
      time_t now;
      int x, y;
               
      /* Create world using Perlin noise algo */
      for(i = 0; i < WORLDX; i++) 
      {
          now = time(0);

          for(j = 0; j < WORLDZ; j++)
          {
              /* create the noise for the landscape */
              noiseReturn = (uint)(turbulence(i, j, t, 32) / 4) - 13;

              /* make sure no mountains touch any clouds */
              if(noiseReturn >= 48)
              {
                  noiseReturn = 47;
              }

              /* populate the world */
              world[i][noiseReturn][j] = 1;
              
              /* fill in the gaps below the cube */
              if(noiseReturn > 3)
              {
                  for( l = 3; l < noiseReturn; l++ )
                  {
                      world[i][l][j] = 1;
                  }
              }

              /* get the time */
              t = now / 40.0;
          }
      }


   }


	/* starts the graphics processing loop */
	/* code after this will not run until the program exits */
   glutMainLoop();
   return 0; 
}

