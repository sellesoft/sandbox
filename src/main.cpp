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
    vec2i window_margins;
    vec2i item_spacing;
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

struct uiDrawCmdCounter{
    u64 n; //number of counts
    vec2i* counts;
    vec2i* sums;
};

struct uiDrawCmd{
    Texture* texture;
    Vertex2* vertices;
    u32*     indicies;
    vec2i    counts;
    struct{
        u64 n;
        vec2i* counts;
        vec2i* sums;
    }counter;
};


//@Item

enum uiItemType_{
    uiItemType_Window, 
    uiItemType_ChildWindow,
    uiItemType_Button,
};typedef u32 uiItemType;

struct uiItem{
    TNode node;
    union{
        struct{
            s32 x;
            s32 y;
        };
        vec2i pos;
    };
    union{
        struct{
            s32 width;
            s32 height;
        };
        vec2i size;
    };

    u64 draw_cmd_count;
    uiDrawCmd* drawcmds;
    uiDrawCmdCounter counter;
    
    uiStyle style; // style at time of making this item. eventually this should be optimized to not store the entire style

    uiItem(){} // C++ is retarded

    str8 file_created;
    upt  line_created;
};

#define uiItemFromNode(x) CastFromMember(uiItem, node, x)

//@Window

struct uiWindow{
    uiItem item;
    str8 name;
    union{
        struct{
            s32 curx;
            s32 cury;
        };
        vec2i cursor;
    };

    uiWindow(){} //C++ is retarded
};
#define uiWindowFromNode(x) CastFromMember(uiWindow, item, CastFromMember(uiItem, node, x))

//@Button

enum {
    uiButtonFlags_ActOnPressed,  // button performs its action as soon as its clicked
    uiButtonFlags_ActOnReleased, // button waits for the mouse to be released before performing its action
    uiButtonFlags_ActOnDown,     // button performs its action for the duration the mouse is held down over it
};

struct uiButton{
    uiItem item;
    Action action;
    void*  action_data;
    Flags  flags;
    b32    clicked;
};
#define uiButtonFromNode(x) CastFromMember(uiButton, item, CastFromMember(uiItem, node, x))

//@Context

#define SetNextPos(x,y) uiContext.nextPos = {x,y};
#define SetNextSize(x,y) uiContext.nextSize = {x,y};
struct Context{
    vec2i nextPos;  // default: -MAx_F32, -MAX_F32; MAX_F32 does nothing
    vec2i nextSize; // default: -MAx_F32, -MAX_F32; MAX_F32 in either indiciates to stretch item to the edge of the window in that direction
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

uiDrawCmd drawcmd_alloc(vec2i counts){
    uiDrawCmd di;
    if(drawcmd_arena->size < drawcmd_arena->used + counts.x * sizeof(Vertex2) + counts.y * sizeof(u32))
        drawcmd_list = create_arena_list(drawcmd_list);
    di.vertices = (Vertex2*)drawcmd_arena->cursor;
    di.indicies = (u32*)drawcmd_arena->cursor + (u32)counts.x * sizeof(Vertex2);
    di.texture = 0;
    di.counts = counts;
    drawcmd_arena->cursor += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    drawcmd_arena->used += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    return di;
}

//@Functions

//@Helpers

vec2i ui_decide_item_pos(uiWindow* window){
    vec2i pos = window->cursor + uiContext.style.window_margins;
    return pos;
}

uiDrawCmdCounter ui_make_drawinfo_counts(u32 count, ...){
    uiDrawCmdCounter counter;
    counter.n = count;
    counter.counts = (vec2i*)memtalloc(sizeof(vec2i)*count);
    counter.sums = (vec2i*)memtalloc(sizeof(vec2i)*count);
    vec2i sum;
    va_list args;
    va_start(args, count);
    forI(count){
        vec2i v = va_arg(args, vec2i);
        sum+=v;
        counter.sums[i] = sum;
        counter.counts[i] = v;
    }
    return counter;
}

//walks up the tree until it finds the top most window containing the item and returns it's screen position
vec2i ui_get_item_screenpos(uiItem* item){
    TNode* node = &item->node;
    vec2i spsum = vec2i::ZERO;
    while(1){
        if(node->type == uiItemType_Window){
            if(node->parent == &uiContext.base){
                return spsum;
            }
        }

    }
}

//initializes a uiItem of some type
// used at the beginning of all uiItem functions
// n_drawcmds is the planned amount of drawcmds an item will have while n_primitives is the total amount
// of shapes an item will have 
#define uiBeginItem(T, name, parent, item_type, n_drawcmds, n_primitives, file, line)                               \
T* name = (T*)arena_add(item_arena, sizeof(T));                                           \
name->item.node.type = item_type;                                                         \
name->item.drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)*n_drawcmds); \
name->item.draw_cmd_count = n_drawcmds;                                                   \
name->item.file_created = file;                                                           \
name->item.line_created = line;                                                           \
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
#define uiMakeWindow(name, pos, size, flags) ui_make_window(STR8(name),pos,size,STR8(__FILE__),__LINE__)
void ui_make_window_gen_drawinfo(uiWindow* win, vec2i pos, vec2i size, Flags flags){
     uiDrawCmdCounter counter = 
        ui_make_drawinfo_counts(2,
            render_make_filledrect_counts(),
            render_make_rect_counts()
        );
    
    uiDrawCmd* di = win->item.drawcmds;
    *di = drawcmd_alloc(counter.sums[counter.n-1]);

    render_make_filledrect(di->vertices, di->indicies, {0,0}, pos, size, uiContext.style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], pos, size, 3, uiContext.style.colors[uiColor_WindowBorder]);
}
uiWindow* ui_make_window(str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line){
    uiBeginItem(uiWindow, win, &uiContext.base, uiItemType_Window, 1, file, line);
    win->item.pos  = pos;
    win->item.size = size;
    win->name      = name;
    win->cursor    = vec2i::ZERO;

    ui_make_window_gen_drawinfo(win, pos, size, flags);
   
    return win;
}

