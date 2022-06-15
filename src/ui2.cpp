#include "ui2.h"
#include "kigu/array.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/window.h"
#include "core/logger.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_memory
#define item_arena g_ui->item_list->arena
#define drawcmd_arena g_ui->drawcmd_list->arena

ArenaList* create_arena_list(ArenaList* old){DPZoneScoped;
	ArenaList* nual = (ArenaList*)memory_alloc(sizeof(ArenaList));
	if(old) NodeInsertNext(&old->node, &nual->node);
	nual->arena = memory_create_arena(Megabytes(1));
	return nual;
}

void* arena_add(Arena* arena, upt size){DPZoneScoped;
	if(arena->size < arena->used+size) 
		g_ui->item_list = create_arena_list(g_ui->item_list);
	u8* cursor = arena->cursor;
	arena->cursor += size;
	arena->used += size;
	return cursor;
}

void push_item(uiItem* item){DPZoneScoped;
	g_ui->item_stack.add(item);
}

uiItem* pop_item(){DPZoneScoped;
	uiItem* ret = *g_ui->item_stack.last;
	g_ui->item_stack.pop();
	return ret;
}

//@uiDrawCmd



//@Item


//@Window


void drawcmd_alloc(uiDrawCmd* drawcmd, RenderDrawCounts counts){DPZoneScoped;
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
//fills an item struct and make its a child of the current item
void ui_fill_item(uiItem* item, str8 id, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* curitem = *g_ui->item_stack.last;
	
	insert_last(&curitem->node, &item->node);
	
	if(style) memcpy(&item->style, style, sizeof(uiStyle));
	else      memcpy(&item->style, ui_initial_style, sizeof(uiStyle));
	
	//TODO(sushi) if an item has an id put it in a map
	item->id = id;
	item->file_created = file;
	item->line_created = line;
}

vec2i calc_text_size(uiText* item){DPZoneScoped;
	uiStyle* style = &item->item.style;
	str8 text = item->text;
	vec2i result = vec2i{0, (s32)style->font_height};
	f32 line_width = 0;
	switch(style->font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += style->font_height;
					line_width = 0;
				}
				line_width += style->font->max_width * style->font_height / style->font->aspect_ratio / style->font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += style->font_height;
					line_width = 0;
				}
				line_width += font_packed_char(style->font, codepoint)->xadvance * style->font_height / style->font->aspect_ratio / style->font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

#define item_error(item, ...)\
LogE("ui","Error on item created in ", item->file_created, " on line ", item->line_created, ":\n",\
          "\t", __VA_ARGS__)



//@Functionality
void ui_gen_item(uiItem* item){DPZoneScoped;
	uiDrawCmd* dc = item->drawcmds;
	Vertex2* vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*     ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	RenderDrawCounts counts = {0};
	if(item->style.background_image){
		counts+=render_make_texture(vp,ip,counts,item->style.background_image,
									vec2(item->sx, item->sy),
									vec2(item->sx+item->width, item->sy),
									vec2(item->sx+item->width, item->sy+item->height),
									vec2(item->sx, item->sy+item->height),
									1,0,0
									);
	}
	else if(item->style.background_color.a){
		counts+=render_make_filledrect(vp, ip, counts, item->spos, item->size, item->style.background_color);
	}
	switch(item->style.border_style){
		case border_none:{}break;
		case border_solid:{
			vec2i tl = item->spos;
			vec2i br = tl+item->size;
			vec2i tr = vec2i{br.x, tl.y};
			vec2i bl = vec2i{tl.x, br.y}; 
			u32 t = item->style.border_width;
			u32 v = counts.vertices; u32 i = counts.indices;
			ip[i+ 0] = v+0; ip[i+ 1] = v+1; ip[i+ 2] = v+3; 
			ip[i+ 3] = v+0; ip[i+ 4] = v+3; ip[i+ 5] = v+2; 
			ip[i+ 6] = v+2; ip[i+ 7] = v+3; ip[i+ 8] = v+5; 
			ip[i+ 9] = v+2; ip[i+10] = v+5; ip[i+11] = v+4; 
			ip[i+12] = v+4; ip[i+13] = v+5; ip[i+14] = v+7; 
			ip[i+15] = v+4; ip[i+16] = v+7; ip[i+17] = v+6; 
			ip[i+18] = v+6; ip[i+19] = v+7; ip[i+20] = v+1; 
			ip[i+21] = v+6; ip[i+22] = v+1; ip[i+23] = v+0;
			vp[v+0].pos = tl;              vp[v+0].uv = {0,0}; vp[v+0].color = item->style.border_color.rgba;
			vp[v+1].pos = tl+vec2i( t, t); vp[v+1].uv = {0,0}; vp[v+1].color = item->style.border_color.rgba;
			vp[v+2].pos = tr;              vp[v+2].uv = {0,0}; vp[v+2].color = item->style.border_color.rgba;
			vp[v+3].pos = tr+vec2i(-t, t); vp[v+3].uv = {0,0}; vp[v+3].color = item->style.border_color.rgba;
			vp[v+4].pos = br;              vp[v+4].uv = {0,0}; vp[v+4].color = item->style.border_color.rgba;
			vp[v+5].pos = br+vec2i(-t,-t); vp[v+5].uv = {0,0}; vp[v+5].color = item->style.border_color.rgba;
			vp[v+6].pos = bl;              vp[v+6].uv = {0,0}; vp[v+6].color = item->style.border_color.rgba;
			vp[v+7].pos = bl+vec2i( t,-t); vp[v+7].uv = {0,0}; vp[v+7].color = item->style.border_color.rgba;
			counts += {8, 24};
		}break;
	}
	int i = 0;
}


uiItem* ui_make_item(str8 id, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
	ui_fill_item(item, id, style, file, line);
	
	item->drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)); 
	
	RenderDrawCounts counts = //reserve enough room for a background and border 
		render_make_filledrect_counts() +
		render_make_rect_counts();
	
	item->draw_cmd_count = 1;
	drawcmd_alloc(item->drawcmds, counts);
	return item;
}

