#ifndef DVB021_ENTITY_H
#define DVB021_ENTITY_H

#include "common.hpp"

class Entity {
public:
	Entity(const Vector2f& pos);

	const Vector2f& world_pos() const;
	
private:
	Vector2f pos;
};

class Building: public Entity {

};

class Creep: public Entity {

};

#endif /* DVB021_ENTITY_H */
