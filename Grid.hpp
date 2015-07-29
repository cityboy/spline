//----
// Grid.hpp
//----

#ifndef GRID_HPP
#define GRID_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>

class Grid {
public:
	Grid (unsigned int sz, float min=-1.0f, float max=1.0f);
	~Grid ();
	void Initialise ();
	void Display (int color=-1);
	void Warp ();
	
private:
	int mSize;
	float mMax, mMin;
	glm::vec2* mVertices;
	unsigned int* mIndices;
	unsigned int mVerticesSize, mIndicesSize;
	GLuint mVao;
	GLuint mVbo[2];
};

#endif
