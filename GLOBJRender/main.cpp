/* demo.c   */

/* Rotating cube demo*/
/* Revised by Ben Cortina, 2012 */

#include <stdlib.h>
#include <math.h>
#include <iostream>


#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "mdl.h"


using namespace std;

#define bool int
#define false 0
#define true 1

/*
** Global settings.
*/

float   FOV = 60;
float   center[3];     /* center of the model */
float	offset[3];	   /* camera coordinates */
float   cam_z;  // these are actually negative

MDLmodel * objModel;
vector<MDLmodel*> models;
GLuint mode = MDL_MATERIAL | MDL_TEXTURE;

int 	winWidth, winHeight;

float 	angle = 0.0, axis[3];
bool 	redrawContinue = false;
bool	directionalLight = true;
bool	spotLight = true;
bool	mouseDown = false;

enum Controlopts {C_ORBIT, C_PAN, C_ZOOM};
Controlopts control = C_ORBIT;
int groupDisp;

GLfloat objectXform[4][4] = {
	{1.0, 0.0, 0.0, 0.0},
	{0.0, 1.0, 0.0, 0.0},
	{0.0, 0.0, 1.0, 0.0},
	{0.0, 0.0, 0.0, 1.0}
};

void display(void);
void spinCube(void);

/*----------------------------------------------------------------------*/
/*
** Initial Settings for OpenGL
*/

// Enables lights and sets their properties.
GLvoid initLights()
{
	//Light 0 (Directional)
			 // create ambient light values
	GLfloat amb0[]={0.2, 0.2, 0.2, 1.0};
	// diffuse 
	GLfloat diff0[]={1.0, 1.0, 1.0, 1.0};
	// specular
	GLfloat spec0[]={1.0, 1.0 ,1.0, 1.0};
	//position
	GLfloat pos0[] ={1.0, 1.0, 1.0, 0.0};
	
	glLightfv(GL_LIGHT0,GL_AMBIENT, amb0);
	glLightfv(GL_LIGHT0,GL_DIFFUSE, diff0);
	glLightfv(GL_LIGHT0,GL_SPECULAR, spec0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos0);

	glEnable(GL_LIGHT0);


	//Light 1 (headlight)

	// ambient 
	GLfloat amb1[]={0.0, 0.0, 0.0, 0.0};
	// diffuse 
	GLfloat diff1[]={1.0, 1.0, 1.0, 1.0};
	// specular
	GLfloat spec1[]={1.0, 1.0, 1.0, 1.0};
	//direction
	GLfloat dir1[]={0.0, 0.0, -1.0};
	// now set the values
	glLightfv(GL_LIGHT1, GL_AMBIENT, amb1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diff1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, spec1);
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.0);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0);

	//glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 15.0);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, dir1);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 2.0);

	glEnable(GL_LIGHT1);

}

void initView()
{
		float x, y, z, opp;
	x = objModel->max[0] - objModel->min[0];
	y = objModel->max[1] - objModel->min[1];
	z = objModel->max[2] - objModel->min[2];
	opp = (x  > y) ? x : y;
	cam_z = -((opp / 2.0) / tan((FOV / 2.0) * (180.0 / M_PI)) + z / 2);
	
	//I could have put this as a part of mdl, but I believe it to be too 
	// specific to this assignment
	center[0] = objModel->min[0] + x/2;
	center[1] = objModel->min[1] + y/2;
	center[2] = objModel->min[2] + z/2;
	
	offset[0] = 0;
	offset[1] = 0;
	offset[2] = 0;

	angle = 0;
	groupDisp = -1;

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			objectXform[i][j] = 0.0;
			if (i == j)
				objectXform[i][j] = 1.0;
		}
}

//intializes OpenGL settings
void initialization(void) 
{
	glClearColor (0.0, 0.0, 0.3, 0.0);
	glClearDepth(1.0f); 
	glEnable(GL_DEPTH_TEST); /* Enable hidden--surface--removal */
	glDepthFunc(GL_LEQUAL);        
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);   
	
	//for transparency
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	initLights();

	glShadeModel (GL_SMOOTH);
	glEnable(GL_LIGHTING);

	GLfloat lmodel_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	GLfloat lmodel_lview[] = { 0.0};
	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_lview);

	glPointSize(4);
	glLineWidth(2);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//load first model
	objModel = models[0];
	mdlLoadModel(objModel);

	//set viewing values
	initView();

}

/*----------------------------------------------------------------------*/
/* 
** These functions implement a simple trackball-like motion control.
*/

