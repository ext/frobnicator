#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include "blueprint.hpp"
#include "game.hpp"
#include "waypoint.hpp"
#include <cstdio>
#include <sstream>
#include <iomanip>

#ifdef WIN32
#define strdup _strdup
extern "C" char* strndup(const char* src, size_t n);
#endif

Entity::Entity(const std::string& id, const Vector2f& pos, const Blueprint* blueprint, unsigned int level)
	: level(level)
	, pos(pos)
	, _id(id)
	, blueprint(blueprint)
	, references(1) {

	hp = max_hp();
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

const std::string Entity::id() const {
	return _id;
}

int Entity::cost()     const { return blueprint->data[level].cost; }
float Entity::splash() const { return blueprint->data[level].splash; }
float Entity::damage() const { return blueprint->data[level].damage; }
float Entity::rof()    const { return blueprint->data[level].rof; }
float Entity::range()  const { return blueprint->data[level].range; }
float Entity::slow()   const { return blueprint->data[level].slow; }
float Entity::poison() const { return blueprint->data[level].poison; }
float Entity::speed()  const { return blueprint->data[level].speed; }
float Entity::armor()  const { return blueprint->data[level].armor; }
float Entity::max_hp()  const { return blueprint->data[level].hp; }
float Entity::current_hp()  const { return hp; }
bool Entity::is_alive() const { return hp > 0.0; }

void Entity::kill(){
	fprintf(stderr, "Entity %s was killed\n", id().c_str());
	Game::remove_entity(id());
	Game::transaction(-cost(), grid_pos());
}

void Entity::damage(float amount){
	if ( !is_alive() ) return;

	hp -= amount;
	if ( hp <= 0.0f ){
		kill();
	}
}

void Entity::inc_ref() const {
	references++;
}

void Entity::dec_ref() const {
	if ( --references == 0 ){
		delete this;
	}
}
