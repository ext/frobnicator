#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "creep.hpp"
#include "game.hpp"
#include "waypoint.hpp"
#include <sstream>
#include <iomanip>

Creep::Creep(const Vector2f& pos, const Blueprint* blueprint, unsigned int level)
	: Entity(generate_id(), pos, blueprint, level)
	, left(Game::inner()) {
}

void Creep::add_buff(const SlowBuff& buf){
	slow_buff = buf;
}

void Creep::add_buff(const PoisonBuff& buf){
	poison_buff = buf;
}

const std::string Creep::generate_id(){
	static int n = 1;
	std::stringstream s;
	s << "creep_" << std::setfill('0') << std::setw(4) << n++;
	return s.str();
}

std::string Creep::get_region() const {
	return region;
}

Creep& Creep::set_region(const std::string& name){
	region = name;
	return *this;
}

Creep& Creep::set_dst(const Vector2f& dst){
	this->dst = dst;
	return *this;
}

void Creep::tick(float dt){
	const Vector2f d = (dst - pos - /* hack hack hack */ Vector2f(24,24)).normalized();
	pos += d * speed() * dt;

	slow_buff.tick(dt);
	poison_buff.tick(dt);
}

float Creep::speed() const {
	float base = Entity::speed();

	if ( slow_buff.duration > 0.0 ){
		base *= slow_buff.amount;
	}

	return base;
}

void Creep::on_enter_region(const Waypoint& region){
	if ( region.name() == "middle" ){
		kill(NULL);
		return;
	}

	std::string name = region.next();
	if ( --left == 0 ){
		name = region.inner();
		left = 7;
	}

	if ( name == "" ){
		fprintf(stderr, "Waypoint `%s' is missing next waypoint.\n", region.name().c_str());
		return;
	}

	const Waypoint* next = Game::find_waypoint(name);
	if ( !next ){
		fprintf(stderr, "Waypoint `%s' refers to non-existing waypoint `%s', ignored.\n", region.name().c_str(), region.next().c_str());
		return;
	}

	set_dst(next->middle());
}

void Creep::on_exit_region(const Waypoint& region){

}
