#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include "backend.hpp"
#include "level.hpp"
#include "tilemap.hpp"
#include "entity.hpp"
#include <cstdlib>
#include <cassert>
#include <vector>
#include <math.h>
#include <sys/time.h>

typedef std::vector<Entity*> EntityVector;

enum Buildings {
	ARROW_TOWER,

	BUILDING_LAST,
};

static bool running = false;
static Backend* backend = NULL;
static Level* level = NULL;
static EntityVector entity;
static Vector2f camera;
static Vector2f cursor;
static bool cursor_ok[4] = {false,false,false,false};
static Tilemap* tilemap;
static int width;
static int height;
static blueprint_t blueprint[BUILDING_LAST];

static void poll(bool&render){
	backend->poll(running);
}

static void render_game(){
	backend->render_begin();
	{
		backend->render_tilemap(level->tilemap(), camera);

		backend->render_entities(entity, camera);

		/* render marker */
		backend->render_marker(cursor, camera, cursor_ok);
	}
	backend->render_end();
}

namespace Game {
	static void build(const Vector2f& pos, Buildings type);

	void init(const std::string& bn, int w, int h){
		width = w;
		height = h;
		backend = Backend::create(bn);

		if ( !backend ){
			fprintf(stderr, "Failed to create backend `%s'.\n", bn.c_str());
			exit(1);
		}

		backend->init(width, height);

		/* load all tower blueprints */
		blueprint[ARROW_TOWER] = Blueprint::from_filename("arrowtower.yaml");
	}

	void cleanup(){
		backend->cleanup();
		delete backend;
	}

	void frobnicate(){
		static const unsigned int framerate = 60;
		static const uint64_t per_frame = 1000000 / framerate;

		running = true;

		/* for calculating dt */
		struct timeval t;
		gettimeofday(&t, NULL);

		/* for calculating framerate */
		struct timeval fref = {t.tv_sec, 0};
		unsigned int fps = 0;

		while ( running ){
			/* frame update */
			poll(running); /* byref */
			render_game();

			struct timeval cur;
			gettimeofday(&cur, NULL);

			/* calculate framerate */
			fps++;
			if ( cur.tv_sec - fref.tv_sec > 1 ){
				fprintf(stderr, "fps: %d\n", fps);
				fref.tv_sec++;
				fps = 0;
			}

			/* calculate dt */
			const uint64_t delta = (cur.tv_sec - t.tv_sec) * 1000000 + (cur.tv_usec - t.tv_usec);
			const  int64_t delay = per_frame - delta;

			/* move time forward */
			t.tv_usec += per_frame;
			if ( t.tv_usec > 1000000 ){
				t.tv_usec -= 1000000;
				t.tv_sec++;
			}

			/* fixed framerate */
			if ( delay > 0 ){
				usleep(delay);
			}
		}
	}

	void load_level(const std::string& filename){
		delete level;
		level = Level::from_filename(filename);
	}

	Tilemap* load_tilemap(const std::string& filename){
		assert(backend);
		tilemap = backend->load_tilemap(filename);
		return tilemap;
	}

	Sprite* load_sprite(const std::string& filename){
		assert(backend);
		return backend->load_sprite(filename);
	}

	void pan(float x, float y){
		const float bx = tilemap->tile_width()  * tilemap->map_width()  - width;
		const float by = tilemap->tile_height() * tilemap->map_height() - height;

		camera.x = clamp(camera.x - x, 0.0f, bx);
		camera.y = clamp(camera.y - y, 0.0f, by);
	}

	static Vector2f transform(const Vector2f& in){
		Vector2f tmp = in + camera + Vector2f(tilemap->tile_width(), tilemap->tile_height()) * 0.5f;
		tmp.x -= fmod(tmp.x, tilemap->tile_width());
		tmp.y -= fmod(tmp.y, tilemap->tile_height());

		return tmp;
	}

	void motion(float x, float y){
		const Vector2f world = transform(Vector2f(x,y));
		cursor = world;

		/* get info about tile under cursor */
		int tx = max(world.x / tilemap->tile_width() - 1, 0.0f);
		int ty = max(world.y / tilemap->tile_height() - 1, 0.0f);

		/** @bug at the far end of the map it will read unallocated memory */
		cursor_ok[0] = tilemap->at(tx  ,ty  ).build;
		cursor_ok[1] = tilemap->at(tx+1,ty  ).build;
		cursor_ok[2] = tilemap->at(tx  ,ty+1).build;
		cursor_ok[3] = tilemap->at(tx+1,ty+1).build;
	}

	void click(float x, float y){
		const Vector2f world = transform(Vector2f(x,y));
		int tx = max(world.x / tilemap->tile_width() - 1, 0.0f);
		int ty = max(world.y / tilemap->tile_height() - 1, 0.0f);

		if ( !(cursor_ok[0] && cursor_ok[1] && cursor_ok[2] && cursor_ok[3]) ){
			return;
		}

		build(Vector2f(tx, ty), ARROW_TOWER);

		tilemap->reserve(tx,ty);
		motion(x, y); /* to update marker */
	}

	static void build(const Vector2f& pos, Buildings type){
		/* insert at correct "depth" */
		EntityVector::iterator it = entity.begin();
		while ( it != entity.end() ){
			Entity* cur = *it;
			if ( cur->world_pos().y >= pos.y ) break;
			++it;
		}

		Building* tmp = new Building(pos, blueprint[type]);
		entity.insert(it, tmp);
	}
};
