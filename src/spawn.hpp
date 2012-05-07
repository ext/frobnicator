#ifndef FROBNICATOR_SPAWN_H
#define FROBNICATOR_SPAWN_H

#include "region.hpp"

class Spawnpoint: public Region {
public:
	static Spawnpoint* from_yaml(yaml_parser_t* parser){
		auto ptr = new Spawnpoint;
		ptr->parse(parser);
		return ptr;
	}

	virtual void set(const std::string& key, const std::string& value){
		if ( key == "next" ){  next = value; }
		else { Region::set(key, value); }
	}

	std::string next; /* hack... */

private:
	Spawnpoint(){

	}
};

#endif /* FROBNICATOR_SPAWN_H */
