#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include <cstdio>
#include <yaml.h>

static const size_t max_levels = 10;

struct blueprint {
	struct level {
		Sprite* sprite;
		const char* name;
		
		/* gameplay stats */
		float splash;
		float damage;
		float rof;
		float range;
		float slow;
		float poison;
	} data[max_levels];
	
};

Entity::Entity(const Vector2f& pos)
	: pos(pos){

	printf("creating entity at (%.0f,%.0f)\n", pos.x, pos.y);
}

const Vector2f& Entity::world_pos() const {
	return pos;
}

const blueprint from_filename(const std::string& filename){
	struct blueprint::level base;
	struct blueprint::level* prev = &base;
}

const Sprite* sprite(const blueprint bp){
	return bp->sprite;
}
