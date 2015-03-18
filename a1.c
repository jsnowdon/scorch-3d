
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
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "graphics.h"

#define CONVERT_TO_DECIMAL 0.0174532925f

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
extern void setViewOrientation(float, float, float);
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
extern void getPlayerPosition(int, float *, float *, float *, float *);
extern void getOldPlayerPosition(int, float *, float *, float *, float *);
extern void hidePlayer(int);
extern void showPlayer(int);

	/* 2D drawing functions */
extern void  draw2Dline(int, int, int, int, int);
extern void  draw2Dbox(int, int, int, int);
extern void  draw2Dtriangle(int, int, int, int, int, int);
extern void  set2Dcolour(float []);


	/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
	/* flag used to indicate that the test world should be used */
extern int testWorld;
	/* flag to print out frames per second */
extern int fps;
	/* flag to indicate the space bar has been pressed */
extern int space;
	/* flag indicates the program is a client when set = 1 */
extern int netClient;
	/* flag indicates the program is a server when set = 1 */
extern int netServer; 
	/* size of the window in pixels */
extern int screenWidth, screenHeight;
	/* flag indicates if map is to be printed */
extern int displayMap;

	/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

	/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

/********* end of extern variable declarations **************/

typedef struct _mobInfo{
  
  int drawMob;
  float timeInMotion;
  float x;
  float y;
  float z;
  float x_angle;
  float y_angle;

}projectile;

projectile bullet[5] = {0,0,0,0,0,0,0};

float velocity = 0.50f;
float angle = 45.00f;
int rightButton = 0;
int reset = 0;
int initialY;
int initialX;
int gravityFlag = 1;
int networkMode = 0;
int firstUpdate = 0;
time_t old;

/* setup server connections */
int socketID, clientSocketID;
struct sockaddr_in server_addr; /* server address */
struct sockaddr_in client_addr; /* client address */
socklen_t addrlen = sizeof(client_addr);
socklen_t server_addrlen = sizeof(server_addr);

/* player A.I. */
int searching = 1;
int fighting = 0;
int onRoute = 1;
int headingNorth = 0, headingSouth = 0, headingEast = 0, headingWest = 0;
int offTrailNorth = 0, offTrailSouth = 0, offTrailWest = 0, offTrailEast = 0;

#define PORT_NUM 3333

#pragma pack(1)   // this helps to pack the struct to 5-bytes
typedef struct packet{

   float x_orient;
   float y_orient;
   float z_orient;
   projectile bullet[5];
   int sendWorld[WORLDX][WORLDY][WORLDZ];

}msg;
#pragma pack(0)   // turn packing off

void sendMsgToServer( int bullet, int mobID, int drawMob, float timeInMotion,
                      float server_x, float server_y, float server_z,
                      float x_angle, float y_angle )
{
   char msgToServer[500000];
   float x_cord,   y_cord,   z_cord;
   float x_orient, y_orient, z_orient;
   float cur_angle = angle;
   float cur_velocity = velocity;

   /* get the view position and orientation */
   getViewPosition(&x_cord, &y_cord, &z_cord);
   getViewOrientation(&x_orient, &y_orient, &z_orient);

   /* create string message to pass to client */
   sprintf(msgToServer, "%d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf"
                        , bullet, mobID, drawMob, timeInMotion, server_x, server_y,
                          server_z, x_angle, y_angle, x_cord, y_cord, z_cord,
                          x_orient, y_orient, z_orient, cur_angle, cur_velocity);

   /* write message to server */
   write(socketID, msgToServer, sizeof(msgToServer));
}

void readClientMsg( int *bullet, int *mobID, int *drawMob, float *timeInMotion,
                    float *server_x, float *server_y, float *server_z,
                    float *x_angle, float *y_angle, float *x_cord,
                    float *y_cord, float *z_cord, float *x_orient, 
                    float *y_orient, float *z_orient, float *gunAngle, float *gunVelocity, char *msg)
{
   /* parse the message */
   sscanf( msg, "%d %d %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f"
               , bullet, mobID, drawMob, timeInMotion, server_x, server_y,
                 server_z, x_angle, y_angle, x_cord, y_cord, z_cord,
                 x_orient, y_orient, z_orient, gunAngle, gunVelocity);
}

/* a function to allow the player to jump 2 vertical square units */
void jump()
{
   int x, y, z;
   float x_cord, y_cord, z_cord;

   /* get the initial position */
   getViewPosition(&x_cord, &y_cord, &z_cord);

   /* convert the cordinates to positive integers */
   x = (int)x_cord * -1;
   y = (int)y_cord * -1;
   z = (int)z_cord * -1;

   if ( world[x][y][z] == 0 && world[x][y+1][z] == 0 && world[x][y+2][z] == 0 && world[x][y+3][z] == 0 )
   {
      /* jump up 2 vertical squares */
      setViewPosition(x_cord, y_cord - 3, z_cord);
   }
}

/* this function checks if the given input puts the AI out of bounds */
int AICheckBounds(int x, int y, int z)
{
   /* ensure the player is on the map */
   if ( x < 2 || x > 98 || y < 4 || y > 47 || z < 2 || z > 98 )
   {
      return 0;
   }

   /* only returns 1 if A.I. is allowed to go there */
   return 1;
}

