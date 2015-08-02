//----
// Grid.hpp
//----

#ifndef GRID_HPP
#define GRID_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <utility>

#define NUM_KNOTS 5

class Grid {
public:
	Grid (unsigned int sz, float min=-1.0f, float max=1.0f);
	~Grid ();
	void Initialise ();
	void Display (int color=-1);
	void Warp (std::pair<glm::vec2,glm::vec2> cp);
	
private:
	int mSize;
	float mMax, mMin;
	glm::vec2* mVertices;
	unsigned int* mIndices;
	unsigned int mVerticesSize, mIndicesSize;
	glm::vec2 mKnots[NUM_KNOTS+3][NUM_KNOTS+3];
	GLuint mVao;
	GLuint mVbo[2];
	void Embedding (glm::vec2 pt, int& i, int& j, float& s, float& t); 
};

#endif
