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
#include "core/platform.h"
#include "core/render.h"
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

//// tunnler inludes ////
#define ZED_NET_IMPLEMENTATION
//// tunnler includes ////
#include "types.h"
#include "board.cpp"
#include "external/zed_net.h"
#include "types.h"
#include "networking.cpp"

int main(){
	//init deshi
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	console_init();
	DeshWindow->Init(str8l("sandbox"), 1280, 900);
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
    render_use_default_camera();
	DeshThreadManager->init();
	net_init_client(str8l("localhost"), 24465);
	
	init_board(20, 10);
	
	//start main loop
	Stopwatch frame_stopwatch = start_stopwatch();
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		DeshWindow->Update();
		platform_update();
		console_update();
		
		draw_board();
		
		{
			using namespace UI;
			static NetInfo info_out;
			static NetInfo info_in;

			Begin(str8l("nettest"));{
				if(Button(str8l("send"))){
					net_client_send(info_out);
				}
				if(Button(str8l("receieve"))){
					NetInfo in = net_client_recieve();
					if(CheckMagic(in)){
						info_in = in;
					}
				}
				if(BeginCombo(str8l("move"), MoveStrs[info_out.move])){
					forI(Move_COUNT){
						if(Selectable(MoveStrs[i], i==info_out.move)){
							info_out.move = i;
						}
					}
					EndCombo();
				}
				PushVar(UIStyleVar_ItemSpacing, vec2::ZERO);
				SetNextWindowSize(MAX_F32, GetWindowRemainingSpace().y / 2);
				BeginChild(str8l("outwin"), vec2::ZERO);{
					BeginRow(str8l("textalign1"), 2, 0, UIRowFlags_AutoSize);
					string out = toStr(info_out.pos);
					Text(str8l("pos: ")); Text({(u8*)out.str, out.count});
					Text(str8l("move: ")); Text(MoveStrs[info_out.move]);
					EndRow();
				}EndChild();
				SetNextWindowSize(MAX_F32, GetWindowRemainingSpace().y);
				BeginChild(str8l("inwin"), vec2::ZERO);{
					BeginRow(str8l("textalign1"), 2, 0, UIRowFlags_AutoSize);
					string out = toStr(info_in.pos);
					Text(str8l("pos: ")); Text({(u8*)out.str, out.count});
					Text(str8l("move: ")); Text(MoveStrs[info_in.move]);
					EndRow();
				}EndChild();
				PopVar();
			}End();
		}
		UI::DemoWindow();
		UI::Update();
		render_update();
		logger_update();
		memory_clear_temp();
		DeshTime->frameTime = reset_stopwatch(&frame_stopwatch);
	}
	
	//cleanup deshi
    render_cleanup();
	DeshWindow->Cleanup();
	logger_cleanup();
	memory_cleanup();
}