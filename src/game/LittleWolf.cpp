// This file is part of the course TPV2@UCM - Samir Genaim

#include "LittleWolf.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "../json/JSONValue.h"
#include "../sdlutils/InputHandler.h"
#include "../sdlutils/SDLUtils.h"
#include "../sdlutils/Texture.h"
#include "Game.h"
#include "Networking.h"

LittleWolf::LittleWolf() :
	_show_help(true), //
	_xres(), //
	_yres(), //
	_walling_width(), // width of walls
	_walling_height(), // height of walls
	_shoot_distace(), // the shoot distance -- note that it's wrt. to the walling size
	_map(), // the map
	_players(), // player array
	_curr_player_id(0), // the id of the current player
	_mute(false) { // we start with player 0
}

LittleWolf::~LittleWolf() {
	// nothing to delete, the wallings are deleted in the Map's destructor
}

void LittleWolf::init(SDL_Window* window, SDL_Renderer* render) {
	// for some reason it is created with a rotation of 90 degrees -- must be easier to
	// manipulate coordinates
	SDL_Texture* const texture = SDL_CreateTexture(sdlutils().renderer(),
		SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, _yres,
		_xres);

	_gpu = { window, render, texture, _xres, _yres };

}

void LittleWolf::update() {

	auto& ihdlr = ih();

	if (ihdlr.keyDownEvent()) {

		if (ihdlr.isKeyDown(SDL_SCANCODE_T)) { // toggle help
			_show_help = !_show_help;
		}
		else if (ihdlr.isKeyDown(SDL_SCANCODE_M)) { // M mutes/unmutes sound
			muteSound();
		}
	}

	Player& p = _players[_curr_player_id];

	// save position and rotation before movement
	Point oldPos = p.where;
	float oldRot = p.theta;

	// check if we can do actions
	if (_canMove && p.state == ALIVE) {
		spin(p);  // handle spinning
		move(p);  // handle moving
		shootNetwork(_curr_player_id); // handle shooting
	}
	// can't move means we are waiting for the game to restart
	else if (!_canMove) {
		auto& vt = sdlutils().virtualTimer();
		if (vt.currRealTime() > _resetTime) { 
			if (Game::Instance()->get_networking().is_master()) { // if we are the master, we restart the game
				Game::Instance()->get_networking().send_restart();
			}
		}
	}

	// send player's state (new position and old position)
	Game::Instance()->get_networking().send_state({ p.where.x, p.where.y },
		p.theta, { oldPos.x, oldPos.y }, oldRot);
}

