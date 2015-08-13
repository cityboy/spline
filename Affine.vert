#version 400

layout(location=0) in vec3 vp;
layout(location=1) in vec2 vUV;
out vec2 UV;

void main () {
	gl_Position = vec4 (vp, 1.0);
	UV = vUV;
}
