#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <stdlib.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include "common.h"
#include "gl_helper.h"

#define CHUNK_WIDTH 128
#define CHUNK_HEIGHT 128
#define CHUNK_DEPTH 128

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
	u8 t_point;
	u8 tex_id;
	f32 ao;
} Vertex;

Vertex new_vert(glm::vec3 edge, glm::vec3 offset, u8 tex_id, u8 t_point, f32 ao, bool flipped) {
	Vertex v;
	v.point = edge + offset;
	v.t_point = t_point << 1 | !!flipped;
	v.tex_id = tex_id;
	v.ao = ao;

	return v;
}

enum {
	SIDE_FRONT    = 0b0000000001,
	SIDE_BACK     = 0b0000000010,
	SIDE_TOP      = 0b0000000100,
	SIDE_BOTTOM   = 0b0000001000,
	SIDE_LEFT     = 0b0000010000,
	SIDE_RIGHT    = 0b0000100000,
	SIDE_TL_DIAG  = 0b0001000000,
	SIDE_TR_DIAG  = 0b0010000000,
	SIDE_BL_DIAG  = 0b0100000000,
	SIDE_BR_DIAG  = 0b1000000000,
};

u16 get_air_neighbors(u8 *chunk, u32 x, u32 y, u32 z) {
	u16 neighbors = 0;
    if (x == 0 || y == 0 || z == 0 || x > CHUNK_WIDTH || y > CHUNK_HEIGHT || z > CHUNK_DEPTH) {
		return neighbors;
	}

	if (chunk[COMPRESS_THREE(x - 1, y, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_LEFT;
	}
	if (chunk[COMPRESS_THREE(x + 1, y, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_RIGHT;
	}
	if (chunk[COMPRESS_THREE(x, y + 1, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_TOP;
	}
	if (chunk[COMPRESS_THREE(x, y - 1, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_BOTTOM;
	}
	if (chunk[COMPRESS_THREE(x, y, z + 1, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_FRONT;
	}
	if (chunk[COMPRESS_THREE(x, y, z - 1, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_BACK;
	}

	if (chunk[COMPRESS_THREE(x - 1, y, z - 1, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_BL_DIAG;
	}
	if (chunk[COMPRESS_THREE(x + 1, y, z - 1, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_BR_DIAG;
	}
	if (chunk[COMPRESS_THREE(x - 1, y, z + 1, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_TL_DIAG;
	}
	if (chunk[COMPRESS_THREE(x + 1, y, z + 1, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] == 0) {
		neighbors |= SIDE_TR_DIAG;
	}

	return neighbors;
}

void add_face(Vertex *mesh, u32 *mesh_size, u16 side, u32 x, u32 y, u32 z, u8 tex_id, u16 neighbors) {
	glm::vec3 offset = glm::vec3(x, y, z);

	u16 g_ao = ~neighbors;
	f32 ao = 1.0;
	f32 tl = ao;
	f32 tr = ao;
	f32 bl = ao;
	f32 br = ao;

	switch (side) {
		case SIDE_TOP: {
			if (g_ao & SIDE_FRONT) {
				tl -= 0.2f;
				tr -= 0.2f;
			}
			if (g_ao & SIDE_BACK) {
				bl -= 0.2f;
				br -= 0.2f;
			}
			if (g_ao & SIDE_LEFT) {
				bl -= 0.2f;
				tl -= 0.2f;
			}
			if (g_ao & SIDE_RIGHT) {
				br -= 0.2f;
				tr -= 0.2f;
			}

			if (g_ao & SIDE_TR_DIAG) {
				tr -= 0.2f;
			}
			if (g_ao & SIDE_TL_DIAG) {
				tl -= 0.2f;
			}
			if (g_ao & SIDE_BL_DIAG) {
				bl -= 0.2f;
			}
			if (g_ao & SIDE_BR_DIAG) {
				br -= 0.2f;
			}

			if (tr + bl > br + tl) {
				mesh[*mesh_size    ] = new_vert(cube_edges[2], offset, tex_id, 0, tl, true);
				mesh[*mesh_size + 1] = new_vert(cube_edges[3], offset, tex_id, 1, tr, true);
				mesh[*mesh_size + 2] = new_vert(cube_edges[6], offset, tex_id, 3, bl, true);
				mesh[*mesh_size + 3] = new_vert(cube_edges[3], offset, tex_id, 0, tr, true);
				mesh[*mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3, br, true);
				mesh[*mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2, bl, true);
			} else {
				mesh[*mesh_size    ] = new_vert(cube_edges[2], offset, tex_id, 0, tl, false);
				mesh[*mesh_size + 1] = new_vert(cube_edges[3], offset, tex_id, 1, tr, false);
				mesh[*mesh_size + 2] = new_vert(cube_edges[7], offset, tex_id, 3, br, false);
				mesh[*mesh_size + 3] = new_vert(cube_edges[2], offset, tex_id, 0, tl, false);
				mesh[*mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3, br, false);
				mesh[*mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2, bl, false);
			}

		} break;
		case SIDE_BOTTOM: {
			if (g_ao & SIDE_BACK) {
				tl -= 0.2f;
				tr -= 0.2f;
			}
			if (g_ao & SIDE_FRONT) {
				bl -= 0.2f;
				br -= 0.2f;
			}
			if (g_ao & SIDE_LEFT) {
				bl -= 0.2f;
				tl -= 0.2f;
			}
			if (g_ao & SIDE_RIGHT) {
				br -= 0.2f;
				tr -= 0.2f;
			}
			mesh[*mesh_size    ] = new_vert(cube_edges[4], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 1] = new_vert(cube_edges[5], offset, tex_id, 1, tr, false);
			mesh[*mesh_size + 2] = new_vert(cube_edges[1], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 3] = new_vert(cube_edges[4], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 4] = new_vert(cube_edges[1], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 5] = new_vert(cube_edges[0], offset, tex_id, 2, bl, false);
		} break;
		case SIDE_LEFT: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= 0.2f;
				tr -= 0.2f;
			}
			if (g_ao & SIDE_TOP) {
				bl -= 0.2f;
				br -= 0.2f;
			}
			if (g_ao & SIDE_BACK) {
				bl -= 0.2f;
				tl -= 0.2f;
			}
			if (g_ao & SIDE_FRONT) {
				br -= 0.2f;
				tr -= 0.2f;
			}

			mesh[*mesh_size    ] = new_vert(cube_edges[4], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 1] = new_vert(cube_edges[0], offset, tex_id, 1, tr, false);
			mesh[*mesh_size + 2] = new_vert(cube_edges[2], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 3] = new_vert(cube_edges[4], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 4] = new_vert(cube_edges[2], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2, bl, false);
		} break;
		case SIDE_RIGHT: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= 0.2f;
				tr -= 0.2f;
			}
			if (g_ao & SIDE_TOP) {
				bl -= 0.2f;
				br -= 0.2f;
			}
			if (g_ao & SIDE_FRONT) {
				bl -= 0.2f;
				tl -= 0.2f;
			}
			if (g_ao & SIDE_BACK) {
				br -= 0.2f;
				tr -= 0.2f;
			}
			mesh[*mesh_size    ] = new_vert(cube_edges[1], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 1] = new_vert(cube_edges[5], offset, tex_id, 1, tr, false);
			mesh[*mesh_size + 2] = new_vert(cube_edges[7], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 3] = new_vert(cube_edges[1], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 5] = new_vert(cube_edges[3], offset, tex_id, 2, bl, false);
		} break;
		case SIDE_FRONT: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= 0.2f;
				tr -= 0.2f;
			}
			if (g_ao & SIDE_TOP) {
				bl -= 0.2f;
				br -= 0.2f;
			}
			if (g_ao & SIDE_LEFT) {
				bl -= 0.2f;
				tl -= 0.2f;
			}
			if (g_ao & SIDE_RIGHT) {
				br -= 0.2f;
				tr -= 0.2f;
			}
			mesh[*mesh_size    ] = new_vert(cube_edges[0], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 1] = new_vert(cube_edges[1], offset, tex_id, 1, tr, false);
			mesh[*mesh_size + 2] = new_vert(cube_edges[3], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 3] = new_vert(cube_edges[0], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 4] = new_vert(cube_edges[3], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 5] = new_vert(cube_edges[2], offset, tex_id, 2, bl, false);
		} break;
		case SIDE_BACK: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= 0.2f;
				tr -= 0.2f;
			}
			if (g_ao & SIDE_TOP) {
				bl -= 0.2f;
				br -= 0.2f;
			}
			if (g_ao & SIDE_RIGHT) {
				bl -= 0.2f;
				tl -= 0.2f;
			}
			if (g_ao & SIDE_LEFT) {
				br -= 0.2f;
				tr -= 0.2f;
			}
			mesh[*mesh_size    ] = new_vert(cube_edges[5], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 1] = new_vert(cube_edges[4], offset, tex_id, 1, tr, false);
			mesh[*mesh_size + 2] = new_vert(cube_edges[6], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 3] = new_vert(cube_edges[5], offset, tex_id, 0, tl, false);
			mesh[*mesh_size + 4] = new_vert(cube_edges[6], offset, tex_id, 3, br, false);
			mesh[*mesh_size + 5] = new_vert(cube_edges[7], offset, tex_id, 2, bl, false);
		} break;
	}

	*mesh_size += 6;
}

u8 *generate_chunk(u32 x_off, u32 z_off) {
	u8 *chunk = (u8 *)malloc((CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2));
	memset(chunk, 0, (CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2));

	f32 min_height = CHUNK_HEIGHT / 6;
	f32 avg_height = CHUNK_HEIGHT / 3;

	for (u32 x = 1; x < CHUNK_WIDTH; ++x) {
		for (u32 z = 1; z < CHUNK_DEPTH; ++z) {

			f32 column_height = avg_height;
			for (u8 o = 5; o < 8; o++) {
				f32 scale = (f32)(2 << o) * 1.01f;
				column_height += (f32)(o << 4) * stb_perlin_noise3((f32)(x + x_off) / scale, (f32)(z + z_off) / scale, o * 2.0f, 256, 256, 256);
			}

			if (column_height > CHUNK_HEIGHT) {
				column_height = CHUNK_HEIGHT;
			}

			if (column_height < min_height) {
				column_height = min_height;
			}

			for (u32 h = min_height - 1; h < column_height; h++) {
				if (h  > ((f32)CHUNK_HEIGHT * (0.66f))) {
					chunk[COMPRESS_THREE(x, h, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] = 3;
				} else if (h  > avg_height) {
					chunk[COMPRESS_THREE(x, h, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] = 1;
				} else {
					chunk[COMPRESS_THREE(x, h, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2)] = 2;
				}
			}
		}
	}

	return chunk;
}

Vertex *generate_mesh(u8 *chunk) {
	Vertex *mesh = (Vertex *)malloc((CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2) * sizeof(Vertex) * 36);
	memset(mesh, 0, (CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2) * 36);

	for (u32 x = 1; x < CHUNK_WIDTH; ++x) {
		for (u32 y = 1; y < CHUNK_HEIGHT; ++y) {
			for (u32 z = 1; z < CHUNK_DEPTH; ++z) {
				u64 id = COMPRESS_THREE(x, y, z, CHUNK_WIDTH + 2, CHUNK_HEIGHT + 2);
				if (chunk[id] != 0) {
					u16 air_neighbors = get_air_neighbors(chunk, x, y, z);

					if (air_neighbors & SIDE_TOP) {
						u16 ao_neighbors = get_air_neighbors(chunk, x, y + 1, z);
						add_face(mesh, &mesh_size, SIDE_TOP, x, y, z, chunk[id], ao_neighbors);
					}
					if (air_neighbors & SIDE_BOTTOM) {
						u16 ao_neighbors = get_air_neighbors(chunk, x, y - 1, z);
						add_face(mesh, &mesh_size, SIDE_BOTTOM, x, y, z, chunk[id], ao_neighbors);
					}
					if (air_neighbors & SIDE_LEFT) {
						u16 ao_neighbors = get_air_neighbors(chunk, x - 1, y, z);
						add_face(mesh, &mesh_size, SIDE_LEFT, x, y, z, chunk[id], ao_neighbors);
					}
					if (air_neighbors & SIDE_RIGHT) {
						u16 ao_neighbors = get_air_neighbors(chunk, x + 1, y, z);
						add_face(mesh, &mesh_size, SIDE_RIGHT, x, y, z, chunk[id], ao_neighbors);
					}
					if (air_neighbors & SIDE_FRONT) {
						u16 ao_neighbors = get_air_neighbors(chunk, x, y, z + 1);
						add_face(mesh, &mesh_size, SIDE_FRONT, x, y, z, chunk[id], ao_neighbors);
					}
					if (air_neighbors & SIDE_BACK) {
						u16 ao_neighbors = get_air_neighbors(chunk, x, y, z - 1);
						add_face(mesh, &mesh_size, SIDE_BACK, x, y, z, chunk[id], ao_neighbors);
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
	GLuint a_ao = glGetAttribLocation(obj_shader, "ao");

	GLuint u_model = glGetUniformLocation(obj_shader, "model");
	GLuint u_pv = glGetUniformLocation(obj_shader, "pv");
	GLuint u_tex = glGetUniformLocation(obj_shader, "tex");

	glViewport(0, 0, screen_width, screen_height);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);
	//glClearColor(0.2, 0.8, 1.0, 1.0);


    u8 *chunk = generate_chunk(0, 0);
	Vertex *mesh = generate_mesh(chunk);

	glBindBuffer(GL_ARRAY_BUFFER, v_mesh);
	glBufferData(GL_ARRAY_BUFFER, mesh_size * sizeof(Vertex), mesh, GL_STATIC_DRAW);

	f32 current_time = (f32)SDL_GetTicks() / 60.0;
	f32 t = 0.0;

	glm::vec3 cam_pos = glm::vec3(0.0, 50.0, 0.0);
	glm::vec3 cam_front = glm::vec3(0.0, 0.0, 1.0);
	glm::vec3 cam_up = glm::vec3(0.0, 1.0, 0.0);

	f32 yaw = 0.0f;
	f32 pitch = 0.0f;
	f32 cam_speed = 0.75f;

	bool running = true;
	bool warped = false;
	bool warp = false;
	while (running) {
		SDL_Event event;

		f32 new_time = (f32)SDL_GetTicks() / 60.0;
		f32 dt = new_time - current_time;
		current_time = new_time;
		t += dt;

		SDL_PumpEvents();
        const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_W]) {
			cam_pos += cam_speed * cam_front * dt;
		}
		if (state[SDL_SCANCODE_S]) {
			cam_pos -= cam_speed * cam_front * dt;
		}
		if (state[SDL_SCANCODE_A]) {
			cam_pos -= glm::normalize(glm::cross(cam_front, cam_up)) * cam_speed * dt;
		}
		if (state[SDL_SCANCODE_D]) {
			cam_pos += glm::normalize(glm::cross(cam_front, cam_up)) * cam_speed * dt;
		}

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE: {
							warp = false;
							SDL_SetRelativeMouseMode(SDL_FALSE);
						} break;
					}
				} break;
				case SDL_MOUSEMOTION: {
					if (!warped) {
						i32 mouse_x, mouse_y;
						SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
						if (warp) {
							SDL_WarpMouseInWindow(window, screen_width / 2, screen_height / 2);
							warped = true;
						} else {
							continue;
						}

						f32 x_off = mouse_x;
						f32 y_off = -mouse_y;

						f32 mouse_speed = 0.20;
						x_off *= mouse_speed;
						y_off *= mouse_speed;

						yaw += x_off;
						pitch += y_off;

						if (pitch > 89.0f) {
							pitch = 89.0f;
						} else if (pitch < -89.f) {
							pitch = -89.0f;
						}

						cam_front = glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw)) * cos(glm::radians(pitch)));

					} else {
						warped = false;
					}
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					SDL_SetRelativeMouseMode(SDL_TRUE);
					warp = true;
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
		glEnableVertexAttribArray(a_ao);

		glBindBuffer(GL_ARRAY_BUFFER, v_mesh);
		glVertexAttribPointer(a_points, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribIPointer(a_tex_side, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, t_point));
		glVertexAttribIPointer(a_tex_idx, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, tex_id));
		glVertexAttribPointer(a_ao, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, ao));

		glBindTexture(GL_TEXTURE_2D, atlas_tex);
		glActiveTexture(GL_TEXTURE0);

		f32 s_ratio = (f32)screen_width / (f32)screen_height;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), s_ratio, 1.0f, 500.0f);
		glm::mat4 view = glm::lookAt(cam_pos, cam_pos + cam_front, cam_up);
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
