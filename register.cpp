/*
	register.cpp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <common/shader.hpp>
#include <common/Image.hpp>
#include <jpeglib.h>
#include <jerror.h>

GLFWwindow* g_MainWindow;

char g_WindowTitle[] = "REGISTER";
float g_WindowWidth = 800.0f;
float g_WindowHeight = 800.0f;
#define DELTA 0.01f
#define ALPHA 1.0f
#define RIGID_ITERATIONS 200
#define ITERATIONS 200

void CallbackWindowSize (GLFWwindow*, int, int);
void CallbackKey (GLFWwindow*, int, int, int, int);
void CallbackCursonPos (GLFWwindow*, double, double);
void CallbackButton (GLFWwindow*, int, int, int);
GLuint CreateTexture (const unsigned char *data, int width, int height);
GLFWwindow* CreateWindow (const char* title, const int width, const int height);
GLuint CreateFrame (int width, int height);
float MeanTexture();

int main (int argc, char** argv)
{
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}
	// This is requied for OS X
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// A window must be created first before init GLEW
	g_MainWindow = glfwCreateWindow( (int)g_WindowWidth, (int)g_WindowHeight, g_WindowTitle, NULL, NULL);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Create and compile our GLSL program from the shaders for Affine transform and calculate SSD
	GLuint transform_shader = LoadShaders("Combined.vert", "Combined.frag");
	GLuint h_src_tex  = glGetUniformLocation(transform_shader, "SourceTextureSampler");
	GLuint h_tgt_tex  = glGetUniformLocation(transform_shader, "TargetTextureSampler");
	GLuint h_params  = glGetUniformLocation(transform_shader, "params");
	GLuint h_weights  = glGetUniformLocation(transform_shader, "weights");
	GLuint h_affine  = glGetUniformLocation(transform_shader, "affine");
	GLuint h_bcps  = glGetUniformLocation(transform_shader, "bcps");
	GLuint h_sqdiff  = glGetUniformLocation(transform_shader, "sqdiff");
	
	// Variables for source and target images
	GLuint srcTexture, tgtTexture;
	int srcWidth, srcHeight, tgtWidth, tgtHeight;
	unsigned char * image;

	// Load images from files as textures
	if (argc>1) 
		image = LoadJpg(argv[1], srcWidth, srcHeight);
	else 
		image = LoadJpg("sh0r.jpg", srcWidth, srcHeight);
	srcTexture = CreateTexture(image, srcWidth, srcHeight);
	if (argc>2) 
		image = LoadJpg(argv[2], tgtWidth, tgtHeight);
	else 
		image = LoadJpg("sh1r.jpg", tgtWidth, tgtHeight);
	tgtTexture = CreateTexture(image, tgtWidth, tgtHeight);

	// Set windows parameteres
	printf("Image size (WxH) = %d,%d\n",srcWidth,srcHeight);
	glfwSetWindowSize(g_MainWindow, srcWidth, srcHeight);

	glfwMakeContextCurrent(g_MainWindow);
	glfwSetWindowSizeCallback(g_MainWindow,CallbackWindowSize);
	glfwSetKeyCallback(g_MainWindow,CallbackKey);
	glfwSetCursorPosCallback(g_MainWindow,CallbackCursonPos);
	glfwSetMouseButtonCallback(g_MainWindow,CallbackButton);
	// Ensure we can capture the escape key being pressed below
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create frame VAO to display image
	GLuint frameVAO = CreateFrame(srcWidth,srcHeight);
	
	// Create texture for receiving the rendered image
	GLuint diffTexture;
	glGenTextures(1, &diffTexture);
	glBindTexture(GL_TEXTURE_2D, diffTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, srcWidth, srcHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // MIPMAP must exist otherwise this will not work - black screen
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap

	glBindTexture(GL_TEXTURE_2D, 0);
	
	// create a framebuffer object
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	// attach the texture to FBO color attachment point
	// Either of the following works
	//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, diffTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, diffTexture, 0); 
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		printf("Fail to setup framebuffer\n");
		return (0);
	}
	// switch back to window-system-provided framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);

	int fbw, fbh;
	int iter = 0;
	// For multi-window display
    glfwMakeContextCurrent(g_MainWindow);
	glfwGetFramebufferSize(g_MainWindow,&fbw,&fbh); // Get window size in Pixels
	
	// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0,0,srcWidth, srcHeight); // Rendered texture will equal to size of source 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//--
	//-- Rigid register
	//--
	printf("Rigid Registrationd\n");
	glUseProgram(transform_shader);

	// Enable Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTexture);
	glUniform1i(h_src_tex,0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tgtTexture);
	glUniform1i(h_tgt_tex,1);
	glUniform1i(h_affine,1);
	glUniform1i(h_bcps,0);
	glUniform1i(h_sqdiff,1);
	// Define optimisation parameters
	float transform[5] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f};
	// Switch to another texture - avoid affecting Texture 0 & 1
	glBindVertexArray(frameVAO);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, diffTexture);
	iter = 0;
	do {
		float m[6];
		// Render to texture
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0,0,srcWidth, srcHeight); // Rendered texture will equal to size of source 

		for (int p=0; p<=5; p++) {
			if (p!=5)
				transform[p] += DELTA;
			// Set transform parameters
			glUniform1fv(h_params,5,transform);
			// Draw square
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_FAN,0,4);
			// Calculate SSD - using mean instead of sum to scale the value down
			m[p] = MeanTexture();
			
			//- Calculate mean value using MipMap does not work 
/*---- TODO: try to derive mean using GPU
			glGenerateMipmap(GL_TEXTURE_2D);
			float texdata;
			int w,h;
			for (int i=0; i<10; i++) {
				glGetTexLevelParameteriv(GL_TEXTURE_2D,i,GL_TEXTURE_WIDTH,&w);
				glGetTexLevelParameteriv(GL_TEXTURE_2D,i,GL_TEXTURE_HEIGHT,&h);
				if ((w==1)&&(h==1)) {
					//glGetTextureImage(diffTexture,i,GL_RGB,GL_UNSIGNED_BYTE,256,texdata);
					glGetTexImage(GL_TEXTURE_2D,i,GL_RED,GL_FLOAT,&texdata);
					printf("%4d %4d %9.7f %9.7f\n",p,i,m[p],texdata);
					break;
				}
			}
			// Store SSD
			f[p] = texdata;
----*/
			if (p!=5)
				transform[p] -= DELTA;
		}
		for (int p=0; p<5; p++)
			transform[p] -= ALPHA * (m[p] - m[5]);
		printf("%3d (%9.7f) %10.7f %10.7f %10.7f %10.7f %10.7f\n",iter,m[5],transform[0],transform[1],transform[2],transform[3],transform[4]);
		iter++;
		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,fbw,fbh);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);
		// Swap buffers
		glfwSwapBuffers(g_MainWindow);

	} while (iter<RIGID_ITERATIONS);

	//--
	//-- Register using BCPS
	//--
	glUniform1i(h_affine,1);
	glUniform1i(h_bcps,1);
	glUniform1i(h_sqdiff,1);
	// Define optimisation parameters
	float weights[16*2];
	for (int i=0; i<32; i++)
		weights[i] = 0.0f;
	// Switch to another texture - avoid affecting Texture 0 & 1
