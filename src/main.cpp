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
	uiInit(0,0);

	uiItem* item = uiItemB();
		item->id = STR8("container");
		item->style.width = 100;
		item->style.background_color = Color_Blue;
		item->style.margin = {20,20};
		item->style.text_wrap = text_wrap_char;
		//uiTextML("here is some text\nhere is some newlined text\nthis text will probably need to be wrapped because it is so long");
		// uiItem* item0 = uiTextML(
		// 	"here is some text that will be pretty long with a newline here\n"
		//     "as well as a couple of tabs here\there\tand here\twith some more text afterwards and another newline\n"
		// 	"also this text should be wrapping by word"
		// 	);
		//item0->id = STR8("container text");
		uiTextML("newline\ntab\ttab\ttab\tnewline\nprobably some wrapping here so hopefully we'll see that and i do think we will, as well as this tab\tthat just happened and this newline\nthat just happened. it would be great if they all showed up and worked properly");
	uiItemE();

	
	// uiItem* item2 = uiItemB();
	// 	item->style.size={100,100};
	// 	item->style.background_color = Color_Blue;
	// 	item->style.margin = {20,20};
	// 	item->style.text_wrap = text_wrap_word;
	// 	uiTextML("here is some text\nhere is some newlined text\nthis text will probably need to be wrapped because it is so long");
	// 	uiItem* item02 = uiTextML(
	// 		"here is some text that will be pretty long with a newline here\n"
	// 	   "as well as a couple of tabs here\there\tand here\twith some more text afterwards and another newline\n"
	// 		"also this text should be wrapping by word"
	// 		);


	// uiItemE();


	static f32 wwidth = 100;
	uiItem* win = uiItemB();
		win->id = STR8("win");
		win->style.background_color = Color_VeryDarkCyan;
		win->style.padding = {20,20};
		win->style.positioning = pos_draggable_fixed;
		uiItem* slider = uiSliderf32(100, 200, &wwidth);
		slider->id = STR8("slider");
		slider->style.size = {100, 10};
	uiItemE();

	f32 test = 0.5;

	f32 test2 = Remap(test, 1.f, 2.f, 0.f, 1.f);
	f32 test3 = Remap(test2, 0.f, 1.f, 1.f, 2.f);
	
	u32 stage = 0;
	//start main loop
	while(platform_update()){DPZoneScoped;
		f32 t = DeshTotalTime/1000;
		item->style.width = wwidth;
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