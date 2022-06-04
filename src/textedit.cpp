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
				if(codepoint == '\n'){
					result.y += config.font_height;
					line_width = 0;
				}
				line_width += config.font->max_width * config.font_height / config.font->aspect_ratio / config.font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += config.font_height;
					line_width = 0;
				}
				line_width += font_packed_char(config.font, codepoint)->xadvance * config.font_height / config.font->aspect_ratio / config.font->max_width;
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
	config.word_wrap      = 0;
    config.font           = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
    config.font_height    = 11; 
	
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
        if(main_cursor.start != main_cursor.chunk->raw.count-1 || main_cursor.chunk != last_insert){//we must branch a new chunk from the loaded file 
            TextChunk* curchunk = main_cursor.chunk;
            if(main_cursor.start == 0){
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
                prev->original = 1;
                prev->newline = 0;
                TextChunk* next = new_chunk();
                next->raw = {curchunk->raw.str + main_cursor.start, curchunk->raw.count - (s64)main_cursor.start};
                next->bg = curchunk->bg;
                next->fg = curchunk->fg;
                next->offset = curchunk->offset + main_cursor.start;
                next->original = 1;
                next->newline = curchunk->newline;
				
                NodeInsertPrev(&curchunk->node, &prev->node);        
                NodeInsertNext(&curchunk->node, &next->node);
                
                curchunk->newline = 0;
            }
            last_insert = curchunk;
            curchunk->raw = {edit_arena->cursor, DeshInput->charCount};
            main_cursor.start = 0;
        }
        Log("Insert", "Writing to ", edit_arena->cursor);
        memcpy(edit_arena->cursor, DeshInput->charIn, DeshInput->charCount);
        main_cursor.start += DeshInput->charCount;
        main_cursor.chunk->raw.count += DeshInput->charCount;
        edit_arena->cursor += DeshInput->charCount;
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
			while(utf8_continuation_byte(*(main_cursor.chunk->raw.str + main_cursor.start - 1))) main_cursor.start -= 1;
			main_cursor.start -= 1;
			main_cursor.count  = 0;
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
			if(   main_cursor.start >= main_cursor.chunk->raw.count
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
		vec2 size = CalcTextSize(chunk->raw); //maybe this can be cached?
		
        //dont render anything if it goes beyond buffer width
        //this should probably not take into account right padding
        //possibly an option
        if(visual_cursor.x > text_space.x) continue;
        //TODO(sushi) word wrapping
        if(config.word_wrap) NotImplemented;
        else{
            render_quad_filled2(visual_cursor, size, chunk->bg);
            render_text2(config.font, chunk->raw, visual_cursor, text_scale, chunk->fg);
            
			//draw cursor
			if(main_cursor.chunk == chunk){
				f32 x_offset = CalcTextSize({chunk->raw.str, (s64)main_cursor.start}).x;
				vec2 cursor_top_left  = vec2(visual_cursor.x+x_offset, visual_cursor.y);
				vec2 cursor_bot_right = vec2(cursor_top_left.x, cursor_top_left.y+config.font_height);
				render_line2(cursor_top_left, cursor_bot_right, config.cursor_color);
			}

#if 0 //DEBUG
            //render chunk outline
            render_quad2(visual_cursor, size, Color_Red);

#endif
			
			if(chunk->newline){
                visual_cursor.y += size.y;
                if(visual_cursor.y > text_space.y) break; //we've gone beyond the visual bounds so no need to render anymore
                visual_cursor.x = config.buffer_margin.x + config.buffer_padding.x;
                file_pos.x++;
            }
            else{
                visual_cursor.x += size.x;
            }


        }
    }
}