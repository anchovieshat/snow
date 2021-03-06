#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include "common.h"
#include "gl_helper.h"

#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 128
#define CHUNK_DEPTH 16

#define NUM_X_CHUNKS 13
#define NUM_Z_CHUNKS 13

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

typedef struct KeyHandler {
	bool up;
	bool down;
	bool left;
	bool right;
} KeyHandler;

typedef struct Vertex {
	glm::vec3 point;
	u8 t_point;
	u8 tex_id;
	u8 ao;
} Vertex;

typedef struct Chunk {
	u8 blocks[CHUNK_WIDTH + 2][CHUNK_HEIGHT + 2][CHUNK_DEPTH + 2];

	Vertex *mesh;
	u32 mesh_size;

	u64 x_off;
	u64 z_off;
} Chunk;

Vertex new_vert(glm::vec3 edge, glm::vec3 offset, u8 tex_id, u8 t_point, u8 ao) {
	Vertex v;
	v.point = edge + offset;
	v.t_point = t_point;
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

u16 get_air_neighbors(Chunk *chunk, u32 x, u32 y, u32 z) {
	u16 neighbors = 0;

    if (x == 0 || y == 0 || z == 0 || x > CHUNK_WIDTH || y > CHUNK_HEIGHT || z > CHUNK_DEPTH) {
		return neighbors;
	}

	if (chunk->blocks[x - 1][y][z] == 0) {
		neighbors |= SIDE_LEFT;
	}
	if (chunk->blocks[x + 1][y][z] == 0) {
		neighbors |= SIDE_RIGHT;
	}
	if (chunk->blocks[x][y + 1][z] == 0) {
		neighbors |= SIDE_TOP;
	}
	if (chunk->blocks[x][y - 1][z] == 0) {
		neighbors |= SIDE_BOTTOM;
	}
	if (chunk->blocks[x][y][z + 1] == 0) {
		neighbors |= SIDE_FRONT;
	}
	if (chunk->blocks[x][y][z - 1] == 0) {
		neighbors |= SIDE_BACK;
	}

	if (chunk->blocks[x - 1][y][z - 1] == 0) {
		neighbors |= SIDE_BL_DIAG;
	}
	if (chunk->blocks[x + 1][y][z - 1] == 0) {
		neighbors |= SIDE_BR_DIAG;
	}
	if (chunk->blocks[x - 1][y][z + 1] == 0) {
		neighbors |= SIDE_TL_DIAG;
	}
	if (chunk->blocks[x + 1][y][z + 1] == 0) {
		neighbors |= SIDE_TR_DIAG;
	}

	return neighbors;
}

void add_face(Chunk *chunk, u16 side, u32 x, u32 y, u32 z, u16 neighbors) {
	glm::vec3 offset = glm::vec3(x + (chunk->x_off), y, z + (chunk->z_off));
	u8 tex_id = chunk->blocks[x][y][z];

	u64 mesh_size = chunk->mesh_size;

	u16 g_ao = ~neighbors;
	u8 ao = 255;
	u8 tl = ao;
	u8 tr = ao;
	u8 bl = ao;
	u8 br = ao;
	u8 dark_val = 50;

	switch (side) {
		case SIDE_TOP: {
			if (g_ao & SIDE_FRONT) {
				tl -= dark_val;
				tr -= dark_val;
			}
			if (g_ao & SIDE_BACK) {
				bl -= dark_val;
				br -= dark_val;
			}
			if (g_ao & SIDE_LEFT) {
				bl -= dark_val;
				tl -= dark_val;
			}
			if (g_ao & SIDE_RIGHT) {
				br -= dark_val;
				tr -= dark_val;
			}

			if (g_ao & SIDE_TR_DIAG) {
				tr -= dark_val;
			}
			if (g_ao & SIDE_TL_DIAG) {
				tl -= dark_val;
			}
			if (g_ao & SIDE_BL_DIAG) {
				bl -= dark_val;
			}
			if (g_ao & SIDE_BR_DIAG) {
				br -= dark_val;
			}

			if (tr + bl > br + tl) {
				chunk->mesh[mesh_size    ] = new_vert(cube_edges[2], offset, tex_id, 0, tl);
				chunk->mesh[mesh_size + 1] = new_vert(cube_edges[3], offset, tex_id, 1, tr);
				chunk->mesh[mesh_size + 2] = new_vert(cube_edges[6], offset, tex_id, 2, bl);
				chunk->mesh[mesh_size + 3] = new_vert(cube_edges[3], offset, tex_id, 1, tr);
				chunk->mesh[mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3, br);
				chunk->mesh[mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2, bl);
			} else {
				chunk->mesh[mesh_size    ] = new_vert(cube_edges[2], offset, tex_id, 0, tl);
				chunk->mesh[mesh_size + 1] = new_vert(cube_edges[3], offset, tex_id, 1, tr);
				chunk->mesh[mesh_size + 2] = new_vert(cube_edges[7], offset, tex_id, 3, br);
				chunk->mesh[mesh_size + 3] = new_vert(cube_edges[2], offset, tex_id, 0, tl);
				chunk->mesh[mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3, br);
				chunk->mesh[mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2, bl);
			}

		} break;
		case SIDE_BOTTOM: {
			if (g_ao & SIDE_BACK) {
				tl -= dark_val;
				tr -= dark_val;
			}
			if (g_ao & SIDE_FRONT) {
				bl -= dark_val;
				br -= dark_val;
			}
			if (g_ao & SIDE_LEFT) {
				bl -= dark_val;
				tl -= dark_val;
			}
			if (g_ao & SIDE_RIGHT) {
				br -= dark_val;
				tr -= dark_val;
			}

			chunk->mesh[mesh_size    ] = new_vert(cube_edges[4], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 1] = new_vert(cube_edges[5], offset, tex_id, 1, tr);
			chunk->mesh[mesh_size + 2] = new_vert(cube_edges[1], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 3] = new_vert(cube_edges[4], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 4] = new_vert(cube_edges[1], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 5] = new_vert(cube_edges[0], offset, tex_id, 2, bl);
		} break;
		case SIDE_LEFT: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= dark_val;
				tr -= dark_val;
			}
			if (g_ao & SIDE_TOP) {
				bl -= dark_val;
				br -= dark_val;
			}
			if (g_ao & SIDE_BACK) {
				bl -= dark_val;
				tl -= dark_val;
			}
			if (g_ao & SIDE_FRONT) {
				br -= dark_val;
				tr -= dark_val;
			}

			chunk->mesh[mesh_size    ] = new_vert(cube_edges[4], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 1] = new_vert(cube_edges[0], offset, tex_id, 1, tr);
			chunk->mesh[mesh_size + 2] = new_vert(cube_edges[2], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 3] = new_vert(cube_edges[4], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 4] = new_vert(cube_edges[2], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 5] = new_vert(cube_edges[6], offset, tex_id, 2, bl);
		} break;
		case SIDE_RIGHT: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= dark_val;
				tr -= dark_val;
			}
			if (g_ao & SIDE_TOP) {
				bl -= dark_val;
				br -= dark_val;
			}
			if (g_ao & SIDE_FRONT) {
				bl -= dark_val;
				tl -= dark_val;
			}
			if (g_ao & SIDE_BACK) {
				br -= dark_val;
				tr -= dark_val;
			}
			chunk->mesh[mesh_size    ] = new_vert(cube_edges[1], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 1] = new_vert(cube_edges[5], offset, tex_id, 1, tr);
			chunk->mesh[mesh_size + 2] = new_vert(cube_edges[7], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 3] = new_vert(cube_edges[1], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 4] = new_vert(cube_edges[7], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 5] = new_vert(cube_edges[3], offset, tex_id, 2, bl);
		} break;
		case SIDE_FRONT: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= dark_val;
				tr -= dark_val;
			}
			if (g_ao & SIDE_TOP) {
				bl -= dark_val;
				br -= dark_val;
			}
			if (g_ao & SIDE_LEFT) {
				bl -= dark_val;
				tl -= dark_val;
			}
			if (g_ao & SIDE_RIGHT) {
				br -= dark_val;
				tr -= dark_val;
			}
			chunk->mesh[mesh_size    ] = new_vert(cube_edges[0], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 1] = new_vert(cube_edges[1], offset, tex_id, 1, tr);
			chunk->mesh[mesh_size + 2] = new_vert(cube_edges[3], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 3] = new_vert(cube_edges[0], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 4] = new_vert(cube_edges[3], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 5] = new_vert(cube_edges[2], offset, tex_id, 2, bl);
		} break;
		case SIDE_BACK: {
			if (g_ao & SIDE_BOTTOM) {
				tl -= dark_val;
				tr -= dark_val;
			}
			if (g_ao & SIDE_TOP) {
				bl -= dark_val;
				br -= dark_val;
			}
			if (g_ao & SIDE_RIGHT) {
				bl -= dark_val;
				tl -= dark_val;
			}
			if (g_ao & SIDE_LEFT) {
				br -= dark_val;
				tr -= dark_val;
			}
			chunk->mesh[mesh_size    ] = new_vert(cube_edges[5], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 1] = new_vert(cube_edges[4], offset, tex_id, 1, tr);
			chunk->mesh[mesh_size + 2] = new_vert(cube_edges[6], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 3] = new_vert(cube_edges[5], offset, tex_id, 0, tl);
			chunk->mesh[mesh_size + 4] = new_vert(cube_edges[6], offset, tex_id, 3, br);
			chunk->mesh[mesh_size + 5] = new_vert(cube_edges[7], offset, tex_id, 2, bl);
		} break;
	}

	chunk->mesh_size += 6;
}

