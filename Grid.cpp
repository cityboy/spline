//----
// Grid.cpp
//----

#include "Grid.hpp"
#include <stdio.h>

Grid::Grid (unsigned int sz, float min, float max) {
	mSize = sz;
	mMin = min;
	mMax = max;
	mVerticesSize = (mSize+1) * (mSize+1);
	mIndicesSize = mSize * (mSize+1) * 4;
	mVertices = new glm::vec2 [mVerticesSize];
	mIndices = new unsigned int [mIndicesSize];

	glGenVertexArrays(1, &mVao);
	glGenBuffers(2, mVbo);

	Initialise();
}

Grid::~Grid () {
	glDeleteBuffers(2,mVbo);
	glDeleteVertexArrays(1,&mVao);
	delete[] mIndices;
	delete[] mVertices;
}

void Grid::Initialise () {
	//-- Initialise vertex positions
	float step = (mMax - mMin) / float(mSize);
	
	// Define vertex positions on the sides of the frame
	glm::vec2 *ptr = mVertices;
	for (int i=0; i<=mSize; i++) {
		for (int j=0; j<=mSize; j++) {
			ptr->x = mMin + step * float(j);	//-- x
			ptr->y = mMin + step * float(i);	//-- y
			ptr++;
		}
	}
	/*--- DEBUG PRINT ---*
	for (int i=0; i<mVerticesSize; i++)
		printf("%4d: %5.2f,%5.2f,%5.2f\n",i,mVertices[i].x,mVertices[i].y,mVertices[i].z);
	printf ("\n");
	*--- DEBUG PRINT ---*/
	// Define the connection sequence
	int idx = 0;
	for (int i=0; i<=mSize; i++) {
		for (int j=0; j<=mSize; j++) {
			if (j!=mSize) {
				mIndices[idx++] = i * (mSize+1) + j;
				mIndices[idx++] = i * (mSize+1) + (j+1);
			}
			if (i!=mSize) {
				mIndices[idx++] = i * (mSize+1) + j;
				mIndices[idx++] = (i+1) * (mSize+1) + j;
			}
		}
	}
	/*--- DEBUG PRINT ---*
	for (int i=0; i<mIndicesSize/2; i++)
		printf("%4d: %4d,%4d\n",i,mIndices[i*2],mIndices[i*2+1]);
	printf ("\n");
	*--- DEBUG PRINT ---*/
	//-- Initialise VAO and VBO
	glBindVertexArray(mVao);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize * sizeof(unsigned int), mIndices, GL_STATIC_DRAW);
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
}

// color has default value of -1, which means DOES NOT change color
void Grid::Display (int color) {
	if (color>=0)
		glUniform3f(color, 0.8f, 0.8f, 0.8f);
	glBindVertexArray(mVao);
	glDrawElements(GL_LINES, mIndicesSize, GL_UNSIGNED_INT, (void*)0);
}

void Grid::Warp () {
	
}