float lastPos[3] = {0.0F, 0.0F, 0.0F};
float lastLoc[2] = {0.0F, 0.0F};
int startX, startY;

void trackball_ptov(int x, int y, int width, int height, float v[3])
{
	float d, a;

	/* project x, y onto a hemi-sphere centered within width, height */
	v[0] = (2.0f * x - width) / width;
	v[1] = (2.0F * y - height ) / height;
	d = (float) sqrt(v[0] * v[0] + v[1] * v[1]);
	v[2] = (float) sqrt(1 - ((d < 1.0F) ? d : 1.0F));
	a = 1.0F / (float) sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] *= a;
	v[1] *= a;
	v[2] *= a;
}

// Runs when mouse button is not down, updates tracking
void passiveMotion(int x, int y)
{
	float curPos[3], dx, dy, dz;

	trackball_ptov(x, y, winWidth, winHeight, curPos);

	dx = (curPos[0] - lastPos[0]);
	dy = (curPos[1] - lastPos[1]);
	dz = (curPos[2] - lastPos[2]);

	lastPos[0] = curPos[0];
	lastPos[1] = curPos[1];
	lastPos[2] = curPos[2];

	lastLoc[0] = x;
	lastLoc[1] = y;
}

//controls how mouse drags are handled
void mouseMotion(int x, int y)
{

	float curPos[3], dx, dy, dz;

	//Orbit calculations

	trackball_ptov(x, y, winWidth, winHeight, curPos);

	dx = (curPos[0] - lastPos[0]);
	dy = (curPos[1] - lastPos[1]);
	dz = (curPos[2] - lastPos[2]);
	
	if (mouseDown) 
    {
		if (!redrawContinue)
			angle = 0;
		if ((dx || dy || dz) && control == C_ORBIT) 
        {
		   angle = 90.0F * sqrt(dx * dx + dy * dy + dz * dz) / 3;

			axis[0] = lastPos[1] * curPos[2] - lastPos[2] * curPos[1];
			axis[1] = lastPos[2] * curPos[0] - lastPos[0] * curPos[2];
			axis[2] = lastPos[0] * curPos[1] - lastPos[1] * curPos[0];
			}

			//angle = 90 * sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] *axis[2])/5;

	
		dx = x - lastLoc[0];
		dy = y - lastLoc[1];

		if (control == C_PAN) 
        {
			float bHeight; //height of back plane
			float bWidth;  //width of back plane

			bHeight = 2 * (offset[2] + cam_z) * tan((FOV / 2.0) * (180.0 / M_PI));
			bWidth = ((float)winWidth / (float)winHeight) * bHeight;

			offset[0] -= (dx / (float)winWidth) * bWidth;
			offset[1] -= (dy / (float)winHeight) * bHeight;
			 
		}
	
		if (control == C_ZOOM) 
        {
			if(dy*dy > dx*dx)
			{
				offset[2] -= (dy / (float)winHeight) * cam_z;
				if (offset[2] + cam_z >= 0)
					offset[2] += (dy / (float)winHeight) * cam_z;
			}
		}
	}

	lastPos[0] = curPos[0];
	lastPos[1] = curPos[1];
	lastPos[2] = curPos[2];
	lastLoc[0] = x;
	lastLoc[1] = y;


	glutPostRedisplay();
}

//starts idle rotation
void startMotion(int x, int y)
{
	redrawContinue = false;
	startX = x; startY = y;
	trackball_ptov(x, y, winWidth, winHeight, lastPos);
}

//stops idle rotation
void stopMotion(int x, int y)
{
	if (startX != x || startY != y) 
		redrawContinue = true;
	else 
	{
		//angle = 0.0F;
		redrawContinue = false;
	}
}

/*----------------------------------------------------------------------*/

// the draw function, sets the camera, updates the headlight
// centers and rotates the model.
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	const GLfloat pos[] = {0.0, 0.0, 0.0, 1.0 };
	const GLfloat dir1[] = {0.0, 0.0, -1.0};
    //set up transformations
	glPushMatrix();
      glPushMatrix();
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
	    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, dir1);
	    glLightfv( GL_LIGHT1, GL_POSITION, pos );
	    glRotatef(angle, axis[0], axis[1], axis[2]);
	    glMultMatrixf((GLfloat *) objectXform);
	    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *) objectXform);
	  glPopMatrix();

	  gluLookAt(offset[0], offset[1], offset[2] + cam_z, 
		offset[0], offset[1], 0, 
		0, 1, 0);

	  glMultMatrixf((GLfloat *) objectXform);

	  glTranslatef(-center[0], -center[1], -center[2]);
      //draw the model
	  mdlDraw(objModel, mode, groupDisp);
	glPopMatrix();

	glutSwapBuffers();
	glFlush();

}

