#version 330

in vec3 points;
in uint tex_side;
in uint tex_idx;
in float ao;

uniform mat4 model;
uniform mat4 pv;

out vec2 f_tex_point;
out float f_ao;
out vec4 f_pos;

void main() {
	gl_Position = pv * model * vec4(points, 1.0);

	f_pos = pv * vec4(points, 1.0);

	float scalar = 0.5f;
	float y = ((tex_idx - 1u) % 2u) * scalar;
	float x = ((tex_idx - 1u) / 2u) * scalar;

	uint v_tex_side = tex_side;

	switch (v_tex_side) {
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

	f_ao = ao;
}
