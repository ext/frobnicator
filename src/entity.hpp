#ifndef DVB021_ENTITY_H
#define DVB021_ENTITY_H

#include "vector.hpp"
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
	float max_hp() const;
	float current_hp() const;

	/**
	 * Kill this entity.
	 */
	void kill();

	/**
	 * Damage this entity.
	 */
	void damage(float amount);

	virtual void on_kill(){}

protected:
	Entity(const std::string& id, const Vector2f& pos, const Blueprint* blueprint, unsigned int level);
	size_t level;
	Vector2f pos;
	float hp;

private:
	const std::string _id;
	const Blueprint* blueprint;
};

#endif /* DVB021_ENTITY_H */
