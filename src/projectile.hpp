#ifndef FROBNICATOR_PROJECTILE_H
#define FROBNICATOR_PROJECTILE_H

#include "vector.hpp"
#include <functional>

class Projectile {
public:
	/**
	 * Create new projectile. Will be added to game automatically upon creation.
	 * Do not deallocate memory, it will free itself when finished.
	 *
	 * @param speed Units per seconds.
	 * @param len Projectile length.
	 * @param ready Called once the projectile has hit the target.
	 */
	Projectile(const Vector2f& src, const Entity* dst, float speed, float len, const std::function<void()>& ready);

	/**
	 * Get the current start- and end-point of the projectiles.
	 */
	void get_points(Vector2f* a, Vector2f* b) const;

	/**
	 * @return true if finished.
	 */
	bool tick(float dt);

private:
	Vector2f src;
	const Entity* dst;
	std::function<void()> ready;
	float delay;
	float cur;
	float len;
};

#endif /* FROBNICATOR_PROJECTILE_H */
