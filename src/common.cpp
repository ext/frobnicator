#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.hpp"
#include <cstdio>
#include <cstdlib>
#include <yaml.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

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

template <class T>
static T scalar_conv(const char* s);

template <>
float scalar_conv<float>(const char* s){
	return (float)atof(s);
}

template <>
int scalar_conv<int>(const char* s){
	return atoi(s);
}

template<class T>
Vector<T> Vector<T>::from_yaml(yaml_parser_t* parser){
	Vector<T> v;

	yaml_event_t event;
	yaml_parser_parse(parser, &event) || yaml_error(parser);
	if ( event.type != YAML_SEQUENCE_START_EVENT ){
		fprintf(stderr, "Vector::YAML sequence expected at %lu:%lu\n",
		        parser->mark.line+1, parser->mark.column+1);
		abort();
	}

	int n = 0;
	do {
		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type == YAML_SEQUENCE_END_EVENT ){
			break;
		} else if ( event.type != YAML_SCALAR_EVENT ){
			fprintf(stderr, "Vector::YAML expected scalar at %lu:%lu\n",
			        parser->mark.line+1, parser->mark.column+1);
			abort();
		}

		if ( n == 2 ){
			fprintf(stderr, "Vector::YAML got too many values, ignoring\n");
			continue;
		}

		v.v[n++] = scalar_conv<T>((const char*)event.data.scalar.value);
	} while ( true );

	return v;
};

/* instantiate templates to get Vector::from_yaml */
template class Vector<float>;
template class Vector<int>;