void LittleWolf::load(std::string filename) {

	// Load JSON walling file.
	std::unique_ptr<JSONValue> jValueRoot(JSON::ParseFromFile(filename));

	// check it was loaded correctly
	// the root must be a JSON object
	if (jValueRoot == nullptr || !jValueRoot->IsObject()) {
		throw "Something went wrong while load/parsing the walling file'"
			+ filename + "'";
	}

	// we know the root is a JSONObject
	JSONObject root = jValueRoot->AsObject();
	JSONValue* jValue = nullptr;

	// ** read the user walling

	// - uh = user wall height, uw = user wall width, and sf = scaling factor
	// - the resolution of the window will be uw*sf x uh*sf
	Uint16 uh = 0, uw = 0, sf = 0;

	// read the scaling factor
	jValue = root["scaling_factor"];
	if (jValue != nullptr) {
		if (jValue->IsNumber()) {
			sf = static_cast<Uint16>(jValue->AsNumber());
		}
		else {
			throw "Incorrect value for scaling_factor";
		}
	}
	else {
		sf = 2;
	}

	if (sf % _walling_size_factor != 0)
		throw "Scaling factor should be divisible by '"
		+ std::to_string(_walling_size_factor) + ": "
		+ std::to_string(sf);

#ifdef _DEBUG
	std::cout << "Resolution scaling factor: " << (int)sf << std::endl;
#endif

	jValue = root["walling"];
	JSONArray json_walling;

	if (jValue != nullptr) {
		if (jValue->IsArray()) {
			json_walling = jValue->AsArray();
			uh = json_walling.size();
			assert(uh > 0);
			uw = json_walling[0]->AsString().size();
		}
	}

	_xres = uw * sf;
	_yres = uh * sf;

#ifdef _DEBUG
	std::cout << "Walling size: " << (int)uw << "x" << (int)uh << std::endl;
	std::cout << "Window size: " << (int)_xres << "x" << (int)_yres
		<< std::endl;
#endif

	_walling_width = _xres / _walling_size_factor;
	_walling_height = _yres / _walling_size_factor;

	// the shoot distance -- note that it's wrt to the walling size
	_shoot_distace = std::min(_walling_width, _walling_height) / 2;

	// rows of the user walling
	Uint8** walling = new Uint8 * [uh];
	for (auto i = 0u; i < uh; i++) {
		std::string row = json_walling[i]->AsString();

		if (row.size() != uw)
			throw "Size of row '" + std::to_string(i)
			+ "' is not equal to the walling width '"
			+ std::to_string(uw) + "'";

		const char* buffer = json_walling[i]->AsString().c_str();

		// create and initialize the row
		walling[i] = new Uint8[uw];
		for (auto j = 0u; j < uw; j++) {
			char tile = buffer[j];
			if (tile < 48 || tile > 57) {
				throw "Invalid value for tile (" + std::to_string(i) + ","
					+ std::to_string(j) + ") :'" + tile
					+ "' (must be a digit character)";
			}
			walling[i][j] = tile - '0';
		}
	}

	// we keep the user walling in the map, just in case it is useful
	// for something later.
	_map.user_walling = walling;
	_map.user_walling_width = uw;
	_map.user_walling_height = uh;

#ifdef _DEBUG
	std::cout << "Checking that borders have no zero cell." << std::endl;
#endif

	auto n = uh - 1;
	for (auto i = 0u; i < uw; i++) {
		if (walling[0][i] == 0 || walling[n][i] == 0) {
			throw "There is a zero valued-cell at border.";
		}
	}

	n = uw - 1;
	for (auto i = 0u; i < uh; i++) {
		if (walling[i][0] == 0 || walling[i][n] == 0) {
			throw "There is a zero valued-cell at border.";
		}
	}

#ifdef _DEBUG
	std::cout << "Loaded the following user walling (" << uh << "x" << uw << ")"
		<< std::endl;
	std::cout << std::endl;
	for (auto i = 0u; i < uh; i++) {
		for (auto j = 0u; j < uw; j++) {
			std::cout << (int)walling[i][j];
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
#endif

	// ** construct the walling

	// We fill it all with red tile first, because the walling_height/walling_width might
	// be not divisible by user_walling_height and map_.user_walling_width.
	_map.walling = new Uint8 * [_walling_height];
	_map.walling_height = _walling_height;
	_map.walling_width = _walling_width;
	for (auto i = 0u; i < _walling_height; i++) {
		_map.walling[i] = new Uint8[_walling_width];
		for (auto j = 0u; j < _walling_width; j++)
			_map.walling[i][j] = 1;
	}

	// each tile in the user's walling will be mapped to a block of tiles (cell) of the same color,
	// with the following width and height
	Uint16 cell_height = _walling_height / _map.user_walling_height;
	Uint16 cell_width = _walling_width / _map.user_walling_width;

	// fill in the walling
	for (int i = 0; i < _map.user_walling_height; i++)
		for (int j = 0; j < _map.user_walling_width; j++)
			for (int k = 0; k < cell_height; k++) // tile (i,j) in the user's walling is mapped to a block of tiles
				for (int l = 0; l < cell_width; l++) {
					_map.walling[i * cell_height + k][j * cell_width + l] =
						_map.user_walling[i][j];
				}

}

bool LittleWolf::addPlayer(Uint8 id, std::string name) {
	assert(id < _max_player);

	if (_players[id].state != NOT_USED)
		return false;
	
	resetPlayer(id, name);

	_curr_player_id = id;

	send_my_info();

	return true;
}

void LittleWolf::resetPlayer(Uint8 id, std::string name) {
	assert(id < _max_player);
	Point oldPlayerPos = _players[id].where;

	if (oldPlayerPos.x >= 0 && oldPlayerPos.x < _map.walling_width && oldPlayerPos.y >= 0 && oldPlayerPos.y < _map.walling_height) {
		_map.walling[(int)oldPlayerPos.y][(int)oldPlayerPos.x] = 0;
	}

	auto& rand = sdlutils().rand();

	// The search for an empty cell start at a random position (orow,ocol)
	Uint16 orow = rand.nextInt(0, _map.walling_height);
	Uint16 ocol = rand.nextInt(0, _map.walling_width);

	// search for an empty cell
	Uint16 row = orow;
	Uint16 col = (ocol + 1) % _map.walling_width;
	while (!((orow == row) && (ocol == col)) && !can_spawn_in_pos(row, col)) {
		col = (col + 1) % _map.walling_width;
		if (col == 0)
			row = (row + 1) % _map.walling_height;
	}

	// handle the case where the search has failed, which in principle should never
	// happen unless we start with map with few empty cells
	if (row >= _map.walling_height)
		return;


	// initialize the player
	Player p = { //
			id, //
				viewport(0.8f), // focal
				{ col + 0.5f, row + 0.5f }, // Where.
				{ 0.0f, 0.0f }, 			// Velocity.
				2.0f, 			// Speed.
				0.9f, 			// Acceleration.
				0.0f, 			// Rotation angle in radians.
				100,			// health points
				0,				// score points
				{}, // name
				ALIVE 			// Player state
	};

	Game::Instance()->string_to_chars(name, p.name);

	// note that player <id> is stored in the map as player_to_tile(id) -- which is id+10
	_map.walling[(int)p.where.y][(int)p.where.x] = player_to_tile(id);
	std::cout << "Map walling: " << (int)_map.walling[(int)p.where.y][(int)p.where.x] << '\n';
	_players[id] = p;

}

// check if we spawn out of bounds or not
bool LittleWolf::can_spawn_in_pos(int row, int col) {
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			int r = row + dy;
			int c = col + dx;

			if (r < 0 || r >= _map.walling_height ||
				c < 0 || c >= _map.walling_width)
				return false;

			if (_map.walling[r][c] != 0)
				return false;
		}
	}
	return true;
}