/* this function will determine the north south obstical avoidance */
void AINorthSouthDetection()
{
   float player_x_cord, player_y_cord, player_z_cord, player_roty = 0.0f;
   int player_x, player_y, player_z, isInBounds1 = 0, isInBounds2 = 0;

   /* the A.I. will need to go off route */
   onRoute = 0;

   /* try bypassing the obstical */
   /* get the initial position */
   getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

   /* convert the cordinates to integers, unlike getViewPosition they are already positive */
   player_x = (int)player_x_cord;
   player_y = (int)player_y_cord;
   player_z = (int)player_z_cord;

      /* try going right 1 square */
   if ( world[player_x+1][player_y][player_z] == 0 && (isInBounds1 = AICheckBounds(player_x+1, player_y, player_z)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord, player_roty );
   }
   else if ( world[player_x+1][player_y+1][player_z] == 0 && world[player_x+1][player_y+2][player_z] == 0
             && (isInBounds1 = AICheckBounds(player_x+1, player_y+1, player_z)) == 1 && (isInBounds2 = AICheckBounds(player_x+1, player_y+2, player_z)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord + 1, player_z_cord, player_roty );
   }

   /* try going left 1 square */
   else if ( world[player_x-1][player_y][player_z] == 0 && (isInBounds1 = AICheckBounds(player_x-1, player_y, player_z)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord, player_roty );
   }
   else if ( world[player_x-1][player_y+1][player_z] == 0 && world[player_x-1][player_y+2][player_z] == 0 
             && (isInBounds1 = AICheckBounds(player_x-1, player_y+1, player_z)) == 1 && (isInBounds2 = AICheckBounds(player_x-1, player_y+2, player_z)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord + 1, player_z_cord, player_roty );
   }

   /* try going 1 right and 1 up (dyagonally 1) */
   else if ( world[player_x+1][player_y][player_z+1] == 0 && (isInBounds1 = AICheckBounds(player_x+1, player_y, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord+1, player_roty );
   }
   else if ( world[player_x+1][player_y+1][player_z+1] == 0 && world[player_x+1][player_y+2][player_z+1] == 0 
             && (isInBounds1 = AICheckBounds(player_x+1, player_y+1, player_z+1)) == 1 && (isInBounds2 = AICheckBounds(player_x+1, player_y+2, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord + 1, player_z_cord + 1, player_roty );
   }

   /* try going 1 left and 1 up (dyagonally 1) */
   else if ( world[player_x-1][player_y][player_z+1] == 0 && (isInBounds1 = AICheckBounds(player_x-1, player_y, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord + 1, player_roty );
   }
   else if ( world[player_x-1][player_y+1][player_z+1] == 0 && world[player_x-1][player_y+2][player_z+1] == 0 
             && (isInBounds1 = AICheckBounds(player_x-1, player_y+1, player_z+1)) == 1 && (isInBounds2 = AICheckBounds(player_x-1, player_y+2, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord + 1, player_z_cord + 1, player_roty );
   }

   /* try going 1 right and 1 down (dyagonally 1) */
   else if ( world[player_x+1][player_y][player_z-1] == 0 && (isInBounds1 = AICheckBounds(player_x+1, player_y, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord-1, player_roty );
   }
   else if ( world[player_x+1][player_y+1][player_z-1] == 0 && world[player_x+1][player_y+2][player_z-1] == 0 
             && (isInBounds1 = AICheckBounds(player_x+1, player_y+1, player_z-1)) == 1 && (isInBounds2 = AICheckBounds(player_x+1, player_y+2, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord + 1, player_z_cord - 1, player_roty );
   }

   /* try going 1 left anf 1 down (dyagonally 1) */
   else if ( world[player_x-1][player_y][player_z-1] == 0 && (isInBounds1 = AICheckBounds(player_x-1, player_y, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
   }
   else if ( world[player_x-1][player_y+1][player_z-1] == 0 && world[player_x-1][player_y+2][player_z-1] == 0 
             && (isInBounds1 = AICheckBounds(player_x-1, player_y+1, player_z-1)) == 1 && (isInBounds2 = AICheckBounds(player_x-1, player_y+2, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord + 1, player_z_cord - 1, player_roty );
   }

   /* try going 1 back */
   else if ( world[player_x][player_y][player_z-1] == 0 && (isInBounds1 = AICheckBounds(player_x, player_y, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord-1, player_roty );
   }
   else if ( world[player_x][player_y+1][player_z-1] == 0 && world[player_x][player_y+2][player_z-1] == 0 
             && (isInBounds1 = AICheckBounds(player_x, player_y+1, player_z-1)) == 1 && (isInBounds2 = AICheckBounds(player_x, player_y+2, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord, player_y_cord + 1, player_z_cord - 1, player_roty );
   } 

   /* he must be stuck in a hole */
   else
   {
      /* he must be stuck in a hole so restart A.I. from middle of map */
      setPlayerPosition( 0, 50, 40, 50, 180.0);

      /* initialize the A.I. to head south on route */
      headingSouth = 1;
      headingNorth = 0;
      headingEast = 0;
      headingWest = 0;
      onRoute = 1;
   }

}

/* this function will determine the east west obstical avoidance */
void AIEastWestDetection()
{
   float player_x_cord, player_y_cord, player_z_cord, player_roty = 0.0f;
   int player_x, player_y, player_z, isInBounds1 = 0, isInBounds2 = 0;

   /* the A.I. will need to go off route */
   onRoute = 0;

   /* try bypassing the obstical */
   /* get the initial position */
   getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

   /* convert the cordinates to integers, unlike getViewPosition they are already positive */
   player_x = (int)player_x_cord;
   player_y = (int)player_y_cord;
   player_z = (int)player_z_cord;

      /* try going right 1 square */
   if ( world[player_x][player_y][player_z+1] == 0 && (isInBounds1 = AICheckBounds(player_x, player_y, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord + 1, player_roty );
   }
   else if ( world[player_x][player_y+1][player_z+1] == 0 && world[player_x][player_y+2][player_z+1] == 0
             && (isInBounds1 = AICheckBounds(player_x, player_y+1, player_z+1)) == 1 && (isInBounds2 = AICheckBounds(player_x, player_y+2, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord, player_y_cord + 1, player_z_cord + 1, player_roty );
   }

   /* try going left 1 square */
   else if ( world[player_x][player_y][player_z-1] == 0 && (isInBounds1 = AICheckBounds(player_x, player_y, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord - 1, player_roty );
   }
   else if ( world[player_x][player_y+1][player_z-1] == 0 && world[player_x][player_y+2][player_z-1] == 0 
             && (isInBounds1 = AICheckBounds(player_x, player_y+1, player_z-1)) == 1 && (isInBounds2 = AICheckBounds(player_x, player_y+2, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord, player_y_cord + 1, player_z_cord - 1, player_roty );
   }

   /* try going 1 right and 1 up (dyagonally 1) */
   else if ( world[player_x+1][player_y][player_z+1] == 0 && (isInBounds1 = AICheckBounds(player_x+1, player_y, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord+1, player_roty );
   }
   else if ( world[player_x+1][player_y+1][player_z+1] == 0 && world[player_x+1][player_y+2][player_z+1] == 0 
             && (isInBounds1 = AICheckBounds(player_x+1, player_y+1, player_z+1)) == 1 && (isInBounds2 = AICheckBounds(player_x+1, player_y+2, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord + 1, player_z_cord + 1, player_roty );
   }

   /* try going 1 left and 1 up (dyagonally 1) */
   else if ( world[player_x+1][player_y][player_z-1] == 0 && (isInBounds1 = AICheckBounds(player_x+1, player_y, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord - 1, player_roty );
   }
   else if ( world[player_x+1][player_y+1][player_z-1] == 0 && world[player_x+1][player_y+2][player_z-1] == 0 
             && (isInBounds1 = AICheckBounds(player_x+1, player_y+1, player_z-1)) == 1 && (isInBounds2 = AICheckBounds(player_x+1, player_y+2, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord + 1, player_y_cord + 1, player_z_cord - 1, player_roty );
   }

   /* try going 1 right and 1 down (dyagonally 1) */
   else if ( world[player_x-1][player_y][player_z+1] == 0 && (isInBounds1 = AICheckBounds(player_x-1, player_y, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord + 1, player_roty );
   }
   else if ( world[player_x-1][player_y+1][player_z+1] == 0 && world[player_x-1][player_y+2][player_z+1] == 0 
             && (isInBounds1 = AICheckBounds(player_x-1, player_y+1, player_z+1)) == 1 && (isInBounds2 = AICheckBounds(player_x-1, player_y+2, player_z+1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord + 1, player_z_cord + 1, player_roty );
   }

   /* try going 1 left anf 1 down (dyagonally 1) */
   else if ( world[player_x-1][player_y][player_z-1] == 0 && (isInBounds1 = AICheckBounds(player_x-1, player_y, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
   }
   else if ( world[player_x-1][player_y+1][player_z-1] == 0 && world[player_x-1][player_y+2][player_z-1] == 0 
             && (isInBounds1 = AICheckBounds(player_x-1, player_y+1, player_z-1)) == 1 && (isInBounds2 = AICheckBounds(player_x-1, player_y+2, player_z-1)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord + 1, player_z_cord - 1, player_roty );
   } 

   /* try going 1 back */
   else if ( world[player_x-1][player_y][player_z] == 0 && (isInBounds1 = AICheckBounds(player_x-1, player_y, player_z)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord, player_roty );
   }
   else if ( world[player_x-1][player_y+1][player_z] == 0 && world[player_x-1][player_y+2][player_z] == 0 
             && (isInBounds1 = AICheckBounds(player_x-1, player_y+1, player_z)) == 1 && (isInBounds2 = AICheckBounds(player_x-1, player_y+2, player_z)) == 1 )
   {
      setPlayerPosition(0, player_x_cord - 1, player_y_cord + 1, player_z_cord, player_roty );
   }

   /* he must be stuck in a hole */
   else
   {
      /* he must be stuck in a hole so restart A.I. from middle of map */
      setPlayerPosition( 0, 50, 40, 50, 180.0);

      /* initialize the A.I. to head south on route */
      headingSouth = 1;
      headingNorth = 0;
      headingEast = 0;
      headingWest = 0;
      onRoute = 1;
   }

}

/* this function will deal with all the AI Collision Detection */
void AICollisionDetection()
{
   /* if the A.I. is heading in the north or south directions */
   if ( headingNorth == 1 || headingSouth == 1 )
   {
      AINorthSouthDetection();
   }
   /* if the A.I. is heading in the east or west directions */
   else if ( headingEast == 1 || headingWest == 1 )
   {
      AIEastWestDetection();
   }  

}

	/*** collisionResponse() ***/
	/* -performs collision detection and response */
	/*  sets new xyz  to position of the viewpoint after collision */
	/* -can also be used to implement gravity by updating y position of vp*/
	/* note that the world coordinates returned from getViewPosition()
	   will be the negative value of the array indices */
void collisionResponse() 
{

   float x_cord, y_cord, z_cord;
   float player_x_cord, player_y_cord, player_z_cord, player_roty;

   static int fallSlowAgain = 0;

   int x, y, z, i;
   int player_x, player_y, player_z;

   /******** GET THE POSITIONS ******************/

   /* get the initial positions */
   getViewPosition(&x_cord, &y_cord, &z_cord);
   getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

   /*convert the cordinates to positive integers */
   x = (int)x_cord * -1;
   y = (int)y_cord * -1;
   z = (int)z_cord * -1;

   /* convert the cordinates to integers, unlike getViewPosition they are already positive */
   player_x = (int)player_x_cord;
   player_y = (int)player_y_cord;
   player_z = (int)player_z_cord;

   /********** View Point ***************/

   /* ensure we are on the map */
   if ( x < 2 || x > 98 || y < 4 || y > 47 || z < 2 || z > 98 )
   {
      getOldViewPosition(&x_cord, &y_cord, &z_cord);
      setViewPosition(x_cord, y_cord, z_cord );
   }

   /* ensure we are not in contact with the player */
   if( x == player_x && y == player_y && z == player_z )
   {
      getOldViewPosition(&x_cord, &y_cord, &z_cord);
      setViewPosition(x_cord, y_cord, z_cord );
   }

   /* allow for climbing 1 block or jump 2 blocks */
   if( world[x][y][z] != 0 )
   {
      if( world[x][y+1][z] == 0 && world[x][y+2][z] == 0)
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
   if( world[x][y][z] == 0 && world[x][y-1][z] == 0 && y > 3 && fallSlowAgain%5 == 0)
   {
      setViewPosition(x_cord, y_cord + 1, z_cord );
   }

   /*********** Player **************/

   /* ensure the player is on the map */
   if ( player_x < 2 || player_x > 98 || player_y < 4 || player_y > 47 || player_z < 2 || player_z > 98 )
   {
      getOldPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);
      setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
   }

   /* ensure the player cannont move to the viewpoints position */
   if( x == player_x && y == player_y && z == player_z )
   {
      getOldPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);
      setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
   }

   /* allow the player to climb 1 block */
   if( world[player_x][player_y][player_z] != 0 )
   {
      if( world[player_x][player_y+1][player_z] == 0 && world[player_x][player_y+2][player_z] == 0)
      {
         setPlayerPosition(0, player_x_cord, player_y_cord + 1, player_z_cord, player_roty );
      }
      else
      {
         getOldPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);
         setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );

         /* find a way around the obstical */
         AICollisionDetection();
      }

   }

   /* apply gravity */
   if( world[player_x][player_y][player_z] == 0 && world[player_x][player_y-1][player_z] == 0 && player_y > 3 && fallSlowAgain%5 == 0)
   {
      setPlayerPosition(0, player_x_cord, player_y_cord - 1, player_z_cord, player_roty );
   }

   fallSlowAgain++;
}


	/******* draw2D() *******/
	/* draws 2D shapes on screen */
	/* use the following functions: 			*/
	/*	draw2Dline(int, int, int, int, int);		*/
	/*	draw2Dbox(int, int, int, int);			*/
	/*	draw2Dtriangle(int, int, int, int, int, int);	*/
	/*	set2Dcolour(float []); 				*/
	/* colour must be set before other functions are called	*/
void draw2D() {

   float x_cord, y_cord, z_cord;
   float player_x_cord, player_y_cord, player_z_cord, player_roty;
   int x, y, z, i;
   int player_x, player_y, player_z;
   int posX, posZ;
   int player_posX, player_posZ;
   int x_topRight, x_bottomRight, z_topRight, z_bottomRight;

   if (testWorld) {
		/* draw some sample 2d shapes */
      GLfloat green[] = {0.0, 0.5, 0.0, 0.5};
      set2Dcolour(green);
      draw2Dline(0, 0, 500, 500, 15);
      draw2Dtriangle(0, 0, 200, 200, 0, 200);

      GLfloat black[] = {0.0, 0.0, 0.0, 0.5};
      set2Dcolour(black);
      draw2Dbox(500, 380, 524, 388);
   }
   else
   {
      GLfloat green[] = {0.0, 0.5, 0.0, 0.5};
      GLfloat red[] = {0.5,0.0,0.0,1.0};

      /* make viewpoint and viewpoint bullets green */
      set2Dcolour(green);

      /* display the small map in the top right corner of the screen */
      if ( displayMap == 1 )
      {
         /* create the coordinates for the top right corner map */
         x_topRight = screenWidth - 2;
         z_topRight = screenHeight - 2;

         x_bottomRight = x_topRight - 200;
         z_bottomRight = z_topRight - 200;

         /* create the coordinates for the center map */
         for ( i = 0; i < 5; i++)
         {
            if( bullet[i].drawMob == 1)
            {
               x = (int)bullet[i].x;
               z = (int)bullet[i].z;

               posX = x_bottomRight + x *2;
               posZ = z_bottomRight + z *2;

               draw2Dbox(posX-3, posZ-3, posX+3, posZ+3);
            }
         }

         /********* VIEWPOINT **********/

         /* get the initial position */
         getViewPosition(&x_cord, &y_cord, &z_cord);

         /* convert the cordinates to positive integers */
         x = (int)x_cord * -1;
         z = (int)z_cord * -1;

         /* get current position on mini map */
         posX = x_bottomRight + x *2;
         posZ = z_bottomRight + z *2;

         /* draw user on the mini map */
         draw2Dbox(posX-5, posZ-5, posX+5, posZ+5);

         /********** A.I. ************/

         /* make A.I. red */
         set2Dcolour(red);

         /* get the A.I. player's position */
         getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

         /* convert the cordinates to integers */
         player_x = (int)player_x_cord;
         player_z = (int)player_z_cord;

         /* get current position on mini map */
         player_posX = x_bottomRight + player_x *2;
         player_posZ = z_bottomRight + player_z *2;

         /* draw A.I. on the mini map */
         draw2Dbox(player_posX-5, player_posZ-5, player_posX+5, player_posZ+5);

         /******** BACKGROUND ************/

         /* draw background for mini map */
         GLfloat black[] = {0.0, 0.0, 0.0, 0.5};
         set2Dcolour(black);
         draw2Dbox(x_bottomRight, z_bottomRight, x_topRight, z_topRight);

         /********* EDGES **************/

         /* draw world edges */
         set2Dcolour(red);
         /* left edge */
         draw2Dline(x_bottomRight, z_bottomRight, x_bottomRight, z_topRight, 15);
         /* bottom edge */
         draw2Dline(x_bottomRight, z_bottomRight, x_topRight, z_bottomRight, 15);
         /* right edge */
         draw2Dline(x_topRight, z_bottomRight, x_topRight, z_topRight, 15);
         /* top edge */
         draw2Dline(x_topRight, z_topRight, x_bottomRight, z_topRight, 15);

      }
      /* display the big map in the middle of the screen */
      else if ( displayMap == 2 )
      {
         /* create the coordinates for the center map */
         x_topRight = screenWidth - 274;
         z_topRight = screenHeight - 116;

         x_bottomRight = x_topRight - 500;
         z_bottomRight = z_topRight - 500;

         /* draw bullets on the mini map when they appear */
         for ( i = 0; i < 5; i++)
         {
            if( bullet[i].drawMob == 1)
            {
               x = (int)bullet[i].x;
               z = (int)bullet[i].z;

               posX = x_bottomRight + x *5;
               posZ = z_bottomRight + z *5;

               draw2Dbox(posX-3, posZ-3, posX+3, posZ+3);
            }
         }

         /********* VIEWPOINT **********/

         /* get the initial position */
         getViewPosition(&x_cord, &y_cord, &z_cord);

         /* convert the cordinates to positive integers */
         x = (int)x_cord * -1;
         z = (int)z_cord * -1;

         /* get current position on mini map */
         posX = x_bottomRight + x *5;
         posZ = z_bottomRight + z *5;

         /* draw user on the mini map */
         draw2Dbox(posX-5, posZ-5, posX+5, posZ+5);

         /********** A.I. ************/

         /* make A.I. red */
         set2Dcolour(red);

         /* get the A.I. player's position */
         getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

         /* convert the cordinates to integers */
         player_x = (int)player_x_cord;
         player_z = (int)player_z_cord;

         /* get current position on mini map */
         player_posX = x_bottomRight + player_x *5;
         player_posZ = z_bottomRight + player_z *5;

         /* draw A.I. on the mini map */
         draw2Dbox(player_posX-5, player_posZ-5, player_posX+5, player_posZ+5);

         /******** BACKGROUND ************/

         /* draw background for mini map */
         GLfloat black[] = {0.0, 0.0, 0.0, 0.5};
         set2Dcolour(black);
         draw2Dbox(x_bottomRight, z_bottomRight, x_topRight, z_topRight);

         /********* EDGES ************/

         /* draw world edges */
         set2Dcolour(red);
         /* left edge */
         draw2Dline(x_bottomRight, z_bottomRight, x_bottomRight, z_topRight, 15);
         /* bottom edge */
         draw2Dline(x_bottomRight, z_bottomRight, x_topRight, z_bottomRight, 15);
         /* right edge */
         draw2Dline(x_topRight, z_bottomRight, x_topRight, z_topRight, 15);
         /* top edge */
         draw2Dline(x_topRight, z_topRight, x_bottomRight, z_topRight, 15);
      }
   }

}

	/*** update() ***/
	/* background process, it is called when there are no other events */
	/* -used to control animations and perform calculations while the  */
	/*  system is running */
	/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
   
   int i, j, k, x, y, z;
   int player_x, player_y, player_z;
   float *la;
   float x_cord, y_cord, z_cord;
   float player_x_cord, player_y_cord, player_z_cord, player_roty;
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
      /* if network mode is on */
      if ( networkMode == 1 )
      {
         /* if the server flag is set */
         if ( netServer == 1 )
         {
            char msg[500000];
            int index, drawMob, isBullet;
            float timeInMotion, server_x, server_y, server_z, x_angle, y_angle;
            float server_XCord, server_YCord, server_ZCord;
            float server_XOrient, server_YOrient, server_ZOrient;
            float gunAngle, gunVelocity;

            /* read socket from server */
            read(clientSocketID, &msg, sizeof(msg));

            /* parse the msg */
            readClientMsg( &isBullet, &index, &drawMob, &timeInMotion,
                           &server_x, &server_y, &server_z,
                           &x_angle, &y_angle, &server_XCord,
                           &server_YCord, &server_ZCord, &server_XOrient, 
                           &server_YOrient, &server_ZOrient, &gunAngle, &gunVelocity, msg);

            if( isBullet == 1 )
            {
               bullet[index].drawMob = drawMob;
               bullet[index].timeInMotion = timeInMotion;
               bullet[index].x = server_x;
               bullet[index].y = server_y;
               bullet[index].z = server_z;
               bullet[index].x_angle = x_angle;
               bullet[index].y_angle = y_angle;
            }

            /* ensure we as the client are at the same location as the server */
            setViewPosition(server_XCord, server_YCord, server_ZCord );

            /* set our orientation to look down the barrel of the gun */
            // 45 is straight
            // 90 is up
            // 0 is down

            /* set globals */
            angle = gunAngle;
            velocity = gunVelocity;

            if (gunAngle > 45.0)
            {
               gunAngle = gunAngle * -1;

               setViewOrientation(server_XOrient, gunAngle, server_ZOrient);

            }
            else if ( gunAngle < 45.0)
            {
               gunAngle = gunAngle * -1;

               setViewOrientation(server_XOrient, gunAngle, server_ZOrient);
            }
            else
            {
               setViewOrientation(server_XOrient, server_YOrient, server_ZOrient);
            }
         }
         /* if the client flag is set */
         else if ( netClient = 1 )
         {
            /* tell server our position */
            sendMsgToServer( 0, 0, 0, 0, 0, 0, 0, 0, 0 );
         }
      }

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

      /******************* MOVING PLAYER ****************************/
      /* if the player A.I. is searching for you */
      if ( searching == 1 )
      {      
         /* if the player A.I. is on its predefined route */
         if ( onRoute == 1  && fallSlow%10 == 0 )
         {
            /* Heading True North */
            if( headingNorth == 1 && headingSouth == 0 && headingWest == 0 && headingEast == 0 )
            {
               /* get the initial position */
               getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

               /* convert the cordinates to integers, unlike getViewPosition they are already positive */
               player_x = (int)player_x_cord;
               player_y = (int)player_y_cord;
               player_z = (int)player_z_cord;

               /* A.I. is moving up the left hand side */
               if( player_x == 5 )
               {
                  /* move player up 1 block */
                  if ( player_z < 95 )
                  {
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  /* change player direction */
                  else if ( player_z == 95 )
                  {
                     /* rotate the player to face east */
                     player_roty = 90.0f;

                     /* set the new player position */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord, player_roty );

                     /* now the A.I. player should head east */
                     headingEast = 1;
                     headingNorth = 0;
                  }
                  else
                  {
                     printf("Heading North, then East Error\n");
                  }
                  
               }
               /* A.I. is moving up the right hand side */
               else if( player_x == 95 )
               {
                  /* move player up 1 block */
                  if ( player_z < 95 )
                  {
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  /* change player direction */
                  else if ( player_z == 95 )
                  {
                     /* rotate the player to face west */
                     player_roty = 90.0f * -1;

                     /* set the new player position */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord, player_roty );
                     
                     /* now the A.I. player should head west */
                     headingWest = 1;
                     headingNorth = 0;
                  }
                  else
                  {
                     printf("Heading North, then West Error\n");
                  }

               }
               else
               {
                  printf("Heading North Error\n");
               }

            }
            /* Heading South */
            else if ( headingNorth == 0 && headingSouth == 1 && headingWest == 0 && headingEast == 0 )
            {
               /* get the initial position */
               getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

               /* convert the cordinates to integers, unlike getViewPosition they are already positive */
               player_x = (int)player_x_cord;
               player_y = (int)player_y_cord;
               player_z = (int)player_z_cord;

               /* A.I. is moving down the middle of the map */
               if( player_x == 50 )
               {
                  /* move player up 1 block */
                  if ( player_z > 5 )
                  {
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  /* change player direction randomly to head east or west */
                  else if ( player_z == 5 )
                  {
                     /* randomly get a 0 or 1 */
                     int choice = rand() % 2;

                     /* if choice is 0 go West */
                     if ( choice == 0 )
                     {
                        /* rotate the player to face west */
                        player_roty = 90.0f * -1;

                        /* set the new player position */
                        setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord, player_roty );

                        /* now the A.I. player should head east */
                        headingWest = 1;
                        headingSouth = 0;

                     }
                     /* if choice is 1 go East */
                     else
                     {
                        /* rotate the player to face east */
                        player_roty = 90.0f;

                        /* set the new player position */
                        setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord, player_roty );

                        /* now the A.I. player should head east */
                        headingEast = 1;
                        headingSouth = 0;
                     }

                  }
                  else
                  {
                     printf("Heading South, then East/West Error\n");
                  }
                  
               }
               else
               {
                  printf("Heading South Error\n");
               }

            }
            /* Heading West */
            else if ( headingNorth == 0 && headingSouth == 0 && headingWest == 1 && headingEast == 0 )
            {
               /* get the initial position */
               getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

               /* convert the cordinates to integers, unlike getViewPosition they are already positive */
               player_x = (int)player_x_cord;
               player_y = (int)player_y_cord;
               player_z = (int)player_z_cord;

               /* A.I. is moving along the bottom right hand side */
               if( player_z == 5 )
               {
                  /* move player up 1 block */
                  if ( player_x > 5 )
                  {
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord, player_roty );
                  }
                  /* change player direction */
                  else if ( player_x == 5 )
                  {
                     /* rotate the player to face north */
                     player_roty = 0.0f;

                     /* set the new player position */
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord + 1, player_roty );

                     /* now the A.I. player should head north */
                     headingWest = 0;
                     headingNorth = 1;
                  }
                  else
                  {
                     /* should be heading east */
                     /* rotate the player to face east */
                     player_roty = 90.0f;

                     /* now the A.I. player should head east */
                     headingEast = 1;
                     headingWest = 0;
                  }
                  
               }
               /* A.I. is moving along the top right hand side */
               else if( player_z == 95 )
               {
                  /* move player up 1 block */
                  if ( player_x > 50 )
                  {
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord, player_roty );
                  }
                  /* change player direction */
                  else if ( player_x == 50 )
                  {
                     /* rotate the player to face south */
                     player_roty = 180.0f;

                     /* set the new player position */
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord - 1, player_roty );
                     
                     /* now the A.I. player should head south */
                     headingWest = 0;
                     headingSouth = 1;
                  }
                  else
                  {
                     /* should be heading east */
                     /* rotate the player to face east */
                     player_roty = 90.0f;

                     /* now the A.I. player should head east */
                     headingEast = 1;
                     headingWest = 0;
                  }

               }
               else
               {
                  printf("Heading West Error\n");
               }

            }
            /* Heading East */
            else if ( headingNorth == 0 && headingSouth == 0 && headingWest == 0 && headingEast == 1 )
            {
               /* get the initial position */
               getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

               /* convert the cordinates to integers, unlike getViewPosition they are already positive */
               player_x = (int)player_x_cord;
               player_y = (int)player_y_cord;
               player_z = (int)player_z_cord;

               /* A.I. is moving along the top left hand side */
               if( player_z == 95 )
               {
                  /* move player up 1 block */
                  if ( player_x < 50 )
                  {
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord, player_roty );
                  }
                  /* change player direction */
                  else if ( player_x == 50 )
                  {
                     /* rotate the player to face south */
                     player_roty = 180.0f;

                     /* set the new player position */
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord - 1, player_roty );

                     /* now the A.I. player should head south */
                     headingEast = 0;
                     headingSouth = 1;
                  }
                  else
                  {
                     /* should be heading west */
                     /* rotate the player to face east */
                     player_roty = 90.0f * -1;

                     /* now the A.I. player should head east */
                     headingEast = 0;
                     headingWest = 1;
                  }
                  
               }
               /* A.I. is moving along the bottom right hand side */
               else if( player_z == 5 )
               {
                  /* move player up 1 block */
                  if ( player_x < 95 )
                  {
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord, player_roty );
                  }
                  /* change player direction */
                  else if ( player_x == 95 )
                  {
                     /* rotate the player to face north */
                     player_roty = 0.0f;

                     /* set the new player position */
                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord + 1, player_roty );
                     
                     /* now the A.I. player should head north */
                     headingEast = 0;
                     headingNorth = 1;
                  }
                  else
                  {
                     /* should be heading west */
                     /* rotate the player to face east */
                     player_roty = 90.0f * -1;

                     /* now the A.I. player should head east */
                     headingEast = 0;
                     headingWest = 1;
                  }

               }
               else
               {
                  printf("Heading East Error\n");
               }

            }
            /* the A.I. is stuck on the route */
            else
            {
               printf("North %d, South %d, East %d, West %d\n", headingNorth, headingSouth, headingEast, headingWest);
               printf("A.I. Stuck need to go of route\n");
            }

         }
         /* player A.I. is not on predefined route */
         else
         {
            /* get the initial position */
            getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

            /* convert the cordinates to integers, unlike getViewPosition they are already positive */
            player_x = (int)player_x_cord;
            player_y = (int)player_y_cord;
            player_z = (int)player_z_cord;

            if ( headingNorth == 1 )
            {
               if( player_x == 5 && player_z >= 5 && player_z <= 95 || player_x == 95 && player_z >= 5 && player_z <= 95 )
               {
                  /* the A.I. is back on route */
                  onRoute = 1;
                  headingNorth = 1;
                  headingSouth = 0;
                  headingEast = 0;
                  headingWest = 0;
               }
               else if ( player_x < 5 )
               {
                  if ( player_z < 5 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  else if ( player_z > 5 && player_z < 95 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  /* we should be going east */
                  else if ( player_z > 95 )
                  {
                     /* face east */
                     player_roty = 90.0f;

                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord - 1, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 1;
                     headingWest = 0;
                  }
               }
               else if ( player_x > 5 && player_x < 50 )
               {
                  if ( player_z < 5 )
                  {
                     /* face west */
                     player_roty = 90.0f * -1;

                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord + 1, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 0;
                     headingWest = 1;
                  }
                  else if ( player_z > 5 && player_z < 95 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  /* we should be going east */
                  else if ( player_z > 95 )
                  {
                     /* face east */
                     player_roty = 90.0f;

                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord - 1, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 1;
                     headingWest = 0;
                  }
               }
               else if ( player_x == 50 )
               {
                  /* face south */
                  player_roty = 180.0f;

                  setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                  headingNorth = 0;
                  headingSouth = 1;
                  headingEast = 0;
                  headingWest = 0;
                  onRoute = 1;
               }
               else if ( player_x > 50 && player_x < 95 )
               {
                  if ( player_z < 5 )
                  {
                     /* face east */
                     player_roty = 90.0f;

                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord + 1, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 1;
                     headingWest = 0;
                  }
                  else if ( player_z > 5 && player_z < 95 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  /* we should be going east */
                  else if ( player_z > 95 )
                  {
                     /* face west */
                     player_roty = 90.0f * -1;

                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 0;
                     headingWest = 1;
                  }
               }
               else if ( player_x > 95 )
               {
                  if ( player_z < 5 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  else if ( player_z > 5 && player_z < 95 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  /* we should be going east */
                  else if ( player_z > 95 )
                  {
                     /* face west */
                     player_roty = 90.0f * -1;

                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 0;
                     headingWest = 1;
                  }
               }

            }
            else if ( headingSouth == 1 )
            {
               if( player_x == 5 || player_x == 95 )
               {
                  /* face north */
                  player_roty = 0.0f;

                  setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                  onRoute = 1;
                  headingNorth = 1;
                  headingSouth = 0;
                  headingEast = 0;
                  headingWest = 0;
               }
               else if ( player_x < 5 )
               {
                  /* try to get back on track */
                  setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord - 1, player_roty );
               }
               else if ( player_x > 5 && player_x < 50 )
               {
                  /* try to get back on track */
                  setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord - 1, player_roty );
               }
               else if ( player_x == 50 )
               {
                  /* the A.I. is back on route */
                  onRoute = 1;
                  headingNorth = 0;
                  headingSouth = 1;
                  headingEast = 0;
                  headingWest = 0;
               }
               else if ( player_x > 50 && player_x < 95 )
               {
                  /* try to get back on track */
                  setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
               }
               else if ( player_x > 95 )
               {
                  /* try to get back on track */
                  setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
               }

            }
            else if ( headingWest == 1 )
            {
               if ( player_z == 5 && player_x < 50 || player_z == 95 && player_x >= 50 )
               {
                  /* we are back on track */
                  onRoute = 1;
                  headingNorth = 0;
                  headingSouth = 0;
                  headingEast = 0;
                  headingWest = 1;
               }
               else if ( player_z < 5 )
               {
                  if ( player_x < 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  /* should be going east instead */
                  else if ( player_x >= 50 )
                  {
                     /* face east */
                     player_roty = 90.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 1;
                     headingWest = 0;
                  }
               }
               else if ( player_z > 5 && player_z < 95 )
               {
                  if ( player_x < 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  else if ( player_x > 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  else if ( player_x == 50 )
                  {
                     /* face south */
                     player_roty = 180.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 1;
                     headingEast = 0;
                     headingWest = 0;
                     onRoute = 1;

                  }
               }
               else if ( player_z > 95 )
               {
                  if ( player_x < 50 )
                  {
                     /* face east */
                     player_roty = 90.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 1;
                     headingWest = 0;
                  }
                  /* should be going west */
                  else if ( player_x > 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  else if ( player_x == 50 )
                  {
                     /* face south */
                     player_roty = 180.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 1;
                     headingEast = 0;
                     headingWest = 0;
                     onRoute = 1;
                  }
               }
            }
            else if ( headingEast == 1 )
            {
               if ( player_z == 5 && player_x > 50 || player_z == 95 && player_x <= 50 )
               {
                  /* we are back on track */
                  onRoute = 1;
                  headingNorth = 0;
                  headingSouth = 0;
                  headingEast = 0;
                  headingWest = 1;
               }
               else if ( player_z < 5 )
               {
                  /* should be going east instead */
                  if ( player_x < 50 )
                  {
                      /* face west */
                     player_roty = 90.0f * -1;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 0;
                     headingWest = 1;
                  }
                  else if ( player_x > 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  else if ( player_x == 50 )
                  {
                     /* face south */
                     player_roty = 180.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 1;
                     headingEast = 0;
                     headingWest = 0;
                     onRoute = 1;
                  }
               }
               else if ( player_z > 5 && player_z < 95 )
               {
                  if ( player_x < 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord + 1, player_roty );
                  }
                  else if ( player_x > 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord - 1, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  else if ( player_x == 50 )
                  {
                     /* face south */
                     player_roty = 180.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 1;
                     headingEast = 0;
                     headingWest = 0;
                     onRoute = 1;

                  }
               }
               else if ( player_z > 95 )
               {
                  if ( player_x < 50 )
                  {
                     /* try to get back on track */
                     setPlayerPosition(0, player_x_cord + 1, player_y_cord, player_z_cord - 1, player_roty );
                  }
                  /* should be going west instead */
                  else if ( player_x > 50 )
                  {
                     /* face east */
                     player_roty = 90.0f * -1;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 0;
                     headingEast = 0;
                     headingWest = 1;
                  }
                  else if ( player_x == 50 )
                  {
                     /* face south */
                     player_roty = 180.0f;

                     setPlayerPosition(0, player_x_cord, player_y_cord, player_z_cord, player_roty );
                     headingNorth = 0;
                     headingSouth = 1;
                     headingEast = 0;
                     headingWest = 0;
                     onRoute = 1;

                  }
               }

            }
         }
      }
      /* the A.I. is in fight mode */
      else if ( searching == 0 )
      {
         // TODO 
         // calculate stright line
         // shoot bullet
      }

      /****************** BULLETS/PROJECTILES **************************/
      for ( i = 0; i < 5; i++)
      {
          if( bullet[i].drawMob == 1)
          {
              bullet[i].x = bullet[i].x + sin(bullet[i].y_angle * CONVERT_TO_DECIMAL );
              bullet[i].z = bullet[i].z - cos(bullet[i].y_angle * CONVERT_TO_DECIMAL );

              bullet[i].y = bullet[i].y + ( velocity * sin(angle) - 0.5 * 9.81 * ( bullet[i].timeInMotion * bullet[i].timeInMotion ) );

              if( bullet[i].x < 1.0f || bullet[i].x > 98.0f || bullet[i].z < 1.0f || bullet[i].z > 98.0f
                  || bullet[i].y < 3.0f || bullet[i].y > 98.0f  )
              {
                  bullet[i].drawMob = 0;
                  hideMob(i);
              }
              else
              {
                showMob(i);
                setMobPosition(i, bullet[i].x, bullet[i].y, bullet[i].z, 0.0);
              }

              bullet[i].timeInMotion += 0.01f;
          }
      }

      for ( i = 0; i < 5; i++)
      {
          if( bullet[i].drawMob == 1)
          {
              int cur_x = (int)bullet[i].x;
              int cur_y = (int)bullet[i].y;
              int cur_z = (int)bullet[i].z;

              if ( world[cur_x][cur_y][cur_z] != 0)
              {
                  
                  /* dont draw this mob again */
                  bullet[i].drawMob = 0;

                  hideMob(i);
                  /* destroy land */
                  world[cur_x][cur_y][cur_z] = 0;
                  world[cur_x+1][cur_y][cur_z] = 0;
                  world[cur_x+2][cur_y][cur_z] = 0;
                  world[cur_x-1][cur_y][cur_z] = 0;
                  world[cur_x-2][cur_y][cur_z] = 0;
                  world[cur_x][cur_y-1][cur_z] = 0;
                  world[cur_x-1][cur_y-1][cur_z] = 0;
                  world[cur_x+1][cur_y-1][cur_z] = 0;
                  world[cur_x][cur_y-2][cur_z] = 0;

              }
          }
      }

      /*********** GRAVITY AND COLISION DETECTION *******************/

      /********* VIEWPOINT GRAVITY *****************/

      /* get the initial position */
      getViewPosition(&x_cord, &y_cord, &z_cord);

      /* convert the cordinates to positive integers */
      x = (int)x_cord * -1;
      y = (int)y_cord * -1;
      z = (int)z_cord * -1;

      /* apply gravity */
      if( world[x][y][z] == 0 && world[x][y-1][z] == 0 && y > 3 && fallSlow%5 == 0 && gravityFlag == 1)
      {
         setViewPosition(x_cord, y_cord + 1, z_cord );
      }

      /********* PLAYER GRAVITY *****************/

      /* get the initial position */
      getPlayerPosition(0, &player_x_cord, &player_y_cord, &player_z_cord, &player_roty);

      /* convert the cordinates to integers, unlike getViewPosition they are already positive */
      player_x = (int)player_x_cord;
      player_y = (int)player_y_cord;
      player_z = (int)player_z_cord;

      /* apply gravity */
      if( world[player_x][player_y][player_z] == 0 && world[player_x][player_y-1][player_z] == 0 && player_y > 3 && fallSlow%5 == 0 && gravityFlag == 1)
      {
         setPlayerPosition(0, player_x_cord, player_y_cord - 1, player_z_cord, player_roty );
      }

      fallSlow++;

      /********************** JUMPING *******************************/
      if ( space == 1 )
      {
         time_t now;
         double seconds;

         /* ensure that this function has been called at least once */
         if ( firstUpdate != 0 )
         {
            /* get the current time */
            time(&now);

            /* calculate the difference in time */
            seconds = difftime(old, now);

            /* only allow jump every 5 seconds */
            if ( seconds < 5.0f * -1 )
            {
               /* jump 2 vertical square units */
               jump();

               /* reverse gravity so that the user feels like they have hangtime */
               gravityFlag = 0;

               /* set old time equal to new time */
               old = now;
            }
         }
         /* first time through, so get the time of first jump */
         else
         {
            /* set firstUpdate flag */
            firstUpdate = 1;

            /* capture the time of inital jump */
            time(&old);

            /* jump 2 vertical square units */
            jump();

            /* reverse gravity so that the user feels like they have hangtime */
            gravityFlag = 0;
         }

         /* unset the space bar 'pressed' flag */
         space = 0;
      }

      /********************** JUMP HANGTIME ***************************************/
      time_t now;
      double seconds;

      /* get the current time */
      time(&now);

      /* calculate the difference in time */
      seconds = difftime(old, now);

      /* reset gravity after 1 second hang time */
      if ( seconds < 0.5f * -1 )
      {
         gravityFlag = 1;
      }
   }
}


	/* called by GLUT when a mouse button is pressed or released */
	/* -button indicates which button was pressed or released */
	/* -state indicates a button down or button up event */
	/* -x,y are the screen coordinates when the mouse is pressed or */
	/*  released */ 
void mouse(int button, int state, int x, int y) {

   float x_orient, mod_x, x_position;
   float y_orient, mod_y, y_position;
   float z_orient, mod_z, z_position;
   int changedVelocity = 0;
   int changedAngle = 0;
   static int mobID = 0;

   /* only allow 10 shots at once */
   if ( mobID >= 5)
   {
        mobID = 0;
   }

   if (button == GLUT_LEFT_BUTTON)
   {
        printf("left button - ");

        /* if in network mode only allow the client to shoot a bullet,
           if not in network mode allow user to shoot a bullet
        */
        if( netClient == 1 || networkMode == 0)
        {
           /* get the current mouse orientation */
           getViewOrientation(&x_orient, &y_orient, &z_orient);

           /* make sure to mod values by 360 degrees */
           mod_x = fmodf(x_orient, 360.0);
           mod_y = fmodf(y_orient, 360.0);

           /* get the current view position */
           getViewPosition(&x_position, &y_position, &z_position);

           /* create the mob */
           createMob(mobID, -1 * x_position, -1 * y_position, -1 * z_position, 0.0 );

           /* add the new mob to the global array of structures */
           bullet[mobID].drawMob = 1;
           bullet[mobID].timeInMotion = 0;
           bullet[mobID].x = x_position * -1;
           bullet[mobID].y = y_position * -1;
           bullet[mobID].z = z_position * -1;
           bullet[mobID].x_angle = mod_x;
           bullet[mobID].y_angle = mod_y;

           /* only need to send message in networking mode */
           if ( netClient == 1 )
           {
               /* send over the bullet information */
               sendMsgToServer( 1, mobID, bullet[mobID].drawMob, bullet[mobID].timeInMotion,
                             bullet[mobID].x, bullet[mobID].y, bullet[mobID].z,
                             bullet[mobID].x_angle, bullet[mobID].y_angle );
           }
        }
   }
   else if (button == GLUT_MIDDLE_BUTTON)
   {

      printf("middle button - ");
   }
   else
   {
      printf("right button - ");

      /* if in network mode only allow the client to move cannon,
         if not in network mode allow user to move cannon
      */
      if( netClient == 1 || networkMode == 0)
      {
         /* decrease the velocity */
         if( button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
         {
            initialY = y;
            initialX = x;
            printf("xy before: %d,%d\n",x,y );
         }
         if( button == GLUT_RIGHT_BUTTON && state == GLUT_UP )
         {
            printf("xy after: %d,%d\n",x,y );
         }

         if( x < initialX && velocity >= 0.005 && velocity <= 1.00 )
         {
            velocity = velocity - 0.005;
            printf("left:%lf m/s\n", velocity );
         }

         else if( x > initialX && velocity <= 0.995 && velocity >= 0.00 )
         {
            velocity = velocity + 0.005;
            printf("right:%lf m/s\n", velocity );
         }

         if( y < initialY && angle <= 89.00 && angle >= 0.00 )
         {
            angle = angle + 5.0;
            printf("up:%lf degrees\n", angle);
         }

         else if( y > initialY && angle >= 1.00 && angle <= 90.00 )
         {
            angle = angle - 5.0;
            printf("down:%lf degrees\n", angle );
         }

         /* only need to send message in networking mode */
         if ( netClient == 1 )
         {
            /* tell server our new angle */
            sendMsgToServer( 0, 0, 0, 0, 0, 0, 0, 0, 0 );
         }
      }
   }

   /* increment number of mobs created */
   mobID++;
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
   /* check if networking is off */
   else if ( netServer == 0 && netClient == 0)
   {
      /* start creating the world */
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
      for(i = 0; i < WORLDX; i++) 
      {
         for(j = 0; j < WORLDZ; j++) 
         {
               world[i][3][j] = 8;
         }
      }

      /* create some random noise */
      generateNoise();
    
      int noiseReturn = 0;
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
              noiseReturn = (int)(turbulence(i, j, t, 32) / 4) - 13;

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

      /* create an A.I. opponent */
      createPlayer(0, 50.0, 40.0, 50.0, 180.0);

      /* initialize the A.I. to head south on route */
      headingSouth = 1;
      headingNorth = 0;
      headingEast = 0;
      headingWest = 0;
      onRoute = 1;

   }
   /* networking on */
   else
   {
      /* set network mode flag to true */
      networkMode = 1;

      if ( netServer == 1 )
      {
         /* start creating the world */
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
         for(i = 0; i < WORLDX; i++) 
         {
            for(j = 0; j < WORLDZ; j++) 
            {
                  world[i][3][j] = 8;
            }
         }

         /* create some random noise */
         generateNoise();
       
         int noiseReturn;
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
                 noiseReturn = (int)(turbulence(i, j, t, 32) / 4) - 13;

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

         /* Create the socket, but also ensure it worked */
         if ( ( socketID = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )
         {
            printf("Failure creating socket\n");
            return 0;
         }

         /* Create the servers socket name */
         server_addr.sin_family = AF_INET;
         server_addr.sin_addr.s_addr = INADDR_ANY; 
         server_addr.sin_port = htons(PORT_NUM);

         /* Bind the socketID to the socket name */
         if ( bind( socketID, ( struct sockaddr * ) &server_addr, sizeof(server_addr) ) < 0 )
          {
            printf("Server bind failed\n");
            return 0;
         }

         /* listen for a client */
         listen(socketID, 5);

         /* accept clients */
         clientSocketID = accept(socketID, (struct sockaddr *) &client_addr, &addrlen);

         char number[10];

         /* Copy the world */
         for(i = 0; i < WORLDX; i++)
         {
            for(j = 0; j < WORLDY; j++)
            {
               for(k = 0; k < WORLDZ; k++)
               {
                  /* send over each block */
                  sprintf(number,"%d", world[i][j][k]);
                  write(clientSocketID, number, sizeof(number));
               }
            } 
         }
      }

      else if ( netClient == 1 )
      {

         /* Create the socket, but also ensure it worked */
         if ( ( socketID = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )
         {
            printf("Failure creating socket\n");
            return 0;
         }

         /* get server information */
         server_addr.sin_family = AF_INET;
         server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
         server_addr.sin_port = htons(PORT_NUM);

         /* connect to the server */
         int result = connect(socketID, (struct sockaddr *)&server_addr, server_addrlen);

         if(result == -1) {
            printf("Connection failed\n");
            exit(1);
         }

         int temp;
         char numberGiven[10];

         /* build the given world */
         for(i = 0; i < WORLDX; i++)
         {
            for(j = 0; j < WORLDY; j++)
            {
               for(k = 0; k < WORLDZ; k++)
               {
                  read(socketID, &numberGiven, sizeof(numberGiven));
                  world[i][j][k] = atoi(numberGiven);
               }
            } 
         }
      }

      /* create an A.I. opponent */
      createPlayer(0, 50.0, 40.0, 50.0, 0.0);
   }

   /* starts the graphics processing loop */
   /* code after this will not run until the program exits */
   glutMainLoop();

   /* only close the socket if the networking was on */
   if( networkMode == 1 )
   {
      /* close socket when game exits */
      close( socketID );
   }

   return 0; 
}

