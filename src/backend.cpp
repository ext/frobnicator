#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "backend.hpp"

Backend::map Backend::factory_map;

Backend::Backend(){

}

Backend::~Backend(){

}

void Backend::register_factory(const std::string& name, Backend::factory_callback func){
	factory_map[name] = func;
}

Backend* Backend::create(const std::string& name){
	auto it = factory_map.find(name);
	if ( it == factory_map.end() ) return NULL;
	return it->second();
}
