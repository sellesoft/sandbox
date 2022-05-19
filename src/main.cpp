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

    Make functions are used with retained UI while Begin is used with immediate UI

    Retained UI is stored in arenas while immediate UI is temp allocated

*/


typedef void (*Action)(void*);

enum{
    uiColor_Text,
    uiColor_Separator,
    uiColor_WindowBg,
    uiColor_WindowBorder,
    uiColor_COUNT
};

enum{
    uiStyle_WindowMargins,
    uiStyle_ItemSpacing,
    uiStyle_WindowBorderSize,
    uiStyle_ButtonBorderSize,
};

struct uiStyle{
    vec2  window_margins;
    vec2  item_spacing;
    f32   window_border_size;
    f32   button_border_size;
    color colors[uiColor_COUNT];
    Font* font;
};

struct uiStyleColorMod{
    Type  col;
    color old;
};

struct uiStyleVarMod{
    Type var;
    f32 old[2];
};

struct uiStyleVarType{
    u32 count;
    u32 offset;
};

local const uiStyleVarType uiStyleTypes[] = {
    {2, offsetof(uiStyle, window_margins)},
    {2, offsetof(uiStyle, item_spacing)},
    {1, offsetof(uiStyle, window_border_size)},
    {1, offsetof(uiStyle, button_border_size)},
};

array<uiStyleVarMod>   stack_var;
array<uiStyleColorMod> stack_color;

//@DrawInfo

struct DrawInfo{
    Vertex2* vertices;
    u32* indicies;
    vec2 counts;
};

struct DrawInfoCounter{
    u64 n; //number of counts
    vec2* counts;
    vec2* sums;
};

//@Item

enum uiItemType_{
    uiItemType_Window,
    uiItemType_Button,
};typedef u32 uiItemType;

struct uiItem{
    TNode node;
    union{
        struct{
            f32 x;
            f32 y;
        };
        vec2 pos;
    };
    union{
        struct{
            f32 width;
            f32 height;
        };
        vec2 size;
    };

    DrawInfo dinfo;
    
    uiItem(){} //C++ is retarded
};

#define ItemFromNode(x) CastFromMember(uiItem, node, x)

//@Window

struct uiWindow{
    uiItem item;
    str8 name;
    union{
        struct{
            f32 curx;
            f32 cury;
        };
        vec2 cursor;
    };

    uiWindow(){} //C++ is retarded
};
#define WindowFromNode(x) CastFromMember(uiWindow, item, CastFromMember(uiItem, node, x))

//@Button

enum {
    uiButtonFlags_ActOnPressed,  // button performs its action when the mouse is pressed over it 
    uiButtonFlags_ActOnReleased, // button performs its action when the mouse is released over it
    uiButtonFlags_ActOnDown,     // button performs its action when the mouse is down over it
};

struct uiButton{
    uiItem item;
    Action action;
    Flags  flags;
    b32    clicked;
};
#define ButtonFromNode(x) CastFromMember(uiButton, item, CastFromMember(uiItem, node, x))

//@Context

#define SetNextPos(x,y) uiContext.nextPos = {x,y};
#define SetNextSize(x,y) uiContext.nextSize = {x,y};
struct Context{
    vec2 nextPos;  // default: -MAx_F32, -MAX_F32; MAX_F32 does nothing
    vec2 nextSize; // default: -MAx_F32, -MAX_F32; MAX_F32 in either indiciates to stretch item to the edge of the window in that direction
    TNode base;
    uiWindow* curwin; //current working window in immediate contexts 
    uiStyle style;
}uiContext;

//we cant grow the arena because it will move the memory, so we must chunk 
struct ArenaList{
    Node node;   
    Arena* arena;
};

#define ArenaListFromNode(x) CastFromMember(ArenaList, node, x);

ArenaList* item_list;
ArenaList* drawinfo_list;
#define item_arena item_list->arena
#define drawinfo_arena drawinfo_list->arena

