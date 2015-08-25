#version 400

// Simple pass-through vertex shader
// Pass over the vertex coordinates and optional texture coordinates

layout(location=0) in vec3 vP;
layout(location=1) in vec2 vT;
out vec2 UV;

void main () {
	gl_Position = vec4 (vP, 1.0);
	UV = vT;
}