void LittleWolf::render() {

	// if the player is dead we the upper view, otherwise the normal view
	if (_players[_curr_player_id].state == DEAD)
		render_upper_view();
	else
		render_map(_players[_curr_player_id]);
	
	// render the identifiers, state, etc
	render_players_info();

	// show help
	if (_show_help) {
		int y = sdlutils().height();
		for (const char* s : { "usage_1", "usage_2", "usage_3", "usage_4",
				"usage_5", _mute ? "usage_6" : "usage_7" }) {
			auto& t = sdlutils().msgs().at(s);
			y = y - t.height() - 10;
			t.render(0, y);
		}
	}

	if (!_canMove) {
		render_restart_message();
	}
}

LittleWolf::Hit LittleWolf::cast(const Point where, Point direction,
	Uint8** walling, bool ignore_players, bool ignore_deads) {
	// Determine whether to step horizontally or vertically on the grid.
	Point hor = sh(where, direction);
	Point ver = sv(where, direction);
	Point ray = mag(sub(hor, where)) < mag(sub(ver, where)) ? hor : ver;
	// Due to floating point error, the step may not make it to the next grid square.
	// Three directions (dy, dx, dc) of a tiny step will be added to the ray
	// depending on if the ray hit a horizontal wall, a vertical wall, or the corner
	// of two walls, respectively.
	Point dc = mul(direction, 0.01f);
	Point dx = { dc.x, 0.0f };
	Point dy = { 0.0f, dc.y };
	Point test = add(ray,
		// Tiny step for corner of two grid squares.
		mag(sub(hor, ver)) < 1e-3f ? dc :
		// Tiny step for vertical grid square.
		dec(ray.x) == 0.0f ? dx :
		// Tiny step for a horizontal grid square.
		dy);
	const Hit hit = { tile(test, walling), ray };

	// If a wall was not hit, continue advancing the ray.
	if (hit.tile > 0 && hit.tile < 10) {
		return hit;
	}
	else if (hit.tile > 9 && !ignore_players
		&& (!ignore_deads || _players[hit.tile - 10].state != DEAD)) {
		return hit;
	}
	else {
		return cast(ray, direction, walling, ignore_players, ignore_deads);
	}
}

LittleWolf::Wall LittleWolf::project(const int xres, const int yres,
	const float focal, const Point corrected) {
	// Normal distance of corrected ray is clamped to some small value else wall size will shoot to infinity.
	const float normal = corrected.x < 1e-2f ? 1e-2f : 0.05 * corrected.x;
	const float size = 0.5f * focal * xres / normal;
	const int top = (yres + size) / 2.0f;
	const int bot = (yres - size) / 2.0f;
	// Top and bottom values are clamped to screen size else renderer will waste cycles
	// (or segfault) when rasterizing pixels off screen.
	const Wall wall = { top > yres ? yres : top, bot < 0 ? 0 : bot, size };
	return wall;
}

