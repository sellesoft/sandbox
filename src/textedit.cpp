/*
Notes
-----

the editor is designed to move memory as little as possible
	the file is loaded into a static arena, assigned a single TextChunk and Lines are cached. 
	edits are stored in edit arenas and are tracked using TextChunks.
	when a user starts editing, a new TextChunk is made and pointed to the end of the edit arena
	this allows us to leave the loaded file untouched until we save or stitch the file back together.

cursor is drawn to the left of the index it represents
cursor favors being at the end of a chunk than at the beginning
buffer memory is NOT contiguous and is stitched together when we save the file


*/

Arena* static_arena;
array<Arena*> edit_arenas;
TextChunk* current_edit_chunk = 0;

Node root_chunk;
#define IsRootChunk(x) ((x) == &root_chunk)
Node root_line;
#define IsRootLine(x) ((x) == &root_line)

mutex linelock; //locks following node
Node root_line_chunks; // maintained indexing of lines

Cursor main_cursor;
array<Cursor> extra_cursors;

//NOTE(sushi) add this to Buffer struct when that is added
b32 buffer_CRLF;
u32 line_end_char; //either \n or \r, so we dont have to keep checking what buffer_CRLF is everywhere
u32 line_end_length; //either 1 or 2, so we dont have to keep checking what buffer_CRLF is everywhere

Config config;
KeyBinds binds;
#define CMI(name, type, var) ConfigMapItem{STR8(name), (type), (var)}
global ConfigMapItem ConfigMap[] = {
	CMI("#---------------------------------#", ConfigValueType_NONE, 0),
	CMI("////////// Buffer Config //////////", ConfigValueType_NONE, 0),
	CMI("#---------------------------------#", ConfigValueType_NONE, 0),
	CMI("this section contains options pertaining to the buffer, the window where text is edited", ConfigValueType_NONE, 0),
	CMI("\n",                     ConfigValueType_PADSECTION,(void*)22),
	CMI("cursor_color", 		  ConfigValueType_Col,  &config.cursor_color),
	CMI("cursor_pulse", 		  ConfigValueType_B32,  &config.cursor_pulse),
	CMI("cursor_pulse_duration",  ConfigValueType_F32,  &config.cursor_pulse_duration),
	CMI("cursor_shape",           ConfigValueType_U32,  &config.cursor_shape), 
	CMI("\n",                     ConfigValueType_PADSECTION,(void*)13),
	CMI("buffer_color",           ConfigValueType_Col,  &config.buffer_color),
	CMI("text_color",             ConfigValueType_Col,  &config.text_color),
	CMI("\n",                     ConfigValueType_PADSECTION,(void*)15),
	CMI("buffer_margin",          ConfigValueType_FV2,  &config.buffer_margin),
	CMI("buffer_padding",         ConfigValueType_FV2,  &config.buffer_padding),
	CMI("\n",                     ConfigValueType_PADSECTION,(void*)10),
	CMI("tab_width",              ConfigValueType_U32,  &config.tab_width),
	CMI("word_wrap",              ConfigValueType_B32,  &config.word_wrap),
	CMI("\n",                     ConfigValueType_PADSECTION,(void*)23),
	CMI("show_symbol_whitespace", ConfigValueType_B32,  &config.show_symbol_whitespace),
	CMI("show_symbol_eol",        ConfigValueType_B32,  &config.show_symbol_eol),
	CMI("show_symbol_wordwrap",   ConfigValueType_B32,  &config.show_symbol_wordwrap),
	CMI("\n",                     ConfigValueType_PADSECTION,(void*)12),
	CMI("font",                   ConfigValueType_Font, &config.font),
	CMI("font_height",            ConfigValueType_U32,  &config.font_height),
	
	CMI("#--------------------------------#", ConfigValueType_NONE, 0),
	CMI("////////// Input Config //////////", ConfigValueType_NONE, 0),
	CMI("#--------------------------------#", ConfigValueType_NONE, 0),
	CMI("this section contains options pertaining to the behavoir of input and keybinds", ConfigValueType_NONE, 0),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)15),
	CMI("repeat_hold",            ConfigValueType_F32, &config.repeat_hold),
	CMI("repeat_throttle",        ConfigValueType_F32, &config.repeat_throttle),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)22),
	CMI("cursor_left",            ConfigValueType_KeyMod, &binds.cursorLeft),
	CMI("cursor_word_left",       ConfigValueType_KeyMod, &binds.cursorWordLeft),
	CMI("cursor_word_part_left",  ConfigValueType_KeyMod, &binds.cursorWordPartLeft),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)23),
	CMI("cursor_right",           ConfigValueType_KeyMod, &binds.cursorRight),
    CMI("cursor_word_right",      ConfigValueType_KeyMod, &binds.cursorWordRight),
    CMI("cursor_word_part_right", ConfigValueType_KeyMod, &binds.cursorWordPartRight),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)12),
	CMI("cursor_up",              ConfigValueType_KeyMod, &binds.cursorUp),
    CMI("cursor_down",            ConfigValueType_KeyMod, &binds.cursorDown),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)14),
    CMI("cursor_anchor",          ConfigValueType_KeyMod, &binds.cursorAnchor),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)15),
    CMI("select_left",            ConfigValueType_KeyMod, &binds.selectLeft),
    CMI("select_right",           ConfigValueType_KeyMod, &binds.selectRight),
    CMI("select_up",              ConfigValueType_KeyMod, &binds.selectUp),
    CMI("select_down",            ConfigValueType_KeyMod, &binds.selectDown),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)22),
	CMI("delete_left",            ConfigValueType_KeyMod, &binds.deleteLeft),
	CMI("delete_word_left",       ConfigValueType_KeyMod, &binds.deleteWordLeft),
	CMI("delete_word_part_left",  ConfigValueType_KeyMod, &binds.deleteWordPartLeft),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)23),
	CMI("delete_right",           ConfigValueType_KeyMod, &binds.deleteRight),
	CMI("delete_word_right",      ConfigValueType_KeyMod, &binds.deleteWordRight),
	CMI("delete_word_part_right", ConfigValueType_KeyMod, &binds.deleteWordPartRight),
	CMI("\n",                     ConfigValueType_PADSECTION, (void*)14),
    CMI("save_buffer",            ConfigValueType_KeyMod, &binds.saveBuffer),
    CMI("reload_config",          ConfigValueType_KeyMod, &binds.reloadConfig),
	
};
#undef CMI


