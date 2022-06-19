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

//NOTE(sushi) i define this locally for my IDE so the unity build issue doesnt happen, dont actually define this in a build
#ifdef FIX_MY_IDE_PLEASE
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/config.h"
#include "core/graphing.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

//// text editor includes ////
#include "types.h"
#endif

Cursor main_cursor;
array<Cursor> extra_cursors;

//NOTE(sushi) add this to Buffer struct when that is added
b32 buffer_CRLF;
u32 line_end_char; //either \n or \r, so we dont have to keep checking what buffer_CRLF is everywhere
u32 line_end_length; //either 1 or 2, so we dont have to keep checking what buffer_CRLF is everywhere

Buffer* buffer;

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

}

//loads a file and creates one TextChunk encompassing the entire file
//and caches line info
void load_file(str8 filepath){DPZoneScoped;
	file = file_init(filepath, FileAccess_ReadWrite);
	if(!file){ Assert(false); return; }
	
	//eventually put these in an array or arena or something crazy like that
	buffer = (Buffer*)memalloc(sizeof(Buffer));

	buffer->capacity = RoundUpTo(file->bytes*2, Kilobytes(4));
	buffer->upperbloc.count = (file->bytes/2);
	buffer->lowerbloc.count = (file->bytes - buffer->upperbloc.count);
	buffer->gap_size = buffer->capacity - file->bytes;
	buffer->upperbloc.str = (u8*)memalloc(buffer->capacity);
	buffer->lowerbloc.str = buffer->upperbloc.str + buffer->upperbloc.count + buffer->gap_size;
	
	file_read(file,buffer->upperbloc.str,file->bytes);

	memmove(buffer->lowerbloc.str,buffer->upperbloc.str+buffer->upperbloc.count,buffer->lowerbloc.count);

	//doesnt work well because it reads a \0 at the end
	//buffer->upperbloc = file_read(file,buffer->upperbloc.str,buffer->upperbloc.count);
	//file_cursor(file, buffer->upperbloc.count);
	//buffer->lowerbloc = file_read(file,buffer->upperbloc.str+buffer->upperbloc.count+buffer->gap_size,buffer->lowerbloc.count);


	buffer->line_starts = (u64*)memalloc(file->bytes);
	buffer->line_starts[0] = 0;
	u64 line_idx = 1;
	u64 arr_idx = 0;
	while(arr_idx < buffer->lowerbloc.count + buffer->upperbloc.count){
		DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, arr_idx), 4);
		if(dc.codepoint==U'\r'){
			buffer->line_starts[line_idx++] = GapToUserSpace(buffer, arr_idx) + 2;
			line_end_char = U'\r';
			line_end_length = 2;
			dc.advance = 2;
		}
		else if(dc.codepoint==U'\n'){ 
			buffer->line_starts[line_idx++] = GapToUserSpace(buffer, arr_idx) + 1;
			line_end_char = U'\n';
			line_end_length = 1;
		}
		arr_idx+=dc.advance;
	}
	buffer->line_starts_count = line_idx;
	buffer->line_starts = (u64*)memrealloc(buffer->line_starts, line_idx*sizeof(u64));
}


void init_editor(){DPZoneScoped;
	extra_cursors = array<Cursor>(deshi_allocator);
	
	main_cursor.pos = 0;
	main_cursor.count = 0;
	main_cursor.line_idx = 0;
	
	load_config();
	
	init_editor_commands();
	
    //load_file(STR8(__FILE__));
	load_file(STR8("src/test.txt"));
}

