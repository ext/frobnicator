#ifndef DVB021_GAME_H
#define DVB021_GAME_H

#include "vector.hpp"
#include <cstddef>
#include <string>
#include <map>
#include <functional>

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
	 * Allocate a new sprite.
	 */
	Sprite* create_sprite();

	/**
	 * Find waypoint by name.
	 * @return Waypoint or NULL if no such waypoint could be found.
	 */
	const Waypoint* find_waypoint(const std::string& name);

	/**
	 * Find entity by id.
	 * @return Entity or NULL if no such entity was found.
	 */
	Entity* find_entity(const std::string& name);

	/**
	 * Remove entity by id.
	 * No-op if entity no such entity was found.
	 */
	void remove_entity(const std::string& name);

	/**
	 * All creep.
	 */
	const std::map<std::string, Creep*>& all_creep();

	/**
	 * Do stuff.
	 */
	void frobnicate();

	/**
	 * Get the width of a tile. Static per level.
	 */
	size_t FROB_PURE tile_width();

	/**
	 * Get the height of a tile. Static per level.
	 */
	size_t FROB_PURE tile_height();

	/**
	 * Pan the camera.
	 */
	void pan(float x, float y);

	/**
	 * Inform about mouse movement (in screenspace).
	 */
	void motion(float x, float y);

	/**
	 * Inform about user pressing mouse-button down.
	 * @param x in screenspace
	 * @param y in screenspace
	 * @param button
	 */
	void button_pressed(float x, float y, int button);

	/**
	 * Inform about user releasing mouse-button.
	 * @param x in screenspace
	 * @param y in screenspace
	 * @param button
	 */
	void button_released(float x, float y, int button);

	/**
	 * Inform about video resize.
	 * @param w New width in pixels.
	 * @param h New height in pixels.
	 */
	void resize(size_t w, size_t h);
};

#endif /* DVB021_GAME_H */
