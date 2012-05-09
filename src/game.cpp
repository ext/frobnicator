#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include "backend.hpp"
#include "blueprint.hpp"
#include "building.hpp"
#include "creep.hpp"
#include "level.hpp"
#include "tilemap.hpp"
#include "entity.hpp"
#include "waypoint.hpp"
#include <cstdlib>
#include <cassert>
#include <vector>
#include <math.h>
#include <algorithm>

#ifdef WIN32
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/time.h>
#else
extern "C" int gettimeofday(struct timeval* tv, struct timezone* tz);
extern "C" void usleep (uint64_t usec);
#endif

typedef std::vector<Entity*> EntityVector;

enum Buildings {
	ARROW_TOWER,

	BUILDING_LAST,
};

static bool running = false;
static Backend* backend = NULL;
static Level* level = NULL;
static std::map<std::string, Building*> building;
static std::map<std::string, Creep*> creep;
static Vector2f camera;
static Vector2f cursor;
static bool cursor_ok[4] = {false,false,false,false};
static Tilemap* tilemap;
static int width;
static int height;
static const Blueprint* blueprint[BUILDING_LAST];
static bool is_panning = false;
static Vector2f panning_ref;    /* reference point when panning using mouse */
static Vector2f panning_cur;    /* where the mouse currently is (to calculate how much to pan) */
static bool show_waypoints = false;
static bool show_aabb = false;
static const time_t wave_delay = 10;
static unsigned int wave_current = 0;

namespace Game {
	static Vector2f clamp_to_world(const Vector2f& v);
}

static void poll(bool&render){
	backend->poll(running);
}

static void render_world(const Vector2f& cam){
	backend->render_tilemap(level->tilemap(), cam);

	/* retrieve all entities and sort them based on "depth" */
	std::vector<Entity*> all;
	std::transform(
		creep.begin(),
		creep.end(),
		std::back_inserter(all),
		[](std::map<std::string, Creep*>::value_type &pair){return pair.second;});
	std::transform(
		building.begin(),
		building.end(),
		std::back_inserter(all),
		[](std::map<std::string, Building*>::value_type &pair){return pair.second;});
	std::sort(
		all.begin(),
		all.end(),
		[](const Entity* a, const Entity* b) -> bool {
			return a->world_pos().y < b->world_pos().y;
		});

	backend->render_entities(all, cam);
}

static void render_cursor(const Vector2f& cam){
	backend->render_marker(cursor, cam, cursor_ok);
}

static void render_waypoints(const Vector2f& cam){
	if ( !show_waypoints ) return;

	/* render waypoints */
	for ( auto it = level->waypoints().begin(); it != level->waypoints().end(); ++it ){
		static float color[3] = {1,1,0};
		backend->render_region(it->second, cam, color);
	}

	/* render spawnpoints */
	for ( auto it = level->spawnpoints().begin(); it != level->spawnpoints().end(); ++it ){
		static float color[3] = {0.7f, 0, 0.7f};
		backend->render_region(it->second, cam, color);
	}

}

static void render_aabb(const Vector2f& cam){
	if ( !show_aabb ) return;

	for ( auto it = building.begin(); it != building.end(); ++it ){
		static float color[3] = {0,0,1};
		backend->render_region(it->second, cam, color);
	}

	for ( auto it = creep.begin(); it != creep.end(); ++it ){
		static float color[3] = {0,1,1};
		backend->render_region(it->second, cam, color);
	}
}

static void render_game(){
	backend->render_begin();
	{
		Vector2f panned_cam = camera;

		if ( is_panning ){
			panned_cam += panning_ref - panning_cur;
			panned_cam = Game::clamp_to_world(panned_cam);
		}

		render_world(panned_cam);
		render_cursor(panned_cam);
		render_waypoints(panned_cam);
		render_aabb(panned_cam);
	}
	backend->render_end();
}

namespace Game {
	static void build(const Vector2i& pos, Buildings type);

