#version 400

in vec2 UV;
layout(location=0) out vec4 frag_colour;
uniform sampler2D SourceTextureSampler;
uniform sampler2D TargetTextureSampler;
// Transformation parameters. Default = identity
uniform float params[6] = float[](1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);  // 2x3 matrix, column first
uniform int sqdiff = 1;	// switch to turn on/off squared difference

void main () {
	// Contruct 3x3 transformation matrix
	mat3 affine = mat3(params[0], params[1], 0.0f, params[2], params[3], 0.0f, params[4], params[5], 1.0f);
	// Transform to new position
	vec3 transformed = affine * vec3(UV, 1.0f);
	float color;
	// Blank the image if it transforms outside the texture
	if ((transformed.x<0.0f)||(transformed.x>1.0f)||(transformed.y<0.0f)||(transformed.y>1.0f))
		color = 0.0f;
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
	frag_colour = vec4(color,0.0f,0.0f,1.0f);
}
