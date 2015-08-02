//----
// Grid.cpp
//----

#include "Grid.hpp"
#include <stdio.h>
#include <math.h>

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
	//-- Initialise knot values
	for (int i=0; i<NUM_KNOTS+3; i++)
		for (int j=0; j<NUM_KNOTS+3; j++)
			mKnots[i][j] = glm::vec2(0.0f,0.0f);

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

void Grid::Warp (std::vector<ControlPoint> cps) {
	int i, j;
	float s, t;
	float bs[4], bt[4];

	//-- Intermediate arrays
	glm::vec2 d[NUM_KNOTS+3][NUM_KNOTS+3];
	float w[NUM_KNOTS+3][NUM_KNOTS+3];
	for (int b=0; b<NUM_KNOTS+3; b++) {
		for (int a=0; a<NUM_KNOTS+3; a++) {
			d[a][b] = glm::vec2(0.0f,0.0f);
			w[a][b] = 0.0f;
		}
	}

	//-- Calculate knots for each control point and update intermediate arrays
	for (std::vector<ControlPoint>::iterator it=cps.begin(); it!=cps.end(); ++it) {
		printf("(%6.3f,%6.3f) (%6.3f,%6.3f)\n",it->Begin().x,it->Begin().y,it->End().x,it->End().y);
		glm::vec2 ctrlPt = it->Begin();
		glm::vec2 delta = it->End() - it->Begin();
	
		// Calculate the embeddings
		Embedding(ctrlPt, i, j, s, t);
		printf("[%2d,%2d] (%6.3f,%6.3f)\n",i,j,s,t);
		// Calculate basis values
		bs[0] = B0(s); bs[1] = B1(s); bs[2] = B2(s); bs[3] = B3(s);
		bt[0] = B0(t); bt[1] = B1(t); bt[2] = B2(t); bt[3] = B3(t);
		float sum_w_sq = 0.0f;
		for (int b=0; b<=3; b++)
			for (int a=0; a<=3; a++)
				sum_w_sq += bs[a]*bs[a]*bt[b]*bt[b];
		// Calculate the shift of 16 neighborhood
		for (int l=0; l<=3; l++) {
			for (int k=0; k<=3; k++) {
				mKnots[i+k-1][j+l-1] = delta * bs[k]*bt[l]/sum_w_sq;
				float w_kl = bs[k]*bs[k]*bt[l]*bt[l];
				d[i+k-1][j+l-1] = d[i+k-1][j+l-1] + mKnots[i+k-1][j+l-1] * w_kl;
				w[i+k-1][j+l-1] += w_kl;				
			}
		}
		/*--- DEBUG PRINT ---*
		for (int b=0; b<NUM_KNOTS+3; b++) {
			printf("b=%2d: ",b);
			for (int a=0; a<NUM_KNOTS+3; a++)
				printf("(%6.3f,%6.3f) ",mKnots[a][b].x,mKnots[a][b].y);
			printf("\n");
		}
		*--- DEBUG PRINT ---*/
	}
	//-- Calculate the combined knots
	for (int b=0; b<NUM_KNOTS+3; b++) {
		for (int a=0; a<NUM_KNOTS+3; a++) {
			if (w[a][b]!=0)
				mKnots[a][b] = d[a][b] / w[a][b];
			else
				mKnots[a][b] = glm::vec2(0.0f,0.0f);
		}
	}
	/*--- DEBUG PRINT ---*/
	for (int b=0; b<NUM_KNOTS+3; b++) {
		printf("b=%2d: ",b);
		for (int a=0; a<NUM_KNOTS+3; a++)
			printf("(%6.3f,%6.3f) ",mKnots[a][b].x,mKnots[a][b].y);
		printf("\n");
	}
	/*--- DEBUG PRINT ---*/
	// Re-evalute vertices positions 
	float step = (mMax - mMin) / float(mSize);
	glm::vec2 *ptr = mVertices;
	for (int b=0; b<=mSize; b++) {
		for (int a=0; a<=mSize; a++) {
			ptr->x = mMin + step * float(b);
			ptr->y = mMin + step * float(a);
			Embedding(*ptr, i, j, s, t);
			bs[0] = B0(s); bs[1] = B1(s); bs[2] = B2(s); bs[3] = B3(s);
			bt[0] = B0(t); bt[1] = B1(t); bt[2] = B2(t); bt[3] = B3(t);
			for (int l=0; l<=3; l++) {
				for (int k=0; k<=3; k++) {
					*ptr = *ptr + mKnots[i+k-1][j+l-1] * bs[k]*bt[l];
				}
			}
			ptr++;
		}
	}	

	glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
}

