#version 330

in vec2 f_tex_points;

uniform sampler2D tex;

out vec3 color;

void main() {
	color = texture(tex, f_tex_points).rgb;
}
