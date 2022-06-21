#include "core/memory.h"

//// kigu includes ////
#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/node.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/networking.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

#include "core/ui2.h"
// #if !DESHI_RELOADABLE_UI
// #  include "ui2.cpp"
// #endif

f64 avg_fps_v;
mutex avgfps_lock;
void avg_fps(void* in){
	const u64 nframes = 1000;
	static f64 fpsarr[nframes] = {0};
	static u32 idx = 0;
	if(avgfps_lock.try_lock()){
		//uiTextData* text = uiGetTextData((uiItem*)in);
		fpsarr[idx++] = DeshTime->deltaTime;
		idx %= nframes;
		f64 sum = 0;
		forI(nframes){
			sum += fpsarr[i];
		}
		//text->text = toStr8(sum/100).fin;
		avg_fps_v = sum/nframes;
		avgfps_lock.unlock();
	}
}

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
	DeshThreadManager->init(1);
	DeshThreadManager->spawn_thread();
	
	//start main loop
	while(platform_update()){DPZoneScoped;
		f32 t = DeshTotalTime/1000;
		uiUpdate();
		str8 fps = toStr8(avg_fps_v, " ", 1000/DeshTime->deltaTime).fin;
		render_start_cmd2(5, Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second->tex, vec2::ZERO, DeshWindow->dimensions);
		render_text2(Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second, fps, vec2(0,DeshWindow->dimensions.y / 2), vec2::ONE, Color_White);
		
		//console_update();
		UI::Update();
		render_update();
		logger_update();
		memory_clear_temp();
	}
	
#if DESHI_RELOADABLE_UI
	file_delete(STR8("deshi_temp.dll"));
#endif //#if DESHI_RELOADABLE_UI
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}