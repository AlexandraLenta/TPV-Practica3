// This file is part of the course TPV2@UCM - Samir Genaim

#pragma once

#include <SDL_mixer.h>
#include <cassert>
#include <cmath>
#include <vector>

#include "../utils/Singleton.h"

/*
 * This class provides an abstraction layer over sdl_mixer. It allows creating
 * groups of tracks (identified by a tag), and playing sound on the first
 * non-busy track of a given group.
 *
 * Tag and tracks are the same as those of sdl_mixer, but we do not allow a track
 * to have several tags nor treat tracks individually, just via their group.
 *
 * By default have a group "bg" with a single track for background music, and a
 * group "se" with 10 tracks for sound effects.
 */

class SoundManager: public Singleton<SoundManager> {
	friend Singleton<SoundManager> ;

public:

	using audio_t = MIX_Audio *;

	// load an audio file
	inline audio_t load_audio(const std::string &fileName) {
		audio_t audio = MIX_LoadAudio(_mixer, fileName.c_str(), true);
		assert(audio != nullptr);
		return audio;
	}

	// release an audio
	inline void release_audio(audio_t a) {
		MIX_DestroyAudio(a);
	}

	// play on a temporary track
	inline bool play(audio_t a) const {
		return MIX_PlayAudio(_mixer, a);
	}

	// play sound on the first free track tagged with `tag`. The `loops`
	// parameter indicates the number of repetitions where -1 is for
	// looping infinitely
	inline bool play(audio_t a, const char *tag, int loops = 0) const {
		assert(tag != nullptr);

		bool played = false;
		MIX_Track **tracks = MIX_GetTaggedTracks(_mixer, tag, nullptr);
		if (tracks) {
			int idx = 0;
			while (tracks[idx] != nullptr && MIX_TrackPlaying(tracks[idx])) {
				idx++;
			}
			if (tracks[idx]) {
				SDL_PropertiesID props = SDL_CreateProperties();
				SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
				MIX_SetTrackAudio(tracks[idx], a);
				MIX_PlayTrack(tracks[idx], props);
				played = true;
			}
		}
		return played;
	}

	// add 'n' tracks with the tag `tag` -- if the tag already exists
	// they are added to existing ones
	inline uint8_t add_tracks(uint8_t n, const char *tag) {
		assert(tag != nullptr);

		// if there are tracks with this tag, get the volume so
		// we set the new ones to that volume
		float vol = fabs(get_track_volume(tag)); // if no tracks vol is 1.0f

		uint8_t num_of_tracks = 0;
		for (auto i = 0u; i < n; i++) {
			MIX_Track *track = MIX_CreateTrack(_mixer);
			if (track) {
				MIX_TagTrack(track, tag);
				MIX_SetTrackGain(track,vol);
				num_of_tracks++;
				_tracks.push_back(track);
			}
		}

		return num_of_tracks;
	}

	// set the master volume -- non-negative number, usually between 0.0f and 1.0f
	// but in practice there is no upper limit.
	// See documentation of MIX_SetMixerGain
	bool set_master_volume(float gain) {
		return MIX_SetMixerGain(_mixer, gain > 0.0f ? gain : 0.0f);
	}

	// get the last set master volume
	float get_master_volume() {
		return MIX_GetMixerGain(_mixer);
	}

	// set tracks volume -- non-negative number, usually between 0.0f and 1.0f
	// but in practice there is no upper limit.
	// See documentation of MIX_SetTagGain
	float set_track_volume(const char *tag, float gain) {
		return MIX_SetTagGain(_mixer, tag, gain > 0.0f ? gain : 0.0f);
	}

	// get the last set track (group) volume -- it returns -1.0 if no tracks
	// with that tag
	float get_track_volume(const char* tag) {
		MIX_Track** tracks = MIX_GetTaggedTracks(_mixer, tag, nullptr);
		float vol = -1.0f;
		if (tracks != nullptr && tracks[0] != nullptr) {
			vol = MIX_GetTrackGain(tracks[0]);
		}
		return vol;
	}

	// stop all tracks
	bool stop_all(Sint64 fade_out_ms) {
		return MIX_StopAllTracks(_mixer, fade_out_ms);
	}

	// pause all tracks
	bool pause_all() {
		return MIX_PauseAllTracks(_mixer);
	}

	// resume all tracks
	bool resume_all() {
		return MIX_ResumeAllTracks(_mixer);
	}

	// stop all tracks of a group
	bool stop_track(const char *tag,Sint64 fade_out_ms) {
		return MIX_StopTag(_mixer, tag, fade_out_ms);
	}

	// pause all tracks of a group
	bool pause_track(const char *tag) {
		return MIX_PauseTag(_mixer, tag);
	}

	// resume all tracks of a group
	bool resume_track(const char *tag) {
		return MIX_ResumeTag(_mixer, tag);
	}

private:

	SoundManager() :
			_mixer(nullptr), //
			_tracks() {
	}

	virtual ~SoundManager() {
		for (MIX_Track *t : _tracks) {
			MIX_DestroyTrack(t);
		}

		if (_mixer)
			MIX_DestroyMixer(_mixer);

		MIX_Quit();
	}

	bool init() {
		if (MIX_Init()) {
			_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			NULL);
			_tracks.reserve(10);

			// default track groups
			add_tracks(1, "bg");
			add_tracks(10, "se");
		}

		return _mixer != nullptr;
	}

	MIX_Mixer *_mixer;
	std::vector<MIX_Track*> _tracks;

};

