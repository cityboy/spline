//----
// Grid.cpp
//----

#include "Grid.hpp"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <Eigen/Dense>

Grid::Grid (unsigned int sz, float min, float max) {
	mSize = sz;
	mMin = min;
	mMax = max;
	mVerticesSize = (mSize+1) * (mSize+1);
	mIndicesSize = mSize * (mSize+1) * 4;
	mVertices = new glm::vec2 [mVerticesSize];
	mIndices = new unsigned int [mIndicesSize];

	glGenVertexArrays(1, &mVaoGrid);
	glGenBuffers(2, mVboGrid);
	glGenVertexArrays(1, &mVaoFrame);
	glGenBuffers(3, mVboFrame);

	Initialise();
}

Grid::~Grid () {
	glDeleteBuffers(3,mVboFrame);
	glDeleteVertexArrays(1,&mVaoFrame);
	glDeleteBuffers(2,mVboGrid);
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

	//-- Create VAO & VBO for frame to display image
	unsigned int frame_index[] = {
		0, 1, 2, 3
	};
	GLfloat frame_vertex[] = {
		mMin, mMin,
		mMax, mMin,
		mMax, mMax,
		mMin, mMax
	};
	GLfloat frame_uv[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	
	glBindVertexArray(mVaoFrame);

	glBindBuffer(GL_ARRAY_BUFFER, mVboFrame[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frame_vertex), frame_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVboFrame[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frame_uv), frame_uv, GL_STATIC_DRAW);
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboFrame[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(frame_index), frame_index, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	//-- Initialise Grid vertex positions
	float step = (mMax - mMin) / float(mSize);
	glm::vec2 *ptr = mVertices;
	for (int i=0; i<=mSize; i++) {
		for (int j=0; j<=mSize; j++) {
			ptr->x = mMin + step * float(j);	//-- x
			ptr->y = mMin + step * float(i);	//-- y
			ptr++;
		}
	}

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

	//-- Initialise VAO and VBO for Grid
	glBindVertexArray(mVaoGrid);
	glBindBuffer(GL_ARRAY_BUFFER, mVboGrid[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVboGrid[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize * sizeof(unsigned int), mIndices, GL_STATIC_DRAW);
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
}

// color has default value of -1, which means DOES NOT change color
void Grid::Display () {
	// Display grid
	glUniform1i(mUseTextureID,0);
	glUniform3f(mColorID, 1.0f, 1.0f, 1.0f);
	glBindVertexArray(mVaoGrid);
	glDrawElements(GL_LINES, mIndicesSize, GL_UNSIGNED_INT, (void*)0);

	// Display image using texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexImage);
	glUniform1i(mUseTextureID,1);
	glUniform1i(mTextureID,0);
	glBindVertexArray(mVaoFrame);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);
}

float RBF (float const rsq) {
	//return 1.0f / sqrt(1.0f + rsq*10.0f);
	return exp(-rsq*2.0f);
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
	
	// Update the Grid vertex positions
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
	glBindBuffer(GL_ARRAY_BUFFER, mVboGrid[0]);
	glBufferData(GL_ARRAY_BUFFER, mVerticesSize * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);

	/*--- This result is not accurate. The warped grid does not align with the warped image. 
	// Reverse the warp direction
	for (int i=0; i<size; i++) {
		Begin1 = cps[i].End();
		End1 = cps[i].Begin();
		bx(i) = End1.x - Begin1.x;
		by(i) = End1.y - Begin1.y;
		for (int j=0; j<size; j++) {
			Begin2 = cps[j].End();
			Ax(i,j) = Ay(i,j) = RBF(Dsq(Begin1,Begin2));
		}
	}
	Wx = Eigen::FullPivLU<Eigen::MatrixXf>(Ax).solve(bx);
	Wy = Eigen::FullPivLU<Eigen::MatrixXf>(Ay).solve(by);
	----*/

	// Warp the image and recreate the texture
	unsigned char *warp_image = (unsigned char*) malloc(sizeof(char) * 3 * mWidth * mHeight);
	memset(warp_image, 50, (sizeof(char) * 3 * mWidth * mHeight));	// fill with dark grey as background
	
	const unsigned char *ptr_org;
	unsigned char *ptr_warp;
	float step_x, step_y, ndc_x, ndc_y;
	int img_x, img_y;
	step_x = (mMax-mMin) / float(mWidth);
	step_y = (mMax-mMin) / float(mHeight);
	//+++ ptr_org = mImageBuffer;
	ptr_warp = warp_image;
	for (int i=0; i<mHeight; i++) {
		for (int j=0; j<mWidth; j++) {
			ndc_x = float(j) * step_x + mMin;
			ndc_y = float(i) * step_y + mMin;
			for (int k=0; k<size; k++) {
				rbf = RBF(Dsq(glm::vec2(ndc_x,ndc_y),cps[k].Begin()));
				//+++ ndc_x = ndc_x + rbf * Wx[k];
				//+++ ndc_y = ndc_y + rbf * Wy[k];
				ndc_x = ndc_x - rbf * Wx[k];
				ndc_y = ndc_y - rbf * Wy[k];
			}
			img_x = int((ndc_x - mMin) / step_x);
			img_y = int((ndc_y - mMin) / step_y);
			if ((img_x<0)||(img_x>(mWidth-1))||(img_y<0)||(img_y>(mHeight-1))) {
				ptr_warp[0] = ptr_warp[1] = ptr_warp[2] = 50;		// dark grey
				ptr_warp += 3;
			} else {
				//+++ ptr_warp = warp_image + (img_y * mWidth + img_x) * 3;
				ptr_org = mImageBuffer + (img_y * mWidth + img_x) * 3;
				*ptr_warp++ = *ptr_org++;
				*ptr_warp++ = *ptr_org++;
				*ptr_warp++ = *ptr_org++;
			}
		}
	}
	mTexImage = CreateTexture(warp_image, mWidth, mHeight);
	free(warp_image);
}


