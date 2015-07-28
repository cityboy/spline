//----
// ControlPoint.cpp
//----

#include "ControlPoint.hpp"
#include <stdio.h>

#define SIZE 0.05f

ControlPoint::ControlPoint (float x, float y) {
	mBeginPos.x = x;
	mBeginPos.y = y;
	mEnded = false;
}

ControlPoint::~ControlPoint () {
	
}

void ControlPoint::SetEnd (float x, float y) {
	mEndPos.x = x;
	mEndPos.y = y;
	mEnded = true;
	printf("[%6.3f,%6.3f] [%6.3f,%6.3f]\n",mBeginPos.x,mBeginPos.y,mEndPos.x,mEndPos.y);
}

bool ControlPoint::Ended () const {
	return mEnded;
}

glm::vec2 ControlPoint::Begin () const {
	return mBeginPos;
}

glm::vec2 ControlPoint::End () const {
	printf("[%6.3f,%6.3f] [%6.3f,%6.3f]\n",mBeginPos.x,mBeginPos.y,mEndPos.x,mEndPos.y);
	return mEndPos;
}

void ControlPoint::Display () {

}

