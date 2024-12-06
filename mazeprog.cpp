#include "segment.h"
#include "kdtree.h"
#include <math.h>
#include <windows.h>
#include <commctrl.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "fatalerror.h"
#include "maze.h"
#include "themouse.h"
#include "textload.h"
#include "wavefront.h"
#include "newfont.h"

#include "dsutil.h"
#define DIALOG_1 130
CSoundManager* g_pSoundManager = NULL;

#define NUMSOUNDCHANNELS 5
#define ROBOTROCKETDELAY 70
#define ROCKETSPEED .0012
#define NUMROBOTS 30

int mousesensitivity = 6;

void fullscreen(int width = 640, int height = 480, int bpp = 32)
{                
	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
	dmScreenSettings.dmSize=sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth	= width;
	dmScreenSettings.dmPelsHeight	= height;
	dmScreenSettings.dmBitsPerPel	= bpp;	
	dmScreenSettings.dmFields=DM_BITSPERPEL|
		DM_PELSWIDTH | DM_PELSHEIGHT;

	if (ChangeDisplaySettings(&dmScreenSettings,
		CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		fatalerror("Could not set screen mode.  Please try a different mode.");
	ShowCursor(FALSE);	    
}

class MikeSound
{
	CSound* pSounds[NUMSOUNDCHANNELS];
	int channelctr;

public:
	void loadsound(CSoundManager* g_pSoundManager, char *filename)
	{
		for (int ctr = 0; ctr < NUMSOUNDCHANNELS; ctr++)
			g_pSoundManager->Create( &pSounds[ctr], filename, 0, GUID_NULL );
		channelctr = 0;
	}

	void play(void)
	{
		pSounds[channelctr++]->Play( 0, 0); 
		channelctr = channelctr % NUMSOUNDCHANNELS;
	}

	void freememory(void)
	{
		for (int ctr = 0; ctr < NUMSOUNDCHANNELS; ctr++) delete pSounds[ctr];  
	}

};

MikeSound robotexplodesound;
MikeSound explodesound;
MikeSound screamsound;
MikeSound rlaunchsound;
MikeSound robotrocketexplodesound;
MikeSound robotlaunchsound;

int sndctr = 0;

#define M_PI 3.1415926

enum gamestatetype {GAMEMENU, ACTION, PAUSED, INSTRUCTIONS, YOULOSE, YOUWIN, MOUSECONFIG};
enum menuselectiontype {PLAY, INFO, EXITGAME};

gamestatetype gamestate = GAMEMENU;
menuselectiontype menusel = PLAY;

int physicsactual=0, physicsgoal=0;

Kdtree *kdtree;

float menuselrotatetime=0;

int bloodquadtimeleft = 0;

int mytexture = -1;
int robotskilled = 0;

// time to wait before another rocket can be fired
int rocketwait = 0;

Newfont *nf;

int health = 100;

// player coordinates
float x=0.59, y=0.475;
// player rotation angle
float theta=-1.57;
// player rotation pitch
float pitch = 0;

Maze *maze;
void physicsloop(void);


double rand01(void)
{
	double num = double(rand())/double(RAND_MAX);
	return num;
}

struct Rocket {
	float x, y, z;
	float dx, dy, dz;
	float yaw, pitch;
	bool playerrocket; // was rocket fired by player
};

struct Robot {
	float x, y, z;

	float vx, vy, vz;
	float destangle;
	float curangle;
	bool alive;
	long lastrockettime;
	Wavefront *mesh;
};

vector<Robot> robots;


LRESULT CALLBACK WndProc (HWND hWnd, UINT message,
						  WPARAM wParam, LPARAM lParam);
void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC);
void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC);

TheMouse *themouse = NULL;

TextureLoader tl;

// rocket launcher
Wavefront launcher("launcher2.obj","launcher2.mtl", .005);

// rocket
Wavefront rocketmesh("rocket2.obj","rocket2.mtl", .001);

// enemy rocket
Wavefront enemyrocketmesh("enemyrocket2.obj","enemyrocket2.mtl", .001);

// explosion mesh
Wavefront robotexplosionmesh("robotexplode2.obj","robotexplode2.mtl", .01);

// explosion mesh
Wavefront explosionmesh("explode1.obj","explode1.mtl", .005);

Wavefront robotgraymesh("robot6.obj","robot6.mtl", .0035);
Wavefront robotpinkmesh("robotpink.obj","robotpink.mtl", .0035);
Wavefront robotredmesh("robotred.obj","robotred.mtl", .0035);


float distance(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float dx = x1 - x2;
	float dy = y1 - y2;
	float dz = z1 - z2;
	return sqrt(dx*dx + dy*dy + dz*dz);
}

