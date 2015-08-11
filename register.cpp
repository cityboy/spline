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
#include <jpeglib.h>
#include <jerror.h>

GLFWwindow* g_MainWindow;

char g_WindowTitle[] = "REGISTER";
float g_WindowWidth = 800.0f;
float g_WindowHeight = 800.0f;
#define BORDER 5

const char* passthrough_vertex_shader =
"#version 400\n"
"layout(location=0) in vec3 vp;\n"
"layout(location=1) in vec2 vUV;\n"
"out vec2 UV;\n"
"void main () {\n"
"  gl_Position = vec4 (vp, 1.0);\n"
"  UV = vUV;\n"
"}";
const char* passthrough_fragment_shader =
"#version 400\n"
"in vec2 UV;\n"
"out vec4 frag_colour;\n"
"uniform sampler2D TextureSampler;\n"
"void main () {\n"
"float color = texture(TextureSampler,UV).r;\n"
"  frag_colour = vec4(color,color,color,1.0f);\n"
"}";

const char* diff_vertex_shader =
"#version 400\n"
"layout(location=0) in vec3 vp;\n"
"layout(location=1) in vec2 vUV;\n"
"out vec2 UV;\n"
"void main () {\n"
"  gl_Position = vec4 (vp, 1.0);\n"
"  UV = vUV;\n"
"}";
const char* diff_fragment_shader =
"#version 400\n"
"in vec2 UV;\n"
"layout(location=0) out vec4 frag_colour;\n"
"uniform sampler2D SourceTextureSampler;\n"
"uniform sampler2D TargetTextureSampler;\n"
"void main () {\n"
"float color = texture(SourceTextureSampler,UV).r - texture(TargetTextureSampler,UV).r;\n"
"	color = color * color;\n"
"  frag_colour = vec4(color,0.0f,0.0f,1.0f);\n"
"}";

