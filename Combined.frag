#version 400

in vec2 UV;
layout(location=0) out vec4 frag_colour;
uniform sampler2D SourceTextureSampler;
uniform sampler2D TargetTextureSampler;
// Parameters for rigid transformation
uniform float params[5] = float[](0.0f, 0.0f, 0.0f, 1.0f, 1.0f);  // shift x,y, rotation, scale x,y
// 16 control points are fixed at 0.2, 0.4, 0.6, 0.8 horizontal and vertical
vec2 knots[16] = vec2[](
	vec2(0.2f,0.2f), vec2(0.4f,0.2f), vec2(0.6f,0.2f), vec2(0.8f,0.2f),
	vec2(0.2f,0.4f), vec2(0.4f,0.4f), vec2(0.6f,0.4f), vec2(0.8f,0.4f),
	vec2(0.2f,0.6f), vec2(0.4f,0.6f), vec2(0.6f,0.6f), vec2(0.8f,0.6f),
	vec2(0.2f,0.8f), vec2(0.4f,0.8f), vec2(0.6f,0.8f), vec2(0.8f,0.8f));
// The weight of the control points are provided by the host.
uniform vec2 weights[16] = vec2[](
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f),
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f),
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f),
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f));
// Turn on/off calculation of squared difference.
// It is used to produce the final transformed image. 
uniform int affine = 0;	
uniform int bcps = 0;	
uniform int sqdiff = 1;	

float RBF (vec2 p0, vec2 p1) {
	vec2 d = p0 - p1;
	return exp(-2.0f*(d.x*d.x+d.y*d.y));
}

float A (vec2 p1, vec2 p2) {
//	return (sqrt( (p1.x*p1.x+p1.y*p1.y)*(p2.x*p2.x+p2.y*p2.y) - 2*(p1.x*p2.x+p1.y*p2.y) + 1) / glm::length(p1-p2));
	float l1 = length(p1);
	float l2 = length(p2);
	float l12 = length(p1-p2);
	float d12 = dot(p1,p2);
	return ( sqrt(l1*l1*l2*l2 - 2*d12 + 1) / l12 );
}

float G (vec2 p1, vec2 p2) {
	float a = A(p1, p2);
	float l = length(p1-p2);
	return ( l*l * ((a*a-1) - log(a*a)) );
}

void main () {
	vec2 transformed = UV;
	if (affine!=0) {
		mat3 affine = mat3(
			cos(params[2])*params[3], sin(params[2])*params[4], 0.0f, 
			-sin(params[2])*params[3], cos(params[2])*params[4], 0.0f,
			params[0], params[1], 1.0f );
		vec3 temp = affine * vec3(UV, 1.0f);
		transformed = temp.xy;
	}
	if (bcps!=0) {
		vec2 point = transformed;
		float rbf;
		for (int i=0; i<16; i++) {
			rbf = RBF(point, knots[i]);
			transformed = transformed + weights[i] * rbf;
		}
	}

	float color;
	// Blank the image if it transforms outside the texture
	if ((transformed.x<0.0f)||(transformed.x>1.0f)||(transformed.y<0.0f)||(transformed.y>1.0f))
		color = 0.0;
	else {
		if (sqdiff!=0) {
			color = texture(SourceTextureSampler,transformed.xy).r - texture(TargetTextureSampler,UV).r;
			color = color * color;
		} else {
			color = texture(SourceTextureSampler,transformed.xy).r;
			//color = texture(TargetTextureSampler,UV).r;
		}
	}
	// Return the squared difference 
	frag_colour = vec4(color,color,color,1.0f);
}
