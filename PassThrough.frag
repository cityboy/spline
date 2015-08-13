#version 400

in vec2 UV;
out vec4 frag_colour;
uniform sampler2D TextureSampler;

void main () {
float color = texture(TextureSampler,UV).r;
	frag_colour = vec4(color,color,color,1.0f);
}
