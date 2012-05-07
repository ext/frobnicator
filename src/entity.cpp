#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include "blueprint.hpp"
#include "game.hpp"
#include "waypoint.hpp"
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
	: Entity(pos, blueprint, level)
	, left(7) {

	fprintf(stderr, "Spawning \"%s\" at (%.0f,%.0f)\n", name().c_str(), pos.x, pos.y);
}

std::string Creep::get_region() const {
	return region;
}

Creep& Creep::set_region(const std::string& name){
	region = name;
	return *this;
}

Creep& Creep::set_dst(const Vector2f& dst){
	this->dst = dst;
	return *this;
}

void Creep::tick(){
	const Vector2f d = (dst - pos - /* hack hack hack */ Vector2f(24,24)).normalized();
	pos += d * 1.6;
}

void Creep::on_enter_region(const Waypoint& region){
	fprintf(stderr, "Entity %p entered `%s'.\n", this, region.name().c_str());

	if ( region.name() == "middle" ){
		return;
	}

	std::string name = region.next();
	if ( --left == 0 ){
		name = region.inner();
		left = 7;
	}

	const Waypoint* next = Game::find_waypoint(name);
	if ( !next ){
		fprintf(stderr, "Waypoint `%s' refers to non-existing waypoint `%s', ignored.\n", region.name().c_str(), region.next().c_str());
		return;
	}

	set_dst(next->middle());
}

void Creep::on_exit_region(const Waypoint& region){
	fprintf(stderr, "Entity %p exited `%s'.\n", this, region.name().c_str());
}
