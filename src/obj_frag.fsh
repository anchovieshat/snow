#version 330

in vec2 f_tex_point;

uniform sampler2D tex;

out vec3 color;

vec3 gamma(vec3 color) {
	return pow(color, vec3(1.0/2.0));
}

void main() {
	color = texture(tex, f_tex_point).rgb;
}
