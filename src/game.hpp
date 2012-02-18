#ifndef DVB021_GAME_H
#define DVB021_GAME_H

#include <cstddef>
#include <string>

namespace Game {
	/**
	 * Initialize engine.
	 */
	void init(const std::string& backend, int width, int height);

	/**
	 * Stop the engine and release all resources.
	 */
	void cleanup();

	/**
	 * Load a new level, overwriting the current (memory is freed)
	 */
	void load_level(const std::string& filename);

	/**
	 * Generate a new tilemap.
	 */
	Tilemap* load_tilemap(const std::string& filename);

	/**
	 * Do stuff.
	 */
	void frobnicate();

	/**
	 * Pan the camera.
	 */
	void pan(float x, float y);

	/**
	 * Inform about mouse movement (in screenspace).
	 */
	void motion(float x, float y);

	/**
	 * Inform about mouseclick (in screenspace).
	 */
	void click(float x, float y);
};

#endif /* DVB021_GAME_H */
