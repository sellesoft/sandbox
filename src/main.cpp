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


#define DESHI_IMPLEMENTATION_UI2
#include "ui2.h"
#if !DESHI_RELOADABLE_UI
#  include "ui2.cpp"
#endif


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
    ui_init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
    log_sizes();
	
	
	vec2 a = vec2{1.f,2.f};
	
	vec2 b = vec2{3.f,2.f};
	
	a+b;
	
    uiWindow* win = uiWindowM("test", vec2::ONE*300, vec2::ONE*300, 0);
	u32 x = 1;
	
	//start main loop
	while(platform_update()){DPZoneScoped;
		Log("", *(f32*)(&x));
		x++;
		console_update();
        ui_update();
        {
            using namespace UI;
            Begin(STR8("debuggingUIwithUI"));{
				
            }End();
			
			if(key_pressed(Key_F5 | InputMod_AnyAlt)){
				ui_reload_functions();
			}
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