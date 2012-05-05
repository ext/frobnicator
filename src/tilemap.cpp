#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tilemap.hpp"
#include "common.hpp"
#include "region.hpp"

#include <yaml.h>
#include <vector>
#include <map>

static const size_t max_tiles = 500;

#ifdef WIN32
#define strncasecmp _strnicmp
#define strncpy(dst, src, n) strncpy_s(dst, n, src, _TRUNCATE)
#endif

class Waypoint: public Region {
public:
	static Waypoint* from_yaml(yaml_parser_t* parser){
		auto ptr = new Waypoint;
		ptr->parse(parser);
		return ptr;
	}

	virtual void set(const std::string& key, const std::string& value){
		if ( key == "inner" ){ inner = value; }
		if ( key == "next" ){ next = value; }
		Region::set(key, value);
	}

private:
	Waypoint(){

	}

public:
	std::string inner; /* name of the next inner waypoint */
	std::string next;  /* name of the next waypoint */
};

class Spawnpoint: public Region {
public:
	static Spawnpoint* from_yaml(yaml_parser_t* parser){
		auto ptr = new Spawnpoint;
		ptr->parse(parser);
		return ptr;
	}

private:
	Spawnpoint(){

	}
};

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
		fprintf(stderr, "  preparing tiledata\n");
		const float dx = 1.0f / tiles_horizontal;
		const float dy = 1.0f / tiles_vertical;
		for ( size_t i = 0; i < tiles_size; i++ ){
			struct Tilemap::Tile& info = tileinfo[i];

			if ( !info.set ){
				info = default_tile;
			}

			const unsigned int x = i % tiles_horizontal;
			const unsigned int y = i / tiles_horizontal;
			info.index = i;
			float s = x * dx;
			float t = y * dy;
			info.uv[0] = s;
			info.uv[1] = t;
			info.uv[2] = s + dx;
			info.uv[3] = t;
			info.uv[4] = s + dx;
			info.uv[5] = t + dy;
			info.uv[6] = s;
			info.uv[7] = t + dy;
		}
		fprintf(stderr, "    * %d tiles loaded\n", tiles_size);

		fprintf(stderr, "  preprocessing map grid\n");
		int n = 0;
		for ( auto it = tile.begin(); it != tile.end(); ++it ){
			Tilemap::Tile& tile = *it;
			tile = tileinfo[tile.index];
			tile.x = n % map_width;
			tile.y = n / map_width;
			n++;
		}
		fprintf(stderr, "    * %zd cells loaded\n", tile.size());
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

			const char* inkey = (const char*)event.data.scalar.value;
			const size_t len = event.data.scalar.length;

			/* key is not null-terminated */
			char key[64]; /* variable sized array not supported, using max 64 bytes which should be enough */
			if ( len > 63 ) abort(); /* 63 because null-terminator must fit into array */
			sprintf(key, "%.*s", (int)len, inkey); /* using sprintf to always get a null-terminator, strcpy might not always add one */

			/* Fill level with info */
			if ( strcmp("meta", key) == 0 ){
				parse_meta(parser);
			} else if ( strcmp("data", key) == 0 ){
				parse_data(parser);
			} else if ( strncmp("tile", key, 4) == 0 || strcmp("default", key) == 0 ){
				parse_tileinfo(parser, key);
			} else if ( strcmp("waypoint", key) == 0 ){
				fprintf(stderr, "  parsing waypoints\n");
				parse_region<Waypoint>(parser, [this](Waypoint* wp){
					waypoint[wp->name()] = wp;
				});
				fprintf(stderr, "    * %zd waypoints loaded\n", waypoint.size());
			} else if ( strcmp("spawn", key) == 0 ){
				fprintf(stderr, "  parsing spawnpoints\n");
				parse_region<Spawnpoint>(parser, [this](Spawnpoint* r){
					spawnpoint[r->name()] = r;
				});
				fprintf(stderr, "    * %zd spawnpoints loaded\n", spawnpoint.size());
			} else {
				/* warning only */
				fprintf(stderr, "  - Unhandled key `%.*s'\n", (int)len, key);
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
			} else if ( strncmp("texture", key, len) == 0 ){
				texture_name = std::string(value, evalue.data.scalar.length);
			} else {
				/* warning only */
				fprintf(stderr, "    - Unhandled key `%.*s'\n", (int)len, key);
			}
		} while ( true );

		map_size = map_width * map_height;
		tiles_size = tiles_horizontal * tiles_vertical;
		meta_set = true;

		if ( tiles_size >= max_tiles ){
			fprintf(stderr, "too many tiles, max is %zd\n", max_tiles);
			abort();
		}

		fprintf(stderr, "    * tile size: %dx%d (%d)\n", map_width, map_height, map_size);
		fprintf(stderr, "    * num tiles: %dx%d (%d)\n", tiles_horizontal, tiles_vertical, tiles_size);
		fprintf(stderr, "    * texture: %s\n", texture_name.c_str());
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

		/* cut "tile[" prefix */
		tilerange += 5;

		/* ensure range is digits */
		if ( !isdigit(tilerange[0]) ){
			fprintf(stderr, "invalid tile range: `%s', ignored\n", tilerange);
			return;
		}

		/* determine range */
		unsigned int lower;
		unsigned int upper;
		char* delim = strchr(tilerange, '-');
		if ( !delim ){ /* single tile only */
			const int index = atoi(tilerange);
			lower = upper = index;
		} else {
			*delim++ = 0;
			lower = atoi(tilerange);
			upper = atoi(delim);
		}

		if ( lower > upper || upper > max_tiles ){
			fprintf(stderr, "  invalid tile range %d-%d, must be min <= max <= %zd\n", lower, upper, max_tiles);
		}

		for ( unsigned int i = lower; i <= upper; i++ ){
			tileinfo[i] = cur;
			tileinfo[i].set = 1;
		}
	}

	template<class T, typename F>
	void parse_region(yaml_parser_t* parser, F func){
		/* Ensure we get a list */
		yaml_event_t event;
		yaml_parser_parse(parser, &event) || yaml_error(parser);
		if ( event.type != YAML_SEQUENCE_START_EVENT ){
			fprintf(stderr, "Waypoints must be contained in a list.\n");
			abort();
		}

		do {
			yaml_parser_parse(parser, &event) || yaml_error(parser);

			if ( event.type == YAML_SEQUENCE_END_EVENT ){
				break;
			}

			if ( event.type != YAML_MAPPING_START_EVENT ){
				fprintf(stderr, "Region definition must be dictionary.\n");
				abort();
			}

			T* ptr = T::from_yaml(parser);
			func(ptr);
		} while ( true );
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

			const size_t tiles_size = this->tiles_size; /* msvc fun again */
			const unsigned int index = [&value, tiles_size]() -> unsigned int {
				unsigned int tmp = atoi((const char*)value.data.scalar.value);
				if ( tmp > 0 ) tmp--; /* tiled uses 1 as first index and 0 as "no tile" */
				if ( tmp >= tiles_size ){
					fprintf(stderr, "warning: tile value to great, got %d max %ld, defaulting to 0\n", tmp, tiles_size-1);
					return 0;
				} else {
					return tmp;
				}
			}();

			Tilemap::Tile tmp;
			tmp.index = index;
			tile.push_back(tmp);
		} while (true);

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
	unsigned int tile_width;       /* tile width in pixels (set by backend after loading) */
	unsigned int tile_height;      /* tile height in pixels (set by backend after loading) */
	unsigned int tiles_size;       /* h * v */
	std::vector<Tilemap::Tile> tile;
	std::string texture_name;
	std::map<std::string, Waypoint*> waypoint;
	std::map<std::string, Spawnpoint*> spawnpoint;

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

