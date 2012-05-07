#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sprite.hpp"
#include "common.hpp"
#include "game.hpp"
#include <yaml.h>

Sprite::Sprite()
	: _offset(0,0)
	, _scale(1,1) {

}

Sprite::~Sprite(){

}

Sprite* Sprite::from_yaml(yaml_parser_t* parser){
	Sprite* sprite = Game::create_sprite();

	yaml_event_t event;
	yaml_event_t evalue;

	yaml_parser_parse(parser, &event) || yaml_error(parser);

	if ( event.type != YAML_MAPPING_START_EVENT ){
		fprintf(stderr, "Sprite expected a mapping at line %lu:%lu\n",
		        parser->mark.line+1, parser->mark.column+1);
		abort();
	}

	do {
		yaml_parser_parse(parser, &event) || yaml_error(parser);

		if ( event.type == YAML_MAPPING_END_EVENT ){
			/* All elements has been consumed */
			return sprite;
		} else if ( event.type != YAML_SCALAR_EVENT ){
			/* For this purpose only strings are allowed as keys */
			fprintf(stderr, "YAML dict key must be string at line %lu:%lu\n",
			        parser->mark.line+1, parser->mark.column+1);
			abort();
		}

		const std::string key((const char*)event.data.scalar.value, event.data.scalar.length);

		if ( key == "texture" ){
			yaml_parser_parse(parser, &evalue) || yaml_error(parser);
			const std::string texture((const char*)evalue.data.scalar.value, evalue.data.scalar.length);
			sprite->load_texture(texture);
		} else if ( key == "offset" ){
			sprite->set_offset(Vector2f::from_yaml(parser));
		} else if ( key == "scale" ){
			sprite->set_scale(Vector2f::from_yaml(parser));
		} else {
			/* warning only */
			fprintf(stderr, "Unhandled key `%s'\n", key.c_str());
		}

	} while (1);
}
