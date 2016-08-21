#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "gl_helper.h"

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 16
#define CHUNK_DEPTH 16

glm::vec3 cube_edges[] = {
	glm::vec3(-0.0f, -0.0f,  1.0f),
	glm::vec3( 1.0f, -0.0f,  1.0f),
	glm::vec3(-0.0f,  1.0f,  1.0f),
	glm::vec3( 1.0f,  1.0f,  1.0f),
	glm::vec3(-0.0f, -0.0f, -0.0f),
	glm::vec3( 1.0f, -0.0f, -0.0f),
	glm::vec3(-0.0f,  1.0f, -0.0f),
	glm::vec3( 1.0f,  1.0f, -0.0f),
};

u32 mesh_size = 0;

typedef struct Vertex {
	glm::vec3 point;
	u32 t_point;
	u32 tex_id;
} Vertex;

Vertex new_vert(glm::vec3 edge, glm::vec3 offset, u8 tex_id, u8 t_point) {
	Vertex v;
	v.point = edge + offset;
	v.t_point = t_point;
	v.tex_id = tex_id;
	return v;
}

enum {
	SIDE_FRONT  = 0b000001,
	SIDE_BACK   = 0b000010,
	SIDE_TOP    = 0b000100,
	SIDE_BOTTOM = 0b001000,
	SIDE_LEFT   = 0b010000,
	SIDE_RIGHT  = 0b100000,
};

u8 get_air_neighbors(u8 chunk[CHUNK_WIDTH + 2][CHUNK_HEIGHT + 2][CHUNK_DEPTH + 2], u32 x, u32 y, u32 z) {
	u8 neighbors = 0;
	if (chunk[x - 1][y][z] == 0) {
		neighbors |= SIDE_LEFT;
	}
	if (chunk[x + 1][y][z] == 0) {
		neighbors |= SIDE_RIGHT;
	}
	if (chunk[x][y + 1][z] == 0) {
		neighbors |= SIDE_TOP;
	}
	if (chunk[x][y - 1][z] == 0) {
		neighbors |= SIDE_BOTTOM;
	}
	if (chunk[x][y][z + 1] == 0) {
		neighbors |= SIDE_FRONT;
	}
	if (chunk[x][y][z - 1] == 0) {
		neighbors |= SIDE_BACK;
	}

	return neighbors;
}

