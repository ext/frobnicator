#ifndef DVB021_COMMON_H
#define DVB021_COMMON_H

#include <math.h>

const char* real_path(const char* filename);
int yaml_error(struct yaml_parser_s* parser);

template <class T> T min(T a, T b){ return (a < b) ? a : b; }
template <class T> T max(T a, T b){ return (a > b) ? a : b; }
template <class T> T clamp(T v, T a, T b){
	if ( v < a ) return a;
	if ( v > b ) return b;
	return v;
}

#endif /* DVB021_COMMON_H */
