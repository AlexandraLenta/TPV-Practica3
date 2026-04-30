// This file is part of the course TPV2@UCM - Samir Genaim

/*
 * The content of this class is based on https://glouw.com/2018/03/11/littlewolf.html
 */

#pragma once

#include <SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include "../utils/Vector2D.h"


#include "../sdlutils/InputHandler.h"

class LittleWolf {
public:

	// a point in a 2D-plane
	struct Point {
		float x;
		float y;

		Point(float x = 0, float y = 0) : x(x), y(y) {

		}

		// construct from Vector2D
		Point(Vector2D v) {
			x = v.getX();
			y = v.getY();
		}
	};

	// Used to represent a position where a ray hits, and which tile
	struct Hit {
		int tile;
		Point where;
	};

	// a line between two points
	struct Line {
		Point a;
		Point b;
	};

	// The information on the window/renderer, the size of the window, and a texture
	// that we use to draw the walls, etc.
	struct Gpu {
		SDL_Window* window;
		SDL_Renderer* renderer;
		SDL_Texture* texture;
		int xres;
		int yres;
	};

	// when we lock a texture, we get an array of pixels where we can draw
	struct Display {
		Uint32* pixels;
		int width;
		int pitch;
	};

	// representing a (project) wall
	struct Wall {
		int top;
		int bot;
		float size;
	};

	// the status of a player
	enum PlayerState : Uint8 {
		NOT_USED, ALIVE, DEAD
	};

	enum SFX {
		GUNSHOT, PAIN
	};

	// player information
	struct Player {
		Uint8 id;            // the id
		Line fov;            // focal view
		Point where;         // current position (wrt. to the grid)
		Point velocity;      // current velocity
		float speed;         // maximum speed
		float acceleration;  // acceleration
		float theta;         // rotation (in rad)
		int hp = 100;		 // health points
		int score = 0;		 // score points
		char name[11];
		PlayerState state;   // the state
	};

	// Representing a map, the user_walling is the walling provided by the user, and
	// walling is a scaled up version
	struct Map {
		Uint8** user_walling;
		Uint8** walling;

		Uint16 user_walling_width;
		Uint16 user_walling_height;
		Uint16 walling_width;
		Uint16 walling_height;

		Map() {
			user_walling = walling = nullptr;
			user_walling_width = user_walling_height = walling_width =
				walling_height = 0u;
		}

		~Map() {
			if (user_walling != nullptr) {
				for (auto i = 0u; i < user_walling_height; i++) {
					delete[] user_walling[i];
				}
				delete[] user_walling;
			}
			if (walling != nullptr) {
				for (auto i = 0u; i < walling_height; i++) {
					delete[] walling[i];
				}
				delete[] walling;
			}
		}
	};

	LittleWolf();
	virtual ~LittleWolf();

	// load a map from a file
	void load(std::string filename);

	// add a new player with identifier <id>, returns false if the id is already occupied
	bool addPlayer(Uint8 id, std::string name);

	// initialize the SDL window information
	void init(SDL_Window* window, SDL_Renderer* render);

	// render the walls, etc
	void render();

	// update the world, etc
	void update();

	Player* getPlayers() {
		return _players;
	}

	int getMaxPlayers() {
		return _max_player;
	}

	void send_my_info();
	void removePlayer(Uint8 id);	
	void killPlayer(Uint8 id);
	void update_player_state(Uint8 id, float x, float y, float rot);
	void update_player_info(Uint8 id, float x, float y, float rot, int hp, int score, Uint8 state);
	void update_player_hp(Uint8 id, int hp);
	void update_player_score(Uint8 id, int score);
	void update_player_name(Uint8 id, std::string name);

	void play_shootSFX(Uint8 id, SFX sound);
	int shoot(Uint8 id);

	bool is_dead(Uint8 id);

	void restart();
	void check_restart();

	// stop movement and register reset time. only the master will use it for calculations, the rest use it for display
	void triggerRestart();

