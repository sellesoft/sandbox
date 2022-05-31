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

enum uiItemType_{
    uiItemType_Window, 
    uiItemType_ChildWindow,
    uiItemType_Button,
    uiItemType_Text,
    uiItemType_COUNT
};typedef u32 uiItemType;

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

struct primcount{
    u32 n;
    vec2i* counts;
    vec2i* sums;
};



struct uiDrawCmd{
    Texture* texture;
    Vertex2* vertices;
    u32*     indicies;
    vec2i    counts;
};


//@Item

struct uiItem{
    TNode node;
    Flags flags;
    union{
        struct{ // position relative to parent
            s32 lx; 
            s32 ly;
        };
        vec2i lpos;
    };
    union{
        struct{ // position relative to screen
            s32 sx;
            s32 sy;
        };
        vec2i spos;
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
    b32    clicked;
};
#define uiButtonFromNode(x) CastFromMember(uiButton, item, CastFromMember(uiItem, node, x))

struct uiText{
    uiItem item;
    str8 text; 
};

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

void drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts){
    if(drawcmd_arena->size < drawcmd_arena->used + counts.x * sizeof(Vertex2) + counts.y * sizeof(u32))
        drawcmd_list = create_arena_list(drawcmd_list);
    drawcmd->vertices = (Vertex2*)drawcmd_arena->cursor;
    drawcmd->indicies = (u32*)drawcmd_arena->cursor + (u32)counts.x * sizeof(Vertex2);
    drawcmd->texture = 0;
    drawcmd->counts = counts;
    drawcmd_arena->cursor += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
    drawcmd_arena->used += (u32)counts.x * sizeof(Vertex2) + (u32)counts.y * sizeof(u32);
}

//@Functions

//@Helpers

vec2i ui_decide_item_pos(uiWindow* window){
    vec2i pos = window->cursor + uiContext.style.window_margins;
    return pos;
}

//counts the amount of primitives 
primcount ui_count_primitives(u32 n_primitives, ...){
    primcount pc;
    pc.counts = (vec2i*)memtalloc(n_primitives*sizeof(vec2i));
    pc.sums = (vec2i*)memtalloc(n_primitives*sizeof(vec2i));
    vec2i sum;
    va_list args;
    va_start(args, n_primitives);
    forI(n_primitives){
        vec2i v = vec2(va_arg(args, vec2));
        sum+=v;
        pc.sums[i] = sum;
        pc.counts[i] = v;
    }
    return pc;
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
// list of things this does:
//  1. Allocates the uiItem that we are making to the item_arena PLUS the uiDrawInfoCounter arrays
//  2. Fills out the uiDrawInfoCounter struct
//  3. Sets the item's type
//  4. Allocates the drawcmds array to the drawcmd_arena and sets the drawcmd_count var
//  5. Logs on what line in what file the item was created at
//  6. Adds the Item to the specified parent
#define uiBeginItem(T, name, parent, item_type, flags, n_drawcmds, file, line)            \
T* name = (T*)arena_add(item_arena, sizeof(T));                                           \
name->item.node.type = item_type;                                                         \
name->item.flags = flags;                                                                 \
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

//item size, number of uiDrawCmds, followed by the number of primitives per drawCmd 
void* ui_alloc_item(upt item_size, upt n_drawcmds, ...){
    //va_list args;
    //va_start(args, n_drawcmds);
    //upt sum = 0;
    //forI(n_drawcmds) sum += va_arg(args, upt);
    //u8* ptr = (u8*)arena_add(item_arena, item_size + n_drawcmds*sizeof(uiDrawCmd) + 2*sum*sizeof(vec2i));
    //uiDrawCmd* drawcmd = (uiDrawCmd*)(ptr + item_size);
    //va_start(args, n_drawcmds);
    //forI(n_drawcmds) {
    //    upt n = va_arg(args, upt);
    //    drawcmd->counter.counts = (vec2i*)(drawcmd+1);
    //    drawcmd->counter.sums = (vec2i*)(drawcmd+1)+n;
    //    drawcmd = (uiDrawCmd*)(drawcmd->counter.sums + n); 
    //}
    return 0;
}

FORCE_INLINE
void ui_gen_window(uiItem* win){
    primcount counter = ui_count_primitives(2,
        render_make_filledrect_counts(),
        render_make_rect_counts()
    );

    uiDrawCmd* di = win->drawcmds;

    render_make_filledrect(di->vertices, di->indicies, {0,0}, win->pos, win->size, uiContext.style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], win->pos, win->size, 3, uiContext.style.colors[uiColor_WindowBorder]);
}

