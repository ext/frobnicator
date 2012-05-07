#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include "blueprint.hpp"
#include "game.hpp"
#include <cstdio>
#include <yaml.h>

#ifdef WIN32
#define strdup _strdup
extern "C" char* strndup(const char* src, size_t n);
#endif

Entity::Entity(const Vector2f& pos, const Blueprint* blueprint, unsigned int level)
	: level(level)
	, pos(pos)
	, blueprint(blueprint) {

}

const Vector2f& Entity::world_pos() const {
	return pos;
}

const Vector2i Entity::grid_pos() const {
	return Vector2i(pos.x / Game::tile_width(), pos.y / Game::tile_height());
}

const Sprite* Entity::sprite() const {
	return blueprint->sprite(level);
}

const std::string Entity::name() const {
	return blueprint->name(level);
}

Building::Building(const Vector2f& pos, const Blueprint* blueprint)
	: Entity(pos, blueprint, 0) {

	fprintf(stderr, "Creating \"%s\" at (%.0f,%.0f)\n", name().c_str(), pos.x, pos.y);
}

Creep::Creep(const Vector2f& pos, const Blueprint* blueprint, unsigned int level)
	: Entity(pos, blueprint, level) {

	fprintf(stderr, "Spawning \"%s\" at (%.0f,%.0f)\n", name().c_str(), pos.x, pos.y);
}

Creep& Creep::set_dst(const Vector2f& dst){
	this->dst = dst;
	return *this;
}

void Creep::tick(){
	const Vector2f d = (dst - pos).normalized();
	pos += d * 0.3;
}