//	glBindVertexArray(frameVAO);
//	glActiveTexture(GL_TEXTURE2);
//	glBindTexture(GL_TEXTURE_2D, diffTexture);
	iter = 0;
	do {
		float m[33];
		// Render to texture
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0,0,srcWidth, srcHeight); // Rendered texture will equal to size of source 

		for (int p=0; p<=32; p++) {
			if (p!=32)
				weights[p] += DELTA;
			// Set transform parameters
			glUniform2fv(h_weights,32,weights);
			// Draw square
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_FAN,0,4);
			// Calculate SSD - using mean instead of sum to scale the value down
			m[p] = MeanTexture();
			
			//- Calculate mean value using MipMap does not work 
/*---- TODO: try to derive mean using GPU
			glGenerateMipmap(GL_TEXTURE_2D);
			float texdata;
			int w,h;
			for (int i=0; i<10; i++) {
				glGetTexLevelParameteriv(GL_TEXTURE_2D,i,GL_TEXTURE_WIDTH,&w);
				glGetTexLevelParameteriv(GL_TEXTURE_2D,i,GL_TEXTURE_HEIGHT,&h);
				if ((w==1)&&(h==1)) {
					//glGetTextureImage(diffTexture,i,GL_RGB,GL_UNSIGNED_BYTE,256,texdata);
					glGetTexImage(GL_TEXTURE_2D,i,GL_RED,GL_FLOAT,&texdata);
					printf("%4d %4d %9.7f %9.7f\n",p,i,m[p],texdata);
					break;
				}
			}
			// Store SSD
			f[p] = texdata;
----*/
			if (p!=32)
				weights[p] -= DELTA;
		}
		for (int p=0; p<32; p++)
			weights[p] -= ALPHA * (m[p] - m[32]);
		printf("%3d (%9.7f) %10.7f %10.7f %10.7f %10.7f %10.7f %10.7f\n",iter,m[32],weights[0],weights[1],weights[2],weights[3],weights[4],weights[5]);
		iter++;
		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,fbw,fbh);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);
		// Swap buffers
		glfwSwapBuffers(g_MainWindow);

	} while (iter<ITERATIONS);

	// Re-generate final result without SSD
	//-- Render directly to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,fbw,fbh);
	glUniform1i(h_sqdiff,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);

	//-- Render to Texture and then display at screen
