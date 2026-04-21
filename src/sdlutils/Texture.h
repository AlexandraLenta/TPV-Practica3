// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <cassert>
#include <iostream>

#include "Font.h"

class Texture {
public:


	// cannot copy textures
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	// can be moved
	Texture& operator=(Texture &&other) noexcept;
	Texture(Texture &&other) noexcept;

	// used to create an empty texture, so we can move something else to it
	// later
	Texture() noexcept;

	// Construct from image
	Texture(SDL_Renderer *renderer, const std::string &fileName);

	// Construct from text
	Texture(SDL_Renderer *renderer, const std::string &text, const Font &font,
			const SDL_Color &fgColor);

	// Construct from text with background
	Texture(SDL_Renderer *renderer, const std::string &text, const Font &font,
			const SDL_Color &fgColor, const SDL_Color &bgColor);


	virtual ~Texture() {
		if (_texture != nullptr)
			SDL_DestroyTexture(_texture); // delete the SDL texture
	}

	inline int width() const {
		return _width;
	}

	inline int height() const {
		return _height;
	}

	// This rendering method corresponds to method SDL_RenderCopyEx.
	//
	// Renders part of the texture (src) to a destination rectangle (dest)
	// with a rotation (angle) around point p (of dest), and flips
	// according to the value of flip. If 'p'is nullptr, the rotation is done
	// wrt. the center
	//
	inline void render(const SDL_FRect &src, const SDL_FRect &dest, double angle,
			const SDL_FPoint *p = nullptr,
			SDL_FlipMode flip = SDL_FLIP_NONE) const {
		assert(_texture != nullptr);
		SDL_RenderTextureRotated(_renderer, _texture, &src, &dest, angle, p, flip);
	}

	// This rendering method corresponds to method SDL_RenderCopy.
	//
	// Renders part of the texture (src) to a destination rectangle (dest).
	// It can be implemented by calling the previous render method as well,
	// but we use SDL_RenderCopy directly since it does less checks so it
	// saves some checks ...
	inline void render(const SDL_FRect &src, const SDL_FRect &dest) const {
		assert(_texture != nullptr);
		SDL_RenderTexture(_renderer, _texture, &src, &dest);
	}

	// render the complete texture at position (x,y).
	inline void render(float x, float y) const {
		SDL_FRect dest = { x, y, static_cast<float>(_width), static_cast<float>(_height) };
		render(dest);
	}

	// renders the complete texture at a destination rectangle (dest)
	inline void render(const SDL_FRect &dest) const {
		SDL_FRect src = { 0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height) };
		render(src, dest);
	}

	// renders the complete texture at a destination rectangle (dest),
	// with rotation
	inline void render(const SDL_FRect &dest, float rotation) const {
		SDL_FRect src = { 0, 0, static_cast<float>(_width), static_cast<float>(_height) };
		render(src, dest, rotation);
	}

private:

	// Construct from text
	void constructFromText(SDL_Renderer *renderer, const std::string &text,
			const Font &font, const SDL_Color *fgColor,
			const SDL_Color *bgColor = nullptr);

	SDL_Texture *_texture;
	SDL_Renderer *_renderer;
	int _width;
	int _height;
};
