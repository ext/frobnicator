#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include "game.hpp"
#include <cstdio>
#include <yaml.h>

#ifdef WIN32
#define strdup _strdup
extern "C" char* strndup(const char* src, size_t n);
#endif

static const size_t max_levels = 10;

struct blueprint {
	struct level {
		Sprite* sprite;
		char* name;

		/* gameplay stats */
		int cost;
		float splash;
		float damage;
		float rof;
		float range;
		float slow;
		float poison;
		float speed;
		float armor;
		int amount;
	} data[max_levels];
};

namespace Blueprint {
	/**
	 * Make a shallow copy of a blueprint where sprite and name is read-only.
	 */
	static const blueprint_t copy(const blueprint_t blueprint);
}

Entity::Entity(const Vector2f& pos, const blueprint_t bp)
	: level(1)
	, pos(pos)
	, blueprint(Blueprint::copy(bp)) {

}

const Vector2f& Entity::world_pos() const {
	return pos;
}

const Vector2i Entity::grid_pos() const {
	return Vector2i(pos.x / Game::tile_width(), pos.y / Game::tile_height());
}

const Sprite* Entity::sprite() const {
	return blueprint->data[level].sprite;
}

Building::Building(const Vector2f& pos, const blueprint_t bp)
	: Entity(pos, bp) {

	fprintf(stderr, "Creating \"%s\" at (%.0f,%.0f)\n", bp->data[0].name, pos.x, pos.y);
}

Creep::Creep(const Vector2f& pos, const blueprint_t bp, unsigned int level)
	: Entity(pos, bp) {
	this->level = level;

	fprintf(stderr, "Spawning \"%s\" at (%.0f,%.0f)\n", bp->data[level].name, pos.x, pos.y);

	fprintf(stderr, "%s %s %s\n", bp->data[0].name, bp->data[1].name, bp->data[2].name);
}

namespace Blueprint {
	void fill_blueprint(blueprint::level* data, yaml_parser_t* parser){
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

			yaml_parser_parse(parser, &evalue) || yaml_error(parser);
			const char* value = (const char*)evalue.data.scalar.value;
			const size_t value_len = evalue.data.scalar.length;

			/* Fill level with info */
			if ( key == "level" ){
				/* ignore */
			} else if ( key == "sprite" ){ data->sprite = Game::load_sprite(std::string(value, value_len));
			} else if ( key == "name"   ){ free(data->name); data->name = strndup(value, value_len);
			} else if ( key == "cost"   ){ data->cost = atoi(value);
			} else if ( key == "splash" ){ data->splash = (float)atof(value);
			} else if ( key == "damage" ){ data->damage = (float)atof(value);
			} else if ( key == "rof"    ){ data->rof = (float)atof(value);
			} else if ( key == "range"  ){ data->range = (float)atof(value);
			} else if ( key == "slow"   ){ data->slow = (float)atof(value);
			} else if ( key == "poison" ){ data->poison = (float)atof(value);
			} else if ( key == "speed"  ){ data->speed = (float)atof(value);
			} else if ( key == "armor"  ){ data->armor = (float)atof(value);
			} else if ( key == "amount" ){ data->amount = (float)atoi(value);
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%s'\n", key.c_str());
			}

		} while (1);
	}

	static const blueprint_t copy(const blueprint_t blueprint){
		blueprint_t tmp = (blueprint_t)malloc(sizeof(struct blueprint));

		/* In general it would be a bad idea, e.g free(name) would corrupt a lot of
		 * instances, but as it is used in a very strict and controlled way it is
		 * deemed safe. */
		memcpy(tmp, blueprint, sizeof(struct blueprint));

		return tmp;
	}

	const blueprint_t from_filename(const std::string& filename){
		blueprint* bp = (blueprint*)malloc(sizeof(blueprint));

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
		unsigned int level = 0;
		struct blueprint::level current = {
			/* yey, not even c++0x supports designated initializers... =( */
			/* .sprite = */ NULL,
			/* .name   = */ strdup("unnamed tower"),
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

			fill_blueprint(&current, &parser);
			bp->data[level++] = current;
		} while(!done);

		yaml_parser_delete(&parser);
		fclose(fp);

		return bp;
	}

	unsigned int amount(blueprint_t bp, unsigned int level){
		return bp->data[level].amount;
	}
}
