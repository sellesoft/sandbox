#include "ui2.h"
#include "kigu/array.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"

#define item_arena g_ui->item_list->arena
#define drawcmd_arena g_ui->drawcmd_list->arena

void push_item(uiItem* item){
    g_ui->item_stack.add(item);
}

uiItem* pop_item(){
    uiItem* ret = *g_ui->item_stack.last;
    g_ui->item_stack.pop();
    return ret;
}

//@uiDrawCmd



//@Item


//@Window


ArenaList* create_arena_list(ArenaList* old){
    ArenaList* nual = (ArenaList*)memalloc(sizeof(ArenaList));
    if(old) NodeInsertNext(&old->node, &nual->node);
    nual->arena = memory_create_arena(Megabytes(1));
    return nual;
}

void* arena_add(Arena* arena, upt size){
    if(arena->size < arena->used+size) 
        g_ui->item_list = create_arena_list(g_ui->item_list);
    u8* cursor = arena->cursor;
    arena->cursor += size;
    arena->used += size;
    return cursor;
}

void drawcmd_alloc(uiDrawCmd* drawcmd, RenderDrawCounts counts){
    if(drawcmd_arena->size < drawcmd_arena->used + counts.vertices * sizeof(Vertex2) + counts.indices * sizeof(u32))
        g_ui->drawcmd_list = create_arena_list(g_ui->drawcmd_list);
    drawcmd->vertices = (Vertex2*)drawcmd_arena->cursor;
    drawcmd->indices = (u32*)drawcmd_arena->cursor + counts.vertices * sizeof(Vertex2);
    drawcmd->texture = 0;
    drawcmd->counts = counts;
    drawcmd_arena->cursor += counts.vertices * sizeof(Vertex2) + counts.indices * sizeof(u32);
    drawcmd_arena->used += counts.vertices * sizeof(Vertex2) + counts.indices * sizeof(u32);
}

//@Functions

//@Helpers


//counts the amount of primitives 
primcount ui_count_primitives(u32 n_primitives, ...){
    //primcount pc;
    //pc.counts = (vec2i*)memtalloc(n_primitives*sizeof(vec2i));
    //pc.sums = (vec2i*)memtalloc(n_primitives*sizeof(vec2i));
    //RenderDrawCounts sum;
    //va_list args;
    //va_start(args, n_primitives);
    //forI(n_primitives){
    //    RenderDrawCounts v = va_arg(args, RenderDrawCounts);
    //    sum+=v;
    //    pc.sums[i] = sum;
    //    pc.counts[i] = v;
    //}
    //return pc;
    return primcount{0};
}

//initializes a uiItem of some type
// used at the beginning of all uiItem functions
#define uiBeginItem(T, name, parent, item_type, flags, n_drawcmds, file, line)            \
T* name = (T*)arena_add(item_arena, sizeof(T));                                           \
name->item.node.type = item_type;                                                         \
name->item.flags = flags;                                                                 \
name->item.drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)*n_drawcmds); \
name->item.draw_cmd_count = n_drawcmds;                                                   \
name->item.file_created = file;                                                           \
name->item.line_created = line;                                                           \
name->item.style = ui_initial_style;                                                      \
insert_last(parent, &name->item.node);                                                    \

//@Functionality
void ui_gen_item(uiItem* item){
    uiDrawCmd* dc = item->drawcmds;
    RenderDrawCounts counts = {0};
    if(item->style.background_color.a){
        counts+=render_make_filledrect(dc->vertices, dc->indices, counts, item->spos, item->size, item->style.background_color);
    }
    switch(item->style.border_style){
        case border_none:{}break;
        case border_solid:{
            render_make_rect(dc->vertices, dc->indices, counts, item->spos, item->size, 2, item->style.border_color);
        }break;
    }
}


uiItem* ui_make_item(str8 id, uiStyle* style, str8 file, upt line){
    uiItem* curitem = *g_ui->item_stack.last;
    uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
    insert_last(&curitem->node, &item->node);
    
    if(style) memcpy(&item->style, style, sizeof(uiStyle));
    else      memcpy(&item->style, ui_initial_style, sizeof(uiStyle));
    
    //TODO(sushi) if an item has an id put it in a map
    item->id = id;
    item->file_created = file;
    item->line_created = line;

    item->drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)*2); 

    RenderDrawCounts counts = //reserve enough room for a background and border 
        render_make_filledrect_counts() +
        render_make_rect_counts();

    item->draw_cmd_count = 1;
    drawcmd_alloc(item->drawcmds, counts);
    g_ui->update_this_frame.add(item);
    return item;
}

uiItem* ui_begin_item(str8 id, uiStyle* style, str8 file, upt line){
    uiItem* item = ui_make_item(id, style, file, line);
    push_item(item);
    return item;
}

void ui_end_item(){
    pop_item();
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
    //primcount counter = ui_count_primitives(2,
	//										render_make_filledrect_counts(),
	//										render_make_rect_counts()
	//										);
	//
    //uiDrawCmd* di = win->drawcmds;
	//
    //render_make_filledrect(di->vertices, di->indicies, {0,0}, win->spos, win->size, g_ui->style.colors[uiColor_WindowBg]);
    //render_make_rect(di->vertices, di->indicies, counter.sums[0], win->spos, win->size, 3, g_ui->style.colors[uiColor_WindowBorder]);
}


