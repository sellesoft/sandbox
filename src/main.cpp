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

Object* object_from_concept(str8 name, vec2 pos){
	smval* get = strmap_at(&storage.concepts, name);
	if(!get) { Log("agentmodel", "unable to create object from concept '", name, "' because it is not defined."); return 0; }
	Entity* e = get->entity;
	Object* obj = ((Object*)storage.objects->start) + storage_add_object();
	CopyMemory(&obj->entity, e, sizeof(Entity));
	obj->pos = pos;
	return obj;
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

	forI(storage.concepts.count){
		Log("", storage.concepts.values[i].entity->name, " ", storage.concepts.values[i].entity->type);
	}

	Object* bed = object_from_concept(STR8("bed"), {1,1});

	Log("", is_subclass_of((Entity*)bed, strmap_at(&storage.concepts, STR8("entity"))->entity));

	//start main loop
	while(platform_update()){DPZoneScoped;
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