	int get_xres() {
		return _xres;
	}

	int get_yres() {
		return _yres;
	}

private:
	bool _canMove = true;
	float _resetTime = 5000; // 5 seconds to reset

	void resetPlayer(Uint8 id, std::string name);
	bool can_spawn_in_pos(int row, int col);

	// remove health and check for death
	void damage_player(Uint8 shooterId, Uint8 victimId);
	
	char* _username;

	//// mark all (used) player alive
	//void bringAllToLife();

	//// switch to the view of the next player
	//void switchToNextPlayer();

	// mute/unmute sound
	void muteSound();

	// Calculates wall size using the <corrected> ray to the wall.
	Wall project(const int xres, const int yres, const float focal,
		const Point corrected);

	// Casts a ray from <where> in unit <direction> until a <walling> tile is hit.
	Hit cast(const Point where, Point direction, Uint8** walling,
		bool ignore_players, bool ignore_deads);

	void shootNetwork(Uint8 id);

	// Spins the player when keys grid_h,l are held down. When left-shit is held down the move is slower
	inline void spin(Player& p);

	// Moves the player when w,a,s,d are held down. Handles collision detection for the walls.
	void move(Player& p);

	// Renders the entire scene from the <current player> perspective given a <map> and a software <gpu>.
	void render_map(Player& p);

	// Renders the entire scene from the <current player> perspective given a <map> and a software <gpu>.
	void render_upper_view();

	// Render a list of current player
	void render_players_info();

	// render the restarting message
	void render_restart_message();

	// These are auxiliary function for vectors, colors, etc. All are from original littlewolf.

	// Changes the field of view. A focal value of 1.0 is 90 degrees.
	inline Line viewport(float focal) {
		Line fov = { { focal, -1.0f }, { focal, +1.0f }, };
		return fov;
	}

	// Rotates the player by some radian value.
	inline Point turn(const Point a, const float t) {
		const Point b = { a.x * cosf(t) - a.y * sinf(t), a.x * sinf(t)
				+ a.y * cosf(t) };
		return b;
	}

	// Rotates a point 90 degrees.
	inline Point rag(const Point a) {
		const Point b = { -a.y, a.x };
		return b;
	}

	// Subtracts two points.
	inline Point sub(const Point a, const Point b) {
		const Point c = { a.x - b.x, a.y - b.y };
		return c;
	}

	// Adds two points.
	inline Point add(const Point a, const Point b) {
		const Point c = { a.x + b.x, a.y + b.y };
		return c;
	}

	// Multiplies a point by a scalar value.
	inline Point mul(const Point a, const float n) {
		const Point b = { a.x * n, a.y * n };
		return b;
	}

	// Returns the magnitude of a point.
	inline float mag(const Point a) {
		return sqrtf(a.x * a.x + a.y * a.y);
	}

	// Returns the unit vector of a point.
	inline Point unit(const Point a) {
		return mul(a, 1.0f / mag(a));
	}

	// Returns the slope of a point.
	inline float slope(const Point a) {
		return a.y / a.x;
	}

	// Rotates a line by some radian amount.
	inline Line rotate(const Line l, const float t) {
		const Line line = { turn(l.a, t), turn(l.b, t) };
		return line;
	}

	// Linear interpolation.
	inline Point lerp(const Line l, const float n) {
		return add(l.a, mul(sub(l.b, l.a), n));
	}

	// Fast floor (math.grid_h is too slow).
	inline int fl(const float x) {
		return (int)x - (x < (int)x);
	}

	// Fast ceil (math.grid_h is too slow).
	inline int cl(const float x) {
		return (int)x + (x > (int)x);
	}

	// Returns a decimal value of the ascii tile value on the map.
	inline Uint8 tile(const Point a, Uint8** tiles) {
		const int x = a.x;
		const int y = a.y;
		return tiles[y][x];
	}

