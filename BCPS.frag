#version 400

in vec2 UV;
layout(location=0) out vec4 frag_colour;
uniform sampler2D SourceTextureSampler;
uniform sampler2D TargetTextureSampler;
// The texture space is rescaled to (-1,+1) 
// 9 control points are fixed at -0.5, 0.0, +0.5 horizontal and vertical
vec2 knots[9] = vec2[](
	vec2(-0.5f,-0.5f), vec2( 0.0f,-0.5f), vec2( 0.5f,-0.5f),
	vec2(-0.5f, 0.0f), vec2( 0.0f, 0.0f), vec2( 0.5f, 0.0f),
	vec2(-0.5f, 0.5f), vec2( 0.0f, 0.5f), vec2( 0.5f, 0.5f));
// The weight of the control points are provided by the host.
uniform vec2 weights[9] = vec2[](
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f),
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f),
	vec2(0.0f,0.0f), vec2(0.0f,0.0f), vec2(0.0f,0.0f));
// Turn on/off calculation of squared difference.
// It is used to produce the final transformed image. 
uniform int sqdiff = 1;	

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
	// Scale the texture coordinates to (-1,1)
	vec2 transformed = UV*2.0f - vec2(1.0f,1.0f);
	// Deform if inside the unit circle
	if (length(transformed)<1.0f) {
		vec2 point = transformed;
		float g;
		for (int i=0; i<9; i++) {
			g = G(point, knots[i]);
			transformed = transformed + weights[i] * g;
		}
		// Scale the texture coordinates back to (0,1)
		transformed = (transformed + vec2(1.0f,1.0f)) * 0.5f;
	} else
		transformed = UV;

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
