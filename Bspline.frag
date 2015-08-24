#version 400

in vec2 UV;
layout(location=0) out vec4 frag_colour;
uniform sampler2D SourceTextureSampler;
uniform sampler2D TargetTextureSampler;

// The number of knots includes those on the edges of the image. 
// numknots=4 divides the image into 3 sections. 
// There are 4 knots within the image area PLUS 2 knots outside the image.
uniform int numknots[2] = int[](4,4);
// 100 knots positions are allocated but not all are used. 
// numknots=4 in both directions uses 6x6 (i.e. 36) positions. 
uniform vec2 knots[225];
// Turn on/off calculation of squared difference.
// It is used to produce the final transformed image. 
uniform int sqdiff = 1;	

// Basis functions
float B0 (float s) {
	return ((1.0f-s)*(1.0f-s)*(1.0f-s)/6.0f);
}
float B1 (float s) {
	return ((3.0f*s*s*s - 6.0f*s*s + 4.0f)/6.0f);
}
float B2 (float s) {
	return ((-3.0f*s*s*s + 3.0f*s*s + 3.0f*s + 1)/6.0f);
}
float B3 (float s) {
	return (s*s*s/6.0f);
}

// Calculate embedding and index of embedding
// Note: index of embedding starts from 0, which is different from the paper
// i.e. Lower left corner of the selection area is (1,1). Upper right corner is (NUM_KNOTS+1,NUM_KNOTS+1)
void Embedding (in vec2 pt, out int i, out int j, out float s, out float t) {
	float step_x = 1 / float(numknots[0]);
	float step_y = 1 / float(numknots[1]);
	float ss = pt.x / step_x;
	float tt = pt.y / step_y;
	i = int(ss) + 1;
	j = int(tt) + 1;
	s = ss - float(i-1);
	t = tt - float(j-1);
}

void main () {

	vec2 transformed = UV;

	int i, j;
	float s, t;
	float bs[4], bt[4];

	Embedding (UV, i, j, s, t);
	bs[0] = B0(s); bs[1] = B1(s); bs[2] = B2(s); bs[3] = B3(s);
	bt[0] = B0(t); bt[1] = B1(t); bt[2] = B2(t); bt[3] = B3(t);
	for (int l=0; l<=3; l++) {
		for (int k=0; k<=3; k++) {
			transformed = transformed + knots[(i+k-1)*(numknots[0]+2)+j+l-1] * bs[k]*bt[l];
		}
	}


	float color;
	// Blank the image if it transforms outside the texture
	if ((transformed.x<0.0f)||(transformed.x>1.0f)||(transformed.y<0.0f)||(transformed.y>1.0f))
		color = 0.2f;
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
