// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

#include <string>
#include <SDL_mixer.h>
#include <cassert>

#include "SoundManager.h"

class SoundEffect {
public:

	// cannot copy objects of this type!
	SoundEffect& operator=(SoundEffect &other) = delete;
	SoundEffect(const SoundEffect&) = delete;

	// used to create an empty font, so we can move something else to it
	// later -- we assume that SoundManager::audio_t has a default constructor
	SoundEffect() noexcept :
			_audio() {
	}

	// cannot move objects of this type!
	SoundEffect(SoundEffect &&other) noexcept {
		_audio = other._audio;
		other._audio = SoundManager::audio_t(); // we assume that SoundManager::audio_t has a default constructor
	}

	SoundEffect& operator=(SoundEffect &&other) noexcept {
		release();
		_audio = other._audio;
		other._audio = SoundManager::audio_t();
		return *this;
	}

	SoundEffect(const std::string &fileName) {
		_audio = SoundManager::Instance()->load_audio(fileName.c_str());
	}

	virtual ~SoundEffect() {
		release();
	}

	// these methods just redirect to those of SoundManager

	inline bool play() const {
		return SoundManager::Instance()->play(_audio);
	}

	inline bool play(const char *tag, int loops = 0) const {
		return SoundManager::Instance()->play(_audio, tag, loops);
	}

private:

	inline void release() {
		SoundManager::Instance()->release_audio(_audio);
	}

	SoundManager::audio_t _audio;
};
