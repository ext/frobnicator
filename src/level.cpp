#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "level.hpp"
#include "common.hpp"
#include <yaml.h>
#include <inttypes.h>


static int yaml_error(yaml_parser_t* parser){
	switch (parser->error){
	case YAML_MEMORY_ERROR:
		fprintf(stderr, "Not enough memory for parsing\n");
		break;

	case YAML_READER_ERROR:
		if (parser->problem_value != -1) {
			fprintf(stderr, "Reader error: %s: #%X at %lu\n", parser->problem, parser->problem_value, parser->problem_offset);
		} else {
			fprintf(stderr, "Reader error: %s at %lu\n", parser->problem, parser->problem_offset);
		}
		break;

	case YAML_SCANNER_ERROR:
		if (parser->context) {
			fprintf(stderr, "Scanner error: %s at line %lu, column %lu\n"
			        "%s at line %lu, column %lu\n", parser->context,
			        parser->context_mark.line+1, parser->context_mark.column+1,
			        parser->problem, parser->problem_mark.line+1,
			        parser->problem_mark.column+1);
		}
		else {
			fprintf(stderr, "Scanner error: %s at line %lu, column %lu\n",
			        parser->problem, parser->problem_mark.line+1,
			        parser->problem_mark.column+1);
		}
		break;

	case YAML_PARSER_ERROR:
		if (parser->context) {
			fprintf(stderr, "Parser error: %s at line %lu, column %lu\n"
			        "%s at line %lu, column %lu\n", parser->context,
			        parser->context_mark.line+1, parser->context_mark.column+1,
			        parser->problem, parser->problem_mark.line+1,
			        parser->problem_mark.column+1);
		}
		else {
			fprintf(stderr, "Parser error: %s at line %lu, column %lu\n",
			        parser->problem, parser->problem_mark.line+1,
			        parser->problem_mark.column+1);
		}
		break;

	default:
		/* Couldn't happen. */
		fprintf(stderr, "Internal error\n");
		break;
	}

	abort();
}

void parse_level(Level* level, yaml_parser_t* parser){
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
			if ( strncmp("title", key, len) == 0 ){
				level->set_title(std::string((const char*)evalue.data.scalar.value, evalue.data.scalar.length));
			} else {
				/* warning only */
				fprintf(stderr, "Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );
}

static void parse_doc(Level* level, yaml_parser_t* parser){
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

	parse_level(level, parser);
}

Level::Level(const std::string& filename)
	: _title("untitled level") {

	fprintf(stderr, "Loading from `%s'.\n", filename.c_str());

	FILE* fp = fopen(real_path(filename.c_str()), "rb");
	if ( !fp ){
		/* fatal */
		fprintf(stderr, "Failed to load level `%s'\n", filename.c_str());
		exit(1);
	}

	yaml_parser_t parser;
	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, fp);
	parse_doc(this, &parser);

	yaml_parser_delete(&parser);
	fclose(fp);

	fprintf(stderr, "Loaded level \"%s\".\n", _title.c_str());
}

Level::~Level(){

}

void Level::set_title(const std::string& title){
	_title = title;
}