// makes a window to place items in
//
//  name: name of the window
//   pos: initial position of the window
//  size: initial size of the window
// flags: collection of uiWindowFlags to apply to the window
uiItem* ui_make_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){
    //uiBeginItem(uiWindow, win, &g_ui->base, uiItemType_Window, flags, 1, file, line);
    uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
    item->node.type = uiItemType_Window;
    
    //T* name =                                            
    //name->item.node.type = item_type;                                                         
    //name->item.flags = flags;                                                                 
    //name->item.drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)*n_drawcmds); 
    //name->item.draw_cmd_count = n_drawcmds;                                                   
    //name->item.file_created = file;                                                           
    //name->item.line_created = line;                                                           
    //name->item.style = ui_initial_style;                                                      
    //insert_last(parent, &name->item.node);                                                    
	//
    //
    ////win->item.spos = win->item.lpos = pos; why doesnt this work?
    //win->name   = name;
    //win->cursor = vec2i::ZERO;
	//
    //primcount counter = 
    //    ui_count_primitives(2,
    //        render_make_filledrect_counts(),
    //        render_make_rect_counts()
    //        );
	//
    //drawcmd_alloc(win->item.drawcmds, counter.sums[1]);
    //ui_gen_window(&win->item);
	
    return 0;
}

uiItem* ui_begin_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){
    ui_make_window(name, flags, style, file, line);
    
    return 0;
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
    //primcount counter = //NOTE(sushi) this can probably be cached on the item at creation time, but the fact that it uses arrays makes that tedious, so probably implement it later
    //    ui_count_primitives(3,
	//						render_make_filledrect_counts(),
	//						render_make_rect_counts()
	//						);
    //uiDrawCmd* di = item->drawcmds;
    //drawcmd_alloc(di, counter.sums[1]);
    //
    //render_make_filledrect(di[0].vertices, di[0].indicies, {0,0}, 
	//					   item->spos, 
	//					   item->size,
	//					   g_ui->style.colors[uiColor_WindowBg]
	//					   );
    
    //render_make_rect(di[0].vertices, di[0].indicies, counter.sums[0], 
	//				 item->spos, 
	//				 item->size, 3, 
	//				 item->style.colors
	//				 );
}



//      window: uiWindow to emplace the button in
//        text: text to be displayed in the button
//      action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//              if 0 is passed, the button will just set it's clicked flag
// action_data: data passed to the action function 
//       flags: uiButtonFlags to be given to the button
uiButton* ui_make_button(uiWindow* window, Action action, void* action_data, Flags flags, str8 file, upt line){
    //uiBeginItem(uiButton, button, &window->item.node, uiItemType_Button, flags, 1, file, line);
    //button->item.lpos   = ui_decide_item_pos(window);
    //button->item.spos   = button->item.lpos + window->item.spos;
    //button->action      = action;
    //button->action_data = action_data;
	//
    ////ui_gen_button(button->item);
    //
    //return button;
    return 0;
}

void ui_gen_text(uiItem* item){

}

uiItem* ui_make_text(str8 text, str8 id, uiStyle* style, str8 file, upt line){
    return 0;
}