/*----------------------------------------------------------------------*/

// Handles the mouse button presses
void mouseButton(int button, int state, int x, int y)
{

	if (button == GLUT_LEFT_BUTTON) 
    {
		switch(state) 
        {
		case GLUT_DOWN:
			mouseDown = true;
			lastLoc[0] = x;
			lastLoc[1] = y;
			if (control == C_ORBIT)
				startMotion(x, y);
			break;
		case GLUT_UP:
			mouseDown = false;
			if (control == C_ORBIT)
				stopMotion(x, y);
			break;
		} 
	}
}

// Reshape function
void myReshape(int w, int h)
{
	glViewport(0, 0, w, h);
	winWidth = w;
	winHeight = h;

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (GLfloat)winWidth/winHeight, 0.01, 10000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
}

// This is the idle function, it will spin the model if redrawContinue
// is set
void spinObject()
{
	if (redrawContinue) 
		glutPostRedisplay();
}
//---------------------------------------------------------------------

/* Top menu controls how the model is drawn
 * Orbit: draging will rotate the model
 * Pan: draging will move the camera along the X and Y axis
 * Zoom: draging will move the camera along the Z axis
 * Reset: resets everything back to its initial settings.
*/
void topMenuFunc(int id)
{
	switch (id)
	{
	case 1: //orbit
		control = C_ORBIT;
		break;
	case 2: //pan
		control = C_PAN;
		break;
	case 3: //zoom
		control = C_ZOOM;
		break;
	case 4: //reset
		initialization();
		break;
	}
	glutPostRedisplay();
}

/* Display menu controls how the model is drawn
 * Points: only draws the vertices of the model
 * Wireframe: only draws the wireframe of the model
 * Solid: draws the model with filled faces.
*/
void displayMenuFunc(int id)
{
	switch (id)
	{
	case 1: //points
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		glDisable(GL_CULL_FACE);
		break;
	case 2: //wireframe
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
		break;
	case 3: //solid
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
		break;
	}
	glutPostRedisplay();
}

/* Light menu controls which lights are on
 * Directional Light: turns on/off the directional light
 * Spot Light: turns on/off the spot light
*/
void lightMenuFunc(int id)
{
	switch (id)
	{
	case 1: // Directional Light
		directionalLight = !directionalLight;
		break;
	case 2: // Spot Light
		spotLight = !spotLight;
		break;
	}
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);

	if (directionalLight)
	{
	    glEnable(GL_LIGHT0);
	}

	if (spotLight)
	{
		glEnable(GL_LIGHT1);
	}
	glutPostRedisplay();
}

/* Shading menu controls what type of shading is used
 * Flat: uses GL_FALT
 * Smooth: uses GL_SMOOTH
 * None: disables lighting
*/
void shadingMenuFunc(int id)
{
	
	switch (id)
	{
	case 1: //flat
		glShadeModel(GL_FLAT);
		glEnable(GL_LIGHTING);
		break;
	case 2: // smooth
		glShadeModel (GL_SMOOTH);
		glEnable(GL_LIGHTING);
		break;
	case 3: //none
		glDisable(GL_LIGHTING);
		break;
	}
	glutPostRedisplay();
}

/* Group menu controls what groups are shown
 * All: shows all groups
 * <Group Name>: only show this group
*/
void groupMenuFunc(int id)
{
	groupDisp = id-2;
	glutPostRedisplay();
}

/* Model menu controls what model is shown
*/
void modelMenuFunc(int id)
{
	objModel = models[id-1];
	mdlLoadModel(objModel);
	initView();
	glutPostRedisplay();
}

/* Colormenu controls what material the surface uses
 * Material: uses the material asigned in the .obj file
 * Color: uses glColor and assigned the color of the diffuse
*/
void colorMenuFunc(int id)
{
	mode = mode | MDL_MATERIAL;
	mode = mode | MDL_COLOR;
	glEnable(GL_CULL_FACE);
	switch (id)
	{	
	case 1: //material
		mode = mode ^ MDL_COLOR;
		break;
	case 2: //color
		mode = mode ^ MDL_MATERIAL;
		break;
	}
	glutPostRedisplay();
}