struct Explosion
{
	float x, y, z;
	int timeleft;
	bool robotexploding;
};

vector<Explosion> explosions;
vector<Rocket> rockets;

float radtodeg(float rad)
{
	return rad*180.0/M_PI;
}

float degtorad(float deg)
{
	return deg*M_PI/180.0;
}

// determines whether a line segment contains no walls
bool islineclear(float x1, float y1, float x2, float y2)
{
	for (float t = 0; t < 1.0; t+=.1)
	{
		float testx = x1*t + x2*(1.0-t);
		float testy = y1*t + y2*(1.0-t);
		if (maze->isblockwall(testx, testy)) return false;    
	}	

	return true;
}

// determines whether a circle contains no walls
bool iscircleclear(float x, float y, float r = 0.004)
{
	bool itsclear = true;
	float precision = 2.0*M_PI/20.0;
	for (float angle = 0.0; angle < 2.0*M_PI; angle+=precision)
	{
		float circlex = x+cos(angle)*r;
		float circley = y+sin(angle)*r;
		if (maze->isblockwall(circlex, circley)) return false;    
	} 
	return true;
}


void keyboardproc(void)
{
	float speed = 0.0005;

	if (GetAsyncKeyState(VK_RIGHT)) 
	{                        
		if (iscircleclear(x, y-cos(theta+M_PI/2.0)*speed)) y-=cos(theta+M_PI/2.0)*speed;
		if (iscircleclear(x+sin(theta+M_PI/2.0)*speed, y)) x+=sin(theta+M_PI/2.0)*speed;                       
	}   

	if (GetAsyncKeyState(VK_LEFT)) 
	{                        
		if (iscircleclear(x, y-cos(theta-M_PI/2.0)*speed)) y-=cos(theta-M_PI/2.0)*speed;
		if (iscircleclear(x+sin(theta-M_PI/2.0)*speed, y)) x+=sin(theta-M_PI/2.0)*speed;                       
	}   


	if (GetAsyncKeyState(VK_UP)) 
	{                        
		if (iscircleclear(x, y-cos(theta)*speed)) y-=cos(theta)*speed;
		if (iscircleclear(x+sin(theta)*speed, y)) x+=sin(theta)*speed;                       
	}    

	if (GetAsyncKeyState(VK_DOWN))
	{
		if (iscircleclear(x, y+cos(theta)*speed)) y+=cos(theta)*speed;
		if (iscircleclear(x-sin(theta)*speed, y)) x-=sin(theta)*speed;                                
	}

	if (GetAsyncKeyState(VK_PRIOR)) pitch-=.01;
	if (GetAsyncKeyState(VK_NEXT)) pitch+=.01;

	if (!themouse) return;

	int dx=0, dy=0;
	themouse->getstate(&dx, &dy);

	pitch+=float(dy)/50.0*float(mousesensitivity)/6.0;
	theta+=float(dx)/50.0*float(mousesensitivity)/6.0;


	if (pitch > (M_PI/2.0)) pitch = (M_PI/2.0);
	if (pitch < -(M_PI/2.0)) pitch = -(M_PI/2.0);

	if (!rocketwait && GetAsyncKeyState(VK_LBUTTON)) {

		rlaunchsound.play();

		// move it over to the right a little
		//float x2=x+sin(theta+M_PI/2.0)*.0008;
		//float y2=y-cos(theta+M_PI/2.0)*.0008;			

		Matrix matyaw = buildrotationmatrix(-theta, 0, 1, 0);			
		Matrix matpitch = buildrotationmatrix(-pitch, 1, 0, 0);

		Vector startpt = matyaw*matpitch*buildvector(.0008, -.0008, 0);
	

		Rocket rocket;
		rocket.x = startpt[0] + x; 
		rocket.y = 0.65;
		rocket.z = startpt[2] + y; 
		rocket.yaw = theta;
		rocket.pitch = pitch;

		Vector dir = matyaw*matpitch*buildvector(0, 0, -ROCKETSPEED);
		dir = buildscalematrix(1, 60, 1)*dir;
		rocket.dx=dir[0];
		rocket.dy=dir[1];
		rocket.dz=dir[2];
		rocket.playerrocket = true;
		rockets.push_back(rocket);

		rocketwait=30;
	}

}

void CALLBACK cbFunct(UINT , UINT , DWORD_PTR , DWORD_PTR , DWORD_PTR) 
{ 
	if (gamestate == ACTION) physicsgoal++; 
	else if (gamestate == GAMEMENU) menuselrotatetime+=.03;
}