void LittleWolf::render_map(Player& p) {
	// lock the texture
	const Display display = lock(_gpu);

	const Line camera = rotate(p.fov, p.theta);

	// Ray cast for all columns of the window.
	for (int x = 0; x < _gpu.xres; x++) {

		// draw walls
		const Point direction = lerp(camera, x / (float)_gpu.xres);
		const Hit hit = cast(p.where, direction, _map.walling, true, false);
		const Point ray = sub(hit.where, p.where);
		const Point corrected = turn(ray, -p.theta);
		const Wall wall = project(_gpu.xres, _gpu.yres, p.fov.a.x, corrected);

		for (int y = 0; y < wall.bot; y++)
			put(display, x, y, 0x00000000);

		for (int y = wall.bot; y < wall.top; y++)
			put(display, x, y, color(hit.tile));

		for (int y = wall.top; y < _gpu.yres; y++)
			put(display, x, y, 0x00000000);

		// draw players
		const Point direction_p = lerp(camera, x / (float)_gpu.xres);
		const Hit hit_p = cast(p.where, direction_p, _map.walling, false,
			false);
		const Point ray_p = sub(hit_p.where, p.where);
		const Point corrected1 = turn(ray_p, -p.theta);
		const Wall wall_p = project(_gpu.xres, _gpu.yres, p.fov.a.x,
			corrected1);

		if (hit_p.tile >= 10) {
			int size_p = 0;
			Uint8 id = hit_p.tile - 10;

			if (_players[id].state == ALIVE)
				size_p = wall_p.top - (wall_p.top - wall_p.bot) / 3;   //
			else
				size_p = wall_p.bot + (wall_p.top - wall_p.bot) / 10; // when it is dead it is shorter

			for (int y = wall_p.bot; y < size_p; y++)
				put(display, x, y, color(hit_p.tile));
		}

	}

	// draw a rifle sight at the center
	for (int i = -10; i < 10; i++) {
		put(display, _gpu.xres / 2, _gpu.yres / 2 + i, 0xAAAAAAAA);
		put(display, _gpu.xres / 2 + i, _gpu.yres / 2, 0xAAAAAAAA);
	}

	// unlock the texture
	unlock(_gpu);

	// copy the texture to the renderer
	const SDL_FRect dst = build_sdlfrect((_gpu.xres - _gpu.yres) / 2.0f, (_gpu.yres
		- _gpu.xres) / 2.0f, _gpu.yres, _gpu.xres);
	SDL_RenderTextureRotated(_gpu.renderer, _gpu.texture, NULL, &dst, -90, NULL,
		SDL_FLIP_NONE);

}

void LittleWolf::render_upper_view() {
	// lock texture
	const Display display = lock(_gpu);

	// put all to black
	std::memset(display.pixels, 0, display.pitch * _xres);

	for (auto x = 0u; x < _map.walling_height; x++)
		for (auto y = 0u; y < _map.walling_width; y++) {

			// each non empty position in the walling is drawn as a square in the window,
			// because the walling size is smaller than the resolution by 'walling_size_factor'
			if (_map.walling[x][y] != 0)
				for (int i = 0; i < _walling_size_factor; i++)
					for (int j = 0; j < _walling_size_factor; j++)
						put(display, y * _walling_size_factor + i,
							_gpu.yres - 1 - x * _walling_size_factor + j,
							color(_map.walling[x][y]));
		}

	// unlock texture
	unlock(_gpu);

	const SDL_FRect dst = build_sdlfrect((_gpu.xres - _gpu.yres) / 2, (_gpu.yres - _gpu.xres)
		/ 2, _gpu.yres, _gpu.xres);
	SDL_RenderTextureRotated(_gpu.renderer, _gpu.texture, NULL, &dst, -90, NULL,
		SDL_FLIP_NONE);

	// add labels to each player, with corresponding rotation
	for (int i = 0u; i < _max_player; i++) {
		Player& p = _players[i];
		if (p.state != NOT_USED) {
			Texture info(sdlutils().renderer(), "P" + std::to_string(i),
				sdlutils().fonts().at("MFR12"),
				build_sdlcolor(color_rgba(i + 10)));

			int w = info.width();
			int h = info.height();

			SDL_FRect src = build_sdlfrect(0.0f, 0.0f, w, h);
			SDL_FRect dest = build_sdlfrect(p.where.x * 2 - w / 2,
				p.where.y * 2 - h / 2, w, h);

			info.render(src, dest,
				p.theta * 180 / 3.14159265358979323846264338327950288f);

		}
	}

}

