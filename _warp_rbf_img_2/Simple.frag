#version 400

in vec2 UV;
out vec4 frag_colour;
uniform int useTexture=0;
uniform vec3 color;
uniform sampler2D Texture;

void main () {
	if (useTexture!=0)
		frag_colour = texture(Texture,UV);
	else
		frag_colour = vec4(color,1.0f);
}