	inline Display lock(const Gpu gpu) {
		void* screen;
		int pitch;
		SDL_LockTexture(gpu.texture, NULL, &screen, &pitch);
		const Display display = { (Uint32*)screen, pitch
				/ (int)sizeof(Uint32), pitch };
		return display;
	}

	// Places a pixels in gpu video memory.
	inline void put(const Display display, const int x, const int y,
		const Uint32 pixel) {
		display.pixels[y + x * display.width] = pixel;
	}

	// Unlocks the gpu, making the pointer to video memory ready for presentation
	inline void unlock(const Gpu gpu) {
		SDL_UnlockTexture(gpu.texture);
	}

	// Steps horizontally along a square grid.
	inline Point sh(const Point a, const Point b) {
		const float x = b.x > 0.0f ? fl(a.x + 1.0f) : cl(a.x - 1.0f);
		const float y = slope(b) * (x - a.x) + a.y;
		const Point c = { x, y };
		return c;
	}

	// Steps vertically along a square grid.
	inline Point sv(const Point a, const Point b) {
		const float y = b.y > 0.0f ? fl(a.y + 1.0f) : cl(a.y - 1.0f);
		const float x = (y - a.y) / slope(b) + a.x;
		const Point c = { x, y };
		return c;
	}

	// Floating point decimal.
	inline float dec(const float x) {
		return x - (int)x;
	}

	// Returns a color value (RGBA) from a decimal tile value.
	// Why we need it? For some reason, the texture used in the original wolf
	// uses ARGB, and for Textures in SDLUtils we use RGBA. I tried to change the
	// one of littlewolf to use RGBA with no success ...
	inline Uint32 color_rgba(const Uint8 tile) {
		Uint32 c = color(tile);
		return (c << 8) | (c >> 24 & 0xff);
	}

	// Returns a color value (ARGB) from a decimal tile value.
	// Tile 1-9 represent walls, 10- are for players, 0 is not handled
	// because it is an empty cell
	inline Uint32 color(const Uint8 tile) {
		switch (tile) {
		case 1:
			return 0xAAAA0000; // Red.
		case 2:
			return 0x0000AA00; // Green.
		case 3:
			return 0x000000AA; // Blue.
		case 4:
			return 0x00FFFF00; // Yellow
		case 5:
			return 0x0000FFFF; //Cyan/Aqua
		case 6:
			return 0x00FF00FF; // Magenta/Fuchsia
		case 7:
			return 0x00C0C0C0; // Silver
		case 8:
			return 0x00808080; // Gray
		case 9:
			return 0x00800000; // Maroon

			// from 10 on are colors of fighters
		case 10:
			return 0x00808000; // Olive
		case 11:
			return 0x00008000; // Green
		case 12:
			return 0x00800080; // Purple
		case 13:
			return 0x00008080; // Teal
		case 14:
			return 0x00000080; // Navy
		default:
			return 0x00f260b0; // Kind of dark green
		}
	}

	inline Uint8 player_to_tile(Uint8 id) {
		return id + 10;
	}

	inline Uint8 tile_to_player(Uint8 tile) {
		assert(tile >= 10);
		return tile - 10;
	}

	// Some fields defining all elements of the world, etc

	bool _show_help;

	// maximum number of player
	static constexpr Uint8 _max_player = 10;

	// resolution
	int _xres;
	int _yres;

	// Relation between window size and walling size
	static constexpr Uint8 _walling_size_factor = 2;

	// width/height of the walling
	Uint16 _walling_width;
	Uint16 _walling_height;

	// a distance after which shoot has no effect
	float _shoot_distace;

	// the damage when shooting at point-blank
	float _original_dmg = 20;

	// the map that includes the walling, etc
	Map _map;

	// array of players
	Player _players[_max_player];

	// id of the current player, used since we allows switching between players
	Uint8 _curr_player_id;

	// the GPU structure with all the needed elements to draw the world
	Gpu _gpu;

	// Mute sound flag
	bool _mute;

};