void physicsloop(void)
{

	keyboardproc();   

	// count down till we can fire again
	if (rocketwait > 0) rocketwait--;

	// have the enemy randomly fire a rocket if the game has been active long enough
	if (physicsactual > 15*70)
		for (int ctr = 0; ctr < robots.size(); ctr++)
		{
			if (rand() % 100) continue;

			Robot &robot = robots[ctr];
			if (physicsactual - robot.lastrockettime < ROBOTROCKETDELAY) continue;
			if (!robot.alive) continue;

			// don't fire if player is too far
			if (distance(x, 0, y, robot.x, 0, robot.z) > .3) continue;

			// make sure there probably aren't walls between player and robot
			if (!islineclear(x, y, robot.x, robot.z)) continue;


			robot.lastrockettime = physicsactual;

			Rocket rocket;
			rocket.x = robot.x;
			rocket.y = 0.25;
			rocket.z = robot.z;
			rocket.yaw = robot.curangle + M_PI/2.0;
			rocket.pitch = 0;

			Matrix matyaw = buildrotationmatrix(-rocket.yaw, 0, 1, 0);			
			Matrix matpitch = buildrotationmatrix(rocket.pitch, 1, 0, 0);
			Vector dir = matyaw*matpitch*buildvector(0, 0, -ROCKETSPEED*0.5);

			dir = buildscalematrix(1, 60, 1)*dir;
			rocket.dx=dir[0];
			rocket.dy=dir[1];
			rocket.dz=dir[2];
			rocket.playerrocket = false;
			rockets.push_back(rocket);
			robotlaunchsound.play();

		}

		// move the robots

		for (int ctr = 0; ctr < robots.size(); ctr++)
		{

			Robot &robot2 = robots[ctr];
			if (!robot2.alive) continue;
			robot2.destangle = atan2(y - robot2.z, x - robot2.x);

			if (abs(robot2.curangle - robot2.destangle) < .1) robot2.curangle=robot2.destangle;
			else if (robot2.curangle < robot2.destangle) robot2.curangle+=0.06;
			else robot2.curangle-=0.06;

			if (iscircleclear(robot2.x+robot2.vx, robot2.z+robot2.vz))
			{
				robot2.x+=robot2.vx;
				robot2.y+=robot2.vy; 
				robot2.z+=robot2.vz;
			}
			else {
				float angle = rand01()*2*M_PI;
				robot2.vx=cos(angle)/5000.0;
				robot2.vy=0;
				robot2.vz=sin(angle)/5000.0;
			}




		}

		for (int ctr = 0; ctr < rockets.size(); ctr++)
		{
			Rocket &rocket = rockets[ctr];
			rocket.x+=rocket.dx;
			rocket.y+=rocket.dy;
			rocket.z+=rocket.dz;
		}

		// count down blood quad
		if (bloodquadtimeleft) bloodquadtimeleft--;

		// count down explosions and remove finished ones
		for (int ctr = 0; ctr < explosions.size(); ctr++)
		{
			Explosion &e = explosions[ctr];	 
			e.timeleft--;	
			if (!e.timeleft) {explosions.erase(explosions.begin()+ctr); ctr--;}
		}

		// check for rocket collision with wall, floor, ceiling, player
		for (int rocketctr = 0; rocketctr < rockets.size(); rocketctr++)
		{
			Rocket &rocket = rockets[rocketctr];
			// check for collision with player
			float thedist = distance(rocket.x, 0, rocket.z, x, 0, y);
			BOOL isplayerhit = (thedist < .0035);

			if (isplayerhit && !rocket.playerrocket) {
				bloodquadtimeleft = 80;
				health-=20;
				if (health < 0) health = 0;
				// remove the rocket
				rockets.erase(rockets.begin()+rocketctr);  
				rocketctr--;
				screamsound.play();
				continue;
			}


			if (!iscircleclear(rocket.x, rocket.z, .001)
				||	rocket.y < 0
				||	rocket.y > 1)
			{
				Explosion explosion;
				explosion.x = rocket.x;
				explosion.y = rocket.y;
				explosion.z = rocket.z;
				explosion.timeleft = 40;
				explosion.robotexploding = false;
				explosions.push_back(explosion);

				if (rocket.playerrocket) explodesound.play();
				else robotrocketexplodesound.play();
				// remove the rocket
				rockets.erase(rockets.begin()+rocketctr);  
				rocketctr--;

			}

		}

		for (int robotctr = 0; robotctr < robots.size(); robotctr++)
		{
			for (int rocketctr = 0; rocketctr < rockets.size(); rocketctr++)
			{

				Rocket &rocket = rockets[rocketctr];
				Robot &robot = robots[robotctr];
				if (!robot.alive) continue;

				// check for collision with robot
				float thedist = distance(rocket.x, rocket.y/60.0, rocket.z, robot.x, robot.y/60.0, robot.z);
				BOOL isrobothit = (thedist < .0035);

				if (isrobothit && rocket.playerrocket) {
					robotskilled++;
					Explosion explosion;
					explosion.x = robot.x;
					explosion.y = robot.y;
					explosion.z = robot.z;
					explosion.timeleft = 120;
					explosion.robotexploding = true;
					explosions.push_back(explosion);

					// remove the rocket
					rockets.erase(rockets.begin()+rocketctr);  
					rocketctr--;
					robot.y=.13;
					robot.alive = false;
					robotexplodesound.play();
					// stop checking rockets against this robot since it's dead
					break;
				}




			}
		}

} 