uiItem* ui_begin_item(str8 id, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_item(id, style, file, line);
	push_item(item);
	return item;
}

void ui_end_item(str8 file, upt line){
	if(*(g_ui->item_stack.last) == &g_ui->base){
		LogE("ui", 
			 "In ", file, " at line ", line, " :\n",
			 "\tAttempted to end base item. Did you call uiItemE too many times? Did you use uiItemM instead of uiItemB?"
			 );
		//TODO(sushi) implement a hint showing what instruction could possibly be wrong 
	}
	pop_item();
	
}

uiItem* ui_make_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){DPZoneScoped;
	//uiBeginItem(uiWindow, win, &g_ui->base, uiItemType_Window, flags, 1, file, line);
	uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
	item->node.type = uiItemType_Window;
	
	return 0;
}

uiItem* ui_begin_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){DPZoneScoped;
	ui_make_window(name, flags, style, file, line);
	
	return 0;
}

uiItem* ui_end_window(){
	return 0;
}

uiButton* ui_make_button(Action action, void* action_data, Flags flags, str8 file, upt line){DPZoneScoped;
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

void ui_gen_text(uiItem* item){DPZoneScoped;
	uiText* t = uiTextFromItem(item);
	RenderDrawCounts counts = {0};
	uiDrawCmd* dc = t->item.drawcmds;
	Vertex2* vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*     ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	render_make_text(vp, ip, counts, t->text, t->item.style.font, t->item.spos, t->item.style.text_color, vec2::ONE * t->item.style.font_height / t->item.style.font->max_height);
	//NOTE(sushi) text size is always determined here and NEVER set by the user
	//TODO(sushi) text sizing should probably be moved to eval_item_branch and made to take into account wrapping
	t->item.style.size = calc_text_size(t);
	dc->scissorOffset = item->spos;
	dc->scissorExtent = item->style.size;
}

uiText* ui_make_text(str8 text, str8 id, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiText* item = (uiText*)arena_add(item_arena, sizeof(uiText));
	ui_fill_item(&item->item, id, style, file, line);
	
	item->item.node.type = uiItemType_Text;
	item->item.drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)); 
	item->text = text;
	
	RenderDrawCounts counts = render_make_text_counts(str8_length(text));
	
	item->item.draw_cmd_count = 1;
	drawcmd_alloc(item->item.drawcmds, counts);
	return item;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context
void ui_init(MemoryContext* memctx, uiContext* uictx){DPZoneScoped;
#if DESHI_RELOADABLE_UI
	g_memory = memctx;
	g_ui     = uictx;
#endif
	
	g_ui->item_list    = create_arena_list(0);
	g_ui->drawcmd_list = create_arena_list(0);
	g_ui->vertex_arena = memory_create_arena(Megabytes(1));
	g_ui->index_arena  = memory_create_arena(Megabytes(1));
	
	ui_initial_style->     positioning = pos_static;
	ui_initial_style->          sizing = 0;
	ui_initial_style->            left = 0;
	ui_initial_style->             top = 0;
	ui_initial_style->           right = MAX_F32;
	ui_initial_style->          bottom = MAX_F32;
	ui_initial_style->           width = size_auto;
	ui_initial_style->          height = size_auto;
	ui_initial_style->     margin_left = 0;
	ui_initial_style->      margin_top = 0;
	ui_initial_style->    margin_right = MAX_F32;
	ui_initial_style->   margin_bottom = MAX_F32;
	ui_initial_style->    padding_left = 0;
	ui_initial_style->     padding_top = 0;
	ui_initial_style->   padding_right = MAX_F32;
	ui_initial_style->  padding_bottom = MAX_F32;
	ui_initial_style->   content_align = 0;
	ui_initial_style->            font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
	ui_initial_style->     font_height = 11;
	ui_initial_style->background_color = color{0,0,0,0};
	ui_initial_style->background_image = 0;
	ui_initial_style->    border_style = border_none;
	ui_initial_style->    border_color = color{180,180,180,255};
	ui_initial_style->    border_width = 1;
	ui_initial_style->      text_color = color{255,255,255,255};
	ui_initial_style->        overflow = overflow_visible;
	
	g_ui->base = uiItem{0};
	g_ui->base.style = *ui_initial_style;
	g_ui->base.node.type = uiItemType_Item;
	g_ui->base.file_created = STR8(__FILE__);
	g_ui->base.line_created = __LINE__;
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.id = STR8("base");
	g_ui->base.style_hash = hash<uiStyle>()(g_ui->base.style);
	push_item(&g_ui->base);
	
	//g_ui->render_buffer = render_create_external_2d_buffer(Megabytes(1), Megabytes(1));
}

