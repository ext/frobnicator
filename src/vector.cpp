#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vector.hpp"
#include "common.hpp"
#include <cstdlib>
#include <yaml.h>

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
