//----
// ControlPoint.hpp
//----

#ifndef CONTROLPOINT_HPP
#define CONTROLPOINT_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>

class ControlPoint {
public:
	ControlPoint (float x, float y);
	~ControlPoint ();
	void SetEnd (float x, float y);
	bool Ended () const;
	glm::vec2 Begin () const;
	glm::vec2 End () const;
	void Display ();
	
private:
	glm::vec2 mBeginPos, mEndPos;
	bool mEnded;
//	glm::vec2 mVertices[8];
//	GLuint mVbo;	
};

#endif
