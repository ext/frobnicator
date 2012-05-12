#ifndef DVB021_BACKEND_H
#define DVB021_BACKEND_H

#include <map>
#include <string>
#include <vector>
#include <functional>
#include "vector.hpp"

class RenderTarget {
public:
	virtual void bind() = 0;
	virtual void unbind() = 0;
};

class Font {

};

class Backend {
public:
	virtual ~Backend();

	/**
	 * Initializes backend (including window)
	 */
	virtual void init(const Vector2i& size) = 0;

	/**
	 * Polling
	 */
	virtual void poll(bool& running) = 0;

	/**
	 * Inverse of init.
	 */
	virtual void cleanup() = 0;

	/**
	 * Bind a bind to an action.
	 */
	virtual void bindkey(const std::string& key, std::function<void()> func) = 0;

	virtual void render_begin(RenderTarget* target) = 0;
	virtual void render_tilemap(const Tilemap& tilemap, const Vector2f& camera) const = 0;
	virtual void render_marker(const Vector2f& pos, const Vector2f& camera, const bool v[]) const = 0;
	virtual void FROB_NONNULL(1) render_region(const Region* region, const Vector2f& camera, float color[3]) const = 0;
	virtual void FROB_NONNULL(1) render_region(const Entity* region, const Vector2f& camera, float color[3]) const = 0;
	virtual void render_entities(std::vector<Entity*>& entities, const Vector2f& camera) const = 0;
	virtual void render_projectiles(std::vector<Projectile*>& projectiles, const Vector2f& camera) const = 0;
	virtual void FROB_NONNULL(1) render_target(RenderTarget* target, const Vector2i& offset) const = 0;
	virtual void render_end() = 0;

	/**
	 * Load a tilemap from a file.
	 */
	virtual Tilemap* load_tilemap(const std::string& filename) = 0;

	virtual Sprite* create_sprite() = 0;
	virtual RenderTarget* create_rendertarget(const Vector2i& size) = 0;
	virtual Font* create_font(const std::string& filename) = 0;

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
