//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @includes
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
#define MINIAUDIO_IMPLEMENTATION
#include "external/zed_net.h"
#include "external/miniaudio.h"
#include "types.h"
#include "sound.cpp"
#include "networking.cpp"
#include "game.cpp"

void update_debug(){
	using namespace UI;
	/*
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
		if(BeginCombo(str8l("move"), MessageStrings[info_out.message])){
			forI(Message_COUNT){
				if(Selectable(MessageStrings[i], i==info_out.message)){
					info_out.message = i;
				}
			}
			EndCombo();
		}
		PushVar(UIStyleVar_ItemSpacing, vec2::ZERO);
		SetNextWindowSize(MAX_F32, GetWindowRemainingSpace().y / 2);
		BeginChild(str8l("outwin"), vec2::ZERO);{
			BeginRow(str8l("textalign1"), 2, 0, UIRowFlags_AutoSize);
			string out = toStr("(",info_out.x,",",info_out.y,")");
			Text(str8l("pos: ")); Text({(u8*)out.str, out.count});
			Text(str8l("move: ")); Text(MessageStrings[info_out.message]);
			EndRow();
		}EndChild();
		SetNextWindowSize(MAX_F32, GetWindowRemainingSpace().y);
		BeginChild(str8l("inwin"), vec2::ZERO);{
			BeginRow(str8l("textalign2"), 2, 0, UIRowFlags_AutoSize);
			string out = toStr("(",info_out.x,",",info_out.y,")");
			Text(str8l("pos: ")); Text({(u8*)out.str, out.count});
			Text(str8l("move: ")); Text(MessageStrings[info_in.message]);
			EndRow();
		}EndChild();
		PopVar();
	}End();
	*/
	//DemoWindow();
	
	if(key_pressed(Key_ENTER | InputMod_AnyCtrl)){
		game_active = true;
	}
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @main
int main(){
	//init deshi
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	console_init();
	DeshWindow->Init(str8l("TUNNLER"));
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
    render_use_default_camera();
	DeshThreadManager->init();
	net_init_client(str8l("192.168.0.255"), 24465);
	sound_init();
	init_game();
	DeshThreadManager->spawn_thread();
	
	//start main loop
	Stopwatch frame_stopwatch = start_stopwatch();
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		sound_update();
		DeshWindow->Update();
		platform_update();
		console_update();
		update_debug();
		if(game_active) update_game();
		else{
			persist u32 menu_state = 0;
			using namespace UI;
			SetNextWindowPos(vec2::ZERO);
			SetNextWindowSize(DeshWinSize);
			Begin(str8l("menu"), UIWindowFlags_NoInteract);{
				PushVar(UIStyleVar_ItemSpacing, vec2(0, 10));
				PushVar(UIStyleVar_FontHeight, 50);
				Text(str8l("TUNNLER"));
				PopVar();
				switch(menu_state){
					case 0:{ //host/join buttons
						//TODO(sushi) nicer button styling PushColor(UIStyleCol_ButtonBg,)
						if(Button(str8l("Join Game"))) menu_state = 1;
						if(Button(str8l("Host Game"))) menu_state = 2;
						
					}break;
					case 1:{ //joining game
						if(net_join_game()){
							f64 t = DeshTotalTime/1000*2;
							CircleFilled(vec2(DeshWinSize.x / 2-15, DeshWinSize.y/2 - 20 - 10*(sin(t+0 - cos(t+0))+1)/2), 2);
							CircleFilled(vec2(DeshWinSize.x / 2+00, DeshWinSize.y/2 - 20 - 10*(sin(t+1 - cos(t+1))+1)/2), 2);
							CircleFilled(vec2(DeshWinSize.x / 2+15, DeshWinSize.y/2 - 20 - 10*(sin(t+2 - cos(t+2))+1)/2), 2);
							str8 text = str8l("searching for game..");
							vec2 size = CalcTextSize(text);
							Text(text, (DeshWinSize - size)/2);
							if(Button(str8l("Cancel"))) {
								menu_state = 0;
								close_listener = true;
								join_phase = 0;
							}
						} else game_active = true;
					}break;
					case 2:{ //hosting game
						if(net_host_game()){
							f64 t = DeshTotalTime/1000*2;
							CircleFilled(vec2(DeshWinSize.x / 2-15, DeshWinSize.y/2 - 20 - 10*(sin(t+0 - cos(t+0))+1)/2), 2);
							CircleFilled(vec2(DeshWinSize.x / 2+00, DeshWinSize.y/2 - 20 - 10*(sin(t+1 - cos(t+1))+1)/2), 2);
							CircleFilled(vec2(DeshWinSize.x / 2+15, DeshWinSize.y/2 - 20 - 10*(sin(t+2 - cos(t+2))+1)/2), 2);
							str8 text = str8l("searching for player..");
							vec2 size = CalcTextSize(text);
							Text(text, (DeshWinSize - size)/2);
							if(Button(str8l("Cancel"))) {
								menu_state = 0;
								close_listener = true;
								host_phase = 0;
								
							}
						} else game_active = true;
					}break;
				}
				PopVar();
			}End();
		}
		
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