void centertext(char *str)
{
	glPushMatrix();
	float fontwid = nf->getwidth(str, .05);
	glTranslatef(-fontwid/2.0, 0, 0);            
	nf->drawstring(str, .05);
	glPopMatrix();
}


void drawhealth(void)
{
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);            
	glPushMatrix ();
	glTranslatef(-6, 4.25, -8);            
	glColor3f(1, 1, 1);
	char str[80];
	sprintf(str, "Health: %d            Robots left: %d", health, NUMROBOTS - robotskilled);
	nf->drawstring(str, .02);
	glPopMatrix ();
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);                 
}

void reseteverything(void)
{
	health=100;              
	physicsactual=0;
	physicsgoal=0;
	robots.clear();
	explosions.clear();
	rockets.clear();
	gamestate = GAMEMENU;  
	robotskilled=0;
	theta=-1.57;
	pitch = 0;

	bloodquadtimeleft = 0;

	// randomly place player

	do {
		x = rand01();
		y = rand01();
	}
	while (!iscircleclear(x, y));

	// randomly place robots

	for (int ctr = 0; ctr < NUMROBOTS; ctr++)
	{
		Robot robot;
		do {
			robot.x = rand01();
			robot.z = rand01();
			robot.y = 0.2;
		}
		while (!iscircleclear(robot.x, robot.z));

		float angle = rand01()*2*M_PI;
		robot.vx=cos(angle)/5000.0;
		robot.vy=0;
		robot.vz=sin(angle)/5000.0;
		robot.destangle = angle;
		robot.curangle = angle;
		robot.alive = true;
		robot.lastrockettime = 0;

		int meshcolor = rand()%3;
		if (meshcolor == 0) robot.mesh = &robotpinkmesh;
		if (meshcolor == 1) robot.mesh = &robotredmesh;
		if (meshcolor == 2) robot.mesh = &robotgraymesh;

		robots.push_back(robot);
	}
}