ArenaList* create_arena_list(ArenaList* old){
    ArenaList* nual = (ArenaList*)memalloc(sizeof(ArenaList));
    if(old) NodeInsertNext(&old->node, &nual->node);
    nual->arena = memory_create_arena(Megabytes(1));
    return nual;
}

void* add_item(void* data, upt size){
    if(item_arena->size < item_arena->used+size) 
        item_list = create_arena_list(item_list);
    u8* cursor = item_arena->cursor;
    CopyMemory(item_arena->cursor, data, size);
    item_arena->cursor += size;
    item_arena->used += size;
    return cursor;
}

void* make_item(upt size){
    if(item_arena->size < item_arena->used+size) 
        item_list = create_arena_list(item_list);
    u8* cursor = item_arena->cursor;
    item_arena->cursor += size;
    item_arena->used += size;
    return cursor;
}

DrawInfo drawinfo_alloc(vec2 counts){
    DrawInfo di;
    if(drawinfo_arena->size < drawinfo_arena->used + counts.x * sizeof(Vertex2) + counts.y * sizeof(u32))
        drawinfo_list = create_arena_list(drawinfo_list);
    di.vertices = (Vertex2*)drawinfo_arena->cursor;
    di.indicies = (u32*)drawinfo_arena->cursor + (u32)counts.x * sizeof(Vertex2);
    drawinfo_arena->cursor += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    drawinfo_arena->used += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    di.counts = counts;
    return di;
}

//@Functions

//@Helpers

vec2 ui_decide_item_pos(uiWindow* window){
    vec2 pos = window->cursor + uiContext.style.window_margins;
    return pos;
}

DrawInfoCounter ui_make_drawinfo_counts(u32 count, ...){
    DrawInfoCounter counter;
    counter.n = count;
    counter.counts = (vec2*)memtalloc(sizeof(vec2)*count);
    counter.sums = (vec2*)memtalloc(sizeof(vec2)*count);
    vec2 sum;
    va_list args;
    va_start(args, count);
    forI(count){
        vec2 v = va_arg(args, vec2);
        sum+=v;
        counter.sums[i] = sum;
        counter.counts[i] = v;
    }
    return counter;
}

//@Functionality

void ui_push_var(Type idx, f32 nu){

}

void ui_push_var(Type idx, vec2 nu){

}

// makes a window to place items in
//
// name:   name of the window
// pos:    initial position of the window
// size:   initial size of the window
uiWindow* ui_make_window(str8 name, vec2 pos, vec2 size){
    uiWindow* me       = (uiWindow*)make_item(sizeof(uiWindow));
    me->item.node.type = uiItemType_Window;
    me->item.pos       = pos;
    me->item.size      = size;
    me->cursor         = vec2::ZERO;
    DrawInfo* di = &me->item.dinfo;
    
    DrawInfoCounter counter = 
        ui_make_drawinfo_counts(2,
            render_make_filledrect_counts(),
            render_make_rect_counts()
        );
    
    *di = drawinfo_alloc(counter.sums[counter.n-1]);
    
    render_make_filledrect(di->vertices, di->indicies, {0,0}, pos, size, uiContext.style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], pos, size, 3, uiContext.style.colors[uiColor_WindowBorder]);

    insert_last(&uiContext.base, &me->item.node);
    return me;
}

// makes a child window inside another window
//
// window: uiWindow to emplace this window in
// name:   name of the window
// pos:    initial position of the window
// size:   initial size of the window
uiWindow* ui_make_child_window(uiWindow* window, str8 name, vec2 size){
    uiWindow* me       = (uiWindow*)make_item(sizeof(uiWindow));
    me->item.node.type = uiItemType_Window;
    me->item.pos       = ui_decide_item_pos(window);
    me->item.size      = size;
    me->cursor         = vec2::ZERO;
    
    vec2 draw_counts = 
        render_make_filledrect_counts() +
        render_make_rect_counts();
    //me->item.dinfo = drawinfo_alloc(draw_counts.x, draw_counts.y);
//
    //insert_last(&window->item.node, &me->item.node);
    return me;
}

