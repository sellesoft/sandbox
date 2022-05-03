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
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/renderer.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

#if BUILD_INTERNAL
#include "misc_testing.cpp"
#endif

int main(){
	//init deshi
	memory_init(Gigabytes(1), Gigabytes(1));
	logger_init();
	console_init();
	DeshWindow->Init("sandbox", 1280, 720);
	Render::Init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix(); //used when the app doesnt make use of deshi's camera
	DeshThreadManager->init();


	//start main loop
	Stopwatch frame_stopwatch = start_stopwatch();
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		DeshWindow->Update();
		console_update();
		UI::Update();
		Render::Update();
		logger_update();
		memory_clear_temp();
		DeshTime->frameTime = reset_stopwatch(&frame_stopwatch);
	}
	
	//cleanup deshi
	Render::Cleanup();
	DeshWindow->Cleanup();
	logger_cleanup();
	memory_cleanup();
}