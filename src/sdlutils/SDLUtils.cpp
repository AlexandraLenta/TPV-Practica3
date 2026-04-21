// This file is part of the course TPV2@UCM - Samir Genaim

#include "SDLUtils.h"

#include <SDL_image.h>

#include <cassert>
#include <memory>

#include "../json/JSON.h"

SDLUtils::SDLUtils() :
		_windowTitle("SDL2 Demo"), //
		_width(600), //
		_height(480), //
		_window(nullptr), //
		_renderer(nullptr), //
		_fontsAccessWrapper(_fonts, "Fonts Table"), //
		_imagesAccessWrapper(_images, "Images Table"), //
		_msgsAccessWrapper(_msgs, "Messages Table"), //
		_soundsAccessWrapper(_sounds, "Sounds Table"), //
		_currTime(currRealTime()), //
		_deltaTime(0) //
{
}

bool SDLUtils::init(const std::string& windowTitle, int width, int height) {
	assert(width > 0);
	assert(height > 0);
	_windowTitle = windowTitle;
	_height = height;
	_width = width;

	initWindow();
	initSDLExtensions();

	// we always return true, because this class either exit or throws an
	// exception on error. If you want to avoid using exceptions you should
	// find a workaround using booleans.
	return true;
}

bool SDLUtils::init(const std::string& windowTitle, int width, int height,
		const std::string& filename) {
	init(windowTitle, width, height);

	loadReasources(filename);

	// we always return true, because this class either exit or throws an
	// exception on error. If you want to avoid using exceptions you should
	// find a workaround using booleans.
	return true;
}

SDLUtils::~SDLUtils() {
	closeSDLExtensions();
	closeWindow();
}

void SDLUtils::initWindow() {

#ifdef _DEBUG
	std::cout << "Initializing SDL" << std::endl;
#endif

	// Initialize SDL
	if (!SDL_Init(
	SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS)) {
		std::cerr << SDL_GetError() << std::endl;
		assert(false);
	}

#ifdef _DEBUG
	std::cout << "Creating SDL window" << std::endl;
#endif

	// Create window
	_window = SDL_CreateWindow(_windowTitle.c_str(), _width, _height, 0);
	assert(_window != nullptr);
#ifdef _DEBUG
	std::cout << "Creating SDL renderer" << std::endl;
#endif

	// Create the renderer
	if (!(_renderer = SDL_CreateRenderer(_window, nullptr))) {
		std::cerr << SDL_GetError() << std::endl;
		assert(false);
	}

// hide cursor by default
	hideCursor();

}

void SDLUtils::closeWindow() {

// destroy renderer and window
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);

	SDL_Quit(); // quit SDL
}

void SDLUtils::initSDLExtensions() {

#ifdef _DEBUG
	std::cout << "Initializing SDL_ttf" << std::endl;
#endif
// initialize SDL_ttf
	if (!TTF_Init()) {
		std::cerr << SDL_GetError() << std::endl;
		exit(1);
	}

#ifdef _DEBUG
	std::cout << "Initializing SDL_Mixer" << std::endl;
#endif
// initialize SDL_Mixer
	SoundManager::Init();

#ifdef _DEBUG
	std::cout << "Initializing the InputHandler" << std::endl;
#endif

}