void drawgamescreen(void)
{
	glEnable(GL_CULL_FACE);

	vector<Point> thepoints;
	kdtree->searchtree(x-.3, y-.3, x+.3, y+.3, thepoints);

	//quads
	vector<Point>::iterator i2 = thepoints.begin();

	glDepthMask(GL_TRUE);

	glPushMatrix();
	glRotatef(radtodeg(pitch), 1, 0, 0);
	glRotatef(radtodeg(theta), 0, 1, 0);
	glScalef(40, 0.66, 40);            
	glTranslatef(-x, -0.7, -y);

	// draw a rocket

	for (int ctr = 0; ctr < rockets.size(); ctr++)
	{
		Rocket &rocket = rockets[ctr];
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);	
		glPushMatrix();

		glTranslatef(rocket.x, rocket.y, rocket.z);

		glScalef(1.0/40.0, 1.0/0.66, 1.0/40.0);
		glRotatef(-radtodeg(rocket.yaw), 0, 1, 0);			
		glRotatef(-radtodeg(rocket.pitch), 1, 0, 0);
		if (rocket.playerrocket)
			rocketmesh.draw();
		else 
			enemyrocketmesh.draw();			
		glPopMatrix();
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);

	}

	// draw the explosions
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);	

	for (int ctr = 0; ctr < explosions.size(); ctr++)
	{
		Explosion &explosion = explosions[ctr];
		glPushMatrix();
		glTranslatef(explosion.x, explosion.y, explosion.z);
		glScalef(1.0/40.0, 1.0/0.66, 1.0/40.0);			
		if (explosion.robotexploding) robotexplosionmesh.draw();			
		else explosionmesh.draw();
		glPopMatrix();
	}

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	// draw robots

	for (int ctr = 0; ctr < robots.size(); ctr++)
	{
		Robot &robot2 = robots[ctr];

		// don't draw far away robots
		if (distance(robot2.x, 0, robot2.z, x, 0, y) > .3) continue;

		// don't draw robots out of field of view

		float yawangle = atan2(-cos(theta), sin(theta));	 
		float robotangle = atan2(robot2.z - y, robot2.x - x);
		float anglediff = abs(yawangle - robotangle);

		if (anglediff > M_PI) anglediff = M_PI*2.0 - anglediff;
		if (anglediff > M_PI/4.0) continue;

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);	

		glPushMatrix();
		glTranslatef(robot2.x, robot2.y, robot2.z);
		glScalef(1.0/40.0, 1.0/0.66, 1.0/40.0);	

		glRotatef(-radtodeg(robot2.curangle), 0, 1, 0);
		if (!robot2.alive) glRotatef(90, 0, 0, 1);

		robot2.mesh->draw();
		glPopMatrix();
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, mytexture);          
	glColor3f(1, 1, 1);  
	glBegin(GL_QUADS);
	while (i2 != thepoints.end())
	{                                
		Point &p = *i2; 
		if (p.seg.x0 == p.seg.x1) glColor3f(.6, .6, .6);
		else glColor3f(.9, .9, .9);

		glTexCoord2f(0, 0);
		glVertex3f(p.seg.x0, 0, p.seg.y0);
		glTexCoord2f(1, 0);
		glVertex3f(p.seg.x1, 0, p.seg.y1); 
		glTexCoord2f(1, 1);
		glVertex3f(p.seg.x1, 1, p.seg.y1); 
		glTexCoord2f(0, 1);
		glVertex3f(p.seg.x0, 1, p.seg.y0);
		++i2;     
	}
	glEnd();  

	glDisable(GL_CULL_FACE);

	//floor
	glColor3f(.6, .6, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, 0, 0);
	glTexCoord2f(0, 60);
	glVertex3f(0, 0, 1);
	glTexCoord2f(60, 60);
	glVertex3f(1, 0, 1);
	glTexCoord2f(60, 0);
	glVertex3f(1, 0, 0);
	glEnd();          

	// ceiling

	glColor3f(1, .5, .5);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, 1, 0);
	glTexCoord2f(0, 60);
	glVertex3f(0, 1, 1);
	glTexCoord2f(60, 60);
	glVertex3f(1, 1, 1);
	glTexCoord2f(60, 0);
	glVertex3f(1, 1, 0);
	glEnd();          
	glPopMatrix();


	glEnable(GL_CULL_FACE);

	// rocket launcher
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);		
	glPushMatrix();
	glTranslatef(.4, -.4, -1);
	launcher.draw();
	glPopMatrix();

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	// draw a blood quad

	if (bloodquadtimeleft)
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1, 0, 0, float(bloodquadtimeleft)/100.0);

		glBegin(GL_QUADS);
		glVertex3f(-1, -1, -1);
		glVertex3f(1, -1, -1);
		glVertex3f(1, 1, -1);
		glVertex3f(-1, 1, -1);
		glEnd();          
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

	}

	drawhealth();


}

// draw the menu screen
void drawgamemenu(void)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);            
	glPushMatrix ();
	glTranslatef(0, .25, -8);            
	glTranslatef(-1.1, 0, 0);            
	glColor3f(0.4, .8, 1);  
	float fontwid;  
	glPushMatrix();
	fontwid = nf->getwidth("Play", .05);
	nf->drawstring("Play", .05);
	glPopMatrix(); 	  	  	  
	glTranslatef(0, -1, 0);            
	glPushMatrix();
	fontwid = nf->getwidth("Instructions", .05);
	nf->drawstring("Instructions", .05);
	glPopMatrix();
	glTranslatef(0, -1, 0);            
	glPushMatrix();
	fontwid = nf->getwidth("Exit", .05);
	nf->drawstring("Exit", .05);
	glPopMatrix();
	glTranslatef(0, 3.5, 4);            
	glColor3f(0.9, .5, 0.3);
	glTranslatef(1.1, 0, 0);            
	glPushMatrix();
	glTranslatef(0, 0, -1.0);              
	centertext("Rockets and Robots");
	glPopMatrix();
	glPopMatrix ();	    
	glPushMatrix ();	    

	if (menusel == PLAY) glTranslatef(-.6, 0.095, -2.5);
	else if (menusel == INFO) glTranslatef(-.6, -0.235, -2.5);
	else if (menusel == EXITGAME) glTranslatef(-.6, -0.525, -2.5);
	glEnable(GL_DEPTH_TEST);   
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND); 
	glEnable(GL_LIGHTING);
	glRotatef(sin(menuselrotatetime)*60-30, 0, 1, 0);
	robotredmesh.draw();
	glPopMatrix ();	       	     
}