/*----
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0,0,srcWidth, srcHeight); // Rendered texture will equal to size of source 
	glUniform1i(sqdiff,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);
	// Render to the screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,fbw,fbh);
	glUseProgram(pass_shader);
	// Enable Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffTexture);
	glUniform1i(texSampler,0);
	// Draw square
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(frameVAO);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);
----*/
	// Swap buffers
	glfwSwapBuffers(g_MainWindow);

	while (!glfwWindowShouldClose(g_MainWindow)) {
		glfwPollEvents();
	} 

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

//---------------------------------
// Supporting call-back functions
//---------------------------------
void CallbackWindowSize (GLFWwindow* window, int width, int height) {
	g_WindowWidth = (double)width;
	g_WindowHeight = (double)height;
}


void CallbackKey (GLFWwindow* window, int key, int scancode, int action, int mods) {
	//printf("KEY - %d %d %d %d\n",key,scancode,action,mods);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void CallbackCursonPos (GLFWwindow* window, double x, double y) {
	bool bMB1 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_1)==GLFW_PRESS);
	bool bMB2 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_2)==GLFW_PRESS);
	bool bMB3 = (glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_3)==GLFW_PRESS);
	bool bCtrl = (glfwGetKey(window,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS) || (glfwGetKey(window,GLFW_KEY_RIGHT_CONTROL)==GLFW_PRESS);
}

void CallbackButton (GLFWwindow* window, int button, int action, int mods) {
	double x,y;
	glfwGetCursorPos(window,&x,&y);
	//printf("BUTTON - %6.1f %6.1f %d %d %d\n",x,y,button,action,mods);
	if (button==GLFW_MOUSE_BUTTON_1 && action==GLFW_PRESS) {

	} 
}

GLuint CreateTexture (const unsigned char *data, int width, int height) {
	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);
	
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	// Poor filtering, or ...
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	// ... nice trilinear filtering.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 

	// Return the ID of the texture we just created
	return textureID;
}

GLuint CreateFrame (int width, int height) {
	GLfloat frame_vertex[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f, 
		 1.0f,  1.0f, 0.0f, 
		-1.0f,  1.0f, 0.0f
	};
	GLfloat frame_uv[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	GLuint frame_Vao;
	glGenVertexArrays(1, &frame_Vao);
	glBindVertexArray(frame_Vao);

	GLuint frame_Vbo[2];
	glGenBuffers(2, frame_Vbo);
	glBindBuffer(GL_ARRAY_BUFFER, frame_Vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frame_vertex), frame_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, frame_Vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frame_uv), frame_uv, GL_STATIC_DRAW);
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	return frame_Vao;
}

float MeanTexture() {
	int w,h,l;
	float texdata[300000];
	float sum=0.0f;
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
	glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,texdata);
	l = w * h;
	for (int i=0; i<l; i++)
		sum += texdata[i];
	return sum/float(l);

}
