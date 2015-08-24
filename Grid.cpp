//----
// Grid.cpp
//----

#include "Grid.hpp"
#include <stdio.h>
#include <math.h>
#include <Eigen/Dense>

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
/*-
	//-- Initialise knot values
	for (int i=0; i<NUM_KNOTS+3; i++)
		for (int j=0; j<NUM_KNOTS+3; j++)
			mKnots[i][j] = glm::vec2(0.0f,0.0f);
-*/
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

/*-
// Basis functions
float B0 (float s) {
	return ((1.0f-s)*(1.0f-s)*(1.0f-s)/6.0f);
}
float B1 (float s) {
	return ((3.0f*s*s*s - 6.0f*s*s + 4.0f)/6.0f);
}
float B2 (float s) {
	return ((-3.0f*s*s*s + 3.0f*s*s + 3.0f*s + 1)/6.0f);
}
float B3 (float s) {
	return (s*s*s/6.0f);
}

// Calculate embedding and index of embedding
// Note: index of embedding starts from 0, which is different from the paper
// i.e. Lower left corner of the selection area is (1,1). Upper right corner is (NUM_KNOTS+1,NUM_KNOTS+1)
void Grid::Embedding (glm::vec2 pt, int& i, int& j, float& s, float& t) {
	float step = (mMax - mMin) / float(NUM_KNOTS);
	float ss = (pt.x - mMin) / step;
	float tt = (pt.y - mMin) / step;
	i = int(ss) + 1;
	j = int(tt) + 1;
	s = ss - float(i-1);
	t = tt - float(j-1);
}
-*/

float RBF (float const rsq) {
	return 1.0f / sqrt(1.0f + rsq*10.0f);
}

float Dsq (glm::vec2 const p1, glm::vec2 const p2) {
	glm::vec2 d = p1 - p2;
	return d.x*d.x + d.y*d.y;
}

void Grid::Warp (std::vector<ControlPoint> cps) {

	// Contruct matrix to solve weights for RBF
	int size = cps.size();
	Eigen::MatrixXf Ax(size,size), Ay(size,size);
	Eigen::VectorXf bx(size), by(size);
	glm::vec2 Begin1, End1, Begin2; 
	
	for (int i=0; i<size; i++) {
		Begin1 = cps[i].Begin();
		End1 = cps[i].End();
		bx(i) = End1.x - Begin1.x;
		by(i) = End1.y - Begin1.y;
		for (int j=0; j<size; j++) {
			Begin2 = cps[j].Begin();
			Ax(i,j) = Ay(i,j) = RBF(Dsq(Begin1,Begin2));
		}
	}

	Eigen::VectorXf Wx = Eigen::FullPivLU<Eigen::MatrixXf>(Ax).solve(bx);
	Eigen::VectorXf Wy = Eigen::FullPivLU<Eigen::MatrixXf>(Ay).solve(by);

	float step = (mMax - mMin) / float(mSize);
	
	// Define vertex positions on the sides of the frame
	glm::vec2 *ptr = mVertices;
	glm::vec2 point;
	float rbf;
	for (int i=0; i<=mSize; i++) {
		for (int j=0; j<=mSize; j++) {
			point.x = mMin + step * float(j);
			point.y = mMin + step * float(i);
			for (int k=0; k<size; k++) {
				rbf = RBF(Dsq(point,cps[k].Begin()));
				point.x = point.x + rbf * Wx[k];
				point.y = point.y + rbf * Wy[k];
			}
			ptr->x = point.x;
			ptr->y = point.y;
			ptr++;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
}