Chunk *generate_chunk(u32 x_off, u32 z_off) {
	Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));
	memset(chunk->blocks, 0, sizeof(chunk->blocks));

	chunk->x_off = x_off * (CHUNK_WIDTH);
	chunk->z_off = z_off * (CHUNK_DEPTH);
	chunk->mesh_size = 0;
	chunk->mesh = NULL;

	f32 min_height = CHUNK_HEIGHT / 6;
	f32 avg_height = CHUNK_HEIGHT / 3;

	for (u32 x = 0; x <= CHUNK_WIDTH + 1; ++x) {
		for (u32 z = 0; z <= CHUNK_DEPTH + 1; ++z) {

			f32 column_height = avg_height;
			for (u8 o = 5; o < 8; o++) {
				f32 scale = (f32)(2 << o) * 1.01f;
				column_height += (f32)(o << 3) * stb_perlin_noise3((f32)(x + chunk->x_off) / scale, (f32)(z + chunk->z_off) / scale, o * 2.0f, 256, 256, 256);
			}

			if (column_height > CHUNK_HEIGHT) {
				column_height = CHUNK_HEIGHT;
			}

			if (column_height < min_height) {
				column_height = min_height;
			}

			for (u32 h = min_height - 1; h < column_height; h++) {
				if ((h % 2) == 0) {
					chunk->blocks[x][h][z] = 1;
				} else if ((h % 3) == 0) {
					chunk->blocks[x][h][z] = 2;
				} else {
					chunk->blocks[x][h][z] = 3;
				}
			}
		}
	}

	return chunk;
}

