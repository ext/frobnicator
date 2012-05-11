#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "projectile.hpp"
#include "common.hpp"
#include "entity.hpp"
#include "game.hpp"
#include "sprite.hpp"

Projectile::Projectile(const Vector2f& src, const Entity* dst, float speed, float len, const std::function<void()>& ready)
	: src(src)
	, dst(dst)
	, ready(ready )
	, delay(Vector2f::distance(src, dst->world_pos()) / speed)
	, cur(0.0f)
	, len(len) {

	Game::add_projectile(this);
	dst->inc_ref();
}

Projectile::~Projectile(){
	dst->dec_ref();
}

void Projectile::get_points(Vector2f* a, Vector2f* b) const {
	const float s = cur / delay;

	const Vector2f offset = Vector2f(
		Game::tile_width() * dst->sprite()->scale().x * 0.5f,
		Game::tile_width() * dst->sprite()->scale().y * 0.5f
	);
	const Vector2f real_dst = dst->world_pos() + offset;
	const float distance = Vector2f::distance(src, real_dst);
	const float l = len / distance;

	*a = Vector2f::lerp(src, real_dst, min(s, 1.0f));
	*b = Vector2f::lerp(src, real_dst, min(s+l, 1.0f));
}

bool Projectile::tick(float dt){
	if ( cur > delay ){
		ready();
		return true;
	}

	cur += dt;
	return false;
}