size_t Tilemap::map_width() const {
	return pimpl->map_width;
}

size_t Tilemap::map_height() const {
	return pimpl->map_height;
}

size_t Tilemap::tile_width() const {
	return pimpl->tile_width;
}

size_t Tilemap::tile_height() const {
	return pimpl->tile_height;
}

const std::string& Tilemap::texture_filename() const {
	return pimpl->texture_name;
}

const Tilemap::Tile& Tilemap::operator[](unsigned int i) const {
	return pimpl->tile[i];
}

const Tilemap::Tile& Tilemap::at(unsigned int x, unsigned int y) const {
	const unsigned int index = x + y * pimpl->map_width;
	return pimpl->tile[index];
}

void Tilemap::reserve(unsigned int x, unsigned int y){
	pimpl->tile[x   + (y  ) * pimpl->map_width].build = 0;
	pimpl->tile[x+1 + (y  ) * pimpl->map_width].build = 0;
	pimpl->tile[x   + (y+1) * pimpl->map_width].build = 0;
	pimpl->tile[x+1 + (y+1) * pimpl->map_width].build = 0;
}

std::vector<Tilemap::Tile>::iterator Tilemap::begin(){
	return pimpl->tile.begin();
}

std::vector<Tilemap::Tile>::iterator Tilemap::end(){
	return pimpl->tile.end();
}

std::vector<Tilemap::Tile>::const_iterator Tilemap::begin() const {
	return pimpl->tile.begin();
}

std::vector<Tilemap::Tile>::const_iterator Tilemap::end() const {
	return pimpl->tile.end();
}

void Tilemap::set_dimensions(size_t w, size_t h){
	pimpl->tile_width  = w / pimpl->tiles_horizontal;
	pimpl->tile_height = h / pimpl->tiles_vertical;
}
