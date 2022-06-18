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

local uiContext deshi_ui{}; uiContext* g_ui = &deshi_ui;
local uiStyle deshi_ui_initial_style{}; uiStyle* ui_initial_style = &deshi_ui_initial_style;
f64 avg_fps_v;
mutex avgfps_lock;
void avg_fps(void* in){
	const u64 nframes = 1000;
	static f64 fpsarr[nframes] = {0};
	static u32 idx = 0;
	if(avgfps_lock.try_lock()){
		//uiTextData* text = uiGetTextData((uiItem*)in);
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
	

	//Vertex2* vbuff = (Vertex2*)memalloc(sizeof(Vertex2)*100);
	//RenderTwodIndex* ibuff = (RenderTwodIndex*)memalloc(sizeof(RenderTwodIndex)*300);
	//RenderDrawCounts counts{0};
	//RenderTwodBuffer buff = render_create_external_2d_buffer(sizeof(Vertex2)*100, sizeof(RenderTwodIndex)*300);
	////render_start_cmd2_exbuff(vbuff, ibuff, 5, 0, vec2::ZERO, DeshWindow->dimensions);
	//counts+=render_make_filledcircle(vbuff, ibuff, counts, vec2::ONE * 100, 40, 20, Color_Cyan);
	//counts+=render_make_filledrect(vbuff, ibuff, counts, vec2(300, 300), vec2::ONE*300, Color_Red);
	//render_update_external_2d_buffer(&buff, vbuff, counts.vertices, ibuff, counts.indices);
	
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
			Assert(!"module is invalid");
		}
#endif //#if DESHI_RELOADABLE_UI
	}
	uiInit(g_memory, g_ui);
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");

	// uiItem* fpstext;
	// uiItem* fpswin = uiItemB();{
	// 	fpswin->style.background_color = color(14,14,14);
	// 	fpswin->style.padding = {10,10};
	// 	fpswin->style.border_style = border_solid;
	// 	fpstext = uiTextML("                 "); //space to reserve room for vertices
	// 	fpstext->action = [](uiItem* item){
	// 		DeshThreadManager->add_job({avg_fps,item});
	// 		DeshThreadManager->wake_threads();
	// 	};
	// 	fpstext->action_trigger = action_act_always;
	// }uiItemE();
	
	// uiItem* item0 = 0;
	// uiItem* item1 = 0;
	// uiItem* item2 = 0;
	// uiItem* item3 = 0;
	// uiStyle style{};
	// memcpy(&style, ui_initial_style, sizeof(uiStyle));
	// style.padding = vec2i(20,20);
	// style.positioning = pos_draggable_relative;
	// style.margin = vec2i(20,20); 
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
	
	
	
	// uiStyle style{};
	// memcpy(&style, ui_initial_style, sizeof(uiStyle));
	// //style.margin = vec2i(3,3);
	// style.padding_left = 3;
	// style.padding_top = 3;
	// style.positioning = pos_relative;
	// style.border_style = border_solid;
	// style.border_color = Color_White;
	// const u32 n = 50;
	// uiItem* items[n] = {0};
	// forI(n){
	// 	style.background_color = color(f32(i)/n*255,40,100);
	// 	items[i] = uiItemBS(&style);
	// }
	
	// style.height = 10;
	// style.width = 10;
	// uiItemBS(&style);
	// uiItemE();
	
	// forI(n){
	// 	uiItemE();
	// }
	

	// uiStyle style{}; style=*ui_initial_style;
	// style.padding = {1,1};
	// style.tl = {100,100};
	// style.border_style = border_solid;
	// style.border_width=1;
	// style.border_color = Color_White;
	// const u32 n = 70;
	// uiItem* items[n+1];
	// forI(n){
	// 	items[i]=uiItemBS(&style);
	// }
	// style.size={1,1};
	// items[n]=uiItemMS(&style);
	// forI(n){
	// 	uiItemE();
	// }

	
	
	// uiStyle style{};style = *ui_initial_style;
	// style.margin = {10,10};
	// style.border_style = border_solid;
	// style.border_color = Color_White;
	// style.height = 20;
	// style.width = 20;
	// style.background_color = Color_DarkGrey;
	// uiItemMS(&style);
	
	// uiStyle style{};style=*ui_initial_style;
	// style.margin = {100,100};
	// style.padding = {10,10};
	// //style.padding = {0};
	// style.background_color = Color_DarkBlue;
	// style.content_align= 0.5;
	// style.size = {100,100};
	// style.positioning = pos_draggable_relative;
	// const u32 n = 20;
	// uiItemBS(&style)->id=STR8("container");{
	// 	style=*ui_initial_style;
	// 	style.margin = {10,10};
	// 	uiTextMLS("hello1", &style)->item.id = STR8("text1");
	// 	uiTextMLS("hello long", &style)->item.id = STR8("text2");
	// 	uiTextMLS("he", &style)->item.id = STR8("text3");
	// 	uiTextMLS(":)", &style)->item.id = STR8("text4");
	// 	uiTextMLS(":O!!", &style)->item.id = STR8("text5");
	// }uiItemE();
	
	// u32 n = 50;
	// uiStyle style{};style=*ui_initial_style;
	// style.padding={1,1};
	// style.margin ={1,1};
	// style.content_align=0.5;
	// style.positioning = pos_draggable_relative;
	// style.background_image=Storage::CreateTextureFromFile(STR8("background.png")).second;
	// forI(n){
	// 	style.background_color = color(0,u8(f32(i)/n*255),(u8)255-u8(f32(i)/n*255));
	// 	uiItemBS(&style);
	// }
	// style=*ui_initial_style;
	// style.size = {2,2};
	// uiItem* i = uiItemBS(&style);
	// uiItemE();
	// forI(n){
	// 	uiItemE();
	// }
	
	// uiItem* item0;
	// uiItem* item1;
	// uiStyle style{};style=*ui_initial_style;
	// style.padding={10,10};
	// style.background_color = Color_DarkRed;
	// style.positioning = pos_draggable_relative;
	// (item0=uiItemBS(&style))->id=STR8("item0");{
	// 	style.background_color = Color_DarkBlue;
	// 	style.size = {20,20};
	// 	(item1=uiItemBS(&style))->id=STR8("item1");{
	
	// 	}uiItemE();
	// }uiItemE();
	
	// uiStyle style{};style=*ui_initial_style;
	// uiItemB();{
	// 	style.size = {10,10};
	// 	style.background_color = Color_White;
	// 	uiItemBS(&style);{
	
	// 	}uiItemE();
	// }uiItemE();
	
	// uiStyle style{};style=*ui_initial_style;
	// style.size = {100,100};
	// style.background_color=Color_Blue;
	// uiItemBS(&style);{
	// 	style.size = {percent(25), percent(75)};
	// 	style.background_color = Color_Red;
	// 	uiItemMS(&style);
	// }uiItemE();

	// uiStyle style{}; style=*ui_initial_style;
	// style.background_color = Color_VeryDarkCyan;
	// style.border_style = border_solid;
	// style.border_width = 5;
	// style.size = {300,300};
	// style.positioning = pos_draggable_relative;
	// uiItem* item0;
	// uiItem* item1;
	// uiItem* item2;

	// item0 = uiItemBS(&style);{
	// 	style=*ui_initial_style;
	// 	style.background_color = Color_Green;
	// 	style.size = {100,100};
	// 	style.margin = {3,3};
	// 	item1 = uiItemBS(&style);{
	// 		style.background_color = Color_White;
	// 		style.size = {10,10};
	// 		item2 = uiItemMS(&style);
	// 	}uiItemE();
	// }uiItemE();

	// item0->id = STR8("item0");
	// item1->id = STR8("item1");
	
	// uiStyle style{}; style=*ui_initial_style;
	// style.overflow = overflow_hidden;
	// style.background_color = color(0,155,0);
	// style.size = {300,300};
	// style.positioning = pos_draggable_relative;
	// uiItemBS(&style)->id=STR8("item0");{
	// 	style.background_color = color(0,0,155);
	// 	style.size = {100,100};
	// 	uiItemBS(&style)->id=STR8("item1");{

	// 	}uiItemE();	
	// }uiItemE();

	// uiStyle style{}; style=*ui_initial_style;
	// style.size = {300,300};
	// style.background_color = Color_DarkCyan;
	// style.positioning = pos_draggable_relative;
	// style.content_align = {0.5,0.5};
	// uiItem* items[10];
	// uiItemBS(&style);{
	// 	style.positioning = pos_static;
	// 	style.size = {300/20,300/20};
	// 	style.margin = {10,10};
	// 	forI(10){
	// 		style.background_color = color(255*f32(i)/10, 100, 100);
	// 		items[i]=uiItemMS(&style);
	// 	}
	// }uiItemE();


	// uiStyle style{}; style=*ui_initial_style;
	// style.size = {300,300};
	// style.background_color = color(100,150,200);
	// uiItemBS(&style);{
	// 	style.size = {30,30};
	// 	style.background_color = color(200,150,100);
	// 	style.flags = uiFlag_ActAlways;
	// 	uiItemMS(&style)->action = 
	// 		[](uiItem* item)->void{
	// 			item->style.background_color = color(255*(sin(DeshTotalTime)+1)/2, 100, 255*(cos(DeshTotalTime)+1)/2);
	// 		};
	// 	style.flags = uiFlag_ActOnMouseHover;
	// 	uiItemMS(&style)->action =
	// 		[](uiItem* item)->void{
	// 			item->style.background_color = color(255*(sin(DeshTotalTime)+1)/2, 100, 255*(cos(DeshTotalTime)+1)/2);
	// 		};
	// 	style.flags = uiFlag_ActOnMousePressed;
	// 	uiItemMS(&style)->action =
	// 		[](uiItem* item)->void{
	// 			item->style.background_color = color(255*(sin(DeshTotalTime)+1)/2, 100, 255*(cos(DeshTotalTime)+1)/2);
	// 		};
	// 	style.flags = uiFlag_ActOnMouseReleased;
	// 	uiItemMS(&style)->action =
	// 		[](uiItem* item)->void{
	// 			item->style.background_color = color(255*(sin(DeshTotalTime)+1)/2, 100, 255*(cos(DeshTotalTime)+1)/2);
	// 		};
	// 	style.flags = uiFlag_ActOnMouseDown;
	// 	uiItemMS(&style)->action =
	// 		[](uiItem* item)->void{
	// 			item->style.background_color = color(255*(sin(DeshTotalTime)+1)/2, 100, 255*(cos(DeshTotalTime)+1)/2);
	// 		};
	// }uiItemE();

	// f32 a = 2;
	// uiItem* item0 = uiItemB();
	// 	item0->id=STR8("item0");
	// 	item0->style.background_color = color(14,14,14);
	// 	item0->style.border_style = border_solid;
	// 	item0->style.size = {100,100};
	// 	item0->style.padding = {10,10};
	// 	item0->style.margin;
	// 	item0->style.positioning = pos_draggable_relative;
	// 	uiItem* item = uiSliderf32(0, 10, &a);
	// 	item->style.background_color = Color_White;
	// 	item->style.size = {80,10};
	// 	item->id=STR8("slider");
	// 	uiGetSliderData(item)->style.rail_thickness = 0.5;
	// 	item->style.sizing = size_percent_x;
	// 	item->style.width = 100;
	
	// uiStyle style{}; style=*ui_initial_style;
	// style.focus = true;
	// style.size = {300,300};
	// style.positioning = pos_draggable_fixed;

	// uiItem* item0 = uiItemB();{
	// 	item0->style.size = {300,300};
	// 	item0->style.background_color = Color_VeryDarkCyan;
	// 	item0->style.padding = {0,0};
	// 	item0->style.positioning = pos_draggable_relative;
	// 	item0->style.overflow = overflow_scroll;
		
	// 	forI(301){
	// 		uiItem* item = uiItemM();
	// 		item->style.background_color = color(f32(i)/300*255,100,100);
	// 		item->style.size = {20,1};
	// 	}	
	// }uiItemE();

	//ui_debug();


	
	// {uiItem* container = uiItemB();
	// 	container->style.size = DeshWindow->dimensions;
	// 	container->style.content_align = {0,0};
	// 	container->id = STR8("container");
	// 	container->style.positioning = pos_draggable_relative;
	// 	{uiItem* win = uiItemB();
	// 		win->id = STR8("win");
	// 		win->style.background_color = color(14,14,14);
	// 		win->style.sizing = size_percent_y | size_square;
	// 		win->style.height = 92;
	// 		win->style.padding = {2,2};
	// 		win->style.content_align = {0,0};
			
	// 		const u64 n = 100;
	
	// 		forI(n){
	// 			uiItem* item = uiItemBS(&win->style);
	// 			item->style.background_color = color((i%2?255:0), 100, 100);
	// 			item->style.border_style = border_solid;
	// 			item->style.border_width = 2;
	// 			item->action = [](uiItem* item){
	// 				item->style.content_align.x = 0;//(sin(DeshTotalTime/1000 + (u64)item)+1)/2;
	// 				item->style.content_align.y = 0;//(cos(DeshTotalTime/1000 + (u64)item)+1)/2;
	// 				item->style.height = Math::BoundedOscillation(90, 95, DeshTotalTime/500 + (u64)item*2*M_PI);
	// 			};
	// 			item->action_trigger = action_act_always;
	// 		}

	// 		forI(n){
	// 			uiItemE();
	// 		} 
	// 	}uiItemE();
	// }uiItemE();

	// {uiItem* item = uiItemB();
	// 	item->style.border_style = border_solid;
	// 	item->style.positioning = pos_draggable_relative;
	// 	item->style.size = {100,100};
	// 	item->action = [](uiItem* item){
	// 		item->style.border_width = BoundTimeOsc(1, 10);
	// 	};
	// }uiItemE();

	// {uiItem* cont = uiItemB();
	// 	cont->style.size = {500,500};
	// 	cont->style.content_align = {0.5,0.5};
	// 	cont->style.background_color = color(50,100,100);
	// 	cont->style.margin = {100,100};
	// 	uiItemM()->style.size = {250,250};

	// }


	// uiItem* item0 = uiItemB();
	// item0->style.size = {300,300};
	// item0->style.background_color = Color_VeryDarkCyan;
	// item0->style.padding = {10,10};
	// item0->style.positioning = pos_draggable_relative;
	// 	uiItem* item1 = uiItemB();
	// 		item1->style.background_color = Color_Red;
	// 		item1->style.size = {100,100};
	// 		uiItem* item2 = uiItemB();
	// 			item2->style.background_color = Color_Blue;
	// 			item2->style.size = {50,50};
	// 		uiItemE();
	// 	uiItemE();
	// 	uiItem* item3 = uiItemB();
	// 		item3->style.background_color = Color_Green;
	// 		item3->style.size = {100,100};
	// 	uiItemE();
	// uiItemE();

	ui_demo();
	
	//start main loop
	while(platform_update()){DPZoneScoped;
		f32 t = DeshTotalTime/1000;
		//DeshThreadManager->add_job({avg_fps,0});
		//DeshThreadManager->wake_threads();

		//item0->style.border_width = BoundTimeOsc(1, 10);
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
		// f64 t = DeshTotalTime/1000;
		// forI(n){
		// 	//items[i]->style.margin_top    = 10*(sin(t+1*i*M_PI/5)+1)/2;
		// 	//items[i]->style.margin_right  = 10*(sin(t+2*i*M_PI/5)+1)/2; 
		// 	//items[i]->style.margin_bottom = 10*(sin(t+3*i*M_PI/5)+1)/2; 
		// 	//items[i]->style.margin_left   = 10*(sin(t+4*i*M_PI/5)+1)/2; 
		// 	items[i]->style.border_width = 10*(sin(t+i)+1)/2; 
		// 	if(items[i]->style.border_width < 1) items[i]->style.border_width = 0; 
		// }

		//item0->style.scry = 
		//item0->dirty = 1;

		//if(key_pressed(Key_SPACE)) item1->style.hidden = true;
		//if(key_released(Key_SPACE)) item1->style.hidden = false;

		
		uiUpdate();
		str8 fps = toStr8(avg_fps_v, " ", 1000/DeshTime->deltaTime).fin;
		render_start_cmd2(5, Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second->tex, vec2::ZERO, DeshWindow->dimensions);
		render_text2(Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second, fps, vec2(0,DeshWindow->dimensions.y / 2), vec2::ONE, Color_White);
		

        {
			//item0->style.border_width = 100*(sin(DeshTotalTime/3000)+1)/2;
			//item1->style.margin = vec2::ONE*10*(sin(DeshTotalTime/3000)+1)/2;

            using namespace UI;
            // Begin(STR8("debuggingUIwithUI"));{
			// 	// string item0p = toStr("item0 pos: ", item0->style.tl);
			// 	// string item1p = toStr("item1 pos: ", item1->style.tl);
			// 	// Text({(u8*)item0p.str, item0p.count});
			// 	// Text({(u8*)item1p.str, item1p.count});
			
            // }End();
			
			if(g_ui->hovered){
				render_start_cmd2(7, 0, vec2::ZERO, DeshWindow->dimensions);
				render_quad2(g_ui->hovered->spos, g_ui->hovered->size, Color_Red);
			}

			// item1->style.positioning = pos_relative;
			// item1->style.left = BoundTimeOsc(-100, 400);

			// item2->style.positioning = pos_relative;
			// item2->style.top = BoundTimeOsc(-100,400);
			//item1->style.margin = vec2::ONE * 10 * (sin(t)+1)/2;
			// forI(10){
			// 	items[i]->style.margin = vec2::ONE * 10 * (sin(t)+1)/2;
			// }

			// if(g_ui->hovered){
			// 	Font* f = g_ui->base.style.font;
			// 	render_start_cmd2(6, f->tex, vec2::ZERO, DeshWindow->dimensions);
			// 	string s = toStr("p: ", g_ui->hovered->spos, "s: ", g_ui->hovered->size);
			// 	vec2 siz = font_visual_size(f, {(u8*)s.str, s.count});
			// 	render_quad_filled2(g_ui->hovered->spos, siz, Color_Black);
			// 	render_text2(f, {(u8*)s.str, s.count}, floor(g_ui->hovered->spos), vec2::ONE, Color_White);
			// }
			

			//debug display item's child bbx
			// Arena* itemA = g_ui->item_list->arena;
			// render_start_cmd2(7, 0, vec2::ZERO, DeshWindow->dimensions);
			// u8* cursor = itemA->start;
			// while(cursor < itemA->start + itemA->used){
			// 	uiItem* item = (uiItem*)cursor;
			// 	render_quad2(item->children_bbx_pos, item->children_bbx_size, Color_Red);
			// 	cursor+=item->memsize;
			// 	//Vertex2* ver = (Vertex2*)g_ui->vertex_arena->start + item->drawcmds->vertex_offset;
			// 	//u32* ind = (u32*)g_ui->index_arena->start + item->drawcmds->index_offset;
			// 	//for(u32 i = 0; i < item->drawcmds[0].counts.indices; i+=3){
			// 	//	render_triangle2(ver[ind[i]].pos, ver[ind[i+1]].pos, ver[ind[i+2]].pos, Color_Red);
			// 	//}
			// }

        }
		
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