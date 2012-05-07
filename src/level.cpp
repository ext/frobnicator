#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "level.hpp"
#include "blueprint.hpp"
#include "game.hpp"
#include "tilemap.hpp"
#include "entity.hpp"
#include "common.hpp"
#include <yaml.h>
#include <cassert>
#include <algorithm>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

class LevelPimpl {
public:
	LevelPimpl(const std::string& filename)
		: title("untitled level")
		, tilemap(NULL) {

		const char* real_filename = real_path(filename.c_str());
		fprintf(stderr, "Loading level `%s'.\n", filename.c_str());

		FILE* fp = fopen(real_filename, "rb");
		if ( !fp ){
			fprintf(stderr, "Failed to load level `%s'\n", filename.c_str());
			exit(1);
		}

		yaml_parser_t parser;
		yaml_parser_initialize(&parser);

		yaml_parser_set_input_file(&parser, fp);
		parse_doc(&parser);

		yaml_parser_delete(&parser);
		fclose(fp);

		/* sanity check */
		if ( !tilemap ){
			fprintf(stderr, "Level missing tilemap\n");
			exit(1);
		}

		fprintf(stderr, "Loaded level \"%s\".\n", title.c_str());
	}

	std::vector<Entity*> spawn(unsigned int level, const Region& region){
		if ( level>= waves->num_levels() ){
			fprintf(stderr, "  Wave not defined\n");
			return std::vector<Entity*>();
		}

		const size_t amount = waves->amount(level);
		fprintf(stderr, "  Spawning %zd units at %s\n", amount, region.name().c_str());

		auto tmp = std::vector<Entity*>(amount);
		std::generate(tmp.begin(), tmp.end(), [this, level, region](){
			return Creep::spawn_at(region.random_point(Vector2i(48,48)), waves, level);
		});
		return tmp;
	}

	~LevelPimpl() {
		delete tilemap;
	}

private:
	void parse_doc(yaml_parser_t* parser){
		yaml_event_t event;
		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_STREAM_START_EVENT ) abort();

		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_DOCUMENT_START_EVENT ) abort();

		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_MAPPING_START_EVENT ){
			fprintf(stderr, "YAML Mapping expected\n");
			abort();
		}

		parse_level(parser);
	}

	void parse_level(yaml_parser_t* parser){
		yaml_event_t ekey;
		yaml_event_t evalue;

		do {
			/* Parse key */
			yaml_parser_parse(parser, &ekey) || yaml_error(parser);

			if ( ekey.type == YAML_MAPPING_END_EVENT ){
				/* All elements has been consumed */
				break;
			} else if ( ekey.type != YAML_SCALAR_EVENT ){
				/* For this purpose only strings are allowed as keys */
				fprintf(stderr, "YAML dict key must be string\n");
				abort();
			}

			const char* key = (const char*)ekey.data.scalar.value;
			const size_t len = ekey.data.scalar.length;

			/* Parse value */
			yaml_parser_parse(parser, &evalue) || yaml_error(parser);
			const char* value = (const char*)evalue.data.scalar.value;
			const size_t value_len = evalue.data.scalar.length;

			/* Fill level with info */
			if ( strncmp("title", key, len) == 0 ){
				title = std::string(value, value_len);
			} else if ( strncmp("tilemap", key, len) == 0 ){
				tilemap = Game::load_tilemap(std::string(value, value_len));
			} else if ( strncmp("waves", key, len) == 0 ){
				waves = Blueprint::from_filename(std::string(value, value_len));
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );
	}

	const Blueprint* waves;

public:
	/* All members are public as the only one that can access them is Level and
	 * creating getters for all of them would just be a waste of time. */
	std::string title;
	Tilemap* tilemap;
};

Level::Level(const std::string& filename)
	: pimpl(new LevelPimpl(filename)) {

}

Level::~Level(){
	delete pimpl;
}

Level* Level::from_filename(const std::string& filename){
	return new Level(filename);
}

const std::string& Level::title() const {
	return pimpl->title;
}

const Tilemap& Level::tilemap() const {
	assert(pimpl->tilemap);
	return *pimpl->tilemap;
}

const std::map<std::string, Waypoint*>& Level::waypoints() const {
	return tilemap().waypoints();
}

const std::map<std::string, Spawnpoint*>& Level::spawnpoints() const {
	return tilemap().spawnpoints();
}

std::vector<Entity*> Level::spawn(unsigned int level, const Region& region) const {
	return pimpl->spawn(level, region);
}
