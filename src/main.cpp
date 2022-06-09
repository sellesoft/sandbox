//// kigu includes ////
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
#include "core/memory.h"
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

#include "ui2.h"
#if !DESHI_RELOADABLE_UI
#  include "ui2.cpp"
#endif

local uiContext deshi_ui{}; uiContext* g_ui = &deshi_ui;
local uiStyle deshi_ui_initial_style{}; uiStyle* ui_initial_style = &deshi_ui_initial_style;

void action(void* data){
	
}

void log_sizes(){
    Log("logsizes", "\n",
        "uiItem:    ", sizeof(uiItem), "\n",
        "uiDrawCmd: ", sizeof(uiDrawCmd), "\n",
        "uiWindow:  ", sizeof(uiWindow), "\n",
        "uiButton:  ", sizeof(uiButton), "\n",
        "vec2i:     ", sizeof(vec2i), "\n",
        "str8:      ", sizeof(str8), "\n",
        "TNode:     ", sizeof(TNode), "\n",
        "uiStyle:   ", sizeof(uiStyle), "\n",
        "upt:       ", sizeof(upt), "\n",
        "u64:       ", sizeof(u64), "\n"
		);
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
	//UI::Init();
	cmd_init();
	window_show(DeshWindow);
    render_use_default_camera();
	DeshThreadManager->init();

	
	{ //load UI funcs
#if DESHI_RELOADABLE_UI
		g_ui->module = platform_load_module(STR8("deshi.dll"));
		if(g_ui->module){
			g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
			g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
			g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
			g_ui->make_child_window = platform_get_module_function(g_ui->module, "ui_make_child_window", ui_make_child_window);
			g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
			g_ui->init              = platform_get_module_function(g_ui->module, "ui_init", ui_init);
			g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
			g_ui->module_valid = (g_ui->make_window && g_ui->begin_window && g_ui->end_window
								  && g_ui->make_child_window && g_ui->make_button && g_ui->init && g_ui->update);
		}
		if(!g_ui->module_valid){
			g_ui->make_window       = ui_make_window__stub;
			g_ui->begin_window      = ui_begin_window__stub;
			g_ui->end_window        = ui_end_window__stub;
			g_ui->make_child_window = ui_make_child_window__stub;
			g_ui->make_button       = ui_make_button__stub;
			g_ui->init              = ui_init__stub;
			g_ui->update            = ui_update__stub;
		}
#endif //#if DESHI_RELOADABLE_UI
	}
	uiInit();
	
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");

	uiItem* mod = 0;
	uiStyle style{};
	style.padding_top = 10;
	uiItemBS(&style)->id = STR8("item0");{
		uiStyle style{};
		style.width = 20;
		style.height = 50;
		style.background_color = Color_DarkCyan;
		style.margin_top = 10;
		(mod=uiItemBS(&style))->id = STR8("item1");{
			
		}uiItemE();
	}uiItemE();


	//start main loop
	while(platform_update()){DPZoneScoped;
#if DESHI_RELOADABLE_UI
		if(key_pressed(Key_F5 | InputMod_AnyAlt)){
			//unload the module
			
			g_ui->module_valid = false;
			if(g_ui->module){
				platform_free_module(g_ui->module);
				g_ui->module = 0;
			}
			
			//load the module
			g_ui->module = platform_load_module(STR8("deshi.dll"));
			if(g_ui->module){
				g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
				g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
				g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
				g_ui->make_child_window = platform_get_module_function(g_ui->module, "ui_make_child_window", ui_make_child_window);
				g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
				g_ui->init              = platform_get_module_function(g_ui->module, "ui_init", ui_init);
				g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
				g_ui->module_valid = (g_ui->make_window && g_ui->begin_window && g_ui->end_window
									  && g_ui->make_child_window && g_ui->make_button && g_ui->init && g_ui->update);
			}
			if(!g_ui->module_valid){
				g_ui->make_window       = ui_make_window__stub;
				g_ui->begin_window      = ui_begin_window__stub;
				g_ui->end_window        = ui_end_window__stub;
				g_ui->make_child_window = ui_make_child_window__stub;
				g_ui->make_button       = ui_make_button__stub;
				g_ui->init              = ui_init__stub;
				g_ui->update            = ui_update__stub;
			}
		}
#endif //#if DESHI_RELOADABLE_UI
		mod->style.margin_top = BoundTimeOsc(0, 50);
		uiUpdate();
        {
            using namespace UI;
            //Begin(STR8("debuggingUIwithUI"));{
				
            //}End();
        }
		//UI::Update();
		render_update();
		logger_update();
		
		memory_clear_temp();
	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}