File* file;



//NOTE(delle) including this after the vars so it can access them
#include "editor_commands.cpp"

void load_config(){DPZoneScoped;
	if(!file_exists(STR8("data/cfg/editor.cfg"))){
		config.cursor_color           = Color_White; 
		config.cursor_pulse           = false;
		config.cursor_pulse_duration  = 1000;
		config.cursor_shape           = CursorShape_VerticalLine;
		
		config.buffer_color           = color( 12, 12, 12,255);
		config.text_color             = color(192,192,192,255);
		
		config.buffer_margin          = vec2{10.f,10.f};
		config.buffer_padding         = vec2{10.f,10.f};
		
		config.tab_width              = 4;
		config.word_wrap              = false;
		
		config.show_symbol_whitespace = false;
		config.show_symbol_eol        = false;
		config.show_symbol_wordwrap   = false;
		
		config.repeat_hold            = 500;
		config.repeat_throttle        = 25;
		
		config.font                   = Storage::CreateFontFromFile(STR8("gohufont-11.bdf"), 11).second;
		config.font_height            = 11; 
		
		binds.cursorLeft          = Key_LEFT | InputMod_None;
		binds.cursorWordLeft      = Key_LEFT | InputMod_AnyCtrl;
		binds.cursorWordPartLeft  = Key_LEFT | InputMod_AnyAlt;
		
		binds.cursorRight         = Key_RIGHT | InputMod_None;
		binds.cursorWordRight     = Key_RIGHT | InputMod_AnyCtrl;
		binds.cursorWordPartRight = Key_RIGHT | InputMod_AnyAlt;
		
		binds.cursorUp            = Key_UP    | InputMod_None;
		binds.cursorDown          = Key_DOWN  | InputMod_None;
		
		binds.cursorAnchor        = Key_SPACE | InputMod_AnyCtrl;
		
		binds.selectLeft          = Key_LEFT  | InputMod_AnyShift;
		binds.selectWordLeft      = Key_LEFT  | InputMod_AnyShift | InputMod_AnyCtrl;
		binds.selectWordPartLeft  = Key_LEFT  | InputMod_AnyShift | InputMod_AnyAlt;
		
		binds.selectRight         = Key_RIGHT | InputMod_AnyShift;
		binds.selectWordRight     = Key_RIGHT | InputMod_AnyShift | InputMod_AnyCtrl;
		binds.selectWordPartRight = Key_RIGHT | InputMod_AnyShift | InputMod_AnyAlt;
		
		binds.selectUp            = Key_UP    | InputMod_AnyShift;
		binds.selectDown          = Key_DOWN  | InputMod_AnyShift;
		
		binds.deleteLeft          = Key_BACKSPACE | InputMod_None;
		binds.deleteWordLeft      = Key_BACKSPACE | InputMod_AnyCtrl;
		binds.deleteWordPartLeft  = Key_BACKSPACE | InputMod_AnyAlt;
		
		binds.deleteRight         = Key_DELETE | InputMod_None;
		binds.deleteWordRight     = Key_DELETE | InputMod_AnyCtrl;
		binds.deleteWordPartRight = Key_DELETE | InputMod_AnyAlt;
		
		binds.saveBuffer          = Key_S | InputMod_AnyCtrl;
		
		binds.reloadConfig        = Key_F5 | InputMod_AnyCtrl;
		
		config_save(STR8("data/cfg/editor.cfg"), ConfigMap, sizeof(ConfigMap)/sizeof(ConfigMapItem));
	}
	else{
		config_load(STR8("data/cfg/editor.cfg"), ConfigMap, sizeof(ConfigMap)/sizeof(ConfigMapItem));
	}
}

