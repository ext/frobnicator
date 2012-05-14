#ifndef FROBNICATOR_BUILDING_H
#define FROBNICATOR_BUILDING_H

#include "entity.hpp"
#include "buff.hpp"

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

	bool have_slow() const;
	bool have_poison() const;
	bool can_upgrade() const;
	int upgrade_cost() const;
	int sell_cost() const;
	void upgrade();
	void sell();

private:
	Building(const Vector2f& pos, const Blueprint* blueprint);

	static const std::string generate_id();

	bool can_fire() const;
	bool have_target() const;
	void fire_at(Creep* creep);

	SlowBuff slow_buff() const;
	PoisonBuff poison_buff() const;

	std::string target;
	uint64_t firing_delta;
	uint64_t last_firing;
};

#endif /* FROBNICATOR_BUILDING_H */