//same as a div in HTML, just a section that items will place themselves in
void ui_make_section(vec2i pos, vec2i size){
	
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context
void ui_init(){
	g_ui->item_list    = create_arena_list(0);
    g_ui->drawcmd_list = create_arena_list(0);
    
    ui_initial_style->     positioning = pos_static;
    ui_initial_style->            left = 0;
    ui_initial_style->             top = 0;
    ui_initial_style->           right = MAX_S32;
    ui_initial_style->          bottom = MAX_S32;
    ui_initial_style->           width = -1;
    ui_initial_style->          height = -1;
    ui_initial_style->     margin_left = 0;
    ui_initial_style->      margin_top = 0;
    ui_initial_style->    margin_right = MAX_S32;
    ui_initial_style->   margin_bottom = MAX_S32;
    ui_initial_style->    padding_left = 0;
    ui_initial_style->     padding_top = 0;
    ui_initial_style->   padding_right = MAX_S32;
    ui_initial_style->  padding_bottom = MAX_S32;
    ui_initial_style->   content_align = vec2{0,0};
    ui_initial_style->            font = 0, //we cant initialize this here because memory hasnt been initialized yet
    ui_initial_style->     font_height = 11;
    ui_initial_style->background_color = color{0,0,0,0};
    ui_initial_style->background_image = 0;
    ui_initial_style->    border_style = border_none;
    ui_initial_style->    border_color = color{180,180,180,255};
    ui_initial_style->      text_color = color{255,255,255,255};


    g_ui->base = uiItem{0};
    g_ui->base.style = *ui_initial_style;
    g_ui->base.node.type = uiItemType_Section;
    g_ui->base.file_created = STR8(__FILE__);
    g_ui->base.line_created = __LINE__;
    g_ui->base.style.width = DeshWindow->width;
    g_ui->base.style.height = DeshWindow->height;
    g_ui->base.id = STR8("base");
    g_ui->base.style_hash = hash<uiStyle>()(&g_ui->base.style);

    push_item(&g_ui->base);
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

struct DrawContext{
    vec2i bbx;
    u64   drawcmd_count;
    uiDrawCmd* drawCmds;
};

//pass 0 for child on first call
TNode* ui_find_static_sized_parent(TNode* node, TNode* child){
    uiItem* item = uiItemFromNode(node);
    if(!child) return ui_find_static_sized_parent(item->node.parent, &item->node);
    if(item->style.width != size_auto && item->style.height != size_auto){
        return &item->node;
    }else{
        return ui_find_static_sized_parent(item->node.parent, &item->node);
    }
}

void redraw_item_branch(uiItem* item){
    if(item != &g_ui->base)
        item->spos = uiItemFromNode(item->node.parent)->spos + item->lpos;
    switch(item->node.type){
        case uiItemType_Item: ui_gen_item(item); break;
        case uiItemType_Text: ui_gen_text(item); break;
        default: Assert(!"unhandled uiItem type"); break;
    }
    
    for_node(item->node.first_child){
        redraw_item_branch(uiItemFromNode(it));
    }
}

//reevaluates an entire brach of items
DrawContext reeval_item_branch(uiItem* item){
    DrawContext drawContext;

    if(item->style.height != size_auto) item->height = item->style.height;
    if(item->style.width != size_auto) item->width = item->style.width;

    vec2i cursor = item->style.paddingtl;
    for_node(item->node.first_child){
        uiItem* child = uiItemFromNode(it);
        DrawContext ret = reeval_item_branch(child);    
        vec2i cursorloc = cursor;
        switch(child->style.positioning){
            case pos_static:{
                child->lpos =  
                    child->style.margintl +
                    item->scroll +
                    cursor;
            }break;
        }
        if(item->style.width == size_auto){
            item->width = Max(item->width, child->spos.x + ret.bbx.x);
        }
        if(item->style.height == size_auto){
            item->height = Max(item->height, child->spos.y + ret.bbx.y);
        }
    }


    //Log("", item->id, " pos: ", item->spos);
    drawContext.bbx.x = item->width;
    drawContext.bbx.y = item->height;

    return drawContext;
}

DrawContext ui_recur(TNode* node){
    uiItem* item = uiItemFromNode(node);
    uiItem* parent = uiItemFromNode(node->parent);

    DrawContext drawContext;
    DrawContext childrenDrawContext;
    //depth first walk
    //for_node(node->first_child){
    //    DrawContext ret = ui_recur(it);
    //}
   

    //check if an item's style was modified, if so reevaluate the item,
    //its children, and every child of its parents until a manually sized parent is found
    u32 nuhash = hash<uiStyle>()(&item->style);
    if(nuhash!=item->style_hash){
        item->style_hash = nuhash; 
        uiItem* sspar = uiItemFromNode(ui_find_static_sized_parent(&item->node, 0));
        reeval_item_branch(sspar);
        redraw_item_branch(sspar);
    } 


    
    //dragging an item
    //it doesnt matter whether its fixed or relative here
    //that only matters in scrolling
    if(item->style.positioning = pos_draggable_fixed || 
       item->style.positioning == pos_draggable_relative){
        vec2i mp_cur = vec2i(DeshInput->mouseX, DeshInput->mouseY); 
        persist b32 dragging = false;
        persist vec2i mp_offset;
        if(key_pressed(Mouse_LEFT) && Math::PointInRectangle(mp_cur, item->spos, item->size)){
            mp_offset = item->spos - mp_cur;
            dragging = true;
        }
        if(key_released(Mouse_LEFT)) dragging = false;
        
        if(dragging){
            item->spos = input_mouse_position() + mp_offset;
            ui_regen_item(item);
        }
    }
    //do updates of each item type
	
    switch(node->type){
        case uiItemType_Window:{ uiWindow* item = uiWindowFromNode(node);
            {//dragging
				
            }
        }break;
        case uiItemType_Button:{ uiButton* item = uiButtonFromNode(node);
            
        }break;
    }
	
   

    return drawContext;
}

void ui_render(TNode* node){
    uiItem* item = uiItemFromNode(node);
    
     //render item
    forI(item->draw_cmd_count){
        render_set_active_surface_idx(0);
        render_start_cmd2(5, 0, item->drawcmds[i].scissorOffset, item->drawcmds[i].scissorExtent);
        render_add_vertices2(5, 
            item->drawcmds[i].vertices, 
            item->drawcmds[i].counts.vertices, 
            item->drawcmds[i].indices, 
            item->drawcmds[i].counts.indices
        );
    }
    
    if(node->child_count){
        for_node(node->first_child){
            ui_recur(it);
        }
    }
}

void ui_update(){
    g_ui->base.style.width = DeshWindow->width;
    g_ui->base.style.height = DeshWindow->height;
    
    if(g_ui->base.node.child_count){
        ui_recur(g_ui->base.node.first_child);
    }
}
