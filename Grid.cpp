//----
// Grid.cpp
//----

#include "Grid.hpp"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <Eigen/Dense>

Grid::Grid (unsigned int sz, float min, float max) {
	mSize = sz;
	mMin = min;
	mMax = max;
	mVerticesSize = (mSize+1) * (mSize+1);
	mIndicesSize[0] = mSize * (mSize+1) * 4;
	mIndicesSize[1] = mSize * mSize * 6;
	mVertices = new glm::vec2 [mVerticesSize];
	mUV = new glm::vec2 [mVerticesSize];
	mIndices = new unsigned int [mIndicesSize[0] + mIndicesSize[1]];

	glGenVertexArrays(1, &mVaoGrid);
	glGenBuffers(3, mVboGrid);

	Initialise();
}

Grid::~Grid () {
	glDeleteBuffers(3,mVboGrid);
	glDeleteVertexArrays(1,&mVaoGrid);
	delete[] mIndices;
	delete[] mVertices;
}

void Grid::SetShaderInterface (GLuint useTexture, GLuint Color, GLuint Texture) {
	mUseTextureID = useTexture;
	mColorID = Color;
	mTextureID = Texture;
}

void Grid::SetImage (const unsigned char *buff, int width, int height) {
	mImageBuffer = buff;
	mWidth = width;
	mHeight = height;
	mTexImage = CreateTexture(mImageBuffer, mWidth, mHeight);
}

void Grid::Initialise () {

	//-- Initialise Grid vertex positions and texture coordinates
	float step = (mMax - mMin) / float(mSize);
	glm::vec2 *ptr_vert = mVertices;
	glm::vec2 *ptr_uv = mUV;
	for (int i=0; i<=mSize; i++) {
		for (int j=0; j<=mSize; j++) {
			ptr_vert->x = mMin + step * float(j);
			ptr_vert->y = mMin + step * float(i);
			ptr_vert++;
			ptr_uv->x = float(j) / float(mSize);
			ptr_uv->y = float(i) / float(mSize);
			ptr_uv++;
		}
	}

	// Define the connection sequence for Grid
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

	// Define the connection sequence for filled triangles
	idx = mIndicesSize[0];
	for (int i=0; i<mSize; i++) {
		for (int j=0; j<mSize; j++) {
			// Triangle 1
			mIndices[idx++] = i*(mSize+1) + j;
			mIndices[idx++] = i*(mSize+1) + (j+1);
			mIndices[idx++] = (i+1)*(mSize+1) + (j+1);
			// Triangle 2
			mIndices[idx++] = i*(mSize+1) + j;
			mIndices[idx++] = (i+1)*(mSize+1) + (j+1);
			mIndices[idx++] = (i+1)*(mSize+1) + j;
		}
	}

	//-- Setup VAO and VBO - combined for both Grid and filled triangles
	glBindVertexArray(mVaoGrid);
	glBindBuffer(GL_ARRAY_BUFFER, mVboGrid[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVboGrid[1]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mUV, GL_STATIC_DRAW);
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboGrid[2]);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize[1] * sizeof(unsigned int), mIndices + mIndicesSize[0], GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (mIndicesSize[0]+mIndicesSize[1]) * sizeof(unsigned int), mIndices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void Grid::Display () {
	// Display grid
	glUniform1i(mUseTextureID,0);
	glUniform3f(mColorID, 1.0f, 1.0f, 1.0f);
	//glBindVertexArray(mVaoGrid[0]);
	//glDrawElements(GL_LINES, mIndicesSize[0], GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(mVaoGrid);
	glDrawElements(GL_LINES, mIndicesSize[0], GL_UNSIGNED_INT, (void*)0);

	// Display image using texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexImage);
	glUniform1i(mUseTextureID,1);
	glUniform1i(mTextureID,0);
	glBindVertexArray(mVaoGrid);
	glDrawElements(GL_TRIANGLES, mIndicesSize[1], GL_UNSIGNED_INT, (void*)(mIndicesSize[0]*sizeof(unsigned int)));
}

float RBF (float const rsq) {
	//return 1.0f / sqrt(1.0f + rsq*10.0f);
	return exp(-rsq*2.0f);
}

float Dsq (glm::vec2 const p1, glm::vec2 const p2) {
	glm::vec2 d = p1 - p2;
	return d.x*d.x + d.y*d.y;
}

float A (glm::vec2 const p1, glm::vec2 const p2) {
//	return (sqrt( (p1.x*p1.x+p1.y*p1.y)*(p2.x*p2.x+p2.y*p2.y) - 2*(p1.x*p2.x+p1.y*p2.y) + 1) / glm::length(p1-p2));
	float l1 = glm::length(p1);
	float l2 = glm::length(p2);
	float l12 = glm::length(p1-p2);
	float d12 = glm::dot(p1,p2);
	return ( sqrt(l1*l1*l2*l2 - 2*d12 + 1) / l12 );
}

float G (glm::vec2 const p1, glm::vec2 const p2) {
	float a = A(p1, p2);
	float l = glm::length(p1-p2);
	return ( l*l * ((a*a-1)/2 - log(a)) );
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
			if (j==i) {
				Ax(i,j) = Ay(i,j) = 1.0f;
			} else {
				Begin2 = cps[j].Begin();
				//Ax(i,j) = Ay(i,j) = RBF(Dsq(Begin1,Begin2));
				Ax(i,j) = Ay(i,j) = G(Begin1,Begin2);
			}
		}
	}
	std::cout << "Ax = " <<std::endl << Ax << std::endl;
	std::cout << "Ay = " <<std::endl << Ay << std::endl;

	Eigen::VectorXf Wx = Eigen::FullPivLU<Eigen::MatrixXf>(Ax).solve(bx);
	Eigen::VectorXf Wy = Eigen::FullPivLU<Eigen::MatrixXf>(Ay).solve(by);
	
	std::cout << "Wx " <<std::endl << Wx << std::endl;
	std::cout << "Wy " <<std::endl << Wy << std::endl;

	float step = (mMax - mMin) / float(mSize);
	
	// Update the Grid vertex positions
	glm::vec2 *ptr = mVertices;
	glm::vec2 point;
	float g;
	for (int i=0; i<=mSize; i++) {
		for (int j=0; j<=mSize; j++) {
			ptr->x = point.x = mMin + step * float(j);
			ptr->y = point.y = mMin + step * float(i);
			if (glm::length(point)<1.0f) {
				for (int k=0; k<size; k++) {
					//rbf = RBF(Dsq(point,cps[k].Begin()));
					g = G(point,cps[k].Begin());
					ptr->x += Wx[k] * g;
					ptr->y += Wy[k] * g;
				}
			}
			ptr++;
		}
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, mVboGrid[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
}


