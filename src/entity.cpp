#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include <cstdio>

Entity::Entity(const Vector2f& pos)
	: pos(pos){

	printf("creating entity at (%.0f,%.0f)\n", pos.x, pos.y);
}

const Vector2f& Entity::world_pos() const {
	return pos;
}
