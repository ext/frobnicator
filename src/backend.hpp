#ifndef DVB021_BACKEND_H
#define DVB021_BACKEND_H

#include <map>
#include <string>
#include "common.hpp"

class Backend {
public:
	virtual ~Backend();

	/**
	 * Initializes backend (including window)
	 */
	virtual void init(int width, int height) = 0;

	/**
	 * Polling
	 */
	virtual void poll(bool& running) = 0;

	/**
	 * Inverse of init.
	 */
	virtual void cleanup() = 0;

	virtual void render_begin() = 0;
	virtual void render_tilemap(const Tilemap& tilemap, const Vector2f& camera) const = 0;
	virtual void render_marker(const Vector2f& pos, const Vector2f& camera, const bool v[]) const = 0;
	virtual void render_end() = 0;

	/**
	 * Load a tilemap from a file.
	 */
	virtual Tilemap* load_tilemap(const std::string& filename) = 0;

	typedef Backend* (*factory_callback)();
	typedef std::map<std::string, factory_callback> map;

	/**
	 * Register a backend so it can be created from name.
	 */
	static void register_factory(const std::string&, factory_callback);

	/**
	 * Create a new backend instance.
	 */
	static Backend* create(const std::string& name);

protected:
	Backend();

private:
	static map factory_map;
};

#define REGISTER_BACKEND(cls)	\
	class SI { public: SI(){ Backend::register_factory(#cls, cls::factory); } }; \
	static SI si

#endif /* DVB021_BACKEND_H */
