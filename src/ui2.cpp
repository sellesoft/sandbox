#include "ui2.h"
#include "kigu/array.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"

#define item_arena g_ui->item_list->arena
#define drawcmd_arena g_ui->drawcmd_list->arena
#define SetNextPos(x,y) g_ui->nextPos = {x,y}
#define SetNextSize(x,y) g_ui->nextSize = {x,y}

//@uiDrawCmd



//@Item


//@Window


void* arena_add(Arena* arena, upt size){
    if(arena->size < arena->used+size) 
        g_ui->item_list = create_arena_list(g_ui->item_list);
    u8* cursor = arena->cursor;
    arena->cursor += size;
    arena->used += size;
    return cursor;
}

void drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts){
    if(drawcmd_arena->size < drawcmd_arena->used + counts.x * sizeof(Vertex2) + counts.y * sizeof(u32))
        g_ui->drawcmd_list = create_arena_list(g_ui->drawcmd_list);
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
    vec2i pos = window->cursor + g_ui->style.window_margins;
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
            if(node->parent == &g_ui->base){
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
	
    render_make_filledrect(di->vertices, di->indicies, {0,0}, win->spos, win->size, g_ui->style.colors[uiColor_WindowBg]);
    render_make_rect(di->vertices, di->indicies, counter.sums[0], win->spos, win->size, 3, g_ui->style.colors[uiColor_WindowBorder]);
}

uiWindow* ui_begin_window(str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line){
    return 0;
}

// makes a window to place items in
//
//  name: name of the window
//   pos: initial position of the window
//  size: initial size of the window
// flags: collection of uiWindowFlags to apply to the window
uiWindow* ui_make_window(str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line){
    uiBeginItem(uiWindow, win, &g_ui->base, uiItemType_Window, flags, 1, file, line);
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
						   g_ui->style.colors[uiColor_WindowBg]
						   );
    
    render_make_rect(di[0].vertices, di[0].indicies, counter.sums[0], 
					 item->spos, 
					 item->size, 3, 
					 g_ui->style.colors[uiColor_WindowBorder]
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
