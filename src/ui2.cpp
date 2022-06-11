#include "ui2.h"
#include "kigu/array.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/window.h"
#include "core/logger.h"

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
    drawcmd->vertex_offset = g_ui->vertex_arena->used / sizeof(Vertex2);
    drawcmd->index_offset = g_ui->index_arena->used / sizeof(u32);
    g_ui->vertex_arena->cursor += counts.vertices * sizeof(Vertex2);
    g_ui->vertex_arena->used += counts.vertices * sizeof(Vertex2);
    g_ui->index_arena->cursor += counts.indices * sizeof(u32);
    g_ui->index_arena->used += counts.indices * sizeof(u32);
    drawcmd->counts = counts;
    drawcmd->texture = 0;
}

//@Functions

//@Helpers

//@Functionality
void ui_gen_item(uiItem* item){
    uiDrawCmd* dc = item->drawcmds;
    Vertex2* vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
    u32*     ip = (u32*)g_ui->index_arena->start + dc->index_offset;
    RenderDrawCounts counts = {0};
    if(item->style.background_color.a){
        counts+=render_make_filledrect(vp, ip, counts, item->spos, item->size, item->style.background_color);
    }
    switch(item->style.border_style){
        case border_none:{}break;
        case border_solid:{
            counts+=render_make_rect(vp, ip, counts, item->spos, item->size, 2, item->style.border_color);
        }break;
    }
    dc->scissorOffset = item->spos;
    dc->scissorExtent = item->size;
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
	
    item->drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)); 
	
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

uiItem* ui_make_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){
    //uiBeginItem(uiWindow, win, &g_ui->base, uiItemType_Window, flags, 1, file, line);
    uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
    item->node.type = uiItemType_Window;

    return 0;
}

uiItem* ui_begin_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){
    ui_make_window(name, flags, style, file, line);
    
    return 0;
}

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


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context
void ui_init(){
	g_ui->item_list    = create_arena_list(0);
    g_ui->drawcmd_list = create_arena_list(0);
    g_ui->vertex_arena = memory_create_arena(Megabytes(1));
    g_ui->index_arena  = memory_create_arena(Megabytes(1));

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
    g_ui->base.node.type = uiItemType_Item;
    g_ui->base.file_created = STR8(__FILE__);
    g_ui->base.line_created = __LINE__;
    g_ui->base.style.width = DeshWindow->width;
    g_ui->base.style.height = DeshWindow->height;
    g_ui->base.id = STR8("base");
    g_ui->base.style_hash = hash<uiStyle>()(&g_ui->base.style);
    //ui_make_item(STR8("base"), ui_initial_style, STR8(__FILE__), __LINE__);
    push_item(&g_ui->base);

    //g_ui->render_buffer = render_create_external_2d_buffer(Megabytes(1), Megabytes(1));

}

//finds the container of an item eg. a window, child window, or section
//probably
TNode* ui_find_container(TNode* item){
    if(item->type == uiItemType_Window || 
       item->type == uiItemType_ChildWindow) return item;
    if(item->parent) return ui_find_container(item->parent);
    return 0;
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

void draw_item_branch(uiItem* item){
    if(item != &g_ui->base)
        item->spos = uiItemFromNode(item->node.parent)->spos + item->lpos;
    
    if(item->draw_cmd_count){
        switch(item->node.type){
            case uiItemType_Item: ui_gen_item(item); break;
            case uiItemType_Text: ui_gen_text(item); break;
            default: Assert(!"unhandled uiItem type"); break;
        }
    }

    //Log("", item->id, " pos: ", item->spos, " size: ", item->size);
    
    for_node(item->node.first_child){
        draw_item_branch(uiItemFromNode(it));
    }
}

//reevaluates an entire brach of items
//NOTE(sushi) DrawContext may be useless here
DrawContext eval_item_branch(uiItem* item){
    DrawContext drawContext;
	
    if(item->style.height != size_auto) item->height = item->style.height;
    else item->height = 0; //always reset size on auto sized items
    if(item->style.width != size_auto) item->width = item->style.width;
    else item->width = 0; //always reset size on auto sized items
    
    vec2i cursor = item->style.paddingtl;
    for_node(item->node.first_child){
        uiItem* child = uiItemFromNode(it);
        DrawContext ret = eval_item_branch(child);    
        vec2i cursorloc = cursor;
        switch(child->style.positioning){
            case pos_static:{
                child->lpos =  
                    child->style.margintl +
                    item->scroll +
                    cursor;
            }break;
            case pos_relative:{
                child->lpos = 
                    child->style.margintl +
                    item->scroll +
                    cursor +
                    child->style.tl;
            }break;
        }
        
        if(item->style.width == size_auto)
            item->width = Max(item->width, child->lpos.x + ret.bbx.x);
        if(item->style.height == size_auto)
            item->height = Max(item->height, child->lpos.y + ret.bbx.y);
        
        if(child->style.margin_bottom == MAX_S32) item->height += child->style.margin_top;
        else if(child->style.margin_bottom > 0) item->height += child->style.margin_bottom;
        if(child->style.margin_right == MAX_S32) item->width += child->style.margin_left;
        else if(child->style.margin_right > 0) item->width += child->style.margin_right;
        


        cursor.x = item->style.padding_left;
        cursor.y = child->lpos.y + ret.bbx.y;
    }

    if(item->style.padding_bottom == MAX_S32) item->height += item->style.padding_top;
    else if(item->style.padding_bottom > 0) item->height += item->style.padding_bottom;
    if(item->style.padding_right == MAX_S32) item->width += item->style.padding_left;
    else if(item->style.padding_right > 0) item->width += item->style.padding_right;
        
    
    drawContext.bbx.x = item->width;
    drawContext.bbx.y = item->height;
	
    return drawContext;
}

void ui_recur(TNode* node){
    uiItem* item = uiItemFromNode(node);
    uiItem* parent = uiItemFromNode(node->parent);
	
    //dragging an item
    //it doesnt matter whether its fixed or relative here
    //that only matters in scrolling
    if(item->style.positioning == pos_draggable_fixed || 
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
            item->lpos = input_mouse_position() + mp_offset;
            //ui_regen_item(item);
        }
    }
	
    //check if an item's style was modified, if so reevaluate the item,
    //its children, and every child of its parents until a manually sized parent is found
    u32 nuhash = hash<uiStyle>()(&item->style);
    if(nuhash!=item->style_hash){
        item->style_hash = nuhash; 
        uiItem* sspar = uiItemFromNode(ui_find_static_sized_parent(&item->node, 0));
        eval_item_branch(sspar);
        draw_item_branch(sspar);
    } 

    //do updates of each item type
    switch(node->type){
        case uiItemType_Window:{ uiWindow* item = uiWindowFromNode(node);
        }break;
        case uiItemType_Button:{ uiButton* item = uiButtonFromNode(node);
        }break;
    }
	
    //render item
    forI(item->draw_cmd_count){
        render_set_active_surface_idx(0);
        render_start_cmd2(5, 0, item->drawcmds[i].scissorOffset, item->drawcmds[i].scissorExtent);
        render_add_vertices2(5, 
            (Vertex2*)g_ui->vertex_arena->start + item->drawcmds[i].vertex_offset, 
            item->drawcmds[i].counts.vertices, 
            (u32*)g_ui->index_arena->start + item->drawcmds[i].index_offset,
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
	//Log("test","ayyyye"); //NOTE(delle) uncomment after reloading .dll for testing
    g_ui->base.style.width = DeshWindow->width;
    g_ui->base.style.height = DeshWindow->height;
    
    if(g_ui->base.node.child_count){
        ui_recur(g_ui->base.node.first_child);
    }
}
