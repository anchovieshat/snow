#version 330

in vec3 points;
in int tex_side;
in int tex_idx;

uniform mat4 model;
uniform mat4 pv;

out vec2 f_tex_point;

void main() {
	gl_Position = pv * model * vec4(points, 1.0);

	float x1 = 0.0;
	float x2 = 0.5;
    switch (tex_idx) {
		case 1: {
			x1 = 0.0;
			x2 = 0.5;
		} break;
		case 2: {
			x1 = 0.5;
			x2 = 1.0;
		} break;
	}

	switch (tex_side) {
		case 0: {
			f_tex_point = vec2(x1, 0.0);
		} break;
		case 1: {
			f_tex_point = vec2(x2, 0.0);
		} break;
		case 2: {
			f_tex_point = vec2(x1, 1.0);
		} break;
		case 3: {
			f_tex_point = vec2(x2, 1.0);
		} break;
	}
}