void add_face(Vertex *mesh, u32 *mesh_size, u8 side, u32 x, u32 y, u32 z, u8 tex_id) {
	glm::vec3 offset = glm::vec3(x, y, z);

	switch (side) {
		case SIDE_TOP: {
			mesh[*mesh_size    ] = new_vert(cube_edges[2], offset, tex_id, 0);
			mesh[*mesh_size + 1] = new_vert(cube_edges[3], offset, tex_id, 1);
			mesh[*mesh_size + 2] = new_vert(cube_edges[7], offset, tex_id, 3);
			mesh[*mesh_size + 3] = new_vert(cube_edges[2], offset, tex_id, 0);
			mesh[*mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3);
			mesh[*mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2);
		} break;
		case SIDE_BOTTOM: {
			mesh[*mesh_size    ] = new_vert(cube_edges[4], offset, tex_id, 0);
			mesh[*mesh_size + 1] = new_vert(cube_edges[5], offset, tex_id, 1);
			mesh[*mesh_size + 2] = new_vert(cube_edges[1], offset, tex_id, 3);
			mesh[*mesh_size + 3] = new_vert(cube_edges[4], offset, tex_id, 0);
			mesh[*mesh_size + 4] = new_vert(cube_edges[1], offset, tex_id, 3);
			mesh[*mesh_size + 5] = new_vert(cube_edges[0], offset, tex_id, 2);
		} break;
		case SIDE_LEFT: {
			mesh[*mesh_size    ] = new_vert(cube_edges[4], offset, tex_id, 0);
			mesh[*mesh_size + 1] = new_vert(cube_edges[0], offset, tex_id, 1);
			mesh[*mesh_size + 2] = new_vert(cube_edges[2], offset, tex_id, 3);
			mesh[*mesh_size + 3] = new_vert(cube_edges[4], offset, tex_id, 0);
			mesh[*mesh_size + 4] = new_vert(cube_edges[2], offset, tex_id, 3);
			mesh[*mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2);
		} break;
		case SIDE_RIGHT: {
			mesh[*mesh_size    ] = new_vert(cube_edges[1], offset, tex_id, 0);
			mesh[*mesh_size + 1] = new_vert(cube_edges[5], offset, tex_id, 1);
			mesh[*mesh_size + 2] = new_vert(cube_edges[7], offset, tex_id, 3);
			mesh[*mesh_size + 3] = new_vert(cube_edges[1], offset, tex_id, 0);
			mesh[*mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3);
			mesh[*mesh_size + 5] = new_vert(cube_edges[3], offset, tex_id, 2);
		} break;
		case SIDE_FRONT: {
			mesh[*mesh_size    ] = new_vert(cube_edges[0], offset, tex_id, 0);
			mesh[*mesh_size + 1] = new_vert(cube_edges[1], offset, tex_id, 1);
			mesh[*mesh_size + 2] = new_vert(cube_edges[3], offset, tex_id, 3);
			mesh[*mesh_size + 3] = new_vert(cube_edges[0], offset, tex_id, 0);
			mesh[*mesh_size + 4] = new_vert(cube_edges[3], offset, tex_id, 3);
			mesh[*mesh_size + 5] = new_vert(cube_edges[2], offset, tex_id, 2);
		} break;
		case SIDE_BACK: {
			mesh[*mesh_size    ] = new_vert(cube_edges[5], offset, tex_id, 0);
			mesh[*mesh_size + 1] = new_vert(cube_edges[4], offset, tex_id, 1);
			mesh[*mesh_size + 2] = new_vert(cube_edges[6], offset, tex_id, 3);
			mesh[*mesh_size + 3] = new_vert(cube_edges[5], offset, tex_id, 0);
			mesh[*mesh_size + 4] = new_vert(cube_edges[6], offset, tex_id, 3);
			mesh[*mesh_size + 5] = new_vert(cube_edges[7], offset, tex_id, 2);
		} break;
	}

	*mesh_size += 6;
}