void LittleWolf::render_players_info() {
	uint_fast16_t y = 0;

	for (auto i = 0u; i < _max_player; i++) {
		Player& p = _players[i];

		// render player info if it is used
		if (p.state != NOT_USED) {
			std::string transformedName = (i == _curr_player_id ? "*" + std::string(p.name) : p.name);
			std::string msg = transformedName + (p.state == DEAD ? " (dead)" : " - " + std::to_string(p.hp) + " | " + std::to_string(p.score) + " pts");

			Texture info(sdlutils().renderer(), msg,
				sdlutils().fonts().at("MFR24"),
				build_sdlcolor(color_rgba(i + 10)));

			SDL_FRect dest = build_sdlfrect(0, y, info.width(), info.height());

			info.render(dest);
			y += info.height() + 5;

		}
	}
}

void LittleWolf::render_restart_message() {
	Texture restart_msg(sdlutils().renderer(), "The game will restart in " + std::to_string(static_cast<int>(std::round(_resetTime - sdlutils().virtualTimer().currRealTime()) / 1000.0f)) + " seconds.", sdlutils().fonts().at("MFR24"), build_sdlcolor(color_rgba(10)));

	SDL_FRect dest = build_sdlfrect(sdlutils().width() / 4, sdlutils().height() / 2, restart_msg.width(), restart_msg.height());

	restart_msg.render(dest);
}

void LittleWolf::move(Player& p) {
	auto& ihdlr = ih();

	// W forwards, S backwards, D right, L left

	const Point last = p.where, zero = { 0.0f, 0.0f };

	// Accelerates with key held down.
	if (ihdlr.isKeyDown(SDL_SCANCODE_W) || ihdlr.isKeyDown(SDL_SCANCODE_S)
		|| ihdlr.isKeyDown(SDL_SCANCODE_D)
		|| ihdlr.isKeyDown(SDL_SCANCODE_A)) {

		const Point reference = { 1.0f, 0.0f };
		const Point direction = turn(reference, p.theta);
		const Point acceleration = mul(direction,
			ihdlr.isKeyDown(SDL_SCANCODE_LSHIFT) ? // when left shift is held the player moves slowly
			p.acceleration / 100.0f : p.acceleration);

		if (ihdlr.isKeyDown(SDL_SCANCODE_W))
			p.velocity = add(p.velocity, acceleration);
		if (ihdlr.isKeyDown(SDL_SCANCODE_S))
			p.velocity = sub(p.velocity, acceleration);
		if (ihdlr.isKeyDown(SDL_SCANCODE_D))
			p.velocity = add(p.velocity, rag(acceleration));
		if (ihdlr.isKeyDown(SDL_SCANCODE_A))
			p.velocity = sub(p.velocity, rag(acceleration));
	}
	else { // Otherwise, decelerates (exponential decay).
		p.velocity = mul(p.velocity, 1.0f - p.acceleration / p.speed);
	}

	// Caps velocity if top speed is exceeded.
	if (mag(p.velocity) > p.speed)
		p.velocity = mul(unit(p.velocity), p.speed);

	// Moves.
	//
	// ***IMPORTANT***
	//
	// We update both the player information 'p' and the walling _map.walling

	p.where = add(p.where, p.velocity);

	// if player hits a wall or a different player, we take the player back
	// to previous position and put velocity to 0
	if (tile(p.where, _map.walling) != 10 + _curr_player_id
		&& tile(p.where, _map.walling) != 0) {
		// Sets velocity to zero if there is a collision and puts p back in bounds.
		p.velocity = zero;
		p.where = last;
	}
	else { // otherwise we make a move
		int y0 = (int)last.y;
		int x0 = (int)last.x;
		int y1 = (int)p.where.y;
		int x1 = (int)p.where.x;
		if (x0 != x1 || y0 != y1) {
			_map.walling[y1][x1] = _map.walling[y0][x0];
			_map.walling[y0][x0] = 0;
		}
	}
}

