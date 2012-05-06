#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "blueprint.hpp"
#include "common.hpp"
#include "entity.hpp"
#include "game.hpp"
#include "sprite.hpp"
#include <yaml.h>

Blueprint::Blueprint(){

}

const Blueprint* Blueprint::from_filename(const std::string& filename){
	auto bp = new Blueprint;

	const char* real_filename = real_path(filename.c_str());
	FILE* fp = fopen(real_filename, "rb");
	if ( !fp ){
		fprintf(stderr, "Failed to load blueprint `%s'\n", filename.c_str());
		exit(1);
	}

	fprintf(stderr, "Loading blueprint `%s'\n", filename.c_str());

	yaml_parser_t parser;
	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, fp);

	yaml_event_t event;
	yaml_parser_parse(&parser, &event) || yaml_error(&parser);
	if ( event.type != YAML_STREAM_START_EVENT ) abort();

	yaml_parser_parse(&parser, &event) || yaml_error(&parser);
	if ( event.type != YAML_DOCUMENT_START_EVENT ) abort();

	yaml_parser_parse(&parser, &event) || yaml_error(&parser);
	if ( event.type != YAML_SEQUENCE_START_EVENT ){
		fprintf(stderr, "YAML sequence expected\n");
		abort();
	}

	/* defaults */
	struct level current = {
		/* yey, not even c++0x supports designated initializers... =( */
		/* .sprite = */ NULL,
		/* .name   = */ std::string("unnamed tower"),
		/* .cost   = */ 100,
		/* .splash = */ 0.0f,
		/* .damage = */ 1.0f,
		/* .rof    = */ 1.0f,
		/* .range  = */ 100.0f,
		/* .slow   = */ 0.0f,
		/* .poison = */ 0.0f,
		/* .speed  = */ 0.0f,
		/* .armor  = */ 0.0f,
		/* .amount = */ 10,
	};

	bool done = false;
	do {
		yaml_parser_parse(&parser, &event) || yaml_error(&parser);
		switch ( event.type ){
		case YAML_SEQUENCE_END_EVENT:
			done = true;
			continue;

		case YAML_MAPPING_START_EVENT:
			break;

		default:
			fprintf(stderr, "YAML mapping expected\n");
			abort();
		}

		parse_leveldata(&current, &parser);
		bp->data.push_back(current);
	} while(!done);

	int n = 1;
	fprintf(stderr, "  * %zd levels loaded\n", bp->data.size());
	for ( auto it = bp->data.begin(); it != bp->data.end(); ++it ){
		fprintf(stderr, "    %2d: %s\n", n++, (*it).name.c_str());
	}

	yaml_parser_delete(&parser);
	fclose(fp);

	return bp;
}

void Blueprint::parse_leveldata(struct level* level, yaml_parser_t* parser){
	yaml_event_t event;
	yaml_event_t evalue;

	do {
		yaml_parser_parse(parser, &event) || yaml_error(parser);

		if ( event.type == YAML_MAPPING_END_EVENT ){
			/* All elements has been consumed */
			return;
		} else if ( event.type != YAML_SCALAR_EVENT ){
			/* For this purpose only strings are allowed as keys */
			fprintf(stderr, "YAML dict key must be string\n");
			abort();
		}

		std::string key((const char*)event.data.scalar.value, event.data.scalar.length);

		/* sprite requires special handling */
		if ( key == "sprite" ){
			level->sprite = Sprite::from_yaml(parser);
			continue;
		}

		yaml_parser_parse(parser, &evalue) || yaml_error(parser);
		const char* value = (const char*)evalue.data.scalar.value;
		const size_t len = evalue.data.scalar.length;

		/* Fill level with info */
		if ( key == "level" ){
			/* ignore */
		} else if ( key == "name"   ){ level->name = std::string(value, len);
		} else if ( key == "cost"   ){ level->cost = atoi(value);
		} else if ( key == "splash" ){ level->splash = (float)atof(value);
		} else if ( key == "damage" ){ level->damage = (float)atof(value);
		} else if ( key == "rof"    ){ level->rof = (float)atof(value);
		} else if ( key == "range"  ){ level->range = (float)atof(value);
		} else if ( key == "slow"   ){ level->slow = (float)atof(value);
		} else if ( key == "poison" ){ level->poison = (float)atof(value);
		} else if ( key == "speed"  ){ level->speed = (float)atof(value);
		} else if ( key == "armor"  ){ level->armor = (float)atof(value);
		} else if ( key == "amount" ){ level->amount = (float)atoi(value);
		} else {
			/* warning only */
			fprintf(stderr, "Unhandled key `%s'\n", key.c_str());
		}
	} while (1);
}