void SDLUtils::loadReasources(const std::string& filename) {
// TODO check the correctness of values and issue a corresponding
// exception. Now we just do some simple checks, and assume input
// is correct.

// Load JSON configuration file. We use a unique pointer since we
// can exit the method in different ways, this way we guarantee that
// it is always deleted
	std::unique_ptr<JSONValue> jValueRoot(JSON::ParseFromFile(filename));

// check it was loaded correctly
// the root must be a JSON object
	if (jValueRoot == nullptr || !jValueRoot->IsObject()) {
		throw "Something went wrong while load/parsing '" + filename + "'";
	}

// we know the root is JSONObject
	JSONObject root = jValueRoot->AsObject();
	JSONValue *jValue = nullptr;

// TODO improve syntax error checks below, now we do not check
//      validity of keys with values as sting or integer

// load fonts
	jValue = root["fonts"];
	if (jValue != nullptr) {
		if (jValue->IsArray()) {
			_fonts.reserve(jValue->AsArray().size()); // reserve enough space to avoid resizing
			for (auto &v : jValue->AsArray()) {
				if (v->IsObject()) {
					JSONObject vObj = v->AsObject();
					std::string key = vObj["id"]->AsString();
					std::string file = vObj["file"]->AsString();
					float size = static_cast<float>(vObj["size"]->AsNumber());
#ifdef _DEBUG
					std::cout << "Loading font with id: " << key << std::endl;
#endif
					_fonts.emplace(key, Font(file, size));
				} else {
					throw "'fonts' array in '" + filename
							+ "' includes and invalid value";
				}
			}
		} else {
			throw "'fonts' is not an array in '" + filename + "'";
		}
	}

// load images
	jValue = root["images"];
	if (jValue != nullptr) {
		if (jValue->IsArray()) {
			_images.reserve(jValue->AsArray().size()); // reserve enough space to avoid resizing
			for (auto &v : jValue->AsArray()) {
				if (v->IsObject()) {
					JSONObject vObj = v->AsObject();
					std::string key = vObj["id"]->AsString();
					std::string file = vObj["file"]->AsString();
#ifdef _DEBUG
					std::cout << "Loading image with id: " << key << std::endl;
#endif
					_images.emplace(key, Texture(renderer(), file));
				} else {
					throw "'images' array in '" + filename
							+ "' includes and invalid value";
				}
			}
		} else {
			throw "'images' is not an array in '" + filename + "'";
		}
	}

// load messages
	jValue = root["messages"];
	if (jValue != nullptr) {
		if (jValue->IsArray()) {
			_msgs.reserve(jValue->AsArray().size()); // reserve enough space to avoid resizing
			for (auto &v : jValue->AsArray()) {
				if (v->IsObject()) {
					JSONObject vObj = v->AsObject();
					std::string key = vObj["id"]->AsString();
					std::string txt = vObj["text"]->AsString();
					auto &font = _fonts.at(vObj["font"]->AsString());
#ifdef _DEBUG
					std::cout << "Loading message with id: " << key
							<< std::endl;
#endif
					if (vObj["bg"] == nullptr)
						_msgs.emplace(key,
								Texture(renderer(), txt, font,
										build_sdlcolor(
												vObj["color"]->AsString())));
					else
						_msgs.emplace(key,
								Texture(renderer(), txt, font,
										build_sdlcolor(
												vObj["color"]->AsString()),
										build_sdlcolor(
												vObj["bg"]->AsString())));
				} else {
					throw "'messages' array in '" + filename
							+ "' includes and invalid value";
				}
			}
		} else {
			throw "'messages' is not an array in '" + filename + "'";
		}
	}

// load sounds
	jValue = root["sounds"];
	if (jValue != nullptr) {
		if (jValue->IsArray()) {
			_sounds.reserve(jValue->AsArray().size()); // reserve enough space to avoid resizing
			for (auto &v : jValue->AsArray()) {
				if (v->IsObject()) {
					JSONObject vObj = v->AsObject();
					std::string key = vObj["id"]->AsString();
					std::string file = vObj["file"]->AsString();
#ifdef _DEBUG
					std::cout << "Loading sound effect with id: " << key
							<< std::endl;
#endif
					_sounds.emplace(key, SoundEffect(file));
				} else {
					throw "'sounds' array in '" + filename
							+ "' includes and invalid value";
				}
			}
		} else {
			throw "'sounds' is not an array";
		}
	}

}

void SDLUtils::closeSDLExtensions() {

	_sounds.clear();
	_msgs.clear();
	_images.clear();
	_fonts.clear();

	if (SoundManager::HasInstance())
		SoundManager::Release();

	TTF_Quit(); // quit SDL_ttf
}

