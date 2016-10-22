#version 130

in vec2 UV;

out vec3 color;

uniform sampler2D myTextureSampler;

void main(){
	color = texture2D( myTextureSampler, UV ).rgb;
}
