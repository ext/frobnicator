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
	, left(7) {
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

	const Waypoint* next = Game::find_waypoint(name);
	if ( !next ){
		fprintf(stderr, "Waypoint `%s' refers to non-existing waypoint `%s', ignored.\n", region.name().c_str(), region.next().c_str());
		return;
	}

	set_dst(next->middle());
}

void Creep::on_exit_region(const Waypoint& region){

}