//returns the number of bytes the cursor moved
u64 move_cursor(Cursor* cursor, KeyCode bind){DPZoneScoped;
	Cursor init = *cursor;
	if      (match_any(bind, binds.cursorRight, binds.selectRight)){/////////////////////////////////// Move/Select Right
		if(cursor->pos==buffer->capacity-buffer->gap_size-line_end_length+1) return 0; //end of file
		DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, cursor->pos), 4);
		if(dc.codepoint == line_end_char){
			dc.advance = line_end_length;
			cursor->line_idx++;
		}
		cursor->pos += dc.advance;
	}else if(match_any(bind, binds.cursorLeft, binds.selectLeft)){///////////////////////////////////// Move/Select Left /////
		if(!cursor->pos) return 0; //beginning of file
		while(utf8_continuation_byte(*UserToMemSpace(buffer, cursor->pos))) cursor->pos--;
		cursor->pos--;
		DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, cursor->pos), 4);
		if(dc.codepoint == U'\n'){
			dc.advance = line_end_length;
			if(dc.advance==2)cursor->pos--;
			cursor->line_idx--;
		}
	}else if(match_any(bind, binds.cursorWordRight, binds.selectWordRight)){
		u32 punc_codepoint;
		DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, cursor->pos), 4);
		if(!isalnum(dc.codepoint)) { punc_codepoint = dc.codepoint; }
		else                       { punc_codepoint = 0; }
		while(1){
 			if(!move_cursor(cursor, binds.cursorRight)) break; //end of file
			DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, cursor->pos), 4);
			if      ( punc_codepoint && dc.codepoint != punc_codepoint) break;
			else if (!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_' ) break;
		}
	}else if(match_any(bind, binds.cursorWordLeft, binds.selectWordLeft)){
		u32 punc_codepoint;
		b32 moved = move_cursor(cursor, binds.cursorLeft);
		DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, cursor->pos), 4);
		if(!isalnum(dc.codepoint)) { punc_codepoint = dc.codepoint; }
		else                       { punc_codepoint = 0; }
		while(moved){
			if(!move_cursor(cursor, binds.cursorLeft)) break; //beginning of file
			DecodedCodepoint dc = decoded_codepoint_from_utf8(UserToMemSpace(buffer, cursor->pos), 4);
			if     ( punc_codepoint && dc.codepoint != punc_codepoint) break;
			else if(!punc_codepoint && !isalnum(dc.codepoint) && dc.codepoint != U'_') break;
		}
		if(moved) move_cursor(cursor, binds.cursorRight);
	}else if(match_any(bind, binds.cursorUp, binds.selectUp)){
		if(!cursor->line_idx) return 0; //first line
		//if cursor's position is greater than the prev line length
		if(LinePosition(buffer, cursor) >= LineLength(buffer, cursor->line_idx-1)){
			cursor->pos -= LinePosition(buffer,cursor)+line_end_length;
			cursor->line_idx--;
		}
		u32 codepoints = 0;
		//scan left and see how many codepoints we pass
		while(cursor->pos!=buffer->line_starts[cursor->line_idx]){
			move_cursor(cursor, binds.cursorLeft);
			codepoints++;
		}
		//set to prev line start
		cursor->line_idx--;
		cursor->pos = buffer->line_starts[cursor->line_idx];
		//scan right till we move as many codepoints 
		while(codepoints--){
			move_cursor(cursor, binds.cursorRight);
		}
	}else if(match_any(bind, binds.cursorDown, binds.selectDown)){
		if(cursor->line_idx==buffer->line_starts[buffer->line_starts_count-1]) return 0; //last line
		//if cursor position is greater than the next line's length
		if(LinePosition(buffer, cursor) >= LineLength(buffer, cursor->line_idx+1)){
			//there is an edge case where if the next line's length is equal to our line end length we have to put it at the beginning of the line
			//so it doesnt move to the next
			u32 nll = LineLength(buffer,cursor->line_idx+1);
			cursor->line_idx++;
			cursor->pos = buffer->line_starts[cursor->line_idx] + (nll==line_end_length?0:nll);//(LineLength(buffer,cursor->line_idx)-LinePosition(buffer,cursor))+(nll==1?0:nll);
		}else{
			u32 codepoints = 0;
			//scan left and see how many codepoints we pass
			while(cursor->pos!=buffer->line_starts[cursor->line_idx]){
				move_cursor(cursor, binds.cursorLeft);
				codepoints++;
			}
			//set to next line start
			cursor->line_idx++;
			cursor->pos = buffer->line_starts[cursor->line_idx];
			while(codepoints--){
				move_cursor(cursor, binds.cursorRight);
			}
		}
	}
	return abs(init.pos - cursor->pos);
}

