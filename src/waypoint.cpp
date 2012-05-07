#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "waypoint.hpp"

Waypoint::Waypoint(){

}

Waypoint* Waypoint::from_yaml(yaml_parser_t* parser){
	auto ptr = new Waypoint;
	ptr->parse(parser);
	return ptr;
}

void Waypoint::set(const std::string& key, const std::string& value){
	     if ( key == "inner" ){ _inner = value; }
	else if ( key == "next" ){  _next = value; }
	else { Region::set(key, value); }
}
