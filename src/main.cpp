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
#include "core/renderer.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

typedef void (*Action)(void*);

struct DrawInfo{
    Vertex2* vertices;
    u32* indicies;
    vec2 counts;
}

enum ItemType_{
    ItemType_Window,
    ItemType_Button,
};typedef u32 ItemType;

struct Item{
    TNode node;
    vec2 pos;
    vec2 size;
    DrawInfo dinfo;
};

struct Window{
    Item item;
};

#define WindowFromNode(x) CastFromMember(Window, item, CastFromMember(Item, node, x))

struct Button{
    Item item;
    Action action;
};

#define SetNextPos(x,y) uiContext.nextPos = {x,y};
#define SetNextSize(x,y) uiContext.nextSize = {x,y};
struct Context{
    vec2 nextPos;  // default: -MAx_F32, -MAX_F32; MAX_F32 does nothing
    vec2 nextSize; // default: -MAx_F32, -MAX_F32; MAX_F32 in either indiciates to stretch item to the edge of the window in that direction
    TNode* base;
}uiContext;

Button* ui_make_button(Action action = 0, Window* window= 0){

}

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
    nual->arena = memory_create_arena(Kilobytes(1));
    return nual;
}

void add_item(void* data, upt size){
    if(item_arena->size < item_arena->used+size) 
        item_list = create_arena_list(item_list);
    memcpy(item_arena->cursor, data, size);
    item_arena->cursor += size;
    item_arena->used += size;
}

DrawInfo drawinfo_alloc(u32 vcount, u32 icount){
    DrawInfo di;
    if(drawinfo_arena->size < drawinfo_arena->used + vcount * sizeof(Vertex2) + icount * sizeof(u32))
        drawinfo_list = create_arena_list(drawinfo_list);
    di.vertices = (Vertex2*)drawinfo_arena->cursor;
    di.indicies = (u32*)drawinfo_arena->cursor + vcount * sizeof(Vertex2);
    drawinfo_arena->cursor += vcount * sizeof(Vertex2) + icount * sizeof(u32);
    drawinfo_arena->used += vcount * sizeof(Vertex2) + icount * sizeof(u32);
    return di;
}

void ui_init(){
    SetNextPos(-MAX_F32, -MAX_F32);
    SetNextSize(-MAX_F32, -MAX_F32);
}

void ui_recur(TNode* node){
    switch(node->type){
        case ItemType_Window:{
            Window* w = WindowFromNode(node);
            Render::StartNewTwodCmd(5, 0, w->item.pos, w->item.size);
            Render::AddTwodVertices(5, w->item.dinfo.vertices, w->item.dinfo.counts.x, w->item.dinfo.indicies, w->item.dinfo.counts.y);
        }break;
    }
    
    
    if(node->child_count){
        for_node(node){

        }
    }
}

void ui_update(){
     if(uiContext.base->child_count){
        ui_recur(uiContext.base->first_child);
    }
}


#define MakeButton()

int main(){
    memory_init(Gigabytes(1), Gigabytes(1));
	logger_init();
	console_init();
	DeshWindow->Init("suugu", 1280, 720);
	Render::Init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
	Render::UseDefaultViewProjMatrix();
	DeshThreadManager->init();

    array<TNode*> items;  
    Window w;

    TNode* t = &w.item.node;

    Window* wp = WindowFromNode(t);

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

}