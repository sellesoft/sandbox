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
	
	//Vertex2* vbuff = (Vertex2*)memalloc(sizeof(Vertex2)*100);
	//RenderTwodIndex* ibuff = (RenderTwodIndex*)memalloc(sizeof(RenderTwodIndex)*300);
	//RenderDrawCounts counts{0};
	//RenderTwodBuffer buff = render_create_external_2d_buffer(sizeof(Vertex2)*100, sizeof(RenderTwodIndex)*300);
	////render_start_cmd2_exbuff(vbuff, ibuff, 5, 0, vec2::ZERO, DeshWindow->dimensions);
	//counts+=render_make_filledcircle(vbuff, ibuff, counts, vec2::ONE * 100, 40, 20, Color_Cyan);
	//counts+=render_make_filledrect(vbuff, ibuff, counts, vec2(300, 300), vec2::ONE*300, Color_Red);
	//render_update_external_2d_buffer(&buff, vbuff, counts.vertices, ibuff, counts.indices);
	
	Assert(false);
	{ //load UI funcs
#if DESHI_RELOADABLE_UI
		if(file_exists(STR8("deshi_temp.dll"))) file_delete(STR8("deshi_temp.dll"));
		file_copy(STR8("deshi.dll"), STR8("deshi_temp.dll"));
		g_ui->module = platform_load_module(STR8("deshi_temp.dll"));
		if(g_ui->module){
			//TODO(delle) functions arent loading
			g_ui->make_item         = platform_get_module_function(g_ui->module, "ui_make_item", ui_make_item);
			g_ui->begin_item        = platform_get_module_function(g_ui->module, "ui_begin_item", ui_begin_item);
			g_ui->end_item          = platform_get_module_function(g_ui->module, "ui_end_item", ui_end_item);
			g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
			g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
			g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
			g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
			g_ui->make_text         = platform_get_module_function(g_ui->module, "ui_make_text", ui_make_text);
			g_ui->init              = platform_get_module_function(g_ui->module, "ui_init", ui_init);
			g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
			g_ui->module_valid = (g_ui->make_item && g_ui->begin_item && g_ui->end_item && g_ui->make_window && g_ui->begin_window
								  && g_ui->end_window && g_ui->make_text && g_ui->make_button && g_ui->init && g_ui->update);
		}
		if(!g_ui->module_valid){
			g_ui->make_item         = ui_make_item__stub;
			g_ui->begin_item        = ui_begin_item__stub;
			g_ui->end_item          = ui_end_item__stub;
			g_ui->make_window       = ui_make_window__stub;
			g_ui->begin_window      = ui_begin_window__stub;
			g_ui->end_window        = ui_end_window__stub;
			g_ui->make_button       = ui_make_button__stub;
			g_ui->make_text         = ui_make_text__stub;
			g_ui->init              = ui_init__stub;
			g_ui->update            = ui_update__stub;
			//TODO(delle) asserts arent working
			Assert(!"module is invalid");
		}
#endif //#if DESHI_RELOADABLE_UI
	}
	uiInit(deshi_allocator, deshi_temp_allocator);
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	
	
	// uiItem* item0 = 0;
	// uiItem* item1 = 0;
	// uiItem* item2 = 0;
	// uiItem* item3 = 0;
	// uiStyle style{};
	// memcpy(&style, ui_initial_style, sizeof(uiStyle));
	// style.paddingtl = vec2i(20,20);
	// style.positioning = pos_draggable_relative;
	// style.margintl = vec2i(20,20); 
	// (item0=uiItemBS(&style))->id = STR8("item0");{
	// 	memcpy(&style, ui_initial_style, sizeof(uiStyle));
	// 	style.height = 50;
	// 	style.background_color = Color_DarkCyan;
	// 	(item1=uiItemBS(&style))->id = STR8("item1");{
	// 		memcpy(&style, ui_initial_style, sizeof(uiStyle));
	// 		style.width = 100;
	// 		style.height = 10;
	// 		style.margin_top = 10;
	// 		style.margin_left = 10;
	// 		style.border_style = border_solid;
	// 		style.border_color = color(255,255,255);
	// 		style.top = 10;
	// 		(item2=uiItemBS(&style))->id = STR8("item2")
	// 		}uiItemE();
	// 		memcpy(&style, ui_initial_style, sizeof(uiStyle));
	// 		style.background_color = color(25,25,25);
	// 		style.height = 20;
	// 		style.width = 10;
	// 		style.margin_top = 10;
	// 		style.margin_left = 10;
	// 		(item3=uiItemBS(&style))->id = STR8("item3")
	// 		}uiItemE();
	// 	}uiItemE();
	// }uiItemE();
	
	
	
	//uiStyle style{};
	//memcpy(&style, ui_initial_style, sizeof(uiStyle));
	////style.margintl = vec2i(3,3);
	//style.padding_left = 3;
	//style.padding_top = 3;
	//style.positioning = pos_relative;
	//style.border_style = border_solid;
	//style.border_color = Color_White;
	//const u32 n = 50;
	//uiItem* items[n] = {0};
	//forI(n){
	//	style.background_color = color(f32(i)/n*255,40,100);
	//	items[i] = uiItemBS(&style);
	//}
	//
	//style.height = 10;
	//style.width = 10;
	//uiItemBS(&style);
	//uiItemE();
	
	//forI(n){
	//	uiItemE();
	//}
	//
	
	
	// uiStyle style{};style = *ui_initial_style;
	// style.margintl = {10,10};
	// style.border_style = border_solid;
	// style.border_color = Color_White;
	// style.height = 20;
	// style.width = 20;
	// style.background_color = Color_DarkGrey;
	// uiItemMS(&style);
	
	// uiStyle style{};style=*ui_initial_style;
	// style.margintl = {100,100};
	// style.paddingtl = {10,10};
	// //style.paddingtl = {0};
	// style.background_color = Color_DarkBlue;
	// style.content_align= 0.5;
	// style.size = {100,100};
	// style.positioning = pos_draggable_relative;
	// const u32 n = 20;
	// uiItemBS(&style)->id=STR8("container");{
	// 	style=*ui_initial_style;
	// 	style.margintl = {10,10};
	// 	uiTextMLS("hello1", &style)->item.id = STR8("text1");
	// 	uiTextMLS("hello long", &style)->item.id = STR8("text2");
	// 	uiTextMLS("he", &style)->item.id = STR8("text3");
	// 	uiTextMLS(":)", &style)->item.id = STR8("text4");
	// 	uiTextMLS(":O!!", &style)->item.id = STR8("text5");
	// }uiItemE();
	
	u32 n = 20;
	uiStyle style{};style=*ui_initial_style;
	style.paddingtl={0,0};
	style.margintl ={0,0};
	style.content_align=1;
	forI(n){
		style.background_color = color(0,u8(f32(i)/n*255),(u8)255-u8(f32(i)/n*255));
		uiItemBS(&style);
		uiTextML("hello");
	}
	style=*ui_initial_style;
	style.size = {10,10};
	uiItemBS(&style);
	uiItemE();
	forI(n){
		uiTextML("hello again");
		uiItemE();
	}
	
	
	
	//start main loop
	while(platform_update()){DPZoneScoped;
		
		//render_start_cmd2_exbuff(buff, 0, counts.indices, vbuff, ibuff, 5, 0, vec2::ZERO, DeshWindow->dimensions);
#if DESHI_RELOADABLE_UI
		if(key_pressed(Key_F5 | InputMod_AnyAlt)){
			//unload the module
			g_ui->module_valid = false;
			if(g_ui->module){
				platform_free_module(g_ui->module);
				g_ui->module = 0;
			}
			
			//load the module
			file_delete(STR8("deshi_temp.dll"));
			file_copy(STR8("deshi.dll"), STR8("deshi_temp.dll"));
			g_ui->module = platform_load_module(STR8("deshi_temp.dll"));
			if(g_ui->module){
				//TODO(delle) functions arent loading
				g_ui->make_item         = platform_get_module_function(g_ui->module, "ui_make_item", ui_make_item);
				g_ui->begin_item        = platform_get_module_function(g_ui->module, "ui_begin_item", ui_begin_item);
				g_ui->end_item          = platform_get_module_function(g_ui->module, "ui_end_item", ui_end_item);
				g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
				g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
				g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
				g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
				g_ui->make_text         = platform_get_module_function(g_ui->module, "ui_make_text", ui_make_text);
				g_ui->init              = platform_get_module_function(g_ui->module, "ui_init", ui_init);
				g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
				g_ui->module_valid = (g_ui->make_item && g_ui->begin_item && g_ui->end_item && g_ui->make_window && g_ui->begin_window
									  && g_ui->end_window && g_ui->make_button && g_ui->make_text && g_ui->init && g_ui->update);
			}
			if(!g_ui->module_valid){
				g_ui->make_item         = ui_make_item__stub;
				g_ui->begin_item        = ui_begin_item__stub;
				g_ui->end_item          = ui_end_item__stub;
				g_ui->make_window       = ui_make_window__stub;
				g_ui->begin_window      = ui_begin_window__stub;
				g_ui->end_window        = ui_end_window__stub;
				g_ui->make_button       = ui_make_button__stub;
				g_ui->make_text         = ui_make_text__stub;
				g_ui->init              = ui_init__stub;
				g_ui->update            = ui_update__stub;
				//TODO(delle) asserts arent working
				Assert(!"module is invalid");
			}
		}
#endif //#if DESHI_RELOADABLE_UI
		
		//forI(n){
		//if(!i) continue;
		////items[i]->style.left = 20*(sin(2*M_PI*DeshTotalTime/1000/3 + i)+1)/2;
		////items[i]->style.top = 20*(cos(2*M_PI*DeshTotalTime/1000/3 + i)+1)/2;
		//items[i]->style.border_width = 20*(sin(2*M_PI*DeshTotalTime/1000/3)+1)/2;
		//}
		render_start_cmd2(5,0,vec2::ZERO,DeshWindow->dimensions);
		render_quad2(vec2::ONE * 300, vec2::ONE*300);
		
		uiUpdate();
		string fps = toStr(1000/DeshTime->deltaTime);
		render_start_cmd2(5, Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second->tex, vec2::ZERO, DeshWindow->dimensions);
		render_text2(Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second, str8{(u8*)fps.str, fps.count}, vec2(0,DeshWindow->dimensions.y / 2), vec2::ONE, Color_White);
		
        {
            using namespace UI;
            //Begin(STR8("debuggingUIwithUI"));{
			//	Text(STR8("here some text"));
            //}End();
        }
		
		//console_update();
		//UI::Update();
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