void drawinstructions(void)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);            
	glPushMatrix ();
	glTranslatef(0, 3.25, -7);            
	glColor3f(0.4, .8, 1);  
	centertext("Instructions");
	glColor3f(1, 1, 1);
	glTranslatef(0, 0, -7);            
	centertext("Your mission: kill all robots");
	glTranslatef(0, -1.25, 0); 
	centertext("Arrow keys to move");	  
	glTranslatef(0, -1.25, 0);            
	centertext("Mouse to aim");
	glTranslatef(0, -1.25, 0);            
	centertext("Left mouse button to fire rocket");
	glTranslatef(0, -1.25, 0);            
	centertext("P to pause or unpause");
	glTranslatef(0, -1.25, 0);            
	centertext("M for mouse config");	
	glColor3f(0.4, .8, 1);  
	glTranslatef(0, -1.25, 0);            
	centertext("Press Enter to return to main menu");     
	glPopMatrix ();
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);                 
}

void drawyoulose(void)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST); 
	glDepthMask(GL_FALSE);
	glPushMatrix ();  
	glTranslatef(0, .5, -3);            	  
	glColor3f(1, 1, 1);
	float fontwid = nf->getwidth("Game over", .05);
	glTranslatef(-fontwid/2.0, 0, 0);                    
	nf->drawstring("Game over", .05);
	glPopMatrix ();	    	  	  	  
	glPushMatrix ();
	glTranslatef(0, -1, -9);      
	centertext("Press Enter to return to main menu");     
	glPopMatrix ();	  	  	  	  
	glEnable(GL_DEPTH_TEST);            
	glDisable(GL_BLEND);   
	glEnable(GL_LIGHTING);    
	glDepthMask(GL_TRUE);
}

void drawyouwin(void)
{
	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);            
	glPushMatrix ();	  
	glTranslatef(0, .5, -3);            
	glColor3f(1, 1, 1);
	centertext("You win!");     
	glPopMatrix ();	    	  	  
	glPushMatrix ();
	glTranslatef(0, -1, -9);      
	centertext("Press Enter to return to main menu");     
	glPopMatrix ();	  	  	  	  
	glEnable(GL_DEPTH_TEST);            
	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_SMOOTH);	    
	glEnable(GL_LIGHTING);  
	glDepthMask(GL_TRUE);
}

void drawpausedscreen(void)
{
	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);            
	glPushMatrix ();
	glTranslatef(-1.20, .25, -2);            
	glColor3f(1, 1, 1);
	nf->drawstring("Paused", .05);
	glPopMatrix ();	    
	glEnable(GL_DEPTH_TEST);            
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING); 
	glDepthMask(GL_TRUE);
}



BOOL CALLBACK ScreenModeProc(HWND hwndDlg, 
							 UINT message, 
							 WPARAM wParam, 
							 LPARAM lParam) 
{ 
	switch (message) 
	{ 
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case 101:
			EndDialog(hwndDlg, 1); 
			return TRUE; 

		case 102:
			EndDialog(hwndDlg, 2); 
			return TRUE; 

		case 103:
			EndDialog(hwndDlg, 3); 
			return TRUE; 

		case 104:
			EndDialog(hwndDlg, 4); 
			return TRUE; 

		case IDCANCEL: 
			EndDialog(hwndDlg, 10); 
			ExitProcess(0);
			return TRUE; 
		} 
	} 
	return FALSE; 
} 



BOOL CALLBACK MouseSensitivityProc(HWND hwndDlg, 
								   UINT message, 
								   WPARAM wParam, 
								   LPARAM lParam) 
{ 
	switch (message) 
	{ 
	case WM_INITDIALOG:
		{
			HWND sliderhwnd = GetDlgItem(hwndDlg, 203);

			SendMessage(                 // returns LRESULT in lResult
				(HWND) sliderhwnd,       // handle to destination control
				(UINT) TBM_SETRANGE,     // message ID
				(WPARAM) TRUE,           // = (WPARAM) (BOOL) fRedraw
				(LPARAM)MAKELONG (1, 11) // = (LPARAM) MAKELONG (lMinimum, lMaximum)
				);


			SendMessage(
				(HWND) sliderhwnd,
				(UINT) TBM_SETTICFREQ,
				(WPARAM) 1,
				(LPARAM) 0
				);

			SendMessage((HWND) sliderhwnd,
				(UINT) TBM_SETPOS,  
				(WPARAM) TRUE,  
				(LPARAM) mousesensitivity     
				);  

			return TRUE;
		}

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK: 

		case 207:
			{
				HWND sliderhwnd = GetDlgItem(hwndDlg, 203);

				mousesensitivity = SendMessage((HWND) sliderhwnd, 
					(UINT) TBM_GETPOS,
					0, 0);  

				EndDialog(hwndDlg, 10);

			}

			return TRUE; 

		case 210:
			EndDialog(hwndDlg, 2); 
			return TRUE; 
		} 
	} 
	return FALSE; 
} 



