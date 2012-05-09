#ifndef FROBNICATOR_BUILDING_H
#define FROBNICATOR_BUILDING_H

#include "entity.hpp"

class Building: public Entity {
public:
	/**
	 * Construct a new building using blueprint bp and place it at the tile given
	 * by pos.
	 */
	static Building* place_at_tile(const Vector2i& pos, const Blueprint* blueprint){
		Vector2f world(pos.x * 48, pos.y * 48);
		return new Building(world, blueprint);
	}

	virtual void tick(float dt);

private:
	Building(const Vector2f& pos, const Blueprint* blueprint);

	static const std::string generate_id();

	bool can_fire() const;
};

#endif /* FROBNICATOR_BUILDING_H */