u64 generate_mesh(Chunk **chunks) {
	u64 total_mesh_size = 0;
	u64 face = 0;
	u64 blocks = 0;
	for (u32 c_x = 1; c_x <= NUM_X_CHUNKS; ++c_x) {
		for (u32 c_z = 1; c_z <= NUM_Z_CHUNKS; ++c_z) {
			Chunk *chunk = chunks[COMPRESS_TWO(c_x, c_z, NUM_X_CHUNKS + 2)];
			if (chunk != NULL) {
				if (chunk->mesh == NULL) {
					chunk->mesh = (Vertex *)malloc((CHUNK_WIDTH + 2) * (CHUNK_HEIGHT + 2) * (CHUNK_DEPTH + 2) * sizeof(Vertex) * 36);
				}

				for (u32 x = 1; x <= CHUNK_WIDTH; ++x) {
					for (u32 y = 1; y <= CHUNK_HEIGHT; ++y) {
						for (u32 z = 1; z <= CHUNK_DEPTH; ++z) {
							if (chunk->blocks[x][y][z] != 0) {
								u16 air_neighbors = get_air_neighbors(chunk, x, y, z);

								u64 tmp_face = face;
								if (air_neighbors & SIDE_TOP) {
									u16 ao_neighbors = get_air_neighbors(chunk, x, y + 1, z);
									add_face(chunk, SIDE_TOP, x, y, z, ao_neighbors);
									face += 1;
								}
								if (air_neighbors & SIDE_BOTTOM) {
									u16 ao_neighbors = get_air_neighbors(chunk, x, y - 1, z);
									add_face(chunk, SIDE_BOTTOM, x, y, z, ao_neighbors);
									face += 1;
								}
								if (air_neighbors & SIDE_LEFT) {
									u16 ao_neighbors = get_air_neighbors(chunk, x - 1, y, z);
									add_face(chunk, SIDE_LEFT, x, y, z, ao_neighbors);
									face += 1;
								}
								if (air_neighbors & SIDE_RIGHT) {
									u16 ao_neighbors = get_air_neighbors(chunk, x + 1, y, z);
									add_face(chunk, SIDE_RIGHT, x, y, z, ao_neighbors);
									face += 1;
								}
								if (air_neighbors & SIDE_FRONT) {
									u16 ao_neighbors = get_air_neighbors(chunk, x, y, z + 1);
									add_face(chunk, SIDE_FRONT, x, y, z, ao_neighbors);
									face += 1;
								}
								if (air_neighbors & SIDE_BACK) {
									u16 ao_neighbors = get_air_neighbors(chunk, x, y, z - 1);
									add_face(chunk, SIDE_BACK, x, y, z, ao_neighbors);
									face += 1;
								}

								if (tmp_face != face) {
									blocks += 1;
								}
							}
						}
					}
				}
			}
			total_mesh_size += chunk->mesh_size;
		}
	}

	printf("blocks: %llu\n", blocks);
	printf("faces: %llu\n", face);
	return total_mesh_size;
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
	glBindBuffer(GL_ARRAY_BUFFER, v_mesh);

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

	glActiveTexture(GL_TEXTURE0);

	GLuint a_points = glGetAttribLocation(obj_shader, "points");
	GLuint a_tex_side = glGetAttribLocation(obj_shader, "tex_side");
	GLuint a_tex_idx = glGetAttribLocation(obj_shader, "tex_idx");
	GLuint a_ao = glGetAttribLocation(obj_shader, "ao");

	GLuint u_model = glGetUniformLocation(obj_shader, "model");
	GLuint u_pv = glGetUniformLocation(obj_shader, "pv");
	GLuint u_tex = glGetUniformLocation(obj_shader, "tex");

	glEnableVertexAttribArray(a_points);
	glEnableVertexAttribArray(a_tex_side);
	glEnableVertexAttribArray(a_tex_idx);
	glEnableVertexAttribArray(a_ao);

	glVertexAttribPointer(a_points, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribIPointer(a_tex_side, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, t_point));
	glVertexAttribIPointer(a_tex_idx, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, tex_id));
	glVertexAttribIPointer(a_ao, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void *)STRUCT_OFFSET(Vertex, ao));

	glViewport(0, 0, screen_width, screen_height);
    glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);

	Chunk **chunks = (Chunk **)malloc(sizeof(Chunk *) * (NUM_X_CHUNKS + 2) * (NUM_Z_CHUNKS + 2));
	for (u32 x = 1; x <= NUM_X_CHUNKS; x++) {
		for (u32 z = 1; z <= NUM_Z_CHUNKS; z++) {
			Chunk *chunk = generate_chunk(x - 1, z - 1);
    		chunks[COMPRESS_TWO(x, z, NUM_X_CHUNKS + 2)] = chunk;

		}
	}
	for (u32 x = 0; x <= NUM_X_CHUNKS; x++) {
		for (u32 z = 0; z <= NUM_Z_CHUNKS; z++) {
			if (x == 0 || z == 0) {
				chunks[COMPRESS_TWO(x, z, NUM_X_CHUNKS + 2)] = NULL;
			}
		}
	}

	u64 total_mesh_size = generate_mesh(chunks);

	glBufferData(GL_ARRAY_BUFFER, total_mesh_size * sizeof(Vertex), NULL, GL_STATIC_DRAW);
	u64 mesh_indent = 0;
	for (u32 x = 1; x <= NUM_X_CHUNKS; ++x) {
		for (u32 z = 1; z <= NUM_Z_CHUNKS; ++z) {
			Chunk *chunk = chunks[COMPRESS_TWO(x, z, NUM_X_CHUNKS + 2)];
			u64 mesh_size = chunk->mesh_size;

			glBufferSubData(GL_ARRAY_BUFFER, mesh_indent * sizeof(Vertex), mesh_size * sizeof(Vertex), chunk->mesh);
			mesh_indent += mesh_size;
		}
	}


	f32 current_time = (f32)SDL_GetTicks() / 60.0;

	f64 fps_last_tick = (f64)SDL_GetTicks() / 1000.0;
	u64 frames = 0;

	glm::vec3 cam_pos = glm::vec3(0.0, 50.0, 0.0);
	glm::vec3 cam_front = glm::vec3(0.0, 0.0, 1.0);
	glm::vec3 cam_up = glm::vec3(0.0, 1.0, 0.0);

	f32 yaw = 0.0f;
	f32 pitch = 0.0f;
	f32 cam_speed = 0.75f;

	KeyHandler keyboard;

	bool running = true;
	bool warped = false;
	bool warp = false;
	while (running) {
		SDL_Event event;

		f32 new_time = (f32)SDL_GetTicks() / 60.0;
		f32 dt = new_time - current_time;
		current_time = new_time;

		f64 fps_curr_tick = (f64)SDL_GetTicks() / 1000.0;
		frames++;

		if (fps_curr_tick - fps_last_tick >= 1.0) {
			printf("%f ms/frame\n", 1000.0/(f32)frames);
			frames = 0;
			fps_last_tick += 1.0;
		}

		const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_W]) {
			keyboard.up = true;
		}
		if (state[SDL_SCANCODE_S]) {
			keyboard.down = true;
		}
		if (state[SDL_SCANCODE_A]) {
			keyboard.left = true;
		}
		if (state[SDL_SCANCODE_D]) {
			keyboard.right = true;
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

		if (keyboard.up) {
			cam_pos += cam_speed * cam_front * dt;
		}
		if (keyboard.down) {
			cam_pos -= cam_speed * cam_front * dt;
		}
		if (keyboard.left) {
			cam_pos -= glm::normalize(glm::cross(cam_front, cam_up)) * cam_speed * dt;
		}
		if (keyboard.right) {
			cam_pos += glm::normalize(glm::cross(cam_front, cam_up)) * cam_speed * dt;
		}
		bzero(&keyboard, sizeof(KeyHandler));

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glUseProgram(obj_shader);

		glBindVertexArray(vao);

		f32 s_ratio = (f32)screen_width / (f32)screen_height;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), s_ratio, 1.0f, 500.0f);
		glm::mat4 view = glm::lookAt(cam_pos, cam_pos + cam_front, cam_up);
		glm::mat4 pv = projection * view;

		glm::mat4 model = glm::mat4(1.0);

		glUniform1i(u_tex, 0);
		glUniformMatrix4fv(u_pv, 1, GL_FALSE, &pv[0][0]);
		glUniformMatrix4fv(u_model, 1, GL_FALSE, &model[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, total_mesh_size * sizeof(Vertex));

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(gl_context);
	SDL_Quit();
	return 0;
}
