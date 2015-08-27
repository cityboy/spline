//----
// Grid.hpp
//----

#ifndef GRID_HPP
#define GRID_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <utility>
#include <vector>
#include "ControlPoint.hpp"

#define NUM_KNOTS 5

extern GLuint CreateTexture (const unsigned char *data, int width, int height);

class Grid {
public:
	Grid (unsigned int sz, float min=-1.0f, float max=1.0f);
	~Grid ();
	void SetShaderInterface (GLuint useTexture, GLuint Color, GLuint Texture);
	void SetImage (const unsigned char *buff, int width, int height);
	void Initialise ();
	void Display ();
	void Warp (std::vector<ControlPoint> cps);
	
private:
	int mSize;
	float mMax, mMin;
	glm::vec2* mVertices;
	glm::vec2* mUV;
	unsigned int* mIndices;
	unsigned int mVerticesSize, mIndicesSize[2];
	glm::vec2 mKnots[NUM_KNOTS+3][NUM_KNOTS+3];
	GLuint mVaoGrid;
	GLuint mVboGrid[3];
	GLuint mUseTextureID, mColorID, mTextureID;
	const unsigned char *mImageBuffer;
	int mWidth, mHeight;
	GLuint mTexImage; 
	void Embedding (glm::vec2 pt, int& i, int& j, float& s, float& t); 
};

#endif
