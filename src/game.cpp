#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include "backend.hpp"
#include "blueprint.hpp"
#include "building.hpp"
#include "common.hpp"
#include "creep.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "projectile.hpp"
#include "sprite.hpp"
#include "tilemap.hpp"
#include "waypoint.hpp"
#include <cstdlib>
#include <cassert>
#include <vector>
#include <math.h>
#include <algorithm>
#include <cstdarg>

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
	ICE_TOWER,

	BUILDING_LAST,
};

enum Mode {
	SELECT,
	BUILD,
};

class Message;
static bool running = false;
static Backend* backend = NULL;
static Level* level = NULL;
static std::map<std::string, Building*> building;
static std::map<std::string, Creep*> creep;
static std::vector<Projectile*> projectile;
static std::vector<Message*> messages;
static Buildings building_selected = BUILDING_LAST;
static Building* selected = nullptr;
static Mode mode = SELECT;
static Vector2f camera;
static Vector2f cursor;
static bool cursor_ok[4] = {false,false,false,false};
static Tilemap* tilemap;
static const Blueprint* blueprint[BUILDING_LAST];
static bool is_panning = false;
static Vector2f panning_ref;    /* reference point when panning using mouse */
static Vector2f panning_cur;    /* where the mouse currently is (to calculate how much to pan) */
static bool show_waypoints = false;
static bool show_aabb = false;
static time_t wave_delay = 5;
static int wave_left = 0;
static unsigned int wave_current = 0;
static int gold = 30;
static int lives = 100;
static Vector2i window_size;
static Vector2i scene_size;
static Vector2i info_size(200,200);
static RenderTarget* scene_target = nullptr;
static RenderTarget* ui_target = nullptr;
static RenderTarget* info_target = nullptr;
static const int ui_height = 50;
static Font* font16;
static Font* font24;
static Font* font34;
static Sprite* ui_bar_left = nullptr;
static Sprite* ui_upgrade = nullptr;
static Sprite* ui_sell = nullptr;

namespace Game {
	static Vector2f clamp_to_world(const Vector2f& v);
}

class Message {
public:
	Message(const Vector2f& pos, const Color& color, const char* fmt, ...)
		: pos(pos)
		, color(color)
		, t(1.0f) {

		va_list ap;
		va_start(ap, fmt);
		vasprintf(&msg, fmt, ap);
		va_end(ap);
	}

	~Message(){
		free(msg);
	}

	bool tick(float dt){
		static float lifespan = 3.0f;

		t += dt;
		const float s = t / lifespan;
		color.a = 1.0f - s;
		pos.y -= 0.5f;
		return t < lifespan;
	}

	Vector2f pos;
	Color color;
	float t;
	char* msg;
};

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
	backend->render_projectiles(projectile, cam);

	std::for_each(messages.begin(), messages.end(), [cam](const Message* msg){
			Vector2f p = msg->pos - cam;
			if ( p.x < 0.0f ) return; /* Font::printf wraps negative positions */
			if ( p.y < 0.0f ) return;
			font24->printf(p.x, p.y, msg->color, "%s", msg->msg);
	});
}

static void render_cursor(const Vector2f& cam){
	if ( mode == BUILD ){
		backend->render_marker(cursor, cam, cursor_ok);
	}
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

static void render_info(const Building* building, bool b1, bool b2){
	static Color c1 = Color::rgba(1,1,1,1.0f);
	static Color c2 = Color::rgba(1,1,1,0.7f);

	backend->render_begin(info_target);
	if ( building ){
		backend->render_clear(Color::rgba(0,0,0,0.5f));
		font24->printf(10,  5, Color::white, "%s", building->name().c_str());

		int line = 0;
		font16->printf(10, 40+line++*16, Color::white, "Level: \t\t%d", building->current_level());
		font16->printf(10, 40+line++*16, Color::white, "Damage: \t%.0f", building->damage());
		font16->printf(10, 40+line++*16, Color::white, "Range: \t\t%.0f", building->range());
		font16->printf(10, 40+line++*16, Color::white, "RoF: \t\t%.0f (per minute)", building->rof());
		if ( building->have_slow() ){
			font16->printf(10, 40+line++*16, Color::white, "Slows target by %.1f%% for %.1f sec", (1.0f-building->slow())*100, building->slow_duration());
		}
		if ( b1 ){
			font16->printf(10, 40+line++*16, Color::white, "Cost: \t\t%d", building->upgrade_cost());
		}
		if ( b2 ){
			font16->printf(10, 40+line++*16, Color::white, "Income: \t\t%d (75%% of value)", building->sell_cost());
		}

		if ( building->can_upgrade() ){
			backend->render_sprite(Vector2i(10,  161), ui_upgrade, b1 ? c1 : c2);
		}
		backend->render_sprite(Vector2i(105, 161), ui_sell,    b2 ? c1 : c2);
	} else {
		backend->render_clear(Color::rgba(0,0,0,0));
	}
	backend->render_end();
}

static void render_game(){
	backend->render_begin(scene_target);
	{
		Vector2f panned_cam = camera;

		if ( is_panning ){
			panned_cam += panning_ref - panning_cur;
			panned_cam = Game::clamp_to_world(panned_cam);
		}

		backend->render_clear(Color::red);
		render_world(panned_cam);
		render_cursor(panned_cam);
		render_waypoints(panned_cam);
		render_aabb(panned_cam);
	}
	backend->render_end();

	backend->render_begin(ui_target);
	{
		backend->render_clear(Color::rgba(0,0,0,0.5f));
		//backend->render_sprite(Vector2i(0,0), ui_bar_left);

		for ( int i = 0; i < BUILDING_LAST; i++ ){
			backend->render_sprite(Vector2i(150 + i * 41, 7), blueprint[i]->icon(1), gold >= blueprint[i]->cost(1) ? Color::white : Color::rgba(0.3,0.3,0.3,1));
		}
		font24->printf(   8,  5, Color::white, "Gold: %4d", gold);
		font24->printf(   7, 22, Color::white, "Lives: %4d", lives);
		font24->printf(-112,  5, Color::white, "Creep: %4zd", creep.size());
		font24->printf(-150, 22, Color::white, "Next wave: %4ds", wave_left);
	}
	backend->render_end();

	backend->render_begin(nullptr);
	{
		backend->render_target(scene_target, Vector2i(0,0));
		backend->render_target(ui_target,    Vector2i(0, -ui_height));
		backend->render_target(info_target,  Vector2i(-info_size.x, -ui_height - info_size.y));

		/* render ui-outline */
		const float w = window_size.x;
		const float h = window_size.y - ui_height;
		const float s = selected != nullptr ? info_size.y : 0;
		Vector2f p[] = {
			Vector2f(0, h),
			Vector2f(w - info_size.x, h),
			Vector2f(w - info_size.x, h - s),
			Vector2f(w, h - s)
		};
		backend->render_lines(Color::white, 3, p, 4);
	}
	backend->render_end();
}

/* build action wrapper function */
std::function<void(Buildings)> build_action = [](Buildings type){
	building_selected = type;
	mode = BUILD;
	if ( gold < blueprint[building_selected]->cost(1) ){
		mode = SELECT;
	}

	/* drop current entity selection */
	selected = nullptr;
	render_info(selected, false, false);
};

namespace Game {
	static void build(const Vector2i& pos, Buildings type);

	void init(const std::string& bn, int w, int h){
		window_size = Vector2i(w, h);
		backend = Backend::create(bn);

		if ( !backend ){
			fprintf(stderr, "Failed to create backend `%s'.\n", bn.c_str());
			exit(1);
		}

		backend->init(window_size);
		backend->bindkey("F1", [](){
				show_waypoints = !show_waypoints;
				fprintf(stderr, "%s waypoints\n", show_waypoints ? "Showing" : "Hiding");
		});
		backend->bindkey("F2", [](){
				show_aabb = !show_aabb;
				fprintf(stderr, "%s AABB\n", show_aabb ? "Showing" : "Hiding");
		});

		backend->bindkey("1", std::bind(build_action, ARROW_TOWER));
		backend->bindkey("2", std::bind(build_action, ICE_TOWER));
		backend->bindkey("ESC", [](){
				mode = SELECT;
		});

		/* create render targets */
		scene_size = window_size;
		scene_target = backend->create_rendertarget(scene_size, false);
		ui_target    = backend->create_rendertarget(Vector2i(window_size.x, ui_height), true);
		info_target  = backend->create_rendertarget(info_size, true);

		/* load fonts and ui elements */
		font16 = backend->create_font("calibri_16.bff");
		font24 = backend->create_font("calibri_24.bff");
		font34 = backend->create_font("calibri_34.bff");
		ui_bar_left = backend->create_sprite()->load_texture("bar_left.png")->autoscale();
		ui_upgrade  = backend->create_sprite()->load_texture("icon_upgrade.png")->autoscale();
		ui_sell     = backend->create_sprite()->load_texture("icon_sell.png")->autoscale();
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
		struct timeval sref = {t.tv_sec + wave_delay, 0};

		while ( running ){
			/* frame update */
			poll(running); /* byref */
			render_game();

			if ( lives == 0 ){
				continue;
			}

			struct timeval cur;
			gettimeofday(&cur, NULL);

			/* calculate framerate */
			fps++;
			if ( cur.tv_sec - fref.tv_sec > 1 ){
				fref.tv_sec++;
				fps = 0;
			}

			/* spawn wave */
			wave_left = (sref.tv_sec - cur.tv_sec);
			if ( wave_left <= 0 ){
				wave_current++;
				wave_delay = 15;
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

			/* update projectiles */
			auto end = std::remove_if(projectile.begin(), projectile.end(), [](Projectile* proj) -> bool {
					return proj->tick(dt);
			});
			projectile.erase(end, projectile.end());

			/* update messages */
			std::remove_if(messages.begin(), messages.end(), [](Message* msg){
				return msg->tick(dt);
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

		/* load all tower blueprints */
		blueprint[ARROW_TOWER] = Blueprint::from_filename("arrowtower.yaml");
		blueprint[ICE_TOWER]   = Blueprint::from_filename("icetower.yaml");
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
		const size_t bx = tilemap->tile_width()  * tilemap->map_width()  - scene_size.x;
		const size_t by = tilemap->tile_height() * tilemap->map_height() - scene_size.y;

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

		/* test if hovering over infobox */
		if ( selected && x > window_size.x - info_size.x && y > window_size.y - ui_height - info_size.y ){
			const Vector2i local((int)x - (window_size.x - info_size.x), (int)y - (window_size.y - ui_height - info_size.y));
			const bool b1 = local.y >= 161 && local.y < 200 && local.x >= 10  && local.x < 95 && selected->can_upgrade();
			const bool b2 = local.y >= 161 && local.y < 200 && local.x >= 105 && local.x < 190;
			render_info(selected, b1, b2);
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

			/* test if clicking on ui-bar */
			if ( y > window_size.y - ui_height ){
				const int ix = (int)x;
				const int bar_min = 150;
				const int bar_max = bar_min + BUILDING_LAST * 41;
				if ( ix > bar_min && ix < bar_max ){
					const int icon = (ix - 150) / 41;
					build_action((Buildings)icon);
				}
				break;
			}

			/* test if clicking on infobox */
			if ( selected && x > window_size.x - info_size.x && y > window_size.y - ui_height - info_size.y ){
				const Vector2i local((int)x - (window_size.x - info_size.x), (int)y - (window_size.y - ui_height - info_size.y));
				const bool b1 = local.y >= 161 && local.y < 200 && local.x >= 10  && local.x < 95 && selected->can_upgrade();
				const bool b2 = local.y >= 161 && local.y < 200 && local.x >= 105 && local.x < 190;

				if ( b1 ){
					selected->upgrade();
				}
				if ( b2 ){
					selected->sell();
					selected = nullptr;
				}

				render_info(selected, b1 && selected->can_upgrade(), b2);
				break;
			}

			/* assume clicking on world */
			if ( mode == BUILD){
				/* try to build at cursor */
				if ( !(cursor_ok[0] && cursor_ok[1] && cursor_ok[2] && cursor_ok[3]) ){
					return;
				}

				build(grid, building_selected);
				motion(x, y); /* to update marker */
				mode = SELECT;
			} else if ( mode == SELECT ){
				selected = nullptr;
				for ( auto it = building.begin(); it != building.end(); ++it ){
					Building* building = it->second;
					if ( building->grid_pos() == grid ){
						selected = building;
						break;
					}
				}
				render_info(selected, false, false);
			}
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

	void resize(const Vector2i& size){
		fprintf(stderr, "Window resized to %dx%d\n", size.x, size.y);

		window_size = size;

		/* recreate render targets */
		delete scene_target;
		delete ui_target;
		delete info_target;
		scene_size = window_size;
		scene_target = backend->create_rendertarget(scene_size, false);
		ui_target    = backend->create_rendertarget(Vector2i(window_size.x, ui_height), true);
		info_target  = backend->create_rendertarget(info_size, true);
	}

	static void build(const Vector2i& pos, Buildings type){
		const int cost = blueprint[type]->cost(1);
		if ( !transaction(cost, Vector2f(pos.x*tile_width(), pos.y*tile_height())) ){
			fprintf(stderr, "Not enough gold, cost %d have %d\n", cost, gold);
			return;
		}

		Building* tmp = Building::place_at_tile(pos, blueprint[type]);
		building[tmp->id()] = tmp;
		tilemap->reserve(pos.x, pos.y);
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
		Entity* ent = find_entity(name);
		if ( !ent ){
			return;
		}
		ent->dec_ref();

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

	void add_projectile(Projectile* proj){
		projectile.push_back(proj);
	}

	bool transaction(int amount, const Vector2f& pos){
		const int tmp = gold - amount;
		if ( tmp < 0 ) return false;
		gold = tmp;

		const Color& color = amount > 0 ? Color::red : Color::yellow;
		messages.push_back(new Message(pos, color, "%d", amount > 0 ? amount : -amount));
		return true;
	}

	void mutilate(){
		if ( --lives == 0 ){
			fprintf(stderr, "Game over.\n");
		}
	}
};