void LittleWolf::spin(Player& p) {
	auto& ihdlr = ih();

	// L spin right, H spin left -- when left shift is held the player spins slowly

	// turn by 0.05rad, but if left shift is pressed make is 0.005rad
	float d = ihdlr.isKeyDown(SDL_SCANCODE_LSHIFT) ? 0.005 : 0.05f;

	if (ihdlr.isKeyDown(SDL_SCANCODE_H))
		p.theta -= d;
	if (ihdlr.isKeyDown(SDL_SCANCODE_L))
		p.theta += d;
}

int LittleWolf::shoot(Uint8 id) {
	Player& p = _players[id];

	// we shoot in several directions, because with projection what you see is not exact
	for (float d = -0.05; d <= 0.05; d += 0.005) {

		// search which tile was hit
		const Line camera = rotate(p.fov, p.theta + d);
		Point direction = lerp(camera, 0.5f);
		direction.x = direction.x / mag(direction);
		direction.y = direction.y / mag(direction);
		const Hit hit = cast(p.where, direction, _map.walling, false, true);

#ifdef _DEBUG
		printf(
			"Shoot by player %d hit a tile with value %d! at distance %f\n",
			p.id, hit.tile, mag(sub(p.where, hit.where)));
#endif

		// if we hit a tile with a player id and the distance from that tile is smaller
		// than shoot_distace, we mark the player as dead
		if (hit.tile > 9 && mag(sub(p.where, hit.where)) < _shoot_distace) {
			Uint8 hitPlayerID = tile_to_player(hit.tile);

			// if the player we hit is the same one that shot, continue;
			if (hitPlayerID == id) continue;

			damage_player(id, hitPlayerID);

			// else we return the id of the player we hit
			return hitPlayerID;
		}
	}
	// return - 1 if we didn't hit anyone
	return -1;
}

void LittleWolf::damage_player(Uint8 shooterId, Uint8 victimId) {
	Player& shooter = _players[shooterId];
	Player& victim = _players[victimId];

	float distance = mag(sub(shooter.where, victim.where));

	// how far along the line between 0 --------- _shoot_distance we shot from
	float distPorcentage = distance / _shoot_distace;

	// we don't just multiply the original dmg, we remove from it the percentage because we want more damage when the players are closer to each other
	float dmg = _original_dmg - _original_dmg * distPorcentage;

	victim.hp -= dmg;

	if (victim.hp <= 0) {
		victim.hp = 0;
		victim.state = DEAD;
	}

	shooter.score += 1;
	
	Game::Instance()->get_networking().send_damaged_info(victimId, victim.hp);
	Game::Instance()->get_networking().send_score_info(shooter.id, shooter.score);
}

bool LittleWolf::is_dead(Uint8 id) {
	return _players[id].state == DEAD;
}

// shoot function
void LittleWolf::shootNetwork(Uint8 id) {
	auto& ihdlr = ih();
	if (ihdlr.keyDownEvent() && ihdlr.isKeyDown(SDL_SCANCODE_SPACE)) {
		Game::Instance()->get_networking().send_shoot(id);
	}
}

void LittleWolf::play_shootSFX(Uint8 id, SFX sound) {
	Player& shooter = _players[id];
	Player& currPlayer = _players[_curr_player_id]; // the current player

	float distance = mag({ shooter.where.x - currPlayer.where.x,
					   shooter.where.y - currPlayer.where.y });

	float volume = std::min(SoundManager::Instance()->get_master_volume(), SoundManager::Instance()->get_master_volume() / distance);

	if (sound == GUNSHOT) { // if the sound comes from a gunshot
		SoundManager::Instance()->set_track_volume("gunshot", volume); // set volume only for this track
		sdlutils().soundEffects().at("gunshot").play("se");
	}
	else if (sound == PAIN) { // if sound comes from someone being shot
		SoundManager::Instance()->set_track_volume("pain", volume); // set volume only for this track
		sdlutils().soundEffects().at("pain").play("se");
	}
}

//void LittleWolf::switchToNextPlayer() {
//
//	// search the next player in the palyer's array
//	int j = (_curr_player_id + 1) % _max_player;
//	while (j != _curr_player_id && _players[j].state == NOT_USED)
//		j = (j + 1) % _max_player;
//
//	// move to the next player view
//	_curr_player_id = j;
//
//}

