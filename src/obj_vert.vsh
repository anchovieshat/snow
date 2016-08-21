#version 330

in vec3 points;
in int tex_side;
in int tex_idx;

uniform mat4 model;
uniform mat4 pv;

out vec2 f_tex_point;

void main() {
	gl_Position = pv * model * vec4(points, 1.0);

	float scalar = 0.5f;
	float y = (tex_idx % 2) * scalar;
	float x = (tex_idx / 2) * scalar;

	switch (tex_side) {
		case 0: {
			f_tex_point = vec2(x, y);
		} break;
		case 1: {
			f_tex_point = vec2(x + scalar, y);
		} break;
		case 2: {
			f_tex_point = vec2(x, y + scalar);
		} break;
		case 3: {
			f_tex_point = vec2(x + scalar, y + scalar);
		} break;
	}
}
