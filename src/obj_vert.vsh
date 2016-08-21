#version 330

in vec3 points;
in int tex_side;

uniform mat4 model;
uniform mat4 pv;

out vec2 f_tex_points;

void main() {
	gl_Position = pv * model * vec4(points, 1.0);
	f_tex_points = vec2(0.0, 0.0);
}
