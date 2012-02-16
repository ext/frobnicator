#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tilemap.hpp"
#include "common.hpp"
#include <yaml.h>

static const size_t max_tiles = 100;

class TilemapPimpl {
public:
	TilemapPimpl(const std::string& filename){
		const char* real_filename = real_path(filename.c_str());
		FILE* fp = fopen(real_filename, "rb");
		if ( !fp ){
			fprintf(stderr, "Failed to load tilemap `%s'\n", filename.c_str());
			exit(1);
		}

		printf("loading tilemap\n");

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
		for ( size_t i = 0; i < max_tiles; i++ ){
			if ( tileinfo[i].set ) continue;
			tileinfo[i] = default_tile;
		}
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

		printf("parse tilemap\n");

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
				fprintf(stderr, "data parsing not implemented\n");
				abort();
			} else {
				char tmp[len+1]; /* key is not null-terminated */
				strncpy(tmp, key, len);
				tmp[len] = 0;
				parse_tileinfo(parser, tmp);
			}
		} while ( true );
	}

	void parse_meta(yaml_parser_t* parser){
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
			const size_t value_len = evalue.data.scalar.length;

			/* Fill level with info */
			if ( strncmp("width", key, len) == 0 ){
				width = atoi(value);
			} else if ( strncmp("height", key, len) == 0 ){
				height = atoi(value);
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );
	}

	void parse_tileinfo(yaml_parser_t* parser, const char* tilerange){
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
			const char* value = (const char*)evalue.data.scalar.value;
			const size_t value_len = evalue.data.scalar.length;

			/* Fill level with info */
			if ( strncmp("build", key, len) == 0 ){
				cur.build = parse_bool(evalue);
				printf("build: %d\n", cur.build);
			} else if ( strncmp("height", key, len) == 0 ){
				height = atoi(value);
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );
	}

	int parse_bool(const yaml_event_t* event){
		if ( event.type != YAML_SCALAR_EVENT ){
			fprintf(stderr, "conversion to bool require scalar value\n");
			abort();
		}

		
	}

public:
	/* All members are public as the only one that can access them is Tilemap and
	 * creating getters for all of them would just be a waste of time. */

	unsigned int width;  /* width in tiles */
	unsigned int height; /* height in tiles */

private:
	struct Tilemap::Tile default_tile;
	struct Tilemap::Tile tileinfo[max_tiles]; /* fulhack */
};

Tilemap::Tilemap(const std::string& filename)
	: pimpl(new TilemapPimpl(filename) ) {
}

Tilemap::~Tilemap(){

}
