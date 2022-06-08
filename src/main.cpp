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
	UI::Init();
	cmd_init();
	window_show(DeshWindow);
    render_use_default_camera();
	DeshThreadManager->init();

	uiStyle style{};

	forI(sizeof(uiStyle)){
		*(((u8*)&style)+1) = rand() % 255; 
	}

	u64 hash = 0xff00ff00ff << 
		style.positioning >>   
    	style.left |
		style.top * 
    	style.right +
		style.bottom ^ 
    	style.width <<
		style.height >>
    	style.margin_left *
		style.margin_top ^
    	style.margin_bottom &
		style.margin_right | 
    	style.padding_left <<
		style.padding_top &
    	style.padding_bottom |
		style.padding_right ^ 
   (u64)style.font >>
    	style.font_height ^
    	style.background_color.r <<
    	style.background_color.g << 
    	style.background_color.b << 
    	style.background_color.a << 
   (u64)style.background_image >> 
    	style.border_style << 
    	style.border_color.r >>
    	style.border_color.g >>
    	style.border_color.b >>
    	style.border_color.a;


	
	{ //load UI funcs
#if DESHI_RELOADABLE_UI
		g_ui->module = platform_load_module(STR8("deshi.dll"));
		if(g_ui->module){
			g_ui->push_f32          = platform_get_module_function(g_ui->module, "ui_push_f32", ui_push_f32);
			g_ui->push_vec2         = platform_get_module_function(g_ui->module, "ui_push_vec2", ui_push_vec2);
			g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
			g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
			g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
			g_ui->make_child_window = platform_get_module_function(g_ui->module, "ui_make_child_window", ui_make_child_window);
			g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
			g_ui->init              = platform_get_module_function(g_ui->module, "ui_init", ui_init);
			g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
			g_ui->module_valid = (g_ui->push_f32 && g_ui->push_vec2 && g_ui->make_window && g_ui->begin_window && g_ui->end_window
								  && g_ui->make_child_window && g_ui->make_button && g_ui->init && g_ui->update);
		}
		if(!g_ui->module_valid){
			g_ui->push_f32          = ui_push_f32__stub;
			g_ui->push_vec2         = ui_push_vec2__stub;
			g_ui->make_window       = ui_make_window__stub;
			g_ui->begin_window      = ui_begin_window__stub;
			g_ui->end_window        = ui_end_window__stub;
			g_ui->make_child_window = ui_make_child_window__stub;
			g_ui->make_button       = ui_make_button__stub;
			g_ui->init              = ui_init__stub;
			g_ui->update            = ui_udpate__stub;
		}
#endif //#if DESHI_RELOADABLE_UI
	}
	uiInit();
	
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	
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
				g_ui->push_f32          = platform_get_module_function(g_ui->module, "ui_push_f32", ui_push_f32);
				g_ui->push_vec2         = platform_get_module_function(g_ui->module, "ui_push_vec2", ui_push_vec2);
				g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
				g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
				g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
				g_ui->make_child_window = platform_get_module_function(g_ui->module, "ui_make_child_window", ui_make_child_window);
				g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
				g_ui->init              = platform_get_module_function(g_ui->module, "ui_init", ui_init);
				g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
				g_ui->module_valid = (g_ui->push_f32 && g_ui->push_vec2 && g_ui->make_window && g_ui->begin_window && g_ui->end_window
									  && g_ui->make_child_window && g_ui->make_button && g_ui->init && g_ui->update);
			}
			if(!g_ui->module_valid){
				g_ui->push_f32          = ui_push_f32__stub;
				g_ui->push_vec2         = ui_push_vec2__stub;
				g_ui->make_window       = ui_make_window__stub;
				g_ui->begin_window      = ui_begin_window__stub;
				g_ui->end_window        = ui_end_window__stub;
				g_ui->make_child_window = ui_make_child_window__stub;
				g_ui->make_button       = ui_make_button__stub;
				g_ui->init              = ui_init__stub;
				g_ui->update            = ui_udpate__stub;
			}
		}
#endif //#if DESHI_RELOADABLE_UI
		
		uiUpdate();
        {
            using namespace UI;
            Begin(STR8("debuggingUIwithUI"));{
				
            }End();
        }
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