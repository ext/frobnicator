#ifndef DVB021_ENTITY_H
#define DVB021_ENTITY_H

#include "common.hpp"
#include <string>

class Entity {
public:
	/**
	 * Position in worldspace.
	 */
	const Vector2f& world_pos() const;

	/**
	 * Position on grid (truncated to first tile)
	 */
	const Vector2i grid_pos() const;

	const Sprite* sprite() const;
	const std::string name() const;

	/**
	 * Update entity. Should be called every frame.
	 */
	virtual void tick(float dt){}

	/**
	 * Get name of entity.
	 */
	const std::string id() const;

	int cost() const;
	float splash() const;
	float damage() const;
	float rof() const;
	float range() const;
	float slow() const;
	float poison() const;
	float speed() const;
	float armor() const;

	/**
	 * Kill this entity.
	 */
	void kill();

	virtual void on_kill(){}

protected:
	Entity(const std::string& id, const Vector2f& pos, const Blueprint* blueprint, unsigned int level);
	size_t level;
	Vector2f pos;

private:
	const std::string _id;
	const Blueprint* blueprint;
};

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

private:
	Building(const Vector2f& pos, const Blueprint* blueprint);

	static const std::string generate_id();
};

#endif /* DVB021_ENTITY_H */