Vertex *generate_mesh(u8 chunk[CHUNK_WIDTH + 2][CHUNK_HEIGHT + 2][CHUNK_DEPTH + 2]) {
	Vertex *mesh = (Vertex *)malloc((CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2) * 36);
	memset(mesh, 0, (CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2) * 36);

	for (u32 x = 1; x <= CHUNK_WIDTH; ++x) {
		for (u32 y = 1; y <= CHUNK_HEIGHT; ++y) {
			for (u32 z = 1; z <= CHUNK_DEPTH; ++z) {
				if (chunk[x][y][z] != 0) {
					u8 neighbors = get_air_neighbors(chunk, x, y, z);

					if (neighbors & SIDE_TOP) {
						add_face(mesh, &mesh_size, SIDE_TOP, x, y, z, chunk[x][y][z]);
					}
					if (neighbors & SIDE_BOTTOM) {
						add_face(mesh, &mesh_size, SIDE_BOTTOM, x, y, z, chunk[x][y][z]);
					}
					if (neighbors & SIDE_LEFT) {
						add_face(mesh, &mesh_size, SIDE_LEFT, x, y, z, chunk[x][y][z]);
					}
					if (neighbors & SIDE_RIGHT) {
						add_face(mesh, &mesh_size, SIDE_RIGHT, x, y, z, chunk[x][y][z]);
					}
					if (neighbors & SIDE_FRONT) {
						add_face(mesh, &mesh_size, SIDE_FRONT, x, y, z, chunk[x][y][z]);
					}
					if (neighbors & SIDE_BACK) {
						add_face(mesh, &mesh_size, SIDE_BACK, x, y, z, chunk[x][y][z]);
					}
				}
			}
		}
	}

	return mesh;
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	int screen_height = 480;
	int screen_width = 640;

	SDL_Window *window = SDL_CreateWindow("Snow", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_GetDrawableSize(window, &screen_width, &screen_height);

    GLuint obj_shader = load_and_build_program("src/obj_vert.vsh", "src/obj_frag.fsh");

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint v_mesh;
	glGenBuffers(1, &v_mesh);

	GLuint atlas_tex;
	SDL_Surface *atlas_surf = IMG_Load("assets/atlas.png");
	glGenTextures(1, &atlas_tex);
	glBindTexture(GL_TEXTURE_2D, atlas_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_surf->w, atlas_surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_surf->pixels);
	free(atlas_surf);

	GLuint a_points = glGetAttribLocation(obj_shader, "points");
	GLuint a_tex_side = glGetAttribLocation(obj_shader, "tex_side");
	GLuint a_tex_idx = glGetAttribLocation(obj_shader, "tex_idx");

	GLuint u_model = glGetUniformLocation(obj_shader, "model");
	GLuint u_pv = glGetUniformLocation(obj_shader, "pv");
	GLuint u_tex = glGetUniformLocation(obj_shader, "tex");

	glViewport(0, 0, screen_width, screen_height);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);

	glm::vec3 cam_pos = glm::vec3(1.0, 0.0, 10.0);

	u8 chunk[CHUNK_WIDTH + 2][CHUNK_HEIGHT + 2][CHUNK_DEPTH + 2];
	memset(chunk, 0, sizeof(chunk));
	chunk[1][1][1] = 1;
	chunk[1][2][1] = 2;
	chunk[1][1][2] = 1;
	chunk[2][2][1] = 2;
	chunk[2][1][2] = 1;
	chunk[2][2][2] = 2;
	Vertex *mesh = generate_mesh(chunk);

	glBindBuffer(GL_ARRAY_BUFFER, v_mesh);
	glBufferData(GL_ARRAY_BUFFER, mesh_size * sizeof(Vertex), mesh, GL_STATIC_DRAW);

	bool running = true;
	while (running) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_UP: {
							cam_pos.y += 0.1f;
						} break;
						case SDLK_DOWN: {
							cam_pos.y -= 0.1f;
						} break;
						case SDLK_LEFT: {
							cam_pos.x -= 0.1f;
						} break;
						case SDLK_RIGHT: {
							cam_pos.x += 0.1f;
						} break;
						case SDLK_LSHIFT: {
							cam_pos.z -= 0.1f;
						} break;
						case SDLK_SPACE: {
							cam_pos.z += 0.1f;
						} break;
					}
				} break;
				case SDL_QUIT: {
					running = false;
				} break;
			}
		}

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glUseProgram(obj_shader);

		glEnableVertexAttribArray(a_points);
		glEnableVertexAttribArray(a_tex_side);
		glEnableVertexAttribArray(a_tex_idx);

		glBindBuffer(GL_ARRAY_BUFFER, v_mesh);
		glVertexAttribPointer(a_points, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribIPointer(a_tex_side, 1, GL_INT, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, t_point));
		glVertexAttribIPointer(a_tex_idx, 1, GL_INT, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, tex_id));

		glBindTexture(GL_TEXTURE_2D, atlas_tex);
		glActiveTexture(GL_TEXTURE0);


		f32 s_ratio = (f32)screen_width / (f32)screen_height;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), s_ratio, 1.0f, 100.0f);
		glm::mat4 view = glm::lookAt(cam_pos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 pv = projection * view;

		glm::mat4 model = glm::mat4(1.0);

		glUniform1i(u_tex, 0);
		glUniformMatrix4fv(u_pv, 1, GL_FALSE, &pv[0][0]);
		glUniformMatrix4fv(u_model, 1, GL_FALSE, &model[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, mesh_size * sizeof(Vertex));

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(gl_context);
	SDL_Quit();
	return 0;
}