int WINAPI WinMain (HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int iCmdShow)
{
	WNDCLASS wc;
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;        
	MSG msg;
	BOOL bQuit = FALSE;

	int winwidth, winheight;


	int screenmode = DialogBox( hInstance, MAKEINTRESOURCE(DIALOG_1), NULL, ScreenModeProc );

	if (screenmode == 1) {winwidth=320; winheight=240;}
	if (screenmode == 2) {winwidth=640; winheight=480;}
	if (screenmode == 3) {winwidth=800; winheight=600;}
	if (screenmode == 4) {winwidth=1024; winheight=768;}

	fullscreen(winwidth, winheight);

	srand(16729);

	maze = new Maze(20, 20);

	srand(time(NULL));


	reseteverything();

	/* register window class */
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = ExtractIcon(hInstance, "rr.ico", 0);

	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GLSample";
	RegisterClass (&wc);

	/* create main window */
	hWnd = CreateWindow (
		"GLSample", "Rockets and Robots", 
		WS_POPUPWINDOW ,
		0, 0, winwidth, winheight,
		NULL, NULL, hInstance, NULL);


	g_pSoundManager = new CSoundManager();
	if (g_pSoundManager == NULL) fatalerror("could not initialize Sound Manager");

	HRESULT hr = g_pSoundManager->Initialize( hWnd, DSSCL_PRIORITY );    
	if (FAILED(hr)) fatalerror("call to g_pSoundManager->Initialize failed");


	hr = g_pSoundManager->SetPrimaryBufferFormat( 2, 22050, 16 );
	if (FAILED(hr)) fatalerror("call to g_pSoundManager->SetPrimaryBufferFormat failed");


	ShowWindow(hWnd, SW_SHOWNORMAL);

	explodesound.loadsound(g_pSoundManager, "rocketexplosion3.wav");
	screamsound.loadsound(g_pSoundManager, "diescream.wav");
	rlaunchsound.loadsound(g_pSoundManager, "rlaunch2.wav");
	robotexplodesound.loadsound(g_pSoundManager, "robotexplosion2.wav");
	robotlaunchsound.loadsound(g_pSoundManager, "robotlaunch6.wav");
	robotrocketexplodesound.loadsound(g_pSoundManager, "robotrocketexplosion.wav");

	EnableOpenGL (hWnd, &hDC, &hRC);

	nf = new Newfont("myfont.obj", "myfont.mtl", 1); 

	timeSetEvent(15, 0, cbFunct, NULL, TIME_PERIODIC);                   

	GLfloat LightAmbient[]= { .1, .1, .1, 1.0f };
	GLfloat LightDiffuse[]= { .04, .04, .04, 1.0f };
	GLfloat LightGlobal[]= { .5, .5, .5, 1.0f };

	GLfloat LightPosition[]= { 0.0f, 0.0f, -100.0f, 1.0f };
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);	
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);	
	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);
	glEnable(GL_LIGHT1);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, LightGlobal);

	mytexture = tl.gettexture("rockblur.jpg");

	if (mytexture == -1) return 1;

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	vector<Segment> segments = maze->getsegments();

	vector<Point> points;

	vector<Segment>::iterator i = segments.begin();
	while (i != segments.end()) 
	{
		Segment &seg = *i;
		Point p((seg.x0+seg.x1)/2.0, (seg.y0+seg.y1)/2.0, seg); 
		points.push_back(p);
		++i;
	}

	kdtree = new Kdtree(points);

	glEnable(GL_TEXTURE_2D);

	themouse = new TheMouse(hWnd);

	/* program main loop */
	while (!bQuit)
	{
		/* check for messages */
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
		{
			/* handle or dispatch messages */
			if (msg.message == WM_QUIT)
			{
				bQuit = TRUE;
			}
			else
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		else
		{

			glClearColor (0.0f, 0.0f, 0.0f, 0.0f);

			// don't waste time clearing the color buffer if we don't have to

			glDepthMask(GL_TRUE);

			if (gamestate == ACTION)  glClear (GL_DEPTH_BUFFER_BIT);
			else glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			glLoadIdentity();
			gluPerspective(60.0f, float(winwidth)/float(winheight), .1, 100.0f);

			if (gamestate == GAMEMENU) {drawgamemenu(); Sleep(20);}
			else if (gamestate == ACTION) drawgamescreen();
			else if (gamestate == PAUSED) 
			{
				drawgamescreen();
				drawpausedscreen();
			}
			else if (gamestate == INSTRUCTIONS) drawinstructions();
			else if (gamestate == YOULOSE) 
			{
				drawgamescreen();     
				drawyoulose();
			}
			else if (gamestate == YOUWIN) 
			{
				drawgamescreen();     
				drawyouwin();
			}

			SwapBuffers (hDC);


			if (gamestate == ACTION)
			{
				// take care of the physics
				while (physicsactual < physicsgoal) {physicsloop(); 
				physicsactual++;}

				if (health == 0) gamestate = YOULOSE;
				if (NUMROBOTS - robotskilled == 0) gamestate = YOUWIN;
			}

			Sleep (1);
		}
	}

	/* shutdown OpenGL */
	DisableOpenGL (hWnd, hDC, hRC);

	/* destroy the window explicitly */
	DestroyWindow (hWnd);

	DestroyIcon(wc.hIcon);

	delete themouse;

	robotexplodesound.freememory();
	explodesound.freememory();
	screamsound.freememory();
	rlaunchsound.freememory();
	robotrocketexplodesound.freememory();
	robotlaunchsound.freememory();

	delete g_pSoundManager;

	// free up the font
	delete nf; 

	// free up kdtree
	delete kdtree;
	return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT message,
						  WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
		return 0;

	case WM_CLOSE:
		PostQuitMessage (0);
		return 0;

	case WM_DESTROY:
		return 0;

	case WM_KEYDOWN:
		switch (wParam)
		{

		case VK_ESCAPE:
			themouse->unacquire();
			gamestate = GAMEMENU;
			return 0;

		case 'P':
			if (gamestate == ACTION) {themouse->unacquire(); gamestate = PAUSED;}
			else if (gamestate == PAUSED) {themouse->acquire(); gamestate = ACTION;}
			return 0;

		case 'M':
			{
				themouse->unacquire();
				gamestatetype oldgamestate = gamestate;
				gamestate = MOUSECONFIG;
				ShowCursor(TRUE);
				DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(200), hWnd, MouseSensitivityProc);
				ShowCursor(FALSE);
				gamestate = oldgamestate;	
				themouse->acquire();
				return 0;
			}
		case VK_UP:
			if (gamestate != GAMEMENU) return 0;
			if (menusel == PLAY) menusel = EXITGAME;
			else if (menusel == INFO) menusel = PLAY;
			else if (menusel == EXITGAME) menusel = INFO;
			return 0;             

		case VK_DOWN:
			if (gamestate != GAMEMENU) return 0;
			if (menusel == PLAY) menusel = INFO;
			else if (menusel == INFO) menusel = EXITGAME;
			else if (menusel == EXITGAME) menusel = PLAY;
			return 0;  

		case VK_RETURN:

			if (gamestate == INSTRUCTIONS) {gamestate = GAMEMENU; return 0;}
			if (gamestate == YOULOSE || gamestate == YOUWIN) {reseteverything(); return 0;}
			if (gamestate != GAMEMENU) return 0;
			if (menusel == PLAY) {themouse->acquire(); gamestate = ACTION;}             
			if (menusel == INFO) gamestate = INSTRUCTIONS;
			if (menusel == EXITGAME) PostQuitMessage (0);             
			return 0;  
		}
		return 0;

	default:
		return DefWindowProc (hWnd, message, wParam, lParam);
	}
}


/*******************
* Enable OpenGL
*
*******************/

void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC)
{
	PIXELFORMATDESCRIPTOR pfd;
	int iFormat;

	/* get the device context (DC) */
	*hDC = GetDC (hWnd);

	/* set the pixel format for the DC */
	ZeroMemory (&pfd, sizeof (pfd));
	pfd.nSize = sizeof (pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | 
		PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	iFormat = ChoosePixelFormat (*hDC, &pfd);
	SetPixelFormat (*hDC, iFormat, &pfd);

	/* create and enable the render context (RC) */
	*hRC = wglCreateContext( *hDC );
	wglMakeCurrent( *hDC, *hRC );

}


/******************
* Disable OpenGL
*
******************/

void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent (NULL, NULL);
	wglDeleteContext (hRC);
	ReleaseDC (hWnd, hDC);
}