vec2 CalcTextSize(str8 text){DPZoneScoped;
	vec2 result = vec2{0, f32(config.font_height)};
	f32 line_width = 0;
	switch(config.font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if      (codepoint == U'\r'){
					continue;
				}else if(codepoint == U'\n'){
					result.y += config.font_height;
					line_width = 0;
				}else if(codepoint == U'\t'){
					line_width += config.tab_width * (config.font->max_width * config.font_height / config.font->max_height);
				}else{
					line_width += config.font->max_width * config.font_height / config.font->max_height;
				}
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if      (codepoint == U'\r'){
					continue;
				}else if(codepoint == U'\n'){
					result.y += config.font_height;
					line_width = 0;
				}else if(codepoint == U'\t'){
					line_width += config.tab_width * (font_packed_char(config.font, U' ')->xadvance * config.font_height / config.font->max_height);
				}else{
					line_width += font_packed_char(config.font, codepoint)->xadvance * config.font_height / config.font->max_height;
				}
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

TextChunk* new_chunk(){DPZoneScoped;
	return (TextChunk*)memalloc(sizeof(TextChunk));
}

Line* new_line(){DPZoneScoped;
	return (Line*)memalloc(sizeof(Line));
}

//inserts line and updates all following lines's indexes
void insert_line(Node* dest, Node* src){DPZoneScoped;
	NodeInsertNext(dest, src);
	Line* destl = LineFromNode(dest);
	Line* srcl = LineFromNode(src);
	srcl->index = destl->index + 1;
	for(Node* n = src->next; !IsRootLine(n); n = n->next){
		LineFromNode(n)->index++;
	}
}

//loads a file and creates one TextChunk encompassing the entire file
//and caches line info
void load_file(str8 filepath){DPZoneScoped;
	file = file_init(filepath, FileAccess_ReadWrite);
	if(!file){ Assert(false); return; }
	
    static_arena = memory_create_arena(file->bytes+1);
	str8 buffer = file_read(file, static_arena->start, static_arena->size);
	
	TextChunk* initial = new_chunk();
	initial->raw = buffer;
	initial->offset = 0;
	NodeInsertPrev(&root_chunk, &initial->node);
	
	u32 index = 0;
	str8 remaining = buffer;
    while(remaining){
		str8 line = str8_eat_until(remaining, U'\n'); line.count++;
		if(*(line.str+line.count-2)=='\r'){
			buffer_CRLF = 1;
			line_end_char = '\r';
			line_end_length = 2;
		}else{
			buffer_CRLF = 0;
			line_end_char = '\n';
			line_end_length = 1;
		}
		str8_increment(&remaining, line.count);
		
		Line* l = new_line();
		l->raw = line;
		l->count = str8_length(line);
		l->index = index++;
		NodeInsertPrev(&root_line, &l->node);
		
		static_arena->cursor += line.count;
		static_arena->used   += line.count;
    }
	initial->line = LineFromNode(root_line.next);
	
	main_cursor.chunk = TextChunkFromNode(root_chunk.next);
	main_cursor.line = LineFromNode(root_line.next);
	main_cursor.chunk_start = 0;
	main_cursor.line_start = 0;
}


void init_editor(){DPZoneScoped;
	edit_arenas = array<Arena*>(deshi_allocator);
	edit_arenas.add(memory_create_arena(Kilobytes(1)));
	extra_cursors = array<Cursor>(deshi_allocator);
	
	root_chunk.next = root_chunk.prev = &root_chunk;
	root_line.next = root_line.prev = &root_line;
	main_cursor.chunk = 0;
	main_cursor.chunk_start = 0;
	main_cursor.line_start = 0;
	main_cursor.count = 0;
	
	load_config();
	
	init_editor_commands();
	
    //load_file(STR8(__FILE__));
	load_file(STR8("data/cfg/editor.cfg"));
}

//calculates the length of a line that a chunk is currently on in codepoints 
u64 calc_line_length(TextChunk* chunk){
	return 0;
}

//returns the number of bytes the cursor moved
u64 move_cursor(Cursor* cursor, KeyCode bind){DPZoneScoped;
	u64 count = 0;
	if      (match_any(bind, binds.cursorRight, binds.selectRight)){////////////////////////////////// Move/Select Right
		DecodedCodepoint dc = str8_index(cursor->chunk->raw, cursor->chunk_start);
		if(dc.codepoint == line_end_char){
			dc.advance = line_end_length;
			cursor->line_start = 0;
			cursor->line = NextLine(cursor->line);
		} else cursor->line_start += dc.advance;
		if(cursor->chunk_start < cursor->chunk->raw.count){
			cursor->chunk_start += dc.advance;
		}else if(!IsRootChunk(cursor->chunk->node.next)){
			//TODO(sushi) confirm if this works when the chunk also ends at a newline
			cursor->chunk = NextTextChunk(cursor->chunk);
			cursor->chunk_start = 0;
		}
		cursor->count = 0;
		count += dc.advance;
	}else if(match_any(bind, binds.cursorLeft, binds.selectLeft)){///////////////////////////////////// Move/Select Left
		if(!cursor->chunk->offset && !cursor->chunk_start) return 0;
		count += utf8_move_back(cursor->chunk->raw.str+cursor->chunk_start);
		count++;
		DecodedCodepoint dc = str8_index(cursor->chunk->raw, cursor->chunk_start);
		if(!IsRootLine(cursor->line->node.prev) && dc.codepoint == '\n'){//we can only hit a \n from here
			count = line_end_length;
			cursor->line = PrevLine(cursor->line);
			cursor->line_start = cursor->line->raw.count - line_end_length;
		} else cursor->line_start -= count;
		if(cursor->chunk_start > 0){
			cursor->chunk_start -= count;
		}else if(!IsRootChunk(cursor->chunk->node.prev)){
			cursor->chunk = PrevTextChunk(cursor->chunk);
			cursor->chunk_start = cursor->chunk->raw.count;
		}	
		cursor->count = 0;
	}
	// u64 count = 0;
	// if      (match_any(bind, binds.cursorLeft, binds.selectLeft)){////////////////////////////////////  Move/Select Left
	// 	if(cursor->start > 0){
	// 		while(utf8_continuation_byte(*(cursor->chunk->raw.str + cursor->start - 1))){
	// 			cursor->start -= 1;
	// 			count++;
	// 		} 
	// 		cursor->start -= 1;
	// 		count++;
	// 		if(bind==binds.selectLeft) cursor->count += 1;
	// 		else                      cursor->count  = 0;
	// 	}else if(cursor->chunk->node.prev != &root_chunk){
	// 		cursor->chunk = TextChunkFromNode(cursor->chunk->node.prev);
	// 		cursor->start = cursor->chunk->raw.count;
	// 		if(!cursor->chunk->newline){
	// 			while(utf8_continuation_byte(*(cursor->chunk->raw.str + cursor->start - 1))){ 
	// 				cursor->start -= 1; 
	// 				count++;
	// 			}
	// 			if(bind==binds.selectLeft) 
	// 				cursor->count += 1;
	// 			cursor->start -= 1;
	// 			cursor->column--;
	// 			count++;
	// 		}
	// 		else {
	// 			cursor->line--;
	// 		}
	// 		if(bind!=binds.selectLeft) 
	// 			cursor->count = 0;
	// 	}
	// }else if(match_any(bind, binds.cursorWordLeft, binds.selectWordLeft)){//////////////////////// Move/Select Word Left
	// 	b32 skip_alnum = -1;
	// 	for(;;){
	// 		if(cursor->start > 0){
	// 			if(skip_alnum == -1) skip_alnum = isalnum(*(cursor->chunk->raw.str + cursor->start - 1));
	
	// 			while(utf8_continuation_byte(*(cursor->chunk->raw.str + cursor->start - 1))){ 
	// 				cursor->start -= 1;
	// 				count++;
	// 			}
	// 			count++;
	// 			cursor->start -= 1;
	// 			cursor->count  = 0;
	
	// 			if( skip_alnum && !isalnum(*(cursor->chunk->raw.str + cursor->start))){ cursor->start += 1; break; }
	// 			if(!skip_alnum &&  isalnum(*(cursor->chunk->raw.str + cursor->start))){ cursor->start += 1; break; }
	// 			if(cursor->start == 0 && cursor->chunk->node.prev != &root_chunk && TextChunkFromNode(cursor->chunk->node.prev)->newline) break;
	// 		}else if(cursor->chunk->node.prev != &root_chunk){
	// 			cursor->chunk = TextChunkFromNode(cursor->chunk->node.prev);
	// 			cursor->start = cursor->chunk->raw.count;
	
	// 			if(skip_alnum == -1) skip_alnum = isalnum(*(cursor->chunk->raw.str + cursor->start - 1));
	
	// 			while(utf8_continuation_byte(*(cursor->chunk->raw.str + cursor->start - 1))){
	// 				cursor->start -= 1;
	// 				count++;
	// 			} 
	// 			count++;
	// 			cursor->start -= 1;
	// 			cursor->count  = 0;
	
	// 			if(cursor->chunk->newline){ cursor->start += 1; break; }
	// 			if( skip_alnum && !isalnum(*(cursor->chunk->raw.str + cursor->start))){ cursor->start += 1; break; }
	// 			if(!skip_alnum &&  isalnum(*(cursor->chunk->raw.str + cursor->start))){ cursor->start += 1; break; }
	// 		}else{
	// 			break;
	// 		}
	// 	}
	// }else if(match_any(bind, binds.cursorRight, binds.selectRight)){////////////////////////////////// Move/Select Right
	// 	if(cursor->start < cursor->chunk->raw.count){
	// 		DecodedCodepoint dc = decoded_codepoint_from_utf8(cursor->chunk->raw.str+cursor->start,4);
	// 		cursor->start += dc.advance;
	// 		count += dc.advance;
	// 		cursor->count  = 0;
	// 	}else if(cursor->chunk->node.next != &root_chunk){
	// 		TextChunk* prev_chunk = cursor->chunk;
	// 		cursor->chunk = TextChunkFromNode(cursor->chunk->node.next);
	// 		if(!prev_chunk->newline){
	// 			DecodedCodepoint dc = decoded_codepoint_from_utf8(cursor->chunk->raw.str+cursor->start, 4);        
	// 			cursor->start = dc.advance;
	// 			count += dc.advance;
	// 		}else{
	// 			cursor->start = 0;
	// 		}
	// 		cursor->count = 0;
	// 	}
	// }else if(match_any(bind, binds.cursorWordRight, binds.selectWordRight)){///////////////////// Move/Select Word Right 
	// 	b32 skip_alnum = -1;
	// 	for(;;){
	// 		if(cursor->start < cursor->chunk->raw.count){
	// 			DecodedCodepoint dc = decoded_codepoint_from_utf8(cursor->chunk->raw.str+cursor->start, 4);
	// 			if(skip_alnum == -1) skip_alnum = isalnum(dc.codepoint);
	
	// 			cursor->start += dc.advance;
	// 			cursor->count  = 0;
	// 			count += dc.advance;
	
	// 			if(cursor->start >= cursor->chunk->raw.count){
	// 				if(cursor->chunk->newline) break;
	// 			}else{
	// 				if( skip_alnum && !isalnum(*(cursor->chunk->raw.str + cursor->start))) break;
	// 				if(!skip_alnum &&  isalnum(*(cursor->chunk->raw.str + cursor->start))) break;
	// 			}
	// 		}else if(cursor->chunk->node.next != &root_chunk){
	// 			TextChunk* prev_chunk = cursor->chunk;
	// 			cursor->chunk = TextChunkFromNode(cursor->chunk->node.next);
	// 			cursor->start = 0;
	// 			cursor->count = 0;
	
	// 			DecodedCodepoint dc = decoded_codepoint_from_utf8(cursor->chunk->raw.str+cursor->start, 4);
	// 			if(skip_alnum == -1) skip_alnum = isalnum(dc.codepoint);
	
	// 			if(prev_chunk->newline) break;
	// 			if( skip_alnum && !isalnum(*(cursor->chunk->raw.str + cursor->start))){ break; }
	// 			if(!skip_alnum &&  isalnum(*(cursor->chunk->raw.str + cursor->start))){ break; }
	// 		}else{
	// 			break;
	// 		}
	// 	}
	// }else if(match_any(bind, binds.cursorUp, binds.selectUp)){/////////////////////////////////////////// Move/Select Up
	// 	if(cursor->chunk->node.prev == &root_chunk) return count;;
	// 	Cursor seek = *cursor;
	// 	u64 chars_moved = 0;
	// 	enum{ Finished, MoveLeftToPrevLine, MoveLeftToLineBegin, MoveRightToChar } state = MoveLeftToPrevLine;
	// 	while(state){
	// 		switch(state){
	// 			case MoveLeftToPrevLine:{ // move to the previous line and count how many chars it takes to get there
	// 				count += move_cursor(&seek, binds.cursorLeft);
	// 				if(seek.chunk != cursor->chunk && seek.chunk->newline) state = MoveLeftToLineBegin;
	// 				else chars_moved++;
	// 			}break;
	// 			case MoveLeftToLineBegin:{ // move to the beginning of the prev line so we can advance to column
	// 				if(seek.chunk->node.prev == &root_chunk || 
	// 					PrevTextChunk(seek.chunk)->newline){ 
	// 						state = MoveRightToChar; 
	// 						seek.start = 0; 
	// 				} 
	// 				else seek.chunk = PrevTextChunk(seek.chunk);
	// 			}break;
	// 			case MoveRightToChar:{ // advance to column
	// 				if(chars_moved--){ 
	// 					u64 moved = move_cursor(&seek, binds.cursorRight); 
	// 					if(!moved){ 
	// 						//if we reach the end of the line early out
	// 						state = Finished; 
	// 						seek.chunk = PrevTextChunk(seek.chunk);
	// 						seek.start = seek.chunk->raw.count;
	// 					} else count += moved;
	// 				} else state = Finished; 
	// 			}break;
	// 		}
	// 	}
	// 	*cursor = seek;
	// }else if(match_any(bind, binds.cursorDown, binds.selectDown)){///////////////////////////////////// Move/Select Down
	// 	if(cursor->chunk->node.next == &root_chunk) return count;
	// 	Cursor seek = *cursor;
	// 	u64 chars_moved = 0;
	// 	enum{ Finished, MoveLeftToLineBegin, MoveRightToNextLine, MoveRightToChar } state = MoveLeftToLineBegin;
	// 	while(state){
	// 		switch(state){
	// 			case MoveLeftToLineBegin:{ // find what coulmn we're on 
	// 				u64 moved = move_cursor(&seek, binds.cursorLeft);
	// 				if(moved) { 
	// 					chars_moved++; 
	// 					count += moved; 
	// 				}
	// 				else {
	// 					state = MoveRightToNextLine; 
	// 					seek = *cursor; //we set the cursor back here because we know that it's closer than seek 
	// 				}
	// 			}break;
	// 			case MoveRightToNextLine:{ // proceed to next line
	// 				if(seek.chunk->newline) {state = MoveRightToChar; seek.start = 0;} 
	// 				seek.chunk = NextTextChunk(seek.chunk);
	// 			}break;
	// 			case MoveRightToChar:{
	// 				if(chars_moved--){ 
	// 					u64 moved = move_cursor(&seek, binds.cursorRight); 
	// 					if(!moved){ 
	// 						//if we reach the end of the line early out
	// 						state = Finished; 
	// 						seek.chunk = PrevTextChunk(seek.chunk);
	// 						seek.start = seek.chunk->raw.count;
	// 					} else count += moved;
	// 				} else state = Finished; 
	// 			}break;
	// 		}
	// 	}
	// 	*cursor = seek;
	// }
	// return count;
	return count;
}

//TODO(sushi) do this (for fun)
void index_lines(){
	linelock.lock();
	Cursor cursor;
	
	if(!root_line_chunks.next){//initialize
		TextChunk* init = new_chunk();
		NodeInsertNext(&root_line_chunks, &init->node);
	}
	
	TextChunk* curline = TextChunkFromNode(root_line_chunks.next);
	TextChunk* line_start = TextChunkFromNode(root_chunk.next);
	for(Node* it = root_chunk.next; it != &root_chunk; it = it->next){
		
	}
	
	linelock.unlock();
	platform_sleep(100);
}


void text_insert(str8 text){DPZoneScoped;
	Arena* edit_arena = *edit_arenas.last;
	TextChunk* curchunk = main_cursor.chunk;
	
	if(main_cursor.chunk_start != curchunk->raw.count || curchunk != current_edit_chunk){
		if(main_cursor.chunk_start == curchunk->raw.count){
			TextChunk* prev = new_chunk();
			*prev = *curchunk;
			NodeInsertPrev(&curchunk->node, &prev->node);
		}else if(!main_cursor.chunk_start){
			TextChunk* next = new_chunk();
			*next = *curchunk;
			NodeInsertNext(&curchunk->node, &next->node);			
		}else{
			TextChunk* prev = new_chunk();
			TextChunk* next = new_chunk();
			prev->raw = {curchunk->raw.str, (s64)main_cursor.chunk_start};
			next->raw = {curchunk->raw.str + main_cursor.chunk_start, curchunk->raw.count - (s64)main_cursor.chunk_start};
			NodeInsertPrev(&curchunk->node, &prev->node);
			NodeInsertNext(&curchunk->node, &next->node);	
		}
	}
	
	
	
	
	while(text){
		DecodedCodepoint dc = str8_advance(&text);
		if(dc.codepoint == '\b'){
			
		}else if(dc.codepoint == 0x1b){}//do nothing on ESC
		else{
			
		}
		
	}
	// if(main_cursor.start != main_cursor.chunk->raw.count || main_cursor.chunk != current_edit_chunk){//we must branch a new chunk from the loaded file 
	// 	TextChunk* curchunk = main_cursor.chunk;
	// 	if(main_cursor.start == 0){
	// 		TextChunk* next = new_chunk();
	// 		memcpy(next, curchunk, sizeof(TextChunk));
	// 		NodeInsertNext(&curchunk->node,&next->node);
	
	// 		curchunk->newline = 0;
	// 	}
	// 	else if (main_cursor.start == main_cursor.chunk->raw.count){
	// 		TextChunk* prev = new_chunk();
	// 		memcpy(prev, curchunk, sizeof(TextChunk));
	// 		prev->newline = false;
	// 		NodeInsertPrev(&curchunk->node, &prev->node);
	// 	}
	// 	else { //split chunk if cursor is in the middle of it 
	// 		TextChunk* prev = new_chunk();
	// 		prev->raw = {curchunk->raw.str, (s64)main_cursor.start};
	// 		prev->bg = curchunk->bg;
	// 		prev->fg = curchunk->fg;
	// 		prev->offset = curchunk->offset;
	// 		prev->newline = 0;
	// 		TextChunk* next = new_chunk();
	// 		next->raw = {curchunk->raw.str + main_cursor.start, curchunk->raw.count - (s64)main_cursor.start};
	// 		next->bg = curchunk->bg;
	// 		next->fg = curchunk->fg;
	// 		next->offset = curchunk->offset + main_cursor.start;
	// 		next->newline = curchunk->newline;
	
	// 		NodeInsertPrev(&curchunk->node, &prev->node);
	// 		NodeInsertNext(&curchunk->node, &next->node);
	
	// 		curchunk->newline = 0;
	// 	}
	// 	current_edit_chunk = curchunk;
	// 	curchunk->raw = {edit_arena->cursor, 0};
	// 	main_cursor.start = 0;
	// }
	// if(edit_arena->used + text.count > edit_arena->size){
	// 	edit_arenas.add(memory_create_arena(Kilobytes(1)));
	// 	edit_arena = *edit_arenas.last;
	// }
	// memcpy(edit_arena->cursor, text.str, text.count);
	// main_cursor.start  += text.count;
	// edit_arena->cursor += text.count;
	// edit_arena->used   += text.count;
	// main_cursor.chunk->raw.count += text.count;
}

void text_delete_left(){DPZoneScoped;
	// Arena* edit_arena = *edit_arenas.last;
	// TextChunk* curchunk = main_cursor.chunk;
	// if(main_cursor.start != main_cursor.chunk->raw.count || main_cursor.chunk != current_edit_chunk){
	// 	if(main_cursor.start == 0){ //special case where cursor is at the beginning of a chunk
	// 		//move cursor into previous chunk and set it to the end so following if can handle it 
	// 		main_cursor.chunk = TextChunkFromNode(main_cursor.chunk->node.prev);
	// 		curchunk = main_cursor.chunk;
	// 		main_cursor.start = curchunk->raw.count;
	// 		if(curchunk->newline) curchunk->newline = 0;
	// 	}
	// 	if(main_cursor.start == curchunk->raw.count){
	// 		//copy old chunks memory into edit arena and repoint it
	// 		if(edit_arena->used + DeshInput->charCount > edit_arena->size){
	// 			edit_arenas.add(memory_create_arena(Kilobytes(1)));
	// 			edit_arena = *edit_arenas.last;
	// 		}
	// 		memcpy(edit_arena->cursor, curchunk->raw.str, curchunk->raw.count);
	// 		curchunk->raw.str = edit_arena->cursor;
	// 		edit_arena->cursor += curchunk->raw.count;
	// 		edit_arena->used += curchunk->raw.count;
	
	// 		current_edit_chunk = curchunk;
	// 	}
	// 	else{ //split chunk 
	// 		TextChunk* next = new_chunk();
	// 		next->raw = {curchunk->raw.str + main_cursor.start, curchunk->raw.count - (s64)main_cursor.start};
	// 		next->bg = curchunk->bg;
	// 		next->fg = curchunk->fg;
	// 		next->offset = curchunk->offset + main_cursor.start;
	// 		next->newline = curchunk->newline;
	
	// 		curchunk->raw.count = main_cursor.start;
	
	
	// 		NodeInsertNext(&curchunk->node, &next->node);
	
	// 		if(edit_arena->used + DeshInput->charCount > edit_arena->size){
	// 			edit_arenas.add(memory_create_arena(Kilobytes(1)));
	// 			edit_arena = *edit_arenas.last;
	// 		}
	// 		memcpy(edit_arena->cursor, curchunk->raw.str, curchunk->raw.count);
	// 		curchunk->raw.str = edit_arena->cursor;
	// 		edit_arena->cursor += curchunk->raw.count;
	// 		edit_arena->used += curchunk->raw.count;
	// 		curchunk->newline = 0;
	// 	}
	// }
	// u64 bytes_moved = move_cursor(&main_cursor, binds.cursorLeft);
	// edit_arena->cursor  -= bytes_moved;
	// edit_arena->used    -= bytes_moved;
	// curchunk->raw.count -= bytes_moved;
	// if(!curchunk->raw.count){
	// 	//if we completely remove a node we move the cursor into the previous chunk and remove the current chunk node
	// 	//TODO(sushi) possible optimization is tracking removed nodes and using them before making new ones
	// 	TextChunk* prev = TextChunkFromNode(main_cursor.chunk->node.prev);
	// 	main_cursor.chunk = prev;
	// 	main_cursor.start = prev->raw.count;
	// 	NodeRemove(&curchunk->node);
	// }
	
}

void text_delete_right(){DPZoneScoped;
	// Arena* edit_arena = *edit_arenas.last;
	// TextChunk* curchunk = main_cursor.chunk;
	// if(main_cursor.start != 0 || main_cursor.chunk != current_edit_chunk){
	// 	if(main_cursor.start == curchunk->raw.count){//
	// 		main_cursor.chunk->newline = 0;
	// 		return;
	// 	}
	// 	else if(main_cursor.start == 0){ 
	// 		//copy old chunks memory into edit arena and repoint it
	// 		if(edit_arena->used + DeshInput->charCount > edit_arena->size){
	// 			edit_arenas.add(memory_create_arena(Kilobytes(1)));
	// 			edit_arena = *edit_arenas.last;
	// 		}
	// 		memcpy(edit_arena->cursor, curchunk->raw.str, curchunk->raw.count);
	// 		curchunk->raw.str = edit_arena->cursor;
	// 		edit_arena->cursor += curchunk->raw.count;
	// 		edit_arena->used += curchunk->raw.count;
	
	// 		current_edit_chunk = curchunk;
	// 	}
	// 	else{ //split chunk 
	// 		TextChunk* prev = new_chunk();
	// 		prev->raw = {curchunk->raw.str, (s64)main_cursor.start};
	// 		prev->bg = curchunk->bg;
	// 		prev->fg = curchunk->fg;
	// 		prev->offset = curchunk->offset;
	// 		prev->newline = 0;
	
	// 		curchunk->raw.str += main_cursor.start;
	// 		curchunk->raw.count = curchunk->raw.count - main_cursor.start;
	
	// 		main_cursor.start = 0;
	
	// 		NodeInsertPrev(&curchunk->node, &prev->node);
	
	// 		if(edit_arena->used + DeshInput->charCount > edit_arena->size){
	// 			edit_arenas.add(memory_create_arena(Kilobytes(1)));
	// 			edit_arena = *edit_arenas.last;
	// 		}
	// 		memcpy(edit_arena->cursor, curchunk->raw.str, curchunk->raw.count);
	// 		curchunk->raw.str = edit_arena->cursor;
	// 		edit_arena->cursor += curchunk->raw.count;
	// 		edit_arena->used += curchunk->raw.count;
	// 	}
	// }
	// str8_advance(&curchunk->raw);
	// if(!curchunk->raw.count){
	// 	//if we completely delete a chunk we move the cursor to the next one and remove the empty chunk
	// 	//TODO(sushi) possible optimization is tracking removed nodes and using them before making new ones
	// 	TextChunk* next = TextChunkFromNode(main_cursor.chunk->node.next);
	// 	main_cursor.chunk = next;
	// 	main_cursor.start = 0;
	// 	NodeRemove(&curchunk->node);
	// }
}

//stitches together the edits into the static arena then flushes it to the file
//TODO(sushi) optimize this by joining chunks 
void save_buffer(){DPZoneScoped;
	// //temp weak approximation of growth
	// //this should be better tracked later in editing functions
	// u64 growth = 0;
	// for(auto ea : edit_arenas){
	// 	growth += ea->used;
	// }
	// Arena* stitched = memory_create_arena(static_arena->size + growth);
	// for(Node* it = root_chunk.next; it != &root_chunk; it = it->next){
	// 	TextChunk* chunk = TextChunkFromNode(it);
	// 	memcpy(stitched->cursor, chunk->raw.str, chunk->raw.count);
	// 	//repoint chunk to stitched memory
	// 	chunk->raw.str = stitched->cursor;
	// 	stitched->cursor += chunk->raw.count;
	// 	stitched->used += chunk->raw.count;
	// 	if(chunk->newline){ //TODO(sushi) handle other line endings i guess
	// 		memset(stitched->cursor, '\n', 1);
	// 		stitched->cursor++;
	// 		stitched->used++;
	// 	}
	// }
	
	// //truncate file
	// file_change_access(file, FileAccess_ReadWriteTruncate);
	
	// //flush file 
	// file_write(file, stitched->start, stitched->used);
	
	// //replace old static arena with new
	// memory_delete_arena(static_arena);
	// static_arena = stitched;
}

void draw_character(u32 character, vec2 scale, color col, vec2* cursor){DPZoneScoped;
	Vertex2         vp[4];
	RenderTwodIndex ip[6];
	if(character == U'\n'){ //NOTE(sushi) \r is skipped outside of this function
		if(config.show_symbol_eol){
			cursor->x += 4;
			if(buffer_CRLF){
				vec2 slashr_size = CalcTextSize(str8l("\\r"));
				render_quad_filled2(*cursor, slashr_size, config.text_color);
				render_text2(config.font, str8l("\\r"), *cursor, scale, config.buffer_color);
				cursor->x += slashr_size.x;
			}
			vec2 slashn_size = CalcTextSize(str8l("\\n"));
			render_quad_filled2(*cursor, slashn_size, config.text_color);
			render_text2(config.font, str8l("\\n"), *cursor, scale, config.buffer_color);
			cursor->x += slashn_size.x;
		}
		cursor->y += config.font->max_height * scale.y;
		cursor->x = config.buffer_margin.x + config.buffer_padding.x;
		return;
	}
	switch(config.font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			f32 w  = config.font->max_width  * scale.x;
			f32 h  = config.font->max_height * scale.y;
			f32 dy = 1.f / (f32)config.font->count;
			
			//handle special characters
			if(character == U'\t'){
				if(config.show_symbol_whitespace){
					render_quad_filled2({cursor->x + w/2.f - 1, cursor->y + h/2.f - 1},
										vec2{(w * (config.tab_width-1)),2}, config.text_color/2.f);
				}
				
				cursor->x += w * config.tab_width;
			}else{
				f32 topoff = (dy * (f32)(character - 32)) + config.font->uv_yoffset;
				f32 botoff = topoff + dy;
				ip[0] = 0; ip[1] = 1; ip[2] = 2; ip[3] = 0; ip[4] = 2; ip[5] = 3;
				vp[0].pos = { cursor->x + 0,cursor->y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = col.rgba; //top left
				vp[1].pos = { cursor->x + w,cursor->y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = col.rgba; //top right
				vp[2].pos = { cursor->x + w,cursor->y + h }; vp[2].uv = { 1,botoff }; vp[2].color = col.rgba; //bot right
				vp[3].pos = { cursor->x + 0,cursor->y + h }; vp[3].uv = { 0,botoff }; vp[3].color = col.rgba; //bot left
				render_add_vertices2(render_active_layer(), vp, 4, ip, 6);
				cursor->x += w;
				
				if(config.show_symbol_whitespace && character == U' '){
					render_quad_filled2({cursor->x - w/2.f - 1, cursor->y + h/2.f - 1}, vec2{2,2}, config.text_color/2.f);
				}
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			//handle special characters
			if(character == U'\t'){
				packed_char* pc = font_packed_char(config.font, U' ');
				f32 w = pc->xadvance * scale.x;
				f32 h = config.font_height * scale.y;
				
				if(config.show_symbol_whitespace){
					render_quad_filled2({cursor->x + w/2.f - 1, cursor->y + h/2.f - 1},
										vec2{(w * (config.tab_width-1)),2}, config.text_color/2.f);
				}
				
				cursor->x += w * config.tab_width;
			}else{
				aligned_quad q = font_aligned_quad(config.font, character, cursor, scale);
				ip[0] = 0; ip[1] = 1; ip[2] = 2; ip[3] = 0; ip[4] = 2; ip[5] = 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = col.rgba; //top left
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = col.rgba; //top right
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = col.rgba; //bot right
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = col.rgba; //bot left
				render_add_vertices2(render_active_layer(), vp, 4, ip, 6);
				
				if(config.show_symbol_whitespace && character == U' '){
					packed_char* pc = font_packed_char(config.font, U' ');
					f32 w = pc->xadvance * scale.x;
					f32 h = config.font_height * scale.y;
					render_quad_filled2({cursor->x - w/2.f - 1, cursor->y + h/2.f - 1}, vec2{2,2}, config.text_color/2.f);
				}
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
}

void update_editor(){DPZoneScoped;
	//if(file->path == 0) DebugBreakpoint;
	//-////////////////////////////////////////////////////////////////////////////////////////////
	//// input
	
	//// repeat timer ////
	persist Stopwatch repeat_hold = start_stopwatch();
	persist Stopwatch repeat_throttle = start_stopwatch();
	b32 repeat = 0;
	if(any_key_pressed() || any_key_released()){
		reset_stopwatch(&repeat_hold);
	}
	if((peek_stopwatch(repeat_hold) > config.repeat_hold) && (peek_stopwatch(repeat_throttle) > config.repeat_throttle)){
		reset_stopwatch(&repeat_throttle);
		repeat = 1;
	}
#define CanDoInput(x) (key_pressed(x) || key_down(x) && repeat)
	
	//// cursor movement ////
	if(CanDoInput(binds.cursorLeft))          { move_cursor(&main_cursor, binds.cursorLeft);}
	if(CanDoInput(binds.cursorWordLeft))      { move_cursor(&main_cursor, binds.cursorWordLeft); }
	if(CanDoInput(binds.cursorWordPartLeft))  { move_cursor(&main_cursor, binds.cursorWordPartLeft); }
	if(CanDoInput(binds.cursorRight))         { move_cursor(&main_cursor, binds.cursorRight); }
	if(CanDoInput(binds.cursorWordRight))     { move_cursor(&main_cursor, binds.cursorWordRight); }
	if(CanDoInput(binds.cursorWordPartRight)) { move_cursor(&main_cursor, binds.cursorWordPartRight); }
	if(CanDoInput(binds.cursorUp))            { move_cursor(&main_cursor, binds.cursorUp); }
	if(CanDoInput(binds.cursorDown))          { move_cursor(&main_cursor, binds.cursorDown); }
	
	//// text input ////
	if(DeshInput->charCount){ 
        text_insert({DeshInput->charIn, (s64)DeshInput->charCount});
	}
	//if(key_pressed(Key_ENTER | InputMod_None)){ text_insert(STR8("\n"));  }
	
	//// text deletion ////
	//need to manually support delete on windows
	if(CanDoInput(binds.deleteRight)) { text_delete_right(); }
	
	if(key_pressed(binds.saveBuffer)){ save_buffer(); }
	if(key_pressed(binds.reloadConfig)){ load_config(); }
	
	
	//-////////////////////////////////////////////////////////////////////////////////////////////
	//// render
	render_start_cmd2(0, 0, vec2::ZERO, DeshWindow->dimensions);
    //buffer background
    render_quad_filled2(config.buffer_margin, DeshWindow->dimensions-config.buffer_margin*2, config.buffer_color);
    //buffer outline
    render_quad2(config.buffer_margin, DeshWindow->dimensions-config.buffer_margin*2, color(200,200,200));
	
    //text
    vec2  visual_cursor = config.buffer_margin + config.buffer_padding;
    vec2  text_space = DeshWindow->dimensions - (config.buffer_margin + config.buffer_padding) * 2;
    vec2  text_scale = vec2::ONE * config.font_height / (f32)config.font->max_height;
	Line* line = LineFromNode(root_line.next);
    
    render_start_cmd2(0, config.font->tex, config.buffer_padding, text_space);
	for(Node* it = root_chunk.next; it != &root_chunk; it = it->next){
        TextChunk* chunk = TextChunkFromNode(it);
		vec2 size = CalcTextSize(chunk->raw); //maybe this can be cached?
		
        //dont render anything if it goes beyond buffer width
        //if(visual_cursor.x > text_space.x){ 
		//	visual_cursor.x = config.buffer_margin.x + config.buffer_padding.x;
		//	visual_cursor.y += size.y;
		//	continue;
		//}
        //TODO(sushi) word wrapping
        if(config.word_wrap) NotImplemented;
        else{
			//draw chunk text
			str8 text = chunk->raw;
			u64 advanced = 0;
			while(text){
				vec2 prevcur = visual_cursor;
				DecodedCodepoint decoded = str8_advance(&text);
				if(decoded.codepoint == '\r'){ decoded = str8_advance(&text); advanced++; }
				draw_character(decoded.codepoint, text_scale, config.text_color, &visual_cursor);	
				//--------------------------------------------------------------------------------------- Draw Cursor//
				if(main_cursor.chunk_start == (decoded.codepoint == '\n' && buffer_CRLF ? advanced-1 : advanced)){
					persist Stopwatch cursor_blink = start_stopwatch();
					if(DeshInput->anyKeyDown) reset_stopwatch(&cursor_blink);
					color cursor_color = config.cursor_color;
					f32 mult = M_PI*peek_stopwatch(cursor_blink)/config.cursor_pulse_duration;
					cursor_color.a = (u8)(255*(sin(mult + M_HALFPI + cos(mult + M_HALFPI))+1)/2);
					switch(config.cursor_shape){
						case CursorShape_VerticalLine:{
							vec2 cursor_top = vec2(prevcur.x, prevcur.y);
							vec2 cursor_bot = vec2(prevcur.x, prevcur.y + config.font_height);
							render_line2(cursor_top, cursor_bot, cursor_color);
						}break;
						case CursorShape_VerticalLineThick:{
							vec2 cursor_top = vec2(prevcur.x, prevcur.y);
							vec2 cursor_bot = vec2(cursor_top.x,           cursor_top.y + config.font_height);
							render_line_thick2(cursor_top, cursor_bot, 2, cursor_color);
						}break;
						case CursorShape_Underline:{
							f32 x_width = visual_cursor.x - prevcur.x;
							vec2 cursor_left  = vec2(prevcur.x,           prevcur.y + config.font_height);
							vec2 cursor_right = vec2(prevcur.x + x_width, prevcur.y + config.font_height);
							render_line2(cursor_left, cursor_right, cursor_color);
						}break;
						case CursorShape_Rectangle:{
							f32 x_width = visual_cursor.x - prevcur.x;
							vec2 cursor_top_left  = vec2(prevcur.x, prevcur.y);
							render_quad2(cursor_top_left, vec2(x_width, config.font_height), cursor_color);
						}break;
						case CursorShape_FilledRectangle:{
							str8 right_char = str8_eat_one(str8{line->raw.str+main_cursor.line_start, (s64)(line->raw.count-main_cursor.line_start)});
							f32 x_width = CalcTextSize(right_char).x;
							vec2 cursor_top_left = vec2(prevcur.x, prevcur.y);
							render_quad_filled2(cursor_top_left, vec2(x_width, config.font_height), cursor_color);
							render_text2(config.font, right_char, cursor_top_left, text_scale, Color_Black);
						}break;
					}
				}
				
				
				advanced += decoded.advance;
				if(decoded.codepoint == '\n') line = NextLine(line); 
				if(visual_cursor.y > text_space.y){ break; }
			} 
			
#if 0 //DEBUG
			//left vertical line
			render_line2(chunk_pos + size.yComp(), chunk_pos + (size.yComp()/2.f), (main_cursor.chunk == chunk ? Color_Cyan : Color_Red));
			
			//right vertical line
			render_line2(chunk_pos + size, chunk_pos + vec2(size.x, size.y/2.f), (main_cursor.chunk == chunk ? Color_Cyan : Color_Red));
			
			//right vertical newline
			if(chunk->newline){
				render_line2(chunk_pos + size.xComp(), chunk_pos + vec2(size.x, size.y/2.f), Color_Yellow);
			}
			
			//bottom line
			render_line2(chunk_pos + size.yComp(), chunk_pos + size, (main_cursor.chunk == chunk ? Color_Cyan : Color_Red));
#endif	
			render_start_cmd2(5, config.font->tex, vec2::ZERO, DeshWindow->dimensions);
			string fps = toStr(1000/DeshTime->deltaTime);
			render_text2(config.font, {(u8*)fps.str,fps.count}, vec2::ZERO, vec2::ONE, Color_White);
			
        }
    }
}