//void LittleWolf::bringAllToLife() {
//	// bring all dead players to life -- all stay in the same position
//	for (auto i = 0u; i < _max_player; i++) {
//		if (_players[i].state == DEAD) {
//			_players[i].state = ALIVE;
//		}
//	}
//}

void LittleWolf::muteSound() {
	_mute = !_mute;
	float gain = _mute ? 0.0f : 1.0f;
	SoundManager::Instance()->stop_all(0);
	SoundManager::Instance()->set_master_volume(gain);
}

void LittleWolf::send_my_info() {
	Player& p = _players[_curr_player_id];

	std::string name;
	Game::Instance()->chars_to_string(name, p.name);

	Game::Instance()->get_networking().send_my_info(Vector2D(p.where.x, p.where.y),
		p.theta, p.hp, p.score, p.state, name);
}

void LittleWolf::update_player_state(Uint8 id, float x, float y, float rot) {

	Player& p = _players[id];

	int old_x = (int)p.where.x; // take position before updating
	int old_y = (int)p.where.y;

	p.where.x = x;
	p.where.y = y;
	p.id = id;
	p.theta = rot;

	if (_map.walling[old_y][old_x] == player_to_tile(id)) { // if this player was there before, remove him
		_map.walling[old_y][old_x] = 0;
	}

	int new_x = (int)p.where.x;
	int new_y = (int)p.where.y;

	if (_map.walling[new_y][new_x] == 0) { // if there is no player in the new tile, place this player there
		_map.walling[new_y][new_x] = player_to_tile(id);
	}
	else { // if there is already a player there, move the current player to his old position
		p.where.x = old_x;
		p.where.y = old_y;
		_map.walling[old_y][old_x] = player_to_tile(id);
	}

}

void LittleWolf::killPlayer(Uint8 id) {
	_players[id].state = PlayerState::DEAD;
}

void LittleWolf::update_player_info(Uint8 id, float x, float y,	float rot, int hp, int score, Uint8 state, std::string name) {
	Player& p = _players[id];

	int old_x = (int)p.where.x; // cogemos la posicion del jugador con el id dado antes de darle el nuevo x e y
	int old_y = (int)p.where.y;

	p.where.x = x;
	p.where.y = y;
	p.id = id;
	p.theta = rot;
	p.state = static_cast<PlayerState>(state);
	p.hp = hp;
	p.score = score;

	Game::Instance()->string_to_chars(name, p.name);

	std::cout << p.hp << " " << p.score << '\n';

	if (_map.walling[old_y][old_x] == player_to_tile(id)) {
		_map.walling[old_y][old_x] = 0;
	}

	int new_x = (int)p.where.x;
	int new_y = (int)p.where.y;

	_map.walling[new_y][new_x] = player_to_tile(id);
}

void LittleWolf::removePlayer(Uint8 id) {
	_players[id].state = PlayerState::NOT_USED;
}

void LittleWolf::restart() {
	_canMove = true;
	// bring all dead players to life -- all stay in the same position
	for (auto i = 0u; i < _max_player; i++) {
		Player& p = _players[i];
		if (p.state == DEAD) {
			p.state = ALIVE;
			p.velocity.x = 0.0f;
			p.velocity.y = 0.f;
		}
		if (p.state != NOT_USED)
			resetPlayer(i, p.name);
	}
}

void LittleWolf::check_restart() {
	int aliveCounter = 0;
	for (auto i = 0u; i < _max_player; i++) {
		Player& p = _players[i];
		if (p.state == ALIVE) {
			aliveCounter++;
		}
		if (aliveCounter >= 2) {
			return;
		}
 	}

	_canMove = false; // no se pueden mover

	Game::Instance()->get_networking().send_restart_trigger();

	_resetTime = sdlutils().virtualTimer().currRealTime() + 5000; // the reset time is 5 seconds after we set the restart
}

void LittleWolf::triggerRestart() {
	_canMove = false;
	_resetTime = sdlutils().virtualTimer().currRealTime() + 5000; // the reset time is 5 seconds after we set the restart
}

void LittleWolf::update_player_hp(Uint8 id, int hp) {
	Player& p = _players[id];

	p.hp = hp;
}

void LittleWolf::update_player_score(Uint8 id, int score) {
	Player& p = _players[id];

	p.score = score;
}


void LittleWolf::update_player_name(Uint8 id, std::string name) {
	Player& p = _players[id];

	Game::Instance()->string_to_chars(name, p.name);
}