// window: uiWindow to emplace the button in
// text:   text to be displayed in the button
// action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//         if 0 is passed, the button will just set it's clicked flag
// flags:  uiButtonFlags to be given to the button
uiButton* ui_make_button(uiWindow* window, str8 text, Action action, Flags flags){
    uiButton* button = (uiButton*)make_item(sizeof(uiButton));
    button->item.node.type = uiItemType_Button;
    button->item.pos = ui_decide_item_pos(window);
    button->action = action;
    button->flags = flags;
    DrawInfo* di = &button->item.dinfo;
    u32 text_size = str8_length(text);

    DrawInfoCounter counter = 
        ui_make_drawinfo_counts(3,
            render_make_filledrect_counts(),
            render_make_rect_counts(),
            render_make_text_counts(str8_length(text))
        );
    
    *di = drawinfo_alloc(counter.sums[counter.n-1]);

    vec2 tsize = UI::CalcTextSize(text);

    button->item.size = vec2(tsize.x*1.4, tsize.y*1.3);

    render_make_filledrect(di->vertices, di->indicies, {0,0}, window->item.pos + button->item.pos, button->item.size, uiContext.style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], window->item.pos + button->item.pos, button->item.size, 3, uiContext.style.colors[uiColor_WindowBorder]);
    render_make_text(di->vertices, di->indicies, counter.sums[1], text, uiContext.style.font, window->item.pos + button->item.pos, uiContext.style.colors[uiColor_Text], vec2::ONE);
    
    insert_last(&window->item.node, &button->item.node);

    return button;
}

void ui_init(){
    item_list = create_arena_list(0);
    drawinfo_list = create_arena_list(0);
    SetNextPos(-MAX_F32, -MAX_F32);
    SetNextSize(-MAX_F32, -MAX_F32);
    uiStyle* style = &uiContext.style;
    style->colors[uiColor_WindowBg]     = color(14,14,14);
    style->colors[uiColor_WindowBorder] = color(170,170,170);
    style->colors[uiColor_Text] = Color_White;

    style->font = Storage::CreateFontFromFileBDF(str8l("gohufont-11.bdf")).second;
}

void ui_recur(TNode* node){
    uiItem* item = ItemFromNode(node);
    render_start_cmd2(5, 0, item->pos, item->size);
    render_add_vertices2(5, item->dinfo.vertices, item->dinfo.counts.x, item->dinfo.indicies, item->dinfo.counts.y);
    
    //switch(node->type){
    //    case uiItemType_Window:{
    //        uiWindow* w = WindowFromNode(node);
    //        render_start_cmd2(5, 0, w->item.pos, w->item.size);
    //        render_add_vertices2(5, w->item.dinfo.vertices, w->item.dinfo.counts.x, w->item.dinfo.indicies, w->item.dinfo.counts.y);
    //    }break;
    //    case uiItemType_Button:{
    //        uiButton* b = ButtonFromNode(node);
    //        render_add_vertices2(5, b->item.dinfo.vertices, b->item.)
    //    }break;
    //}
    
    if(node->child_count){
        for_node(node->first_child){
            ui_recur(it);
        }
    }
}

void ui_update(){
    if(uiContext.base.child_count){
        ui_recur(uiContext.base.first_child);
    }
}


#define MakeButton()

int main(){
    memory_init(Gigabytes(1), Gigabytes(1));
    platform_init();
	logger_init();
	console_init();
	DeshWindow->Init(str8l("sandbox"), 1280, 720);
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
    render_use_default_camera();
	DeshThreadManager->init();
    ui_init();

    uiWindow* win = ui_make_window(str8l("test"), vec2::ONE*300, vec2::ONE*300);
    uiButton* button = ui_make_button(win, str8l("button"), 0, 0);

    Stopwatch frame_stopwatch = start_stopwatch();
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
        
        DeshWindow->Update();
		
        console_update();
		ui_update();
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