void text_insert(str8 text){DPZoneScoped;
	if(main_cursor.pos < buffer->upperbloc.count){
		//cursor is before the gap  
		u64 movesize = buffer->upperbloc.count - main_cursor.pos;
		buffer->lowerbloc.str -= movesize;
		buffer->upperbloc.count -= movesize;
		buffer->lowerbloc.count += movesize;
		memmove(buffer->lowerbloc.str, buffer->upperbloc.str + buffer->upperbloc.count, movesize);
		
	}else if(main_cursor.pos > buffer->upperbloc.count){
		//cursor is after the gap
		u64 movesize = main_cursor.pos - buffer->upperbloc.count;
		memmove(buffer->upperbloc.str+buffer->upperbloc.count, buffer->lowerbloc.str, movesize);
		buffer->lowerbloc.str += movesize;
		buffer->upperbloc.count += movesize;
		buffer->lowerbloc.count -= movesize;
	}
	

	memcpy(buffer->upperbloc.str+buffer->upperbloc.count, text.str, text.count);
	buffer->upperbloc.count+=text.count;
	main_cursor.pos = buffer->upperbloc.count;

	//adjust line starts if we arent on the last line
	//oh brother this guy reeks
	//TODO(sushi) maybe there is a better way to handle this?
	forI(buffer->line_starts_count - main_cursor.line_idx){
		buffer->line_starts[i] += text.count;
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
	// Arena* stitched = memory_create_arena(buffer_arena->size + growth);
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
	// memory_delete_arena(buffer_arena);
	// buffer_arena = stitched;
}

void draw_character(u32 character, vec2 scale, color col, vec2* cursor){DPZoneScoped;
	Vertex2         vp[4];
	RenderTwodIndex ip[6];
	if(character == U'\r') return;
	if(character == U'\n'){ 
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
	str8b out; str8_builder_init(&out, STR8(""), deshi_temp_allocator);
	forI(buffer->upperbloc.count + buffer->lowerbloc.count){
		str8_builder_append(&out, str8{&buffer->upperbloc.str[UserToGapSpace(buffer, i)], 1});
	}
	
	//// repeat timer ////
	persist Stopwatch repeat_hold = start_stopwatch();
	persist Stopwatch repeat_throttle = start_stopwatch();
	b32 repeat = 0;
	if(any_key_pressed() || any_key_released()){
		reset_stopwatch(&repeat_hold);
	}
	else if((peek_stopwatch(repeat_hold) > config.repeat_hold) && (peek_stopwatch(repeat_throttle) > config.repeat_throttle)){
		reset_stopwatch(&repeat_throttle);
		repeat = 1;
	}
	// Log("", peek_stopwatch(repeat_hold));
	// Log("", peek_stopwatch(repeat_throttle));


#define CanDoInput(x) (key_pressed(x) || key_down(x) && repeat)
	
	//// cursor movement ////
	static u64 last_move_count = 0; //debug
	if(CanDoInput(binds.cursorLeft))          { last_move_count = move_cursor(&main_cursor, binds.cursorLeft);}
	if(CanDoInput(binds.cursorWordLeft))      { last_move_count = move_cursor(&main_cursor, binds.cursorWordLeft); }
	if(CanDoInput(binds.cursorWordPartLeft))  { last_move_count = move_cursor(&main_cursor, binds.cursorWordPartLeft); }
	if(CanDoInput(binds.cursorRight))         { last_move_count = move_cursor(&main_cursor, binds.cursorRight); }
	if(CanDoInput(binds.cursorWordRight))     { last_move_count = move_cursor(&main_cursor, binds.cursorWordRight); }
	if(CanDoInput(binds.cursorWordPartRight)) { last_move_count = move_cursor(&main_cursor, binds.cursorWordPartRight); }
	if(CanDoInput(binds.cursorUp))            { last_move_count = move_cursor(&main_cursor, binds.cursorUp); }
	if(CanDoInput(binds.cursorDown))          { last_move_count = move_cursor(&main_cursor, binds.cursorDown); }
	
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

	//render char at cursor
	render_start_cmd2(0, config.font->tex, vec2::ZERO, DeshWindow->dimensions);
	vec2 cursor = config.buffer_padding + config.buffer_margin;
	str8 scan = buffer->upperbloc;
	b32 passed = 0;
	u64 pos = 0;
	while(scan){
		vec2 prevpos = cursor;
		DecodedCodepoint dc = str8_advance(&scan);
		draw_character(dc.codepoint, vec2::ONE, Color_White, &cursor);
		if(main_cursor.pos == pos){
			render_line2(vec2(prevpos.x,prevpos.y), vec2(prevpos.x,prevpos.y+config.font_height),Color_White);

		}
		pos+=dc.advance;
		if(!passed&&!scan) scan = buffer->lowerbloc, passed = 1;
	}

	{//cursor debug
		render_start_cmd2(1, config.font->tex, vec2::ZERO, DeshWindow->dimensions);
		str8 info=toStr8(
			"cursor","\n",
			"------", "\n",
			"pos:     ", main_cursor.pos,"\n",
			"count:   ", main_cursor.count,"\n",
			"line:    ", main_cursor.line_idx, "\n",
			"linelen: ", LineLength(buffer, main_cursor.line_idx), "\n",
			"linepos: ", LinePosition(buffer, &main_cursor), "\n",
			"move:    ", last_move_count
		).fin;
		vec2 size = font_visual_size(config.font, info);
		vec2 pos = DeshWindow->dimensions - size;
		render_quad_filled2(pos, size, Color_Black);
		while(info){
			str8 line = str8_eat_until(info, '\n');
			render_text2(config.font, line, pos, vec2::ONE, Color_White);
			info = str8{line.str+line.count+1, info.count - (line.count+1)};
			pos.y += config.font->max_height;
		}
	}
}



// 	//--------------------------------------------------------------------------------------- Draw Cursor//
			// 	if(main_cursor.chunk_start == (decoded.codepoint == '\n' && buffer_CRLF ? advanced-1 : advanced)){
			// 		persist Stopwatch cursor_blink = start_stopwatch();
			// 		if(DeshInput->anyKeyDown) reset_stopwatch(&cursor_blink);
			// 		color cursor_color = config.cursor_color;
			// 		f32 mult = M_PI*peek_stopwatch(cursor_blink)/config.cursor_pulse_duration;
			// 		cursor_color.a = (u8)(255*(sin(mult + M_HALFPI + cos(mult + M_HALFPI))+1)/2);
			// 		switch(config.cursor_shape){
			// 			case CursorShape_VerticalLine:{
			// 				vec2 cursor_top = vec2(prevcur.x, prevcur.y);
			// 				vec2 cursor_bot = vec2(prevcur.x, prevcur.y + config.font_height);
			// 				render_line2(cursor_top, cursor_bot, cursor_color);
			// 			}break;
			// 			case CursorShape_VerticalLineThick:{
			// 				vec2 cursor_top = vec2(prevcur.x, prevcur.y);
			// 				vec2 cursor_bot = vec2(cursor_top.x,           cursor_top.y + config.font_height);
			// 				render_line_thick2(cursor_top, cursor_bot, 2, cursor_color);
			// 			}break;
			// 			case CursorShape_Underline:{
			// 				f32 x_width = visual_cursor.x - prevcur.x;
			// 				vec2 cursor_left  = vec2(prevcur.x,           prevcur.y + config.font_height);
			// 				vec2 cursor_right = vec2(prevcur.x + x_width, prevcur.y + config.font_height);
			// 				render_line2(cursor_left, cursor_right, cursor_color);
			// 			}break;
			// 			case CursorShape_Rectangle:{
			// 				f32 x_width = visual_cursor.x - prevcur.x;
			// 				vec2 cursor_top_left  = vec2(prevcur.x, prevcur.y);
			// 				render_quad2(cursor_top_left, vec2(x_width, config.font_height), cursor_color);
			// 			}break;
			// 			case CursorShape_FilledRectangle:{
			// 				str8 right_char = str8_eat_one(str8{line->raw.str+main_cursor.line_start, (s64)(line->raw.count-main_cursor.line_start)});
			// 				f32 x_width = CalcTextSize(right_char).x;
			// 				vec2 cursor_top_left = vec2(prevcur.x, prevcur.y);
			// 				render_quad_filled2(cursor_top_left, vec2(x_width, config.font_height), cursor_color);
			// 				render_text2(config.font, right_char, cursor_top_left, text_scale, Color_Black);
			// 			}break;
			// 		}
			// 	}
				