#version 330

in vec2 f_tex_point;
in vec4 f_pos;
in float f_ao;

uniform sampler2D tex;

out vec3 color;

void main() {
	color = texture(tex, f_tex_point).rgb * f_ao;
}
