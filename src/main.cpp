#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif
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

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/config.h"
#include "core/graphing.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/text.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

//// text editor includes ////
#include "types.h"
#include "textedit.cpp"

int main(){
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	window_create(str8l("textedit"));
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	uiInit(0,0);
	//console_init();
	window_show(DeshWindow);
	render_use_default_camera();
	DeshThreadManager->init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");


	//ui_demo();

	uiItem* win = uiItemB();
		win->style.background_color = Color_VeryDarkCyan;
		win->style.positioning = pos_draggable_relative;
		win->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
		win->style.font_height = 11;
		win->style.text_color = Color_White;
		win->style.height = 100;
		win->style.sizing = size_auto_x;
		win->style.text_wrap = text_wrap_word;
		uiItem* itc = uiItemB();
			itc->style.background_color = color(30,30,30);
			itc->style.sizing = size_auto;
			uiItem* inputtext = uiInputTextM();
				inputtext->style.min_height = win->style.font_height;
				inputtext->style.min_width = 80;
		uiTextML("hello here is some text ok!");

		uiItemE();
	uiItemE();

	// Arena* idxa = g_ui->index_arena;

	//init_editor();
	//start main loop
	while(platform_update()){DPZoneScoped;
		//update_editor();
		//console_update();
		UI::Update();
		ui_debug();
		uiUpdate();

		if(key_down(Key_SPACE)){
			uiImmediateBP(win);{
				forI(100){
					uiTextML("hello");
				}
			}uiImmediateE();
		}

		render_update();
		logger_update();
		memory_clear_temp();

	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}