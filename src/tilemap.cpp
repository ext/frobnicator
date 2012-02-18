#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tilemap.hpp"
#include "common.hpp"
#include <yaml.h>
#include <vector>

static constexpr size_t max_tiles = 100;

class TilemapPimpl {
public:
	TilemapPimpl(const std::string& filename)
		: map_width(0)
		, map_height(0)
		, map_size(-1)
		, tiles_horizontal(0)
		, tiles_vertical(0)
		, tiles_size(-1)
		, meta_set(false) {

		const char* real_filename = real_path(filename.c_str());
		FILE* fp = fopen(real_filename, "rb");
		if ( !fp ){
			fprintf(stderr, "Failed to load tilemap `%s'\n", filename.c_str());
			exit(1);
		}

		fprintf(stderr, "Loading tilemap `%s'\n", filename.c_str());

		/* reset all tile info */
		for ( size_t i = 0; i < max_tiles; i++ ){
			tileinfo[i].set = 0;
		}

		yaml_parser_t parser;
		yaml_parser_initialize(&parser);

		yaml_parser_set_input_file(&parser, fp);
		parse_doc(&parser);

		yaml_parser_delete(&parser);
		fclose(fp);

		/* fill default value for unset tiles */
		fprintf(stderr, "  preparing tiles\n");
		const float dx = 1.0f / tiles_horizontal;
		const float dy = 1.0f / tiles_vertical;
		for ( size_t i = 0; i < tiles_size; i++ ){
			if ( !tileinfo[i].set ){
				tileinfo[i] = default_tile;
			}

			const unsigned int x = i % tiles_horizontal;
			const unsigned int y = i / tiles_horizontal;
			tileinfo[i].uv.s = (x  )*dx;
			tileinfo[i].uv.t = (y  )*dy;
			tileinfo[i].uv.u = (x+1)*dx;
			tileinfo[i].uv.v = (y+1)*dy;
		}
		fprintf(stderr, "    %d tiles loaded\n", tiles_size);
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

		parse_tilemap(parser);
	}

	void parse_tilemap(yaml_parser_t* parser){
		yaml_event_t event;

		do {
			/* Parse key */
			yaml_parser_parse(parser, &event) || yaml_error(parser);

			if ( event.type == YAML_MAPPING_END_EVENT ){
				/* All elements has been consumed */
				break;
			} else if ( event.type != YAML_SCALAR_EVENT ){
				/* For this purpose only strings are allowed as keys */
				fprintf(stderr, "YAML dict key must be string\n");
				abort();
			}

			const char* key = (const char*)event.data.scalar.value;
			const size_t len = event.data.scalar.length;

			/* Fill level with info */
			if ( strncmp("meta", key, len) == 0 ){
				parse_meta(parser);
			} else if ( strncmp("data", key, len) == 0 ){
				parse_data(parser);
			} else {
				char tmp[len+1]; /* key is not null-terminated */
				strncpy(tmp, key, len);
				tmp[len] = 0;
				parse_tileinfo(parser, tmp);
			}
		} while ( true );
	}

	void parse_meta(yaml_parser_t* parser){
		fprintf(stderr, "  parsing metadata\n");
		/* Ensure meta is a dict */
		yaml_event_t event;
		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_MAPPING_START_EVENT ){
			fprintf(stderr, "Tilemap meta must be dictionary.\n");
			abort();
		}

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

