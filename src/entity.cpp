#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include "game.hpp"
#include <cstdio>
#include <yaml.h>

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
	} data[max_levels];
};

namespace Blueprint {
	/**
	 * Make a shallow copy of a blueprint where sprite and name is read-only.
	 */
	static const blueprint_t copy(const blueprint_t blueprint);
}

Entity::Entity(const Vector2f& pos, const blueprint_t bp)
	: pos(pos)
	, level(1)
	, blueprint(Blueprint::copy(bp)) {

	printf("creating entity at (%.0f,%.0f)\n", pos.x, pos.y);
}

const Vector2f& Entity::world_pos() const {
	return pos;
}

const Sprite* Entity::sprite() const {
	return blueprint->data[level].sprite;
}

Building::Building(const Vector2f& pos, const blueprint_t bp)
	: Entity(pos, bp) {

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

			const char* key = (const char*)event.data.scalar.value;
			const size_t len = event.data.scalar.length;

			yaml_parser_parse(parser, &evalue) || yaml_error(parser);
			const char* value = (const char*)evalue.data.scalar.value;
			const size_t value_len = evalue.data.scalar.length;

			/* Fill level with info */
			if ( strncmp("level", key, len) == 0 ){
				/* ignore */
			} else if ( strncmp("sprite", key, len) == 0 ){
				data->sprite = Game::load_sprite(std::string(value, value_len));
			} else if ( strncmp("name", key, len) == 0 ){
				free(data->name);
				data->name = strndup(value, value_len);
			} else if ( strncmp("cost", key, len) == 0 ){
				data->cost = atoi(value);
			} else if ( strncmp("splash", key, len) == 0 ){
				data->splash = atof(value);
			} else if ( strncmp("damage", key, len) == 0 ){
				data->damage = atof(value);
			} else if ( strncmp("rof", key, len) == 0 ){
				data->rof = atof(value);
			} else if ( strncmp("range", key, len) == 0 ){
				data->range = atof(value);
			} else if ( strncmp("slow", key, len) == 0 ){
				data->slow = atof(value);
			} else if ( strncmp("poison", key, len) == 0 ){
				data->poison = atof(value);
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%.*s'\n", (int)len, key);
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
}