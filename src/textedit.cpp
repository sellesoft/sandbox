/*
Notes
-----
cursor is drawn to the left of the index it represents

TextChunks will never stretch across lines

*/

Arena* static_arena;
array<Arena*> edit_arenas;

Node root_chunk;

Cursor main_cursor;
array<Cursor> extra_cursors;

Config config;

File* file;

//helpers
//this should probably be moved to render
vec2 CalcTextSize(str8 text){DPZoneScoped;
	vec2 result = vec2{0, f32(config.font_height)};
	f32 line_width = 0;
	switch(config.font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if      (codepoint == U'\n'){
					result.y += config.font_height;
					line_width = 0;
				}else if(codepoint == U'\t'){
					line_width += config.tab_width * (config.font->max_width * config.font_height / config.font->aspect_ratio / config.font->max_width);
				}else{
					line_width += config.font->max_width * config.font_height / config.font->aspect_ratio / config.font->max_width;
				}
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if      (codepoint == U'\n'){
					result.y += config.font_height;
					line_width = 0;
				}else if(codepoint == U'\t'){
					line_width += config.tab_width * (font_packed_char(config.font, U' ')->xadvance * config.font_height / config.font->aspect_ratio / config.font->max_width);
				}else{
					line_width += font_packed_char(config.font, codepoint)->xadvance * config.font_height / config.font->aspect_ratio / config.font->max_width;
				}
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

TextChunk* new_chunk(){
	return (TextChunk*)memalloc(sizeof(TextChunk));
}

//loads a file and chunks it by line 

void load_file(str8 filepath){
	file = file_init(filepath, FileAccess_ReadWrite);
	if(!file){ Assert(false); return; }
	
    static_arena = memory_create_arena(file->bytes+1);
	str8 buffer = file_read(file, static_arena->start, static_arena->size);
	
	str8 remaining = buffer;
    while(remaining){
		str8 line = str8_eat_until(remaining, U'\n');
		str8_increment(&remaining, line.count+1);
		if(*(line.str+line.count-1) == '\r') line.count -= 1;
		
		TextChunk* t = new_chunk();
		NodeInsertPrev(&root_chunk, &t->node);
		t->raw     = line;
		t->offset  = line.str - static_arena->start;
		//t->count   = str8_length(line);
		t->bg      = Color_Blank;
		t->fg      = config.text_color;
		t->newline = (line.str+line.count < buffer.str+buffer.count) ? true : false;
		static_arena->cursor += line.count;
		static_arena->used   += line.count;
    }
	
	main_cursor.chunk = TextChunkFromNode(root_chunk.next);
	main_cursor.start = 0;
	main_cursor.count = 0;
}

void init_editor(){
	edit_arenas = array<Arena*>(deshi_allocator);
	edit_arenas.add(memory_create_arena(Kilobytes(1)));
	extra_cursors = array<Cursor>(deshi_allocator);
	
	root_chunk.next = root_chunk.prev = &root_chunk;
	main_cursor.chunk = 0;
	main_cursor.start = 0;
	main_cursor.count = 0;
	
	config.cursor_color   = Color_White; 
	config.cursor_pulse   = false;
	config.cursor_pulse_duration = 1000;
	config.cursor_shape   = CursorShape_VerticalLine;
	config.buffer_color   = color( 12, 12, 12,255);
	config.text_color     = color(192,192,192,255);
    config.buffer_margin  = vec2{10.f,10.f};
    config.buffer_padding = vec2{10.f,10.f};
	config.tab_width      = 4;
	config.word_wrap      = false;
	config.show_symbol_whitespace = true;
	config.show_symbol_eol        = true;
	config.show_symbol_wordwrap   = true;
    config.font           = Storage::CreateFontFromFile(STR8("gohufont-11.bdf"), 11).second;
    config.font_height    = 11; 
	
    //load_file(STR8(__FILE__));
	load_file(STR8("src/test.txt"));
}

void update_editor(){
	//-////////////////////////////////////////////////////////////////////////////////////////////
	//// input
	//// text input ////
	if(DeshInput->charCount){
        persist TextChunk* last_insert = 0;
        Arena* edit_arena = *edit_arenas.last;
		//TODO(delle) handle multiple cursor input  
        if(main_cursor.start != main_cursor.chunk->raw.count || main_cursor.chunk != last_insert){//we must branch a new chunk from the loaded file 
            TextChunk* curchunk = main_cursor.chunk;
            if(main_cursor.start == 0){
                Log("chnksplt", "   Splitting at beginning of old chunk");
                TextChunk* next = new_chunk();
                memcpy(next, curchunk, sizeof(TextChunk));
                NodeInsertNext(&curchunk->node,&next->node);
				
                curchunk->newline = 0;
            }
            else if (main_cursor.start == main_cursor.chunk->raw.count){
                TextChunk* prev = new_chunk();
                memcpy(prev, curchunk, sizeof(TextChunk));
                prev->newline = false;
                NodeInsertPrev(&curchunk->node, &prev->node);
            }
            else{ //split chunk if cursor is in the middle of it 
                TextChunk* prev = new_chunk();
                prev->raw = {curchunk->raw.str, (s64)main_cursor.start};
                prev->bg = curchunk->bg;
                prev->fg = curchunk->fg;
                prev->offset = curchunk->offset;
                prev->newline = 0;
                TextChunk* next = new_chunk();
                next->raw = {curchunk->raw.str + main_cursor.start, curchunk->raw.count - (s64)main_cursor.start};
                next->bg = curchunk->bg;
                next->fg = curchunk->fg;
                next->offset = curchunk->offset + main_cursor.start;
                next->newline = curchunk->newline;
				
                NodeInsertPrev(&curchunk->node, &prev->node);
                NodeInsertNext(&curchunk->node, &next->node);
                
                curchunk->newline = 0;
            }
			//edit_arena->cursor += 1;
            last_insert = curchunk;
            curchunk->raw = {edit_arena->cursor, 0};
            main_cursor.start = 0;
        }
        memcpy(edit_arena->cursor, DeshInput->charIn, DeshInput->charCount);
        main_cursor.start += DeshInput->charCount;
        edit_arena->cursor += DeshInput->charCount;
        main_cursor.chunk->raw.count += DeshInput->charCount;
	}
	
	//// text deletion ////
	
	
	//// cursor movement ////
	if(key_pressed(Bind_Cursor_Left)){
		if(main_cursor.start > 0){
			while(utf8_continuation_byte(*(main_cursor.chunk->raw.str + main_cursor.start - 1))) main_cursor.start -= 1;
			main_cursor.start -= 1;
			main_cursor.count  = 0;
		}else if(main_cursor.chunk->node.prev != &root_chunk){
			main_cursor.chunk = TextChunkFromNode(main_cursor.chunk->node.prev);
			main_cursor.start = main_cursor.chunk->raw.count;
			main_cursor.count = 0;
		}
	}
	if(key_pressed(Bind_Cursor_WordLeft)){
		
		
		
	}
	if(key_pressed(Bind_Cursor_WordPartLeft)){
		
	}
	
	if(key_pressed(Bind_Cursor_Right)){
		if(main_cursor.start < main_cursor.chunk->raw.count){
			DecodedCodepoint dc = decoded_codepoint_from_utf8(main_cursor.chunk->raw.str+main_cursor.start, 4);
			main_cursor.start += dc.advance;
			if(   main_cursor.start > main_cursor.chunk->raw.count
			   && main_cursor.chunk->node.next != &root_chunk){
				main_cursor.chunk = TextChunkFromNode(main_cursor.chunk->node.next);
				main_cursor.start = 0;
			}
			main_cursor.count  = 0;
		}else if(main_cursor.chunk->node.next != &root_chunk){
			main_cursor.chunk = TextChunkFromNode(main_cursor.chunk->node.next);
			main_cursor.start = 0;
			main_cursor.count = 0;
		}
	}
	if(key_pressed(Bind_Cursor_WordRight)){
		
	}
	if(key_pressed(Bind_Cursor_WordPartRight)){
		
	}
	
	if(key_pressed(Bind_Cursor_Up)){
		
	}
	if(key_pressed(Bind_Cursor_Down)){
		
	}
	
	
	//-////////////////////////////////////////////////////////////////////////////////////////////
	//// render
	render_start_cmd2(0, 0, vec2::ZERO, DeshWindow->dimensions);
    //buffer background
    render_quad_filled2(config.buffer_padding, DeshWindow->dimensions-config.buffer_padding*2, config.buffer_color);
    //buffer outline
    render_quad2(config.buffer_padding, DeshWindow->dimensions-config.buffer_padding*2, color(200,200,200));
	
    //text
    vec2i visual_cursor = config.buffer_margin + config.buffer_padding;
    vec2i file_pos = vec2::ZERO; //line/column TODO(sushi) decide if this is necessary
    vec2  text_space = DeshWindow->dimensions - (config.buffer_margin + config.buffer_padding) * 2;
    vec2  text_scale = vec2::ONE * config.font_height / (f32)config.font->max_height;
    
    render_start_cmd2(0, config.font->tex, config.buffer_padding, text_space);
	for(Node* it = root_chunk.next; it != &root_chunk; it = it->next){
        TextChunk* chunk = TextChunkFromNode(it);
		vec2i chunk_pos = visual_cursor;
		vec2 size = CalcTextSize(chunk->raw); //maybe this can be cached?
		
        //dont render anything if it goes beyond buffer width
        //this should probably not take into account right padding
        //possibly an option
        if(visual_cursor.x > text_space.x) continue;
        //TODO(sushi) word wrapping
        if(config.word_wrap) NotImplemented;
        else{
			//draw chunk bg
            render_quad_filled2(visual_cursor, size, chunk->bg);
			
			//draw chunk text
            //render_text2(config.font, chunk->raw, visual_cursor, text_scale, chunk->fg);
			str8 text = chunk->raw;
			Vertex2         vp[4];
			RenderTwodIndex ip[6];
			switch(config.font->type){
				//// BDF (and NULL) font rendering ////
				case FontType_BDF: case FontType_NONE:{
					f32 w  = config.font->max_width  * text_scale.x;
					f32 h  = config.font->max_height * text_scale.y;
					f32 dy = 1.f / (f32)config.font->count;
					
					while(text){
						DecodedCodepoint decoded = str8_advance(&text);
						if(decoded.codepoint == 0) break;
						
						//handle special characters
						if(decoded.codepoint == U'\t'){
							if(config.show_symbol_whitespace){
								render_quad_filled2({visual_cursor.x + w/2.f - 1, visual_cursor.y + h/2.f - 1},
													vec2{(w * (config.tab_width-1)),2}, config.text_color/2.f);
							}
							
							visual_cursor.x += w * config.tab_width;
						}else{
							f32 topoff = (dy * (f32)(decoded.codepoint - 32)) + config.font->uv_yoffset;
							f32 botoff = topoff + dy;
							ip[0] = 0; ip[1] = 1; ip[2] = 2; ip[3] = 0; ip[4] = 2; ip[5] = 3;
							vp[0].pos = { (f32)visual_cursor.x + 0,(f32)visual_cursor.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = chunk->fg.rgba; //top left
							vp[1].pos = { (f32)visual_cursor.x + w,(f32)visual_cursor.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = chunk->fg.rgba; //top right
							vp[2].pos = { (f32)visual_cursor.x + w,(f32)visual_cursor.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = chunk->fg.rgba; //bot right
							vp[3].pos = { (f32)visual_cursor.x + 0,(f32)visual_cursor.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = chunk->fg.rgba; //bot left
							render_add_vertices2(render_active_layer(), vp, 4, ip, 6);
							visual_cursor.x += w;
							
							if(config.show_symbol_whitespace && decoded.codepoint == U' '){
								render_quad_filled2({visual_cursor.x - w/2.f - 1, visual_cursor.y + h/2.f - 1}, vec2{2,2}, config.text_color/2.f);
							}
						}
					}
				}break;
				//// TTF font rendering ////
				case FontType_TTF:{
					while(text){
						DecodedCodepoint decoded = str8_advance(&text);
						if(decoded.codepoint == 0) break;
						
						//handle special characters
						if(decoded.codepoint == U'\t'){
							packed_char* pc = font_packed_char(config.font, U' ');
							f32 w = (pc->xoff2 - pc->xoff) * text_scale.x;
							f32 h = (pc->yoff2 - pc->yoff) * text_scale.y;
							
							if(config.show_symbol_whitespace){
								render_quad_filled2({visual_cursor.x + w/2.f - 1, visual_cursor.y + h/2.f - 1},
													vec2{(w * (config.tab_width-1)),2}, config.text_color/2.f);
							}
							
							visual_cursor.x += w * config.tab_width;
						}else{
							vec2 temp(visual_cursor);
							aligned_quad q = font_aligned_quad(config.font, decoded.codepoint, &temp, text_scale);
							visual_cursor = {(s32)temp.x, (s32)temp.y};
							ip[0] = 0; ip[1] = 1; ip[2] = 2; ip[3] = 0; ip[4] = 2; ip[5] = 3;
							vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = chunk->fg.rgba; //top left
							vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = chunk->fg.rgba; //top right
							vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = chunk->fg.rgba; //bot right
							vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = chunk->fg.rgba; //bot left
							render_add_vertices2(render_active_layer(), vp, 4, ip, 6);
							
							if(config.show_symbol_whitespace && decoded.codepoint == U' '){
								render_quad_filled2({visual_cursor.x - (q.x1 - q.x0)/2.f - 1, visual_cursor.y + (q.y1 - q.y0)/2.f - 1}, vec2{2,2}, config.text_color/2.f);
							}
						}
					}break;
					default: Assert(!"unhandled font type"); break;
				}
			}
			
#if 1 //DEBUG
            //render chunk outline
            render_quad2(visual_cursor, size, (main_cursor.chunk == chunk ? Color_Cyan : Color_Red));
#endif
			
			if(chunk->newline){
				if(config.show_symbol_eol){
					if(*(chunk->raw.str + chunk->raw.count) == '\r'){
						vec2 slashr_size = CalcTextSize(str8l("\\r"));
						render_quad_filled2(visual_cursor, slashr_size, config.text_color);
						render_text2(config.font, str8l("\\r"), visual_cursor, text_scale, config.buffer_color);
						visual_cursor.x += slashr_size.x;
					}
					vec2 slashn_size = CalcTextSize(str8l("\\n"));
					render_quad_filled2(visual_cursor, slashn_size, config.text_color);
					render_text2(config.font, str8l("\\n"), visual_cursor, text_scale, config.buffer_color);
					visual_cursor.x += slashn_size.x;
				}
				
                visual_cursor.y += size.y;
                if(visual_cursor.y > text_space.y) break; //we've gone beyond the visual bounds so no need to render anymore
                visual_cursor.x = config.buffer_margin.x + config.buffer_padding.x;
                file_pos.x += 1;
            }
			
			//draw cursor
			if(main_cursor.chunk == chunk){
				f32 x_offset = CalcTextSize({chunk->raw.str, (s64)main_cursor.start}).x;
				vec2 cursor_top_left  = vec2(chunk_pos.x+x_offset, chunk_pos.y);
				vec2 cursor_bot_right = vec2(cursor_top_left.x, cursor_top_left.y+config.font_height);
				render_line2(cursor_top_left, cursor_bot_right, config.cursor_color);
			}
        }
    }
}