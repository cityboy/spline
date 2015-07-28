//----
// ControlPoint.cpp
//----

#include "ControlPoint.hpp"
#include <stdio.h>

#define SIZE 0.05f

ControlPoint::ControlPoint (float x, float y) {
	mBeginPos.x = x;
	mBeginPos.y = y;
	mEndPos.x = -1.0f;
	mEndPos.y = -1.0f;
	mEnded = false;
}

ControlPoint::~ControlPoint () {
	
}

void ControlPoint::SetEnd (float x, float y) {
	mEndPos.x = x;
	mEndPos.y = y;
	mEnded = true;
}

bool ControlPoint::Ended () const {
	return mEnded;
}

glm::vec2 ControlPoint::Begin () const {
	return mBeginPos;
}

glm::vec2 ControlPoint::End () const {
	return mEndPos;
}

