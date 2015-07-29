//----
// ControlPoint.cpp
//----

#include "ControlPoint.hpp"
#include <stdio.h>

#define SIZE 0.01f

ControlPoint::ControlPoint (float x, float y) {
	// Set begining position
	mVertices[8].x = x;
	mVertices[8].y = y;
	mEnded = false;
	// Calculate vertices
	mVertices[0].x = x - SIZE;	mVertices[0].y = y - SIZE;
	mVertices[1].x = x + SIZE;	mVertices[1].y = y - SIZE;
	mVertices[2].x = x + SIZE;	mVertices[2].y = y + SIZE;
	mVertices[3].x = x - SIZE;	mVertices[3].y = y + SIZE;
	// Set VAO & VBO
	glGenVertexArrays(1, &mVao);
	glBindVertexArray(mVao);
	glEnableVertexAttribArray(0);
	glGenBuffers(1, &mVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
}

ControlPoint::~ControlPoint () {
	
}

void ControlPoint::SetEnd (float x, float y) {
	mVertices[9].x = x;
	mVertices[9].y = y;
	mEnded = true;
	// Calculate vertices
	mVertices[4].x = x - SIZE;	mVertices[4].y = y - SIZE;
	mVertices[5].x = x + SIZE;	mVertices[5].y = y - SIZE;
	mVertices[6].x = x + SIZE;	mVertices[6].y = y + SIZE;
	mVertices[7].x = x - SIZE;	mVertices[7].y = y + SIZE;
	// Update vertices buffer
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, 10 * sizeof(glm::vec2), mVertices, GL_STATIC_DRAW);
}

bool ControlPoint::Ended () const {
	return mEnded;
}

glm::vec2 ControlPoint::Begin () const {
	return mVertices[8];
}

glm::vec2 ControlPoint::End () const {
	return mVertices[9];
}

// color has default value of -1, which means DOES NOT change color
void ControlPoint::Display (int color) {
	if (color>=0)
		glUniform3f(color, 1.0f, 1.0f, 0.0f);
	glBindVertexArray(mVao);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
	if (mEnded) {
		if (color>=0)
			glUniform3f(color, 0.5f, 0.5f, 0.0f);
		glDrawArrays(GL_TRIANGLE_FAN,4,4);
		if (color>=0)
			glUniform3f(color, 0.7f, 0.7f, 0.7f);
		glDrawArrays(GL_LINES,8,2);
	}
}

