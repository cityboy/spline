#version 400

in vec2 UV;
layout(location=0) out vec4 frag_colour;
uniform sampler2D SourceTextureSampler;
uniform sampler2D TargetTextureSampler;
uniform float params[25] = float[](
		5.0f, 5.0f, 		// number of control points in x and Y directions
		0.0f, 0.0f, 0.0f, 	// shift x,y, rotation
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 	// weight for each control point 
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);	// up to 10 control points each direction

uniform int sqdiff = 1;	// switch to turn on/off squared difference

float RBF (float p0, float p1) {
	float d = p0 - p1;
	return exp(-2.0f*d*d);
}

void main () {
	// Contruct 3x3 transformation matrix
	//--mat3 affine = mat3(params[0], params[1], 0.0f, params[2], params[3], 0.0f, params[4], params[5], 1.0f);
	// Transform to new position
	//--mat3 affine = mat3(
	//--	cos(params[2])*params[3], sin(params[2])*params[4], 0.0f, 
	//--	-sin(params[2])*params[3], cos(params[2])*params[4], 0.0f,
	//--	params[0], params[1], 1.0f );
	//--vec3 transformed = affine * vec3(UV, 1.0f);
	vec2 transformed = UV;
	for (int i=0; i< params[0]; i++)
		transformed.x += params[5+i] * RBF(float(i+1)/params[0],UV.x);
	for (int i=0; i< params[1]; i++)
		transformed.y += params[15+i] * RBF(float(i+1)/params[1],UV.y);
	float color;
	// Blank the image if it transforms outside the texture
	if ((transformed.x<0.0f)||(transformed.x>1.0f)||(transformed.y<0.0f)||(transformed.y>1.0f))
		color = 0.0;
	else {
		if (sqdiff!=0) {
			color = texture(SourceTextureSampler,transformed).r - texture(TargetTextureSampler,UV).r;
			color = color * color;
		} else {
			color = texture(SourceTextureSampler,transformed).r;
			//color = texture(TargetTextureSampler,UV).r;
		}
	}
	// Return the squared difference 
	frag_colour = vec4(color,color,color,1.0f);
}
