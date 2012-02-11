#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.hpp"
#include <cstdio>
#include <cstdlib>
#include <yaml.h>

static const char* datapath(){
	const char* path = getenv("DATA_DIR");
	if ( !path ){
		path = DATA_DIR;
	}
	return path;
}

/**
 * Expands the path to the data-directory. Returns memory to static memory which
 * will be overwritten between successive calls.
 */
const char* real_path(const char* filename){
	static char buffer[4096];

	if ( filename[0] == '/' || filename[1] == ':' ){
		return filename;
	}

	if ( snprintf(buffer, 4096, "%s/%s", datapath(), filename) == -1 ){
		abort();
	}

	return buffer;
}

int yaml_error(yaml_parser_t* parser){
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
		} else {
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
		} else {
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

	exit(1);
}
