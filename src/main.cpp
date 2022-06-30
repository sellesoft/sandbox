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
	const u64 nframes = 100;
	static f64 fpsarr[nframes] = {0};
	static u32 idx = 0;
	if(avgfps_lock.try_lock()){
		//uiText* text = uiGetTextData((uiItem*)in);
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

	uiStyle win{0};
	win.background_color = color(14,14,14);
	win.border_color = Color_White;
	win.border_style = border_solid;
	win.border_width = 1;
	win.size = {300,400};
	win.positioning = pos_draggable_relative;
	win.display = display_flex;

	uiStyle text_style= {0};
	text_style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
	text_style.font_height = 11;
	text_style.text_color = Color_White;

	uiStyle content_style = {0};
	content_style.sizing = size_flex | size_percent_x;
	content_style.height = 2;
	content_style.width = 100;
	content_style.padding = {3,3,3,3};
	content_style.margin = {2,2,2,2}; 
	content_style.border_color = Color_White;
	content_style.border_style = border_solid;
	content_style.border_width = 1;

	uiItem* window = uiItemBS(&win);
		window->id = STR8("window");
		// uiItem* title = uiItemB();
		// 	title->id = STR8("title");
		// 	title->style.background_color = color(30,30,30);
		// 	title->style.sizing = size_auto_y | size_percent_x;
		// 	title->style.width = 100;
		// 	title->style.padding = {5, 0, 0, 0};
		// 	title->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
		// 	title->style.font_height = 11;
		// 	title->style.text_color = Color_White;
		// 	uiTextML("window");
		// uiItemE();
		uiItem* content = uiItemBS(&content_style);
			content->id = STR8("content1");
			content->style.background_color = color(50,100,100);
		uiItemE();
		// uiItem* content2 = uiItemBS(&content_style);
		// 	content2->id = STR8("content2");
		// 	content2->style.background_color = color(30,30,100);
		// uiItemE();
		// uiItem* content3 = uiItemBS(&content_style);
		// 	content3->id = STR8("content3");
		// 	content3->style.background_color = color(30,100,30);
		// uiItemE();
		// uiItem* content4 = uiItemBS(&content_style);
		// 	content4->id = STR8("content4");
		// 	content4->style.background_color = color(100,30,100);
		// uiItemE();
		// uiItem* footer = uiItemB();
		// 	footer->style.background_color = color(30,30,30);
		// 	footer->style.sizing = size_auto_y | size_percent_x;
		// 	footer->style.width = 100;
		// 	footer->style.padding = {5,5,5,5};
		// 	footer->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
		// 	footer->style.font_height = 11;
		// 	footer->style.text_color = Color_White;
		// 	uiTextML("footer");
		// uiItemE();
	uiItemE();
	b32 header_open = 0;
	
	u32 idx = 0;

	//start main loop
	while(platform_update()){DPZoneScoped;

		DeshThreadManager->add_job({avg_fps, 0});
		DeshThreadManager->wake_threads();

		f32 t = DeshTotalTime/1000;
		//ui_debug();

		//content->style.height = BoundTimeOsc(0.5, 2);
		if(content->max_scroll.y)
			content->style.scry = BoundTimeOsc(0, content->max_scroll.y);

		// if(key_pressed(Key_SPACE)){
		// 	switch(idx){
		// 		case 0: content1->style.height = 1.5; break;
		// 		case 1: content2->style.height = 0.5; break;
		// 		case 2: content3->style.height = 0.2; break;
		// 		//case 3: content4->style.height = 1.8; break;
		// 		case 4: content1->style.height = 1; break;
		// 		case 5: content2->style.height = 1.5; break;
		// 		case 6: content3->style.height = 1.2; break;
		// 		//case 7: content4->style.height = 2; break;
		// 	}
		// 	idx = (idx == 7 ? 0 : idx + 1);
		// }

		// content1->style.height = Math::BoundedOscillation(1, 5, DeshTotalTime / 1000);
		// content3->style.height = Math::BoundedOscillation(1, 5, DeshTotalTime / 1000 + M_HALFPI);


	

		//title->style.padding = vec4::ONE * BoundTimeOsc(1, 10);
		
		uiImmediateBP(content);{

			uiItem* header = uiItemB();
				header->id = STR8("header");
				header->style.sizing = size_percent_x;
				header->style.height = 13;
				header->style.width = 100;
				header->style.background_color = color(50,75,50);
				header->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
				header->style.text_color = Color_White;
				header->style.text_wrap = text_wrap_none;
				header->style.font_height = 11;
				header->style.content_align.x = 1;

				header->action = [](uiItem* item){
					*(b32*)item->action_data = !*(b32*)item->action_data;
				}; header->action_trigger = action_act_mouse_released;
				header->action_data = &header_open;
				
				uiItem* header_text = uiTextMSL(&text_style, "a header");
				
			uiItemE();

			if(header_open){
				uiItem* header_cont = uiItemB();
					header_cont->id = STR8("container");
					header_cont->style.sizing = size_percent_x;
					header_cont->style.height = 1000;
					header_cont->style.width = 100;
					header_cont->style.background_color = color(75,50,75);
					header_cont->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
					header_cont->style.text_color = Color_White;
					header_cont->style.text_wrap = text_wrap_none;
					header_cont->style.font_height = 11;
					header_cont->style.content_align = {0.5, 1};
					uiTextMSL(&text_style, "hi");

				uiItemE();
			}

			// uiItem* after_header = uiItemB();
			// 	after_header->style.background_color = Color_Cyan;
			// 	after_header->style.sizing = size_auto;
			// 	after_header->style.padding = {10,10,10,10};
			// 	uiTextMSL(&text_style, "this should appear after the header and its container");

			// uiItemE();

		}uiImmediateE();


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