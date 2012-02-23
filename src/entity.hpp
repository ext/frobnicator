#ifndef DVB021_ENTITY_H
#define DVB021_ENTITY_H

#include "common.hpp"
#include <string>

class Sprite {

};

/* Act II
 * At this point I'm completely fed up with c++ classes.
 * Enter c-style object orientation with real encapsulation */

/**
 * A blueprint is essentially a flyweight for buildings.
 * Everything that is common between each instance of a building type is
 * held in the blueprint.
 */
typedef struct blueprint* blueprint_t;
namespace Blueprint {
	const blueprint_t from_filename(const std::string& filename);

	/* small hack to get creep amount for a level */
	unsigned int amount(blueprint_t bp, unsigned int level);
}

class Entity {
public:
	const Vector2f& world_pos() const;
	const Sprite* sprite() const;

protected:
	Entity(const Vector2f& pos, const blueprint_t blueprint);
	size_t level;

private:
	Vector2f pos;
	blueprint_t blueprint;
};


class Building: public Entity {
public:
	Building(const Vector2f& pos, const blueprint_t bp);
};

class Creep: public Entity {
public:
	Creep(const Vector2f& pos, const blueprint_t bp, unsigned int level);
};

#endif /* DVB021_ENTITY_H */
