#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec4 particlecolor;

// Ouput data
out vec4 color;

uniform sampler2D myTextureSampler;

void main(){
	// Output color = color of the texture at the specified UV
	vec3 col=texture( myTextureSampler, UV ).xyz;
	color = vec4(col,0.3);
//	color = vec4(col, 1.0);

}