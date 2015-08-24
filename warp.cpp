/*
	warp.cpp
	Warpping the grid with mouse input.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <utility>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
GLFWwindow* window;

#include <glm/glm.hpp>
using namespace glm;

#include "Grid.hpp"
#include "ControlPoint.hpp"

char sWindowTitle[] = "WARP";
float win_width = 800.0f;
float win_height = 800.0f;

Grid* grid;
#define GRID_SZ 50
#define GRID_MIN -0.95f
#define GRID_MAX 0.95f

std::vector<ControlPoint> controlPoints;

void window_size_callback(GLFWwindow*, int, int);
void key_callback (GLFWwindow*, int, int, int, int);
void cursor_pos_callback (GLFWwindow*, double, double);
void button_callback (GLFWwindow*, int, int, int);
GLuint LoadShader (const char *vertex_shader, const char *fragment_shader);
GLuint CreateShape ();

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}
	// This is requied for OS X
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( (int)win_width, (int)win_height, sWindowTitle, NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetWindowSizeCallback(window,window_size_callback);
	glfwSetKeyCallback(window,key_callback);
	//glfwSetCursorPosCallback(window,cursor_pos_callback);
	glfwSetMouseButtonCallback(window,button_callback);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create and compile our GLSL program from the shaders
//	GLuint programme = LoadSimpleShader(vertex_shader,fragment_shader);
	GLuint programme = LoadShader("Simple.vert","Simple.frag");
    glUseProgram (programme);
	GLuint ColorID = glGetUniformLocation(programme, "color");

	grid = new Grid(GRID_SZ, GRID_MIN, GRID_MAX);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	while( !glfwWindowShouldClose(window) ) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (std::vector<ControlPoint>::iterator it=controlPoints.begin(); it!=controlPoints.end(); ++it) {
			it->Display(ColorID);
		}

		grid->Display(ColorID);
		
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} 

	for (std::vector<ControlPoint>::iterator it=controlPoints.begin(); it!=controlPoints.end(); ++it) {
		vec2 b = it->Begin();
		vec2 e = it->End();
		printf("%6.3f,%6.3f - %6.3f,%6.3f\n",b.x,b.y,e.x,e.y);
	}

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

//---------------------------------
// Supporting call-back functions
//---------------------------------
void window_size_callback (GLFWwindow* window, int width, int height) {
	win_width = (double)width;
	win_height = (double)height;
}

void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		grid->Warp(controlPoints);
}

void cursor_pos_callback (GLFWwindow* window, double x, double y) {
	bool bMB1 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_1)==GLFW_PRESS);
	bool bMB2 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_2)==GLFW_PRESS);
	bool bMB3 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_3)==GLFW_PRESS);
	bool bCtrl = (glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) || (glfwGetKey(window,GLFW_KEY_RIGHT_CONTROL)==GLFW_PRESS);
}

void ScalePosition (double mouse_x, double mouse_y, float& scaled_x, float& scaled_y) {
	scaled_x = float(mouse_x / win_width) * 2.0f - 1.0f;
	scaled_y = 1.0f - float(mouse_y / win_height) * 2.0f;
}

void button_callback (GLFWwindow* window, int button, int action, int mods) {
	double x,y;
	glfwGetCursorPos(window,&x,&y);
	//printf("BUTTON - %6.1f %6.1f %d %d %d\n",x,y,button,action,mods);
	if (button==GLFW_MOUSE_BUTTON_1 && action==GLFW_PRESS) {
		float sx,sy;
		ScalePosition(x,y,sx,sy);
		ControlPoint ctrlPt(sx,sy);
		controlPoints.push_back(ctrlPt);
		std::cout << "P:" << x << "," << y << " -- " << sx << "," << sy << std::endl;
	} 
	if (button==GLFW_MOUSE_BUTTON_1 && action==GLFW_RELEASE) {
		float sx,sy;
		ScalePosition(x,y,sx,sy);
		if ((sx<GRID_MIN) || (sx>GRID_MAX) || (sy<GRID_MIN) || (sy>GRID_MAX))
			controlPoints.pop_back();	// control point is invalid - DISCARD
		else {
			ControlPoint ctrlPt = controlPoints.back();
			if ((ctrlPt.Begin().x==sx) && (ctrlPt.Begin().y==sy))
				controlPoints.pop_back();	// control point is invalid - DISCARD
			else 
				controlPoints.back().SetEnd(sx,sy);
		}
		std::cout << "R:" << x << "," << y << " -- " << sx << "," << sy << std::endl;
		
	} 
}

//-- Read content of file 
int ReadFile (const char *filename, char **buffer) {
	FILE *fp;
	int fsize;

	fp = fopen(filename,"rb");
	if (fp==NULL)
		return 0;
	//-- Find file size
	fseek(fp, 0, SEEK_END); 
	fsize = ftell(fp); 
	fseek(fp, 0, SEEK_SET);
	//-- Allocate memory to keep to content
	if (*buffer!=NULL)
		free(*buffer);
	*buffer = (char*)malloc(fsize+1);
	//-- Read file content
	fread(*buffer, 1, fsize, fp);
	(*buffer)[fsize] = 0;
	//-- Close file and return file size
	fclose(fp);
	return fsize;
}

GLuint LoadShader (const char *vertex_shader, const char *fragment_shader) {
	char *shader_code;
	GLint blen=0;
	GLsizei slen=0;
	
	ReadFile(vertex_shader,&shader_code);
    GLuint vs = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (vs, 1, &shader_code, NULL);
    glCompileShader (vs);
	glGetShaderiv(vs, GL_INFO_LOG_LENGTH , &blen);       
	if (blen > 1) {
		GLchar* compiler_log = (GLchar*)malloc(blen);
		glGetShaderInfoLog(vs, blen, &slen, compiler_log);
		printf("VTX compiler_log:%s\n", compiler_log);
		free (compiler_log);
		return(0);
	}
	//free (shader_code); //-- unable to free memory allocated in other part of the code

	ReadFile(fragment_shader,&shader_code);
    GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fs, 1, &shader_code, NULL);
    glCompileShader (fs);
	glGetShaderiv(fs, GL_INFO_LOG_LENGTH , &blen);       
	if (blen > 1) {
		GLchar* compiler_log = (GLchar*)malloc(blen);
		glGetShaderInfoLog(fs, blen, &slen, compiler_log);
		printf("FRAG compiler_log:%s\n", compiler_log);
		free (compiler_log);
		return(0);
	}
	//free (shader_code); //-- unable to free memory allocated in other part of the code

    GLuint shader_programme = glCreateProgram ();
    glAttachShader (shader_programme, fs);
    glAttachShader (shader_programme, vs);
    glLinkProgram (shader_programme);
	return shader_programme;
}