			/* Fill level with info */
			if ( strncmp("width", key, len) == 0 ){
			  map_width = atoi(value);
			} else if ( strncmp("height", key, len) == 0 ){
				map_height = atoi(value);
			} else if ( strncmp("tiles_horizontal", key, len) == 0 ){
				tiles_horizontal = atoi(value);
			} else if ( strncmp("tiles_vertical", key, len) == 0 ){
				tiles_vertical = atoi(value);
			} else {
				/* warning only */
				fprintf(stderr, "    - Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );

		map_size = map_width * map_height;
		tiles_size = tiles_horizontal * tiles_vertical;
		meta_set = true;

		fprintf(stderr, "    * size: %dx%d (%d)\n", map_width, map_height, map_size);
		fprintf(stderr, "    * tileset: %dx%d (%d)\n", tiles_horizontal, tiles_vertical, tiles_size);
	}

	void parse_tileinfo(yaml_parser_t* parser, char* tilerange){
		struct Tilemap::Tile cur;

		/* Ensure meta is a dict */
		yaml_event_t event;
		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_MAPPING_START_EVENT ){
			fprintf(stderr, "Tileinfo must be dictionary.\n");
			abort();
		}

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

			/* Fill level with info */
			if ( strncmp("build", key, len) == 0 ){
				cur.build = parse_bool(&evalue);
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );

		if ( strcmp(tilerange, "default") == 0 ){
			default_tile = cur;
			return;
		}

		/* determine range */
		int lower;
		int upper;
		char* delim = strchr(tilerange, '-');
		if ( !delim ){ /* single tile only */
			const int index = atoi(tilerange);
			lower = upper = index;
		} else {
			*delim++ = 0;
			lower = atoi(tilerange);
			upper = atoi(delim);
		}

		for ( int i = lower; i <= upper; i++ ){
			tileinfo[i] = cur;
			tileinfo[i].set = 1;
		}
	}

	void parse_data(yaml_parser_t* parser){
		fprintf(stderr, "  parsing data\n");

		if ( !meta_set ){
			fprintf(stderr, "tilemap meta not set (yet), ensure meta section is available before data section\n");
			abort();
		}

		/* Ensure data is a list */
		yaml_event_t event;
		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_SEQUENCE_START_EVENT ){
			fprintf(stderr, "Tilemap data must be list.\n");
			abort();
		}

		yaml_event_t value;

		do {
			/* Parse key */
			yaml_parser_parse(parser, &value) || yaml_error(parser);

			if ( value.type == YAML_SEQUENCE_END_EVENT ){
				/* All elements has been consumed */
				break;
			} else if ( value.type != YAML_SCALAR_EVENT ){
				/* For this purpose only strings are allowed as keys */
				fprintf(stderr, "Only scalar allowed in tilemap data\n");
				abort();
			}

			const unsigned int index = atoi((const char*)value.data.scalar.value);

			if ( index >= tiles_size ){
				fprintf(stderr, "warning: tile value to great, got %d max %d, defaulting to 0\n", index, tiles_size-1);
				tile.push_back(0);
				continue;
			}

			tile.push_back(index);
		} while ( true );

		/* warn if there was an unexpected number of tiles */
		if ( tile.size() < map_size ){
			fprintf(stderr, "warning: too few tiles in data\n");
		} else if ( tile.size() > map_size ){
			fprintf(stderr, "warning: too many tiles in data\n");
		}
	}

	/**
	 * "yes", "true", "1", "on" -> 1
	 * * -> 0
	 */
	int parse_bool(const yaml_event_t* event){
		if ( event->type != YAML_SCALAR_EVENT ){
			fprintf(stderr, "conversion to bool require scalar value\n");
			abort();
		}

		if ( !event->data.scalar.plain_implicit ){
			fprintf(stderr, "bool is only handled as plain_implict\n");
			abort();
		}

		const char* key = (const char*)event->data.scalar.value;
		const size_t len = event->data.scalar.length;

		return
			strncasecmp("yes",key,len) == 0 ||
			strncasecmp("true",key,len) == 0 ||
			strncasecmp("1",key,len) == 0 ||
			strncasecmp("on",key,len) == 0;
	}

public:
	/* All members are public as the only one that can access them is Tilemap and
	 * creating getters for all of them would just be a waste of time. */

	unsigned int map_width;        /* width in tiles */
	unsigned int map_height;       /* height in tiles */
	unsigned int map_size;         /* width * height */
	unsigned int tiles_horizontal; /* how many horizontal tiles in texture */
	unsigned int tiles_vertical;   /* how many vertical tiles in texture */
	unsigned int tiles_size;       /* h * v */
	std::vector<int> tile;

private:
	bool meta_set;
	struct Tilemap::Tile default_tile;
	struct Tilemap::Tile tileinfo[max_tiles]; /* fulhack */
};

Tilemap::Tilemap(const std::string& filename)
	: pimpl(new TilemapPimpl(filename) ) {
}

Tilemap::~Tilemap(){

}

size_t Tilemap::size() const {
	return pimpl->map_size;
}

size_t Tilemap::width() const {
	return pimpl->map_width;
}