	void init(const std::string& bn, int w, int h){
		width = w;
		height = h;
		backend = Backend::create(bn);

		if ( !backend ){
			fprintf(stderr, "Failed to create backend `%s'.\n", bn.c_str());
			exit(1);
		}

		backend->init(width, height);
		backend->bindkey("F1", [](){
				show_waypoints = !show_waypoints;
				fprintf(stderr, "%s waypoints\n", show_waypoints ? "Showing" : "Hiding");
		});
		backend->bindkey("F2", [](){
				show_aabb = !show_aabb;
				fprintf(stderr, "%s AABB\n", show_aabb ? "Showing" : "Hiding");
		});

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
		static const float dt = 1.0f / framerate;
		running = true;

		/* for calculating dt */
		struct timeval t;
		gettimeofday(&t, NULL);

		/* for calculating framerate */
		struct timeval fref = {t.tv_sec, 0};
		unsigned int fps = 0;

		/* spawn timer */
		struct timeval sref = {t.tv_sec, 0};
		fprintf(stderr, "Next wave will start in %lld seconds.\n", (long long)wave_delay);

		while ( running ){
			/* frame update */
			poll(running); /* byref */
			render_game();

			struct timeval cur;
			gettimeofday(&cur, NULL);

			/* calculate framerate */
			fps++;
			if ( cur.tv_sec - fref.tv_sec > 1 ){
				fref.tv_sec++;
				fps = 0;
			}

			/* spawn wave */
			if ( cur.tv_sec - sref.tv_sec > wave_delay ){
				wave_current++;
				sref.tv_sec += wave_delay;

				fprintf(stderr, "Spawning wave %d\n", wave_current);
				EntityVector wave = level->spawn(wave_current);
				std::for_each(wave.begin(), wave.end(), [](Entity* e){ creep[e->id()] = static_cast<Creep*>(e); });
			}

			/* calculate dt */
			const uint64_t delta = (cur.tv_sec - t.tv_sec) * 1000000 + (cur.tv_usec - t.tv_usec);
			const  int64_t delay = per_frame - delta;

			/* update creep */
			std::for_each(creep.begin(), creep.end(), [](std::pair<const std::string, Creep*>& pair){
				Creep* creep = pair.second;
				creep->tick(dt);

				/* find what region the creep is in */
				const Waypoint* region = NULL;
				for ( auto jt = level->waypoints().begin(); jt != level->waypoints().end(); ++jt ){
					const Waypoint* wp = jt->second;

					if ( wp->contains(creep->world_pos(), Vector2f(47,47), true) ){
						region = wp;
						break;
					}
				}
				bool found = region;

				/* creep exited a region */
				if ( !found && creep->get_region() != "" ){
					creep->on_exit_region(*Game::find_waypoint(creep->get_region()));
				}

				/* creep entered a new region */
				if ( found && creep->get_region() != region->name() ){
					creep->on_enter_region(*region);
				}

				/* remember current region */
				creep->set_region(region ? region->name() : "");
			});

			/* update towers */
			std::for_each(building.begin(), building.end(), [](std::pair<const std::string, Building*>& pair){
				pair.second->tick(dt);
			});

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

	Sprite* create_sprite(){
		assert(backend);
		return backend->create_sprite();
	}

	/**
	 * Takes a position in world-space and clips it to the area defined by the
	 * tilemap - window. Can be used to prevent user from moving outside of map.
	 */
	static Vector2f clamp_to_world(const Vector2f& v){
		const size_t bx = tilemap->tile_width()  * tilemap->map_width()  - width;
		const size_t by = tilemap->tile_height() * tilemap->map_height() - height;

		return Vector2f(
			clamp(v.x, 0.0f, (float)bx),
			clamp(v.y, 0.0f, (float)by)
		);
	}

	void pan(float x, float y){
		camera = clamp_to_world(camera - Vector2f(x,y));
	}

	/**
	 * Transform a coordinate from screenspace to worldspace.
	 */
	static Vector2f transform(const Vector2f& in){
		Vector2f tmp = in + camera + Vector2f((float)tilemap->tile_width(), (float)tilemap->tile_height()) * 0.5f;
		tmp.x -= fmod(tmp.x, (float)tilemap->tile_width());
		tmp.y -= fmod(tmp.y, (float)tilemap->tile_height());

		return tmp;
	}

	void motion(float x, float y){
		const auto world = [x,y]() -> Vector2f {
			if ( is_panning ){
				return transform(panning_ref);
			} else {
				return transform(Vector2f(x,y));
			}
		}();
		cursor = world;

		/* get info about tile under cursor */
		int tx = (int)max(world.x / tilemap->tile_width() - 1, 0.0f);
		int ty = (int)max(world.y / tilemap->tile_height() - 1, 0.0f);

		/** @bug at the far end of the map it will read unallocated memory */
		cursor_ok[0] = tilemap->at(tx  , ty  ).build;
		cursor_ok[1] = tilemap->at(tx+1, ty  ).build;
		cursor_ok[2] = tilemap->at(tx  , ty+1).build;
		cursor_ok[3] = tilemap->at(tx+1, ty+1).build;

		if ( is_panning ){
			panning_cur.x = x;
			panning_cur.y = y;

			/* offset reference point while at the edges by taking the difference
			 * between the original point and the clamped point. Without this
			 * you would have to move the mouse back a lot before you could continue
			 * panning. */
			const Vector2f p = camera + panning_ref - panning_cur;
			const Vector2f c = Game::clamp_to_world(p);
			panning_ref -= p-c;
		}
	}

	void button_pressed(float x, float y, int button){
		const Vector2f world = transform(Vector2f(x,y));

		/* note that result is truncated */
		const Vector2i grid(
			(int)max(world.x / tilemap->tile_width()  - 1, 0.0f),
			(int)max(world.y / tilemap->tile_height() - 1, 0.0f)
		);

		switch ( button ){
		case 1: /* left button */
			/* try to build at cursor */
			if ( !(cursor_ok[0] && cursor_ok[1] && cursor_ok[2] && cursor_ok[3]) ){
				return;
			}

			build(grid, ARROW_TOWER);

			tilemap->reserve(grid.x, grid.y);
			motion(x, y); /* to update marker */
			break;

		case 3: /* right button */
			/* panning_ref is used to calculate how much the user has panned */
			panning_ref.x = panning_cur.x = x;
			panning_ref.y = panning_cur.y = y;
			is_panning = true;
			break;
		}
	}

	void button_released(float x, float y, int button){
		switch ( button ){
		case 3: /* right button */
			panning_cur.x = x;
			panning_cur.y = y;
			is_panning = false;

			{
				const Vector2f v = panning_cur - panning_ref;
				pan(v.x, v.y);
			}
		}
	}

	void resize(size_t w, size_t h){
		fprintf(stderr, "Window resized to %zdx%zd\n", w, h);

		width = w;
		height = h;
	}

	static void build(const Vector2i& pos, Buildings type){
		Building* tmp = Building::place_at_tile(pos, blueprint[type]);
		building[tmp->id()] = tmp;
	}

	size_t tile_width(){
		return tilemap->tile_width();
	}

	size_t tile_height(){
		return tilemap->tile_height();
	}

	const Waypoint* find_waypoint(const std::string& name){
		auto it = level->waypoints().find(name);
		if ( it != level->waypoints().end() ){
			return it->second;
		} else {
			return NULL;
		}
	}

	Entity* find_entity(const std::string& name){
		/* hack to determine if it is building or creep */
		const bool is_creep = name[0] == 'c';

		if ( is_creep ){
			auto it = creep.find(name);
			if ( it != creep.end() ){
				return it->second;
			} else {
				return NULL;
			}
		} else {
			auto it = building.find(name);
			if ( it != building.end() ){
				return it->second;
			} else {
				return NULL;
			}
		}
	}

	void remove_entity(const std::string& name){
		/* hack to determine if it is building or creep */
		const bool is_creep = name[0] == 'c';

		if ( is_creep ){
			creep.erase(name);
		} else {
			building.erase(name);
		}
	}

	const std::map<std::string, Creep*>& all_creep(){
		return ::creep;
	}

};