/* Texturemenu controls what thexture enviroment the textures use
*/
void textureMenuFunc(int id)
{
	mode = mode | MDL_TEXTURE;
	switch (id)
	{	
	case 1: //DECAL
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		break;
	case 2: //REPLACE
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	case 3: //MODULATE
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case 4: //BLEND
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		break;
	case 5: //NONE
		mode = mode ^ MDL_TEXTURE;
		break;
	}
	glutPostRedisplay();
}

/* Creates the various menus and submenus
 */
void createMenu()
{
	int topMenu, displayMenu, lightMenu, shadingMenu, groupMenu, colorMenu, textureMenu, modelMenu;
    MDLgroup * group;

	shadingMenu = glutCreateMenu(shadingMenuFunc);
	glutAddMenuEntry("Flat", 1);
	glutAddMenuEntry("Smooth", 2);
	glutAddMenuEntry("None", 3);

	groupMenu = glutCreateMenu(groupMenuFunc);
	glutAddMenuEntry("ALL", 1);
	for (int i = 0; i < objModel->numgroups; i++)
    {
        group = objModel->groups[i];
        glutAddMenuEntry(group->name.c_str(), i+1);
	}

	textureMenu = glutCreateMenu(textureMenuFunc);
	glutAddMenuEntry("DECAL", 1);
	glutAddMenuEntry("REPLACE", 2);
	glutAddMenuEntry("MODULATE", 3);
	glutAddMenuEntry("BLEND", 4);
	glutAddMenuEntry("NONE", 5);

	colorMenu = glutCreateMenu(colorMenuFunc);
	glutAddMenuEntry("Material", 1);
	glutAddMenuEntry("Color", 2);
	

	displayMenu = glutCreateMenu(displayMenuFunc);
	glutAddSubMenu("Shading", shadingMenu);
	glutAddSubMenu("Show Group", groupMenu);
	glutAddSubMenu("Coloring", colorMenu);
	glutAddSubMenu("TexEnv", textureMenu);
	glutAddMenuEntry("Points", 1);
	glutAddMenuEntry("Wireframe", 2);
    glutAddMenuEntry("Solid", 3);

	lightMenu = glutCreateMenu(lightMenuFunc);
	glutAddMenuEntry("Directional Light", 1);
	glutAddMenuEntry("Spot Light", 2);

	modelMenu = glutCreateMenu(modelMenuFunc);
	for (int i = 0; i < models.size(); i++)
    {
        glutAddMenuEntry(models[i]->pathname.c_str(), i+1);
	}

	topMenu = glutCreateMenu(topMenuFunc);
	glutAddSubMenu("Display", displayMenu);
	glutAddSubMenu("Light", lightMenu);
	glutAddSubMenu("Model", modelMenu);
	glutAddMenuEntry("Orbit", 1);
	glutAddMenuEntry("Pan", 2);
	glutAddMenuEntry("Zoom", 3);
	glutAddMenuEntry("Reset", 4);

	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/*----------------------------------------------------------------------*/

/* Reads all models
 * assumes argv[1] is the directory and all following argvs are the names 
 * of models (names get .obj added to the end)
 */
void readModels(int argc, char **argv)
{
	string dir = argv[1]; //get the model folder
	
	// Add the correct slash at the end
	char good, bad;
	#if defined(_WIN32) || defined (_WIN64)
	  good = '\\';
	  bad = '/';
    #else
      good = '/';
	  bad = '\\';
	#endif
	if(dir[dir.size()-1] != good)
	{
		if(dir[dir.size()-1] == bad)
			dir[dir.size()-1] = good;
		else
			dir = dir + good;
	}


	string file;
	string filepath;
	for(int i = 2; i < argc; i++)
	{
		file = argv[i];
		file.append(".obj");
		filepath = dir + file;
		printf("Loading: %s\n", filepath.c_str());
	    models.push_back(mdlReadOBJ(filepath));
		mdlPrintStats(models[i-2]);
	}
}

/* Entry point, runs all initialization before reaching glutMainLoop()
 */
int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(500, 500);
	glutCreateWindow("Object Viewer");
	if (argc < 3)
	{
		printf("No .obj file specified\n");
		exitError(1);
	}
	readModels(argc, argv);
	initialization();
	createMenu();
	glutReshapeFunc(myReshape);
	glutDisplayFunc(display);
	glutIdleFunc(spinObject);
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(passiveMotion);
	

	glutMainLoop();
    return 0;
}