// This file is part of the course TPV2@UCM - Samir Genaim

#include <SDL.h>

#include "sdlutils_demo.h"

#include "InputHandler.h"
#include "macros.h"
#include "SDLUtils.h"

using namespace std;

void sdlutils_basic_demo() {

	// Initialize the SDLGame singleton
	if (!SDLUtils::Init("SDLGame Demo!", 800, 600,
			"resources/config/sdlutilsdemo.resources.json")) {
		std::cerr << "Something went wrong while initializing SDLUtils!"
				<< std::endl;
		return;
	}

	// Initialize the input handler
	if (!InputHandler::Init()) {
		std::cerr << "Something went wrong while initializing InputHandler!"
				<< std::endl;
		return;

	}

	// Reference to the SDLUtils Singleton. You could use it as a pointer as well.
	// You can also use the inline method sdlutils() that simply returns the value
	// of *SDLUtils::instance() --- it is defined at the end of SDLUtils.h
	//
	auto &sdl = *SDLUtils::Instance();

	//show the cursor
	sdl.showCursor();

	// store the 'renderer' in a local variable, just for convenience
	SDL_Renderer *renderer = sdl.renderer();

	// we can take textures from the predefined ones, and we can create a custom one as well
	auto &sdlLogo = sdl.images().at("sdl_logo");
	auto &helloSDL = sdl.msgs().at("HelloSDL");

	// we can create out own textures
	Texture pressKey(renderer, "Press ESC to exit, SPACE to laugh",
			sdl.fonts().at("ARIAL24"), build_sdlcolor(0x112233ff),
			build_sdlcolor(0xffffffff));

	// volume related variables and textures
	float vol = SoundManager::Instance()->get_master_volume();

	// writing float to a char* with 1 decimal precision
	char vol_str[32];
	SDL_snprintf(vol_str, sizeof(vol_str), "%.1f", vol);

	Texture volumeCtrlMsg(renderer, "Volume +/-/M:", sdl.fonts().at("ARIAL24"),
			build_sdlcolor(0xff0000ff), build_sdlcolor(0xffff00ff));

	Texture volumeMsg(renderer, vol_str, sdl.fonts().at("ARIAL24"),
			build_sdlcolor(0xffffffff));


	// some coordinates
	auto winWidth = sdl.width();
	auto winHeight = sdl.height();
	auto x0 = (winWidth - pressKey.width()) / 2.0f;
	auto y0 = (winHeight - pressKey.height()) / 2.0f;
	auto x1 = 0.0f;
	auto y1 = y0 - 4.0f * pressKey.height();
	auto x2 = (winWidth - sdlLogo.width()) / 2.0f;
	auto y2 = y0 + 2.0f * pressKey.height();
	auto vx = volumeCtrlMsg.width() + 20.f; // x-coordinate of the volume text

	// start the music in a loop
	sdl.soundEffects().at("beat").play("bg", -1);

	// reference to the input handler (we could use a pointer, I just . rather than ->).
	// you can also use the inline method ih() that is defined in InputHandler.h
	auto &ih = *InputHandler::Instance();

	// a boolean to exit the loop
	bool exit_ = false;

	while (!exit_) {
		Uint64 startTime = sdl.currRealTime();

		// update the event handler
		ih.refresh();

		// exit when any key is down
		if (ih.keyDownEvent() && ih.isKeyDown(SDLK_ESCAPE))
			exit_ = true;

		if (ih.keyDownEvent() && ih.isKeyDown(SDLK_SPACE))
			sdlutils().soundEffects().at("laugh").play("se");

		// volume up
		if (ih.keyDownEvent() && ih.isKeyDown(SDLK_PLUS)) {
			vol = SoundManager::Instance()->get_master_volume();
			vol += 0.1f;
			SoundManager::Instance()->set_master_volume(vol);
			SDL_snprintf(vol_str, sizeof(vol_str), "%.1f", vol);
			volumeMsg = Texture(renderer, vol_str, sdl.fonts().at("ARIAL24"),
					build_sdlcolor(0xffffffff));
		}

		// volume down
		if (ih.keyDownEvent() && ih.isKeyDown(SDLK_MINUS)) {
			vol = SoundManager::Instance()->get_master_volume();
			vol -= 0.1f;
			if (vol < 0.0f)
				vol = 0.0f;
			SoundManager::Instance()->set_master_volume(vol);
			SDL_snprintf(vol_str, sizeof(vol_str), "%.1f", vol);
			volumeMsg = Texture(renderer, vol_str, sdl.fonts().at("ARIAL24"),
					build_sdlcolor(0xffffffff));
		}

		// mute
		if (ih.keyDownEvent() && ih.isKeyDown(SDLK_M)) {
			vol = 0.0f;
			SoundManager::Instance()->set_master_volume(vol);
			SDL_snprintf(vol_str, sizeof(vol_str), "%.1f", vol);
			volumeMsg = Texture(renderer, vol_str, sdl.fonts().at("ARIAL24"),
					build_sdlcolor(0xffffffff));
		}

		// clear screen
		sdl.clearRenderer();

		// render Hello SDL
		helloSDL.render(x1, y1);
		if (x1 + helloSDL.width() > winWidth)
			helloSDL.render(x1 - winWidth, y1);
		x1 = (x1 + 5);
		if (x1 >= winWidth) // modulo
			x1 = x1 - winWidth;

		// render Press Key
		pressKey.render(x0, y0);

		// render volume
		volumeCtrlMsg.render(10.0f, 10.0f);
		volumeMsg.render(vx, 10.0f);

		// render the SDLogo
		sdlLogo.render(x2, y2);

		// present new frame
		sdl.presentRenderer();

		Uint64 frameTime = sdl.currRealTime() - startTime;

		if (frameTime < 20)
			SDL_Delay(static_cast<Uint32>(20 - frameTime));
	}

	// Release InputHandler and then SDLUtils (initialization reverse order)
	if (InputHandler::HasInstance())
		InputHandler::Release();
	if (SDLUtils::HasInstance())
		SDLUtils::Release();

}

