#ifndef FROBNICATOR_CREEP_H
#define FROBNICATOR_CREEP_H

#include "entity.hpp"
#include "buff.hpp"

class Creep: public Entity {
public:
	/**
	 * Spawn new creep at world space coordinate given by pos.
	 */
	static Creep* spawn_at(const Vector2f& pos, const Blueprint* blueprint, unsigned int level){
		return new Creep(pos, blueprint, level);
	}

	void add_buff(const SlowBuff& buf);
	void add_buff(const PoisonBuff& buf);

	/**
	 * Mark what region it currently is in.
	 */
	Creep& set_region(const std::string& name);

	/**
	 * Get what region it is currently in.
	 * @return Empty string if outside any region.
	 */
	std::string get_region() const;

	/**
	 * Set where it is going.
	 */
	Creep& set_dst(const Vector2f& dst);

	virtual void tick(float dt);

	virtual float speed() const;

	/** Triggers **/

	/**
	 * Called when entity enters a new region.
	 */
	void on_enter_region(const Waypoint& region);

	/**
	 * Called when entity exits a region.
	 */
	void on_exit_region(const Waypoint& region);

private:
	Creep(const Vector2f& pos, const Blueprint* blueprint, unsigned int level);

	static const std::string generate_id();

	Vector2f dst;
	std::string region;
	int left;
	SlowBuff slow_buff;
	PoisonBuff poison_buff;
};

#endif /* FROBNICATOR_CREEP_H */