// makes a child window inside another window
//
// window: uiWindow to emplace this window in
//   name: name of the window
//    pos: initial position of the window
//   size: initial size of the window
uiWindow* ui_make_child_window(uiWindow* window, str8 name, vec2i size, Flags flags, str8 file, upt line){
    uiBeginItem(uiWindow, win, &window->item.node, uiItemType_Window, 1, file, line);
    win->item.pos  = ui_decide_item_pos(window);
    win->item.size = size;
    win->name      = name;
    win->cursor    = vec2i::ZERO;
    
    vec2 draw_counts = 
        render_make_filledrect_counts() +
        render_make_rect_counts();
    return win;
}

//      window: uiWindow to emplace the button in
//        text: text to be displayed in the button
//      action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//              if 0 is passed, the button will just set it's clicked flag
// action_data: data passed to the action function 
//       flags: uiButtonFlags to be given to the button
#define uiMakeButton(window, text, action, action_data, flags) ui_make_button(window, STR8(text), action, action_data, flags, STR8(__FILE__), __LINE__)
void ui_make_button_gen_drawinfo(uiWindow* window, uiButton* button, str8 text, Flags flags){
    uiDrawCmdCounter counter = //NOTE(sushi) this can probably be cached on the item at creation time, but the fact that it uses arrays makes that tedious, so probably implement it later
        ui_make_drawinfo_counts(3,
            render_make_filledrect_counts(),
            render_make_rect_counts(),
            render_make_text_counts(str8_length(text))
        );
    uiDrawCmd* di = button->item.drawcmds;
    di[0] = drawcmd_alloc(counter.sums[1]);
    di[1] = drawcmd_alloc(counter.counts[2]);
    di[1].texture = uiContext.style.font->tex;
   
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
}
uiButton* ui_make_button(uiWindow* window, str8 text, Action action, void* action_data, Flags flags, str8 file, upt line){
    uiBeginItem(uiButton, button, &window->item.node, uiItemType_Button, 2, file, line);
    button->item.pos    = ui_decide_item_pos(window);
    button->action      = action;
    button->action_data = action_data;
    button->flags       = flags;

    u32 text_size = str8_length(text);
    vec2 tsize = UI::CalcTextSize(text);
    button->item.size = vec2i(tsize.x*1.4, tsize.y*1.3);

    ui_make_button_gen_drawinfo(window, button, text, flags);
    
    return button;
}

//same as a div in HTML, just a section that items will place themselves in
void ui_make_section(vec2i pos, vec2i size){

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

void ui_adjust_item_pos(vec2i offset){

}

void ui_adjust_item_size(){

}

void ui_recur(TNode* node, vec2i parent_offset){
    //do updates of each item type
    switch(node->type){
        case uiItemType_Window:{ uiWindow* item = uiWindowFromNode(node);
            {//dragging
                vec2i mp = vec2i(DeshInput->mouseX, DeshInput->mouseY); 

            }
        }break;
        case uiItemType_Button:{ uiButton* item = uiButtonFromNode(node);
            
        }break;
    }

    //render item
    uiItem* item = uiItemFromNode(node);
    forI(item->draw_cmd_count){
        render_set_active_surface_idx(0);
        render_start_cmd2(5, item->drawcmds[i].texture, parent_offset + item->pos, item->size);
        render_add_vertices2(5, item->drawcmds[i].vertices, item->drawcmds[i].counts.x, item->drawcmds[i].indicies, item->drawcmds[i].counts.y);
    }
    
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



void action(void* data){

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


    uiWindow* win = uiMakeWindow("test", vec2::ONE*300, vec2::ONE*300);
    uiButton* button = uiMakeButton(win,"button", 0, 0);
	
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