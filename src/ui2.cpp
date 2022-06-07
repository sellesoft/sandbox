#include "ui2.h"
#include "kigu/array.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"

#ifdef DESHI_RELOADABLE_UI
# define UI_API __declspec(dllexport)
#else
# define UI_API 
#endif


array<uiStyleVarMod>   stack_var;
array<uiStyleColorMod> stack_color;

//@uiDrawCmd



//@Item


//@Window


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

void ui_push_f32(Type idx, f32 nu){
	
}

void ui_push_vec2(Type idx, vec2 nu){
	
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
	
    render_make_filledrect(di->vertices, di->indicies, {0,0}, win->spos, win->size, uiContext.style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], win->spos, win->size, 3, uiContext.style.colors[uiColor_WindowBorder]);
}

UI_API uiWindow* ui_begin_window(str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line){
    return 0;
}

// makes a window to place items in
//
//  name: name of the window
//   pos: initial position of the window
//  size: initial size of the window
// flags: collection of uiWindowFlags to apply to the window
uiWindow* ui_make_window(str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line){
    uiBeginItem(uiWindow, win, &uiContext.base, uiItemType_Window, flags, 1, file, line);
    //win->item.spos = win->item.lpos = pos; why doesnt this work?
    win->item.spos = pos;
    win->item.lpos = pos;
    win->item.size = size;
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
uiButton* ui_make_button(uiWindow* window, Action action, void* action_data, Flags flags, str8 file, upt line){
    uiBeginItem(uiButton, button, &window->item.node, uiItemType_Button, flags, 1, file, line);
    button->item.lpos   = ui_decide_item_pos(window);
    button->item.spos   = button->item.lpos + window->item.spos;
    button->action      = action;
    button->action_data = action_data;
	
    //ui_gen_button(button->item);
    
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
    return 0;
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
                if(key_pressed(Mouse_LEFT) && Math::PointInRectangle(mp_cur, item->item.spos, item->item.size)){
                    mp_offset = item->item.spos - mp_cur;
                    dragging = true;
                }
                if(key_released(Mouse_LEFT)) dragging = false;
				
                if(dragging){
                    item->item.spos = input_mouse_position() + mp_offset;
                    ui_regen_item(&item->item);
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
        render_start_cmd2(5, item->drawcmds[i].texture, item->spos, item->size);
        render_add_vertices2(5, item->drawcmds[i].vertices, item->drawcmds[i].counts.x, item->drawcmds[i].indicies, item->drawcmds[i].counts.y);
    }
    
    if(node->child_count){
        for_node(node->first_child){
            ui_recur(it, item->spos);
        }
    }
}

void ui_update(){
    if(uiContext.base.child_count){
        ui_recur(uiContext.base.first_child, vec2::ZERO);
    }
}