void CallbackWindowSize (GLFWwindow*, int, int);
void CallbackKey (GLFWwindow*, int, int, int, int);
void CallbackCursonPos (GLFWwindow*, double, double);
void CallbackButton (GLFWwindow*, int, int, int);
GLuint LoadShader  (const char *vertex_shader, const char *fragment_shader);
GLuint LoadBmpTexture (const char* bmpfilename);
GLuint LoadJpegTexture (const char* jpegfilename, int* width=NULL, int* height=NULL);
GLFWwindow* CreateWindow (const char* title, const int width, const int height);
GLuint CreateFrame (int width, int height, int border);

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

	// Create and compile our GLSL program from the shaders
	GLuint diff_shader = LoadShader(diff_vertex_shader, diff_fragment_shader);
	GLuint srcSampler  = glGetUniformLocation(diff_shader, "SourceTextureSampler");
	GLuint tgtSampler  = glGetUniformLocation(diff_shader, "TargetTextureSampler");

	GLuint pass_shader = LoadShader(passthrough_vertex_shader, passthrough_fragment_shader);
	GLuint texSampler  = glGetUniformLocation(pass_shader, "TextureSampler");

	// Variables for source and target images
	GLuint srcTexture, tgtTexture;
	int srcWidth, srcHeight, tgtWidth, tgtHeight;

	// Load images as textures
	if (argc>1) 
		srcTexture = LoadJpegTexture(argv[1], &srcWidth, &srcHeight);
	else 
		srcTexture = LoadJpegTexture("sh0r.jpg", &srcWidth, &srcHeight);

	if (argc>2) 
		tgtTexture = LoadJpegTexture(argv[2], &tgtWidth, &tgtHeight);
	else 
		tgtTexture = LoadJpegTexture("sh1r.jpg", &tgtWidth, &tgtHeight);
	
	printf("Image size (WxH) = %d,%d\n",srcWidth,srcHeight);
	glfwSetWindowSize(g_MainWindow, srcWidth+2*BORDER, srcHeight+2*BORDER);

	glfwMakeContextCurrent(g_MainWindow);
	glfwSetWindowSizeCallback(g_MainWindow,CallbackWindowSize);
	glfwSetKeyCallback(g_MainWindow,CallbackKey);
	glfwSetCursorPosCallback(g_MainWindow,CallbackCursonPos);
	glfwSetMouseButtonCallback(g_MainWindow,CallbackButton);
	// Ensure we can capture the escape key being pressed below
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Create frame to show image
	GLuint frameVAO = CreateFrame(srcWidth,srcHeight,BORDER);
	
	GLuint diffTexture;
	glGenTextures(1, &diffTexture);
	glBindTexture(GL_TEXTURE_2D, diffTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, srcWidth, srcHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
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

	while( !glfwWindowShouldClose(g_MainWindow) ) {
		
		// Render to our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0,0,srcWidth, srcHeight); // Rendered texture will equal to size of source 

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(diff_shader);
		// Enable Texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, srcTexture);
		glUniform1i(srcSampler,0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tgtTexture);
		glUniform1i(tgtSampler,1);
		// Draw square
		glBindVertexArray(frameVAO);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);

		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		int fbw, fbh;
		glfwGetFramebufferSize(g_MainWindow,&fbw,&fbh); // Get window size in Pixels
		glViewport(0,0,fbw,fbh);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(pass_shader);
		// Enable Texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffTexture);
		glUniform1i(texSampler,0);

		// Draw square
		glBindVertexArray(frameVAO);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);

		// Swap buffers
		glfwSwapBuffers(g_MainWindow);
		glfwPollEvents();

	} 
	glGenerateMipmap(GL_TEXTURE_2D);

	// Get texture data
	unsigned char texdata[500000];
	int w,h;
	for (int i=5; i<10; i++) {
		glGetTexLevelParameteriv(GL_TEXTURE_2D,i,GL_TEXTURE_WIDTH,&w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,i,GL_TEXTURE_HEIGHT,&h);
		printf("\n>> %d %d %d\n",i,w,h);
		if ((w<1)||(h<1))
			break;
		glGetTexImage(GL_TEXTURE_2D,i,GL_RGBA,GL_UNSIGNED_BYTE,texdata);
		//glGetTextureImage(diffTexture,i,GL_RGB,GL_UNSIGNED_BYTE,256,texdata);
		for (int n=0; n<w*h; n++)
			printf("%4d %3d %3d %3d %3d\n",(n/w),texdata[n*4],texdata[n*4+1],texdata[n*4+2],texdata[n*4+3]);
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

GLuint LoadShader (const char *vertex_shader, const char *fragment_shader) {
	GLint blen=0;
	GLsizei slen=0;
	
    GLuint vs = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (vs, 1, &vertex_shader, NULL);
    glCompileShader (vs);
	glGetShaderiv(vs, GL_INFO_LOG_LENGTH , &blen);       
	if (blen > 1) {
		GLchar* compiler_log = (GLchar*)malloc(blen);
		glGetShaderInfoLog(vs, blen, &slen, compiler_log);
		printf("VTX compiler_log:%s\n", compiler_log);
		free (compiler_log);
		return(0);
	}
    GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fs, 1, &fragment_shader, NULL);
    glCompileShader (fs);
	glGetShaderiv(fs, GL_INFO_LOG_LENGTH , &blen);       
	if (blen > 1) {
		GLchar* compiler_log = (GLchar*)malloc(blen);
		glGetShaderInfoLog(fs, blen, &slen, compiler_log);
		printf("FRAG compiler_log:%s\n", compiler_log);
		free (compiler_log);
		return(0);
	}
    GLuint shader_programme = glCreateProgram ();
    glAttachShader (shader_programme, fs);
    glAttachShader (shader_programme, vs);
    glLinkProgram (shader_programme);
	return shader_programme;
}

GLuint LoadBmpTexture (const char* bmpfilename) {

	printf("Reading image %s\n", bmpfilename);

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(bmpfilename,"rb");
	if (!file) {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", bmpfilename); 
		getchar(); 
		return 0;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if ( fread(header, 1, 54, file)!=54 ){ 
		printf("Not a correct BMP file\n");
		return 0;
	}
	// A BMP files always begins with "BM"
	if ( header[0]!='B' || header[1]!='M' ){
		printf("Not a correct BMP file\n");
		return 0;
	}
	// Make sure this is a 24bpp file
	if ( *(int*)&(header[0x1E])!=0  )         {printf("Not a correct BMP file\n");    return 0;}
	if ( *(int*)&(header[0x1C])!=24 )         {printf("Not a correct BMP file\n");    return 0;}

	// Read the information about the image
	dataPos    = *(int*)&(header[0x0A]);
	imageSize  = *(int*)&(header[0x22]);
	width      = *(int*)&(header[0x12]);
	height     = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize==0)    imageSize=width*height*3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos==0)      dataPos=54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char [imageSize];

	// Read the actual data from the file into the buffer
	fread(data,1,imageSize,file);

	// Everything is in memory now, the file wan be closed
	fclose (file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);
	
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete [] data;

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

GLuint LoadJpegTexture (const char* jpegfilename, int* width, int* height) {

	printf("Reading image %s\n", jpegfilename);

	// Open the file
	FILE * file = fopen(jpegfilename,"rb");
	if (!file) {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", jpegfilename); 
		getchar(); 
		return 0;
	}

	//Init the structs required by libjpeg
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	//Redirect stderr so things aren't messy
	cinfo.err = jpeg_std_error(&jerr);

	//Init the decompress struct
	jpeg_create_decompress(&cinfo);

	//Point libjpeg at the file
	jpeg_stdio_src(&cinfo, file);
	//Read in the JPEG header
	jpeg_read_header(&cinfo, TRUE);
	
	if (width!=NULL)
		*width = cinfo.image_width;
	if (height!=NULL)
		*height = cinfo.image_height;
	
	// Create a buffer
	unsigned char* data = (unsigned char*) malloc(sizeof(char) * cinfo.num_components * cinfo.image_width * cinfo.image_height);

	//Begin magic.
	jpeg_start_decompress(&cinfo);

	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char* line = data + (cinfo.num_components * cinfo.image_width) * (cinfo.image_height-cinfo.output_scanline-1);
		jpeg_read_scanlines(&cinfo, &line, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	// Everything is in memory now, the file wan be closed
	fclose (file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);
	
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	if (cinfo.num_components==1)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cinfo.image_width, cinfo.image_height, 0, GL_RED	, GL_UNSIGNED_BYTE, data);
	else 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cinfo.image_width, cinfo.image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete [] data;

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

GLuint CreateFrame (int width, int height, int border) {
	static unsigned int square_index[] = {
		0, 1, 2, 3
	};
	static GLfloat square_vertex[] = {
		-0.9f, -0.9f, 0.0f,
		 0.9f, -0.9f, 0.0f, 
		 0.9f,  0.9f, 0.0f, 
		-0.9f,  0.9f, 0.0f
	};
	static GLfloat square_uv[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	
	float rel_width = float(width) / float(width + 2*border);
	float rel_height = float(height) / float(height + 2*border);
	square_vertex[0] = -rel_width;
	square_vertex[1] = -rel_height;
	square_vertex[3] =  rel_width;
	square_vertex[4] = -rel_height;
	square_vertex[6] =  rel_width;
	square_vertex[7] =  rel_height;
	square_vertex[9] = -rel_width;
	square_vertex[10] = rel_height;
	
	GLuint squareVao;
	glGenVertexArrays(1, &squareVao);
	glBindVertexArray(squareVao);

	GLuint squareVbo[3];
	glGenBuffers(3, squareVbo);
	glBindBuffer(GL_ARRAY_BUFFER, squareVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertex), square_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, squareVbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(square_uv), square_uv, GL_STATIC_DRAW);
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareVbo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(square_index), square_index, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	return squareVao;
}


