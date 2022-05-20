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

    Any time an item is modified or removed we find it's parent window and recalculate it entirely

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

//@uiDrawCmd

struct uiDrawCmd{
    Texture* texture;
    Vertex2* vertices;
    u32*     indicies;
    vec2     counts;
};

struct uiDrawCmdCounter{
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

    u64 draw_cmd_count;
    uiDrawCmd* draw_cmds;
    
    uiStyle style; // style at time of making this item. eventually this should be optimized to not store the entire style

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
ArenaList* drawcmd_list;
#define item_arena item_list->arena
#define drawcmd_arena drawcmd_list->arena

ArenaList* create_arena_list(ArenaList* old){
    ArenaList* nual = (ArenaList*)memalloc(sizeof(ArenaList));
    if(old) NodeInsertNext(&old->node, &nual->node);
    nual->arena = memory_create_arena(Megabytes(1));
    return nual;
}

void* arena_add(Arena* arena, upt size){
    if(arena->size < arena->used+size) 
        item_list = create_arena_list(item_list);
    u8* cursor = arena->cursor;
    arena->cursor += size;
    arena->used += size;
    return cursor;
}

uiDrawCmd drawcmd_alloc(vec2 counts){
    uiDrawCmd di;
    if(drawcmd_arena->size < drawcmd_arena->used + counts.x * sizeof(Vertex2) + counts.y * sizeof(u32))
        drawcmd_list = create_arena_list(drawcmd_list);
    di.vertices = (Vertex2*)drawcmd_arena->cursor;
    di.indicies = (u32*)drawcmd_arena->cursor + (u32)counts.x * sizeof(Vertex2);
    di.texture = 0;
    drawcmd_arena->cursor += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    drawcmd_arena->used += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    di.counts = counts;
    return di;
}

//@Functions

//@Helpers

vec2 ui_decide_item_pos(uiWindow* window){
    vec2 pos = window->cursor + uiContext.style.window_margins;
    return pos;
}

uiDrawCmdCounter ui_make_drawinfo_counts(u32 count, ...){
    uiDrawCmdCounter counter;
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

//initializes a uiItem of some type
//used at the beginning of all uiItem functions
#define uiBeginItem(T, name, parent, item_type, n_drawcmds)                               \
T* name = (T*)arena_add(item_arena, sizeof(T));                                           \
name->item.node.type = item_type;                                                         \
name->item.draw_cmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)*n_drawcmds);\
name->item.draw_cmd_count = n_drawcmds;                                                   \
insert_last(parent, &name->item.node);                                                    \

//@Functionality

void ui_push_var(Type idx, f32 nu){

}

void ui_push_var(Type idx, vec2 nu){

}

// makes a window to place items in
//
// name: name of the window
//  pos: initial position of the window
// size: initial size of the window
uiWindow* ui_make_window(str8 name, vec2 pos, vec2 size){
    uiBeginItem(uiWindow, win, &uiContext.base, uiItemType_Window, 1);
    win->item.pos       = pos;
    win->item.size      = size;
    win->cursor         = vec2::ZERO;

    uiDrawCmdCounter counter = 
        ui_make_drawinfo_counts(2,
            render_make_filledrect_counts(),
            render_make_rect_counts()
        );
    
    uiDrawCmd* di = win->item.draw_cmds;
    *di = drawcmd_alloc(counter.sums[counter.n-1]);

    render_make_filledrect(di->vertices, di->indicies, {0,0}, pos, size, uiContext.style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], pos, size, 3, uiContext.style.colors[uiColor_WindowBorder]);

    return win;
}

// makes a child window inside another window
//
// window: uiWindow to emplace this window in
//   name: name of the window
//    pos: initial position of the window
//   size: initial size of the window
uiWindow* ui_make_child_window(uiWindow* window, str8 name, vec2 size){
    uiBeginItem(uiWindow, win, &window->item.node, uiItemType_Window, 1);
    win->item.pos       = ui_decide_item_pos(window);
    win->item.size      = size;
    win->cursor         = vec2::ZERO;
    
    vec2 draw_counts = 
        render_make_filledrect_counts() +
        render_make_rect_counts();
    return win;
}

// window: uiWindow to emplace the button in
// text:   text to be displayed in the button
// action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//         if 0 is passed, the button will just set it's clicked flag
// flags:  uiButtonFlags to be given to the button
uiButton* ui_make_button(uiWindow* window, str8 text, Action action, Flags flags){
    uiBeginItem(uiButton, button, &window->item.node, uiItemType_Button, 2);
    button->item.pos = ui_decide_item_pos(window);
    button->action = action;
    button->flags = flags;

    u32 text_size = str8_length(text);
    vec2 tsize = UI::CalcTextSize(text);
    button->item.size = vec2(tsize.x*1.4, tsize.y*1.3);

    uiDrawCmdCounter counter = 
        ui_make_drawinfo_counts(3,
            render_make_filledrect_counts(),
            render_make_rect_counts(),
            render_make_text_counts(str8_length(text))
        );
    uiDrawCmd* di = button->item.draw_cmds;
    di[0] = drawcmd_alloc(counter.sums[1]);
    di[1] = drawcmd_alloc(counter.counts[2]);
   
    render_make_filledrect(di[0].vertices, di[0].indicies, {0,0}, 
        window->item.pos + button->item.pos, 
        button->item.size,
        uiContext.style.colors[uiColor_WindowBg]);
    
    render_make_rect(di[0].vertices, di[0].indicies, counter.sums[0], 
        window->item.pos + button->item.pos, 
        button->item.size, 3, 
        uiContext.style.colors[uiColor_WindowBorder]);

    render_make_text(di[1].vertices, di[1].indicies, {0,0}, 
        text, 
        uiContext.style.font, 
        window->item.pos + button->item.pos, 
        uiContext.style.colors[uiColor_Text], 
        vec2::ONE);

    di[1].texture = uiContext.style.font->tex;
    return button;
}

void ui_init(){
    item_list = create_arena_list(0);
    drawcmd_list = create_arena_list(0);
    SetNextPos(-MAX_F32, -MAX_F32);
    SetNextSize(-MAX_F32, -MAX_F32);
    uiStyle* style = &uiContext.style;
    style->colors[uiColor_WindowBg]     = color(14,14,14);
    style->colors[uiColor_WindowBorder] = color(170,170,170);
    style->colors[uiColor_Text] = Color_White;

    style->font = Storage::CreateFontFromFileBDF(str8l("gohufont-11.bdf")).second;
}

void ui_recur(TNode* node, vec2 parent_offset){
    uiItem* item = ItemFromNode(node);
    forI(item->draw_cmd_count){
        render_set_active_surface_idx(0);
        render_start_cmd2(5, item->draw_cmds[i].texture, parent_offset + item->pos, item->size);
        render_add_vertices2(5, item->draw_cmds[i].vertices, item->draw_cmds[i].counts.x, item->draw_cmds[i].indicies, item->draw_cmds[i].counts.y);
    }
    
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
            ui_recur(it, item->pos);
        }
    }
}

void ui_update(){
    if(uiContext.base.child_count){
        ui_recur(uiContext.base.first_child, vec2::ZERO);
    }
}


#define MakeButton()

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


    uiWindow* win = ui_make_window(str8l("test"), vec2::ONE*300, vec2::ONE*300);
    uiButton* button = ui_make_button(win, str8l("button"), 0, 0);
	
	//start main loop
	while(platform_update()){DPZoneScoped;
		console_update();
        ui_update();
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