//finds the container of an item eg. a window, child window, or section
//probably
TNode* ui_find_container(TNode* item){DPZoneScoped;
	if(item->type == uiItemType_Window || 
	   item->type == uiItemType_ChildWindow) return item;
	if(item->parent) return ui_find_container(item->parent);
	return 0;
}

//pass 0 for child on first call
TNode* ui_find_static_sized_parent(TNode* node, TNode* child){DPZoneScoped;
	uiItem* item = uiItemFromNode(node);
	if(!child) return ui_find_static_sized_parent(item->node.parent, &item->node);
	if(item->style.width != size_auto && item->style.height != size_auto){
		return &item->node;
	}else{
		return ui_find_static_sized_parent(item->node.parent, &item->node);
	}
}

void draw_item_branch(uiItem* item){DPZoneScoped;
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
void eval_item_branch(uiItem* item){DPZoneScoped;
	uiItem* parent = uiItemFromNode(item->node.parent);
	
	b32 wauto = item->style.width == size_auto;
	b32 hauto = item->style.height == size_auto;
	u32 wborder = item->style.border_width;
    
    if(!hauto){
        if(HasFlag(item->style.sizing, size_percent_y)){
            if(item->style.height < 0) 
                item_error(item, "Sizing value was specified with size_percent_y, but the given value for height '", item->style.height, "' is negative.");
            if(HasFlag(parent->style.sizing, size_percent_y) || HasFlag(parent->style.sizing, size_fill_percent_y)){
                item->height = item->style.height/100.f * parent->height;
            }else if (parent->style.height >= 0){
                item->height = item->style.height/100.f * parent->style.height;
            }else{
                item_error(item, "Sizing value was specified with size_percent_y, but its parent's height is not explicitly sized.");
                hauto = 1;
            }
        }else if(HasFlag(item->style.sizing, size_fill_percent_y)){
            NotImplemented;
        }else item->height = item->style.height;
    }else item->height = 0;

    if(!wauto){
        if(HasFlag(item->style.sizing, size_percent_x)){
            if(item->style.width < 0) 
                item_error(item, "Sizing value was specified with size_percent_x, but the given value for width '", item->style.width, "' is negative.");
            if(HasFlag(parent->style.sizing, size_percent_x) || HasFlag(parent->style.sizing, size_fill_percent_x)){
                item->width = item->style.width/100.f * parent->width;
            }else if (parent->style.width >= 0){
                item->width = item->style.width/100.f * parent->style.width;
            }else{
                item_error(item, "Sizing value was specified with size_percent_x, but its parent's width is not explicitly sized.");
                hauto = 1;
            }
        }else if(HasFlag(item->style.sizing, size_fill_percent_x)){
            NotImplemented;
        }else item->width = item->style.width;
    }else item->width = 0;

	vec2i cursor = item->style.paddingtl;
	for_node(item->node.first_child){
		uiItem* child = uiItemFromNode(it);
		eval_item_branch(child);    
		vec2i cursorloc = cursor;
		switch(child->style.positioning){
			case pos_static:{
				child->lpos =  child->style.margintl;
				if(item->style.border_style)
					child->lpos += floor(item->style.border_width) * vec2::ONE;
				child->lpos += item->scroll;
				child->lpos += cursor;
			}break;
			case pos_relative:
			case pos_draggable_relative:{
				child->lpos =  child->style.margintl;
				if(item->style.border_style)
					child->lpos += floor(item->style.border_width) * vec2::ONE;
				child->lpos += item->scroll;
				child->lpos += cursor;
				child->lpos += child->style.tl;
			}break;
		}
		
        cursor.x = item->style.padding_left;
        cursor.y = child->lpos.y + child->height;
    }

    if(hauto){
        if(item->style.padding_bottom == MAX_F32) item->height += item->style.padding_top;
        else if(item->style.padding_bottom > 0) item->height += item->style.padding_bottom;
    }
    if(wauto){
        if(item->style.padding_right == MAX_F32) item->width += item->style.padding_left;
        else if(item->style.padding_right > 0) item->width += item->style.padding_right;
    }

    item->width += (wauto ? 1 : 2) * wborder;
    item->height += (hauto ? 1 : 2) * wborder;

    if(item->style.content_align > 0){
        f32 last_static_offset = 0;
        f32 padr = item->style.padding_right;
        f32 padl = item->style.padding_left;
        f32 child_space = (item->width - (padr==MAX_F32?padl:padr)) - padl;
        for_node(item->node.first_child){
            uiItem* child = uiItemFromNode(it);
            if(child->style.positioning == pos_static){
                last_static_offset = child->lpos.x;
                f32 marr = child->style.margin_right;
                f32 marl = child->style.margin_left;
                f32 true_width = child->width + (marr==MAX_F32?marl:marr) + marl;
                child->lpos.x = item->style.padding_left+ child->style.margin_left + item->style.content_align * (child_space - true_width);
                last_static_offset = child->lpos.x - last_static_offset;
            }else if(child->style.positioning==pos_relative){
                child->lpos.x += last_static_offset;
            }
        }
    }
}

void drag_item(uiItem* item){DPZoneScoped;
	if(!item) return;
	if(item->style.positioning == pos_draggable_fixed || 
	   item->style.positioning == pos_draggable_relative){
		vec2 mp_cur = input_mouse_position();
		persist b32 dragging = false;
		persist vec2 mp_offset;
		if(key_pressed(Mouse_LEFT) && Math::PointInRectangle(mp_cur, item->spos, item->size)){
			mp_offset = item->style.tl - mp_cur;
			dragging = true;
			g_ui->istate = uiISDragging;            
		}
		if(key_released(Mouse_LEFT)){ dragging = false;  g_ui->istate = uiISNone; }
		
		if(dragging){
			item->style.tl = input_mouse_position() + mp_offset;
		}
	}
}

//depth first walk to ensure we find topmost hovered item
b32 find_hovered_item(uiItem* item){DPZoneScoped;
	for_node(item->node.first_child){
		if(find_hovered_item(uiItemFromNode(it))) return 1;
	}
	if(Math::PointInRectangle(input_mouse_position(), item->spos, item->size)){
		g_ui->hovered = item;
		return 1;
	}
	return 0;
}

void ui_recur(TNode* node){DPZoneScoped;
	uiItem* item = uiItemFromNode(node);
	uiItem* parent = uiItemFromNode(node->parent);
	
	//check if an item's style was modified, if so reevaluate the item,
	//its children, and every child of its parents until a manually sized parent is found
	u32 nuhash = hash<uiStyle>()(item->style);
	if(nuhash!=item->style_hash){
		item->style_hash = nuhash; 
		uiItem* sspar = uiItemFromNode(ui_find_static_sized_parent(&item->node, 0));
		eval_item_branch(sspar);
		draw_item_branch(sspar);
	} 
	
	//do updates of each item type
	switch(node->type){
		case uiItemType_Button:{ uiButton* item = uiButtonFromNode(node);
		}break;
	}
	
	//render item
	forI(item->draw_cmd_count){
		render_set_active_surface_idx(0);
		Texture* tex = 0;
		if(item->node.type == uiItemType_Text){
			tex = item->style.font->tex;
		}else if(item->style.background_image){
			tex = item->style.background_image;
		}
		vec2 scoff;
		vec2 scext;
		uiItem* parent = uiItemFromNode(item->node.parent); 
		if(parent->style.overflow == overflow_hidden){
			scoff = parent->spos; scext = parent->size; 
		}else{
			scoff = vec2::ZERO; scext = DeshWindow->dimensions;
		}
		render_start_cmd2(5, tex, scoff, scext);
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

void ui_update(){DPZoneScoped;
	//Log("test","ayyyye"); //NOTE(delle) uncomment after reloading .dll for testing
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	
	//in order to prevent the mouse from entering another window during input
	//this can probably be moved to only be done on mouse input, but maybe left so 
	//user can get this info at any time OR make a function for that
	if(g_ui->istate == uiISNone) 
		find_hovered_item(&g_ui->base);
	
	if(g_ui->istate == uiISNone || g_ui->istate == uiISDragging) 
		drag_item(g_ui->hovered);
	
	if(g_ui->base.node.child_count){
		ui_recur(g_ui->base.node.first_child);
	}
}

#undef ui_alloc
#undef ui_realloc
#undef ui_free
#undef ui_talloc
#undef ui_create_arena
#undef ui_delete_arena
#undef item_arena
#undef drawcmd_arena