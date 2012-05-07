#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "region.hpp"
#include "common.hpp"
#include <yaml.h>

Region::Region(){

}

Vector2f Region::random_point() const {
	const int rx = rand() % _w;
	const int ry = rand() % _h;
	return Vector2f(_x + rx, _y + ry);
}

void Region::set(const std::string& key, const std::string& value){
	if ( key == "name" ){ _name = value; }
	if ( key == "x" ){ _x = atoi(value.c_str()); }
	if ( key == "y" ){ _y = atoi(value.c_str()); }
	if ( key == "w" ){ _w = atoi(value.c_str()); }
	if ( key == "h" ){ _h = atoi(value.c_str()); }
}

void Region::parse(yaml_parser_t* parser){
	yaml_event_t ekey;
	yaml_event_t eval;

	do {
		/* parse key */
		yaml_parser_parse(parser, &ekey) || yaml_error(parser);

		if ( ekey.type == YAML_MAPPING_END_EVENT ){
			/* finished */
			break;
		} else if ( ekey.type != YAML_SCALAR_EVENT ){
			/* malformed yaml */
			fprintf(stderr, "YAML dict key must be string\n");
			abort();
		}

		/* Parse value */
		yaml_parser_parse(parser, &eval) || yaml_error(parser);

		/* delegate parsing of value */
		const std::string key = std::string((const char*)ekey.data.scalar.value, ekey.data.scalar.length);
		const std::string val = std::string((const char*)eval.data.scalar.value, eval.data.scalar.length);
		this->set(key, val);

	} while ( true );
}
