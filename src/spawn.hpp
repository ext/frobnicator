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

private:
	Spawnpoint(){

	}
};

#endif /* FROBNICATOR_SPAWN_H */