// makes a window to place items in
//
//  name: name of the window
//   pos: initial position of the window
//  size: initial size of the window
// flags: collection of uiWindowFlags to apply to the window
#define uiMakeWindow(name, pos, flags) ui_make_window(STR8(name),pos,size,flags,STR8(__FILE__),__LINE__)
uiWindow* ui_begin_window(str8 name, vec2i pos, Flags flags, str8 file, upt line){
    uiBeginItem(uiWindow, win, &uiContext.base, uiItemType_Window, flags, 1, file, line);
    //win->item.spos = win->item.lpos = pos; why doesnt this work?
    win->item.spos = pos;
    win->item.lpos = pos;
    win->item.size = ui_decide_item_size();
    win->name      = name;
    win->cursor    = vec2i::ZERO;

    primcount counter = ui_count_primitives(2,
        render_make_filledrect_counts(),
        render_make_rect_counts()
    );

    drawcmd_alloc(win->item.drawcmds, counter.sums[1]);
    ui_gen_window(&win->item);
   
    return win;
}

// makes a child window inside another window
//
// window: uiWindow to emplace this window in
//   name: name of the window
//    pos: initial position of the window
//   size: initial size of the window
uiWindow* ui_make_child_window(uiWindow* window, str8 name, vec2i size, Flags flags, str8 file, upt line){
    //uiBeginItem(uiWindow, win, &window->item.node, uiItemType_Window, 1, file, line);
    //win->item.pos  = ui_decide_item_pos(window);
    //win->item.size = size;
    //win->name      = name;
    //win->cursor    = vec2i::ZERO;
    //
    //vec2 draw_counts = 
    //    render_make_filledrect_counts() +
    //    render_make_rect_counts();
    //return win;
    return 0;
}

FORCE_INLINE
void ui_gen_button(uiItem* item){
    primcount counter = //NOTE(sushi) this can probably be cached on the item at creation time, but the fact that it uses arrays makes that tedious, so probably implement it later
        ui_count_primitives(3,
            render_make_filledrect_counts(),
            render_make_rect_counts()
        );
    uiDrawCmd* di = item->drawcmds;
    drawcmd_alloc(di, counter.sums[1]);
    
    render_make_filledrect(di[0].vertices, di[0].indicies, {0,0}, 
        item->spos, 
        item->size,
        uiContext.style.colors[uiColor_WindowBg]
    );
    
    render_make_rect(di[0].vertices, di[0].indicies, counter.sums[0], 
        item->spos, 
        item->size, 3, 
        uiContext.style.colors[uiColor_WindowBorder]
    );
}

//      window: uiWindow to emplace the button in
//        text: text to be displayed in the button
//      action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//              if 0 is passed, the button will just set it's clicked flag
// action_data: data passed to the action function 
//       flags: uiButtonFlags to be given to the button
#define uiMakeButton(window, text, action, action_data, flags) ui_make_button(window, STR8(text), action, action_data, flags, STR8(__FILE__), __LINE__)
uiButton* ui_make_button(uiWindow* window, Action action, void* action_data, Flags flags, str8 file, upt line){
    uiBeginItem(uiButton, button, &window->item.node, uiItemType_Button, flags, 1, file, line);
    button->item.lpos   = ui_decide_item_pos(window);
    button->item.spos   = button->item.lpos + window->item.spos;
    button->action      = action;
    button->action_data = action_data;

    ui_gen_button(button->item);
    
    return button;
}

//same as a div in HTML, just a section that items will place themselves in
void ui_make_section(vec2i pos, vec2i size){

}

void ui_init(){
    item_list = create_arena_list(0);
    drawcmd_list = create_arena_list(0);
    SetNextPos(-MAX_S32, -MAX_S32);
    SetNextSize(-MAX_S32, -MAX_S32);
    uiStyle* style = &uiContext.style;
    style->colors[uiColor_WindowBg]     = color(14,14,14);
    style->colors[uiColor_WindowBorder] = color(170,170,170);
    style->colors[uiColor_Text] = Color_White;

    style->font = Storage::CreateFontFromFileBDF(str8l("gohufont-11.bdf")).second;
}

//finds the container of an item eg. a window, child window, or section
//probably
TNode* ui_find_container(TNode* item){
    if(item->type == uiItemType_Window || 
       item->type == uiItemType_ChildWindow) return item;
    if(item->parent) return ui_find_container(item->parent);
}

void ui_regen_item(uiItem* item){
    switch(item->node.type){
        case uiItemType_Window: ui_gen_window(item); break;
        case uiItemType_Button: ui_gen_button(item); break;
    }
    for_node(item->node.first_child) ui_regen_item(uiItemFromNode(it));
}
void ui_recur(TNode* node, vec2i parent_offset){
    //do updates of each item type
    switch(node->type){
        case uiItemType_Window:{ uiWindow* item = uiWindowFromNode(node);
            {//dragging
                vec2i mp_cur = vec2i(DeshInput->mouseX, DeshInput->mouseY); 
                persist b32 dragging = false;
                persist vec2i mp_offset;
                if(key_pressed(Mouse_LEFT) && Math::PointInRectangle(mp_cur, item->item.pos, item->item.size)){
                    mp_offset = item->item.pos - mp_cur;
                    dragging = true;
                }
                if(key_released(Mouse_LEFT)) dragging = false;

                if(dragging){
                    item->item.pos = input_mouse_position() + mp_offset;
                    ui_regen_item(item->item);
                }
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