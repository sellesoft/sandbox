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

/*  NOTES

    Everything in this new system is a uiItem.

    (maybe) Make functions are used with retained UI while Begin is used with immediate UI

    Retained UI is stored in arenas while immediate UI is temp allocated

    Any time an item is modified or removed we find it's parent window and recalculate it entirely
        this can probably be optimized
    
    For the reason above, initial generation of a uiItem and it's actual drawinfo generation
        are in separate functions

    UI macros always automatically apply the str8_lit macro to arguments that ask for text

    uiWindow cursors never take into account styling such as padding (or well, not sure yet)

    Everything in the interface is prefixed with "ui" (always lowercase)
        type and macro names follow the prefix and are UpperCamelCase
        function names have a _ after the prefix and are lower_snake_case

    Everything in this system is designed to be as dynamic as possible to enable
    minimal rewriting when it comes to tweaking code. Basically, there should be almost
    no hardcoding of anything and everything that can be abstracted out to a single function
    should be.

*/


#include "ui2.h"
#include "ui2.cpp"


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

    uiWindow* win = uiMakeWindow("test", vec2::ONE*300, vec2::ONE*300, 0);


	//start main loop
	while(platform_update()){DPZoneScoped;
		console_update();
        ui_update();
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