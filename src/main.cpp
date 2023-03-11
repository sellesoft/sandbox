#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif

//// kigu includes ////
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/graphing.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/assets.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

#include "stb/stb_ds.h"

#include "types.h"
#include "parser.cpp"

struct{
	Entity* entity;
	Entity* object;
	Entity* agent;
}bases;

enum{
	one_second = 1,
	one_minute = one_second * 60,
	one_hour = one_minute * 60,
	one_day = one_hour * 24,
	one_month = one_day * 31,
	one_year = one_month * 12,
};

b32 object_from_template(Object* obj, strmap templ){
	if(templ["adverts"]){
		strmap adverts = templ["adverts"]->map;
		forX(i, adverts.count){
			Advert* ad = arraddnptr(obj->adverts, 1);
			ad->name = adverts.keys[i].key_string;
			strmap action_template = adverts.values[i].map["action"]->map;
			Action* action = arraddnptr(ad->actions, 1);
			action->name = ad->name;
			if(action_template["time"]){
				action->time = action_template["time"]->unsigned_int;
			}
			if(action_template["costs"]){
				strmap costs = action_template["costs"]->map;
				if(costs["hunger"]) action->costs.hunger = costs["hunger"]->floating;
				if(costs["sleep"]) action->costs.sleep = costs["sleep"]->floating;
				if(costs["bladder"]) action->costs.bladder = costs["bladder"]->floating;
				if(costs["mood"]) action->costs.mood = costs["mood"]->floating;
				if(costs["food"]) action->costs.food = costs["food"]->floating;
			}
		}
	}
	return 1;
}

b32 object_from_concept(Object* obj, str8 name, vec2 pos){
	smval* get = strmap_at(&storage.concepts, name);
	if(!get) { LogE("agentmodel", "unable to create object from concept '", name, "' because it is not defined."); return 0; }
	Entity* e = get->entity;
	if(e->type != entity_object){ LogE("agentmodel", "attempting to create entity '", name, "' as an object, but it is defined as an ", (e->type == entity_entity ? "entity." : "agent.")); return 0; }
	CopyMemory(&obj->entity, e, sizeof(Entity));
	obj->pos = pos;

	smval* get_template = e->predicates["template"];
	if(get_template) object_from_template(obj, get_template->map);

	return 1;
}	

b32 agent_from_concept(Agent* agent, str8 name, vec2 pos){
	smval* get = strmap_at(&storage.concepts, name);
	if(!get) { LogE("agentmodel", "unable to create agent from concept '", name, "' because it is not defined."); return 0; }
	Entity* e = get->entity;
	if(e->type != entity_agent){ LogE("agentmodel", "attempting to create entity '", name, "' as an agent, but it is defined as an ", (e->type == entity_entity ? "entity." : "object.")); return 0; }
	CopyMemory(&agent->object.entity, e, sizeof(Entity));
	agent->object.pos = pos;

	smval* get_template = e->predicates["template"];
	if(get_template) object_from_template((Object*)agent, get_template->map);


	return 1;
}

void sim(){
	forI(storage.agents->used / sizeof(Agent)){
		Agent* agent = (Agent*)storage.agents->start + i;
		if(!arrlen(agent->action_queue.actions)){
			// if the agent isn't doing anything, scan over all existing objects and do something
			Advert* top = 0;
			f32 top_score = 0;
			forI(storage.objects->used / sizeof(Object)){
				Object* object = (Object*)storage.objects->start + i;
				forI(arrlen(object->adverts)){
					//scoring 
					Needs costs = advert_costs(object->adverts + i);
					Needs needs = agent->needs;
					f32 sum = 0;
					forI(need_COUNT){
						sum += costs.arr[i]/(costs.arr[i]*(costs.arr[i] + needs.arr[i])+1e-8);
					}
					f32 distance_effect = Min(1.f, (object->pos - agent->object.pos).magSq());
					f32 score = sum/need_COUNT/distance_effect;
					if(score > top_score){
						top_score = score;
						top = object->adverts + i;
					} 
				}
			}
			if(!top){ // not a good place for the idle action to be put because top will never not be 0 
				arrpush(agent->action_queue.actions, &singletons.actions.idle);
				arrpush(agent->action_queue.times, singletons.actions.idle.time);
			}else{
				forI(arrlen(top->actions)){
					arrpush(agent->action_queue.actions, top->actions + i);
					arrpush(agent->action_queue.times, (top->actions + i)->time);
				}
			}

		}
		Action* action = agent->action_queue.actions[arrlen(agent->action_queue.actions)-1];
		u64* time = agent->action_queue.times + arrlen(agent->action_queue.actions)-1;
		if(!(*time)){
			Log("", "Agent has finished action '", action->name, "'.");
			arrpop(agent->action_queue.actions);
			arrpop(agent->action_queue.times);
			if(!arrlen(agent->action_queue.actions)){
				continue;
			}
			action = agent->action_queue.actions[arrlen(agent->action_queue.actions)-1];
			time = agent->action_queue.times + arrlen(agent->action_queue.actions)-1;

		}
		forI(need_COUNT){
			agent->needs.arr[i] += action->costs.arr[i] / action->time;
			agent->needs.arr[i] = Min(1.f, agent->needs.arr[i]);
		}
		*time -= 1;

	}
}

int main(){
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	window_create(str8l("suugu"));
	render_init();
	assets_init();
	uiInit(0,0);
	console_init();
	cmd_init();
	window_show(DeshWindow);
	render_use_default_camera();
	threader_init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	
	storage.entities = memory_create_arena(sizeof(Entity) * 128);
	storage.objects = memory_create_arena(sizeof(Object) * 128);
	storage.agents = memory_create_arena(sizeof(Agent) * 128);

	parse_ontology();

	singletons.concepts.entity = strmap_at(&storage.concepts, STR8("entity"))->entity;
	singletons.concepts.object = strmap_at(&storage.concepts, STR8("object"))->entity;
	singletons.concepts.agent = strmap_at(&storage.concepts, STR8("agent"))->entity;

	singletons.actions.idle.name = STR8("idle");
	singletons.actions.idle.time = one_second;
	singletons.actions.idle.costs = Needs{1.f/one_day, 1.f/(20*one_hour), 0.f, 0.f, 1.f/one_day};

	Object* apple = storage_add_object();
	object_from_concept(apple, STR8("apple"), {1,1});

	Agent* bob = storage_add_agent();
	agent_from_concept(bob, STR8("human"), {3,3});

	Object* bed = storage_add_object();
	object_from_concept(bed, STR8("bed"), {-1,-2});

	//start main loop
	while(platform_update()){DPZoneScoped;
		sim();
		console_update();
		render_update();
		logger_update();
		memory_clear_temp();
	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}