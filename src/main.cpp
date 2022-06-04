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
#include "core/file.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "math/math.h"

#if BUILD_INTERNAL
#include "misc_testing.cpp"
#endif

struct XonoticPlayer{
	f32 ukn0;
	vec3 pos0;
	vec3 pos1;
	vec3 ukn1;
	vec3 pos2;
	vec3 pos3;
	vec3 vel;
	f32  ukn2;
	vec3 look_ang0;
	vec3 look_ang_mod;
	f32  ukns[28];
};

int main(){
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	window_create(str8l("suugu"));
	console_init();
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	window_show(DeshWindow);
	render_use_default_camera();
	DeshThreadManager->init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	
	File* in = file_init(STR8("H:/HACKING!!!!/xonotic/addr_lists/zpos.csv"), FileAccess_Read);
	str8 buffer = file_read_alloc(in, in->bytes, deshi_allocator);

	Process ac = platform_get_process_by_name(STR8("ac_client.exe"));
	str8 cursor = buffer;
	upt ptr;
	platform_process_read(ac, 0x400000+0x0017e0a8, &ptr, sizeof(ptr));
	vec3 pos;

	//start main loop
	while(platform_update()){DPZoneScoped;
		using namespace UI;
		platform_process_read(ac, ptr+0x30-sizeof(f32)*2, &pos, sizeof(pos));
		Begin(STR8("testaddrs"));{
			string ok = toStr(pos);
			Text(str8{(u8*)ok.str, ok.count});
		}End();	

		console_update();
		UI::Update();
		render_update();
		logger_update();
		memory_clear_temp();
	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}