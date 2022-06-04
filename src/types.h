#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/color.h"
#include "core/input.h"

enum{
	CursorShape_VerticalLine,
	CursorShape_VerticalLineThick,
	CursorShape_Underline,
	CursorShape_Rectangle,
	CursorShape_FilledRectangle,
};

struct Config{
	//// cursor config ////
	color cursor_color;
	b32   cursor_pulse; //whether the cursor transitions from transparent to visible
	f32   cursor_pulse_duration; //full cycle time from visible to transparent to visisble in ms
	Type  cursor_shape;
	
    color buffer_color;
	color text_color;
    vec2  buffer_margin;
    vec2  buffer_padding;
	
	u32 tab_width;
    b32 word_wrap;
	b32 show_symbol_whitespace; //spaces and tabs
	b32 show_symbol_eol; //carraige return and newline
	b32 show_symbol_wordwrap; //word wrap symbol
	
    u32   font_height; //visual height of font when rendered
    Font* font;
};

struct TextChunk{
	Node  node;
	str8  raw;
	u64   offset; //byte offset from beginning of file
    //u64   count;  //codepoint count
	color bg; 
	color fg; 
	b32   newline;
};
#define TextChunkFromNode(x) CastFromMember(TextChunk, node, x)

struct Cursor{
	TextChunk* chunk;
	u64 start; //byte index into chunk
	u64 count; //byte selection size
};

typedef KeyCode Bind; enum{
    Bind_CursorLeft         = Key_LEFT | InputMod_None,
	Bind_CursorWordLeft     = Key_LEFT | InputMod_AnyCtrl,
	Bind_CursorWordPartLeft = Key_LEFT | InputMod_AnyAlt,
    
	Bind_CursorRight         = Key_RIGHT | InputMod_None,
    Bind_CursorWordRight     = Key_RIGHT | InputMod_AnyCtrl,
    Bind_CursorWordPartRight = Key_RIGHT | InputMod_AnyAlt,
    
	Bind_CursorUp    = Key_UP    | InputMod_None,
    Bind_CursorDown  = Key_DOWN  | InputMod_None,
	
    Bind_CursorAnchor = Key_SPACE | InputMod_AnyCtrl,
	
    Bind_SelectLeft  = Key_LEFT  | InputMod_AnyShift,
    Bind_SelectRight = Key_RIGHT | InputMod_AnyShift,
    Bind_SelectUp    = Key_UP    | InputMod_AnyShift,
    Bind_SelectDown  = Key_DOWN  | InputMod_AnyShift,
	
	Bind_DeleteLeft         = Key_BACKSPACE | InputMod_None,
	Bind_DeleteWordLeft     = Key_BACKSPACE | InputMod_AnyCtrl,
	Bind_DeleteWordPartLeft = Key_BACKSPACE | InputMod_AnyAlt,
	
	Bind_DeleteRight         = Key_DELETE | InputMod_None,
	Bind_DeleteWordRight     = Key_DELETE | InputMod_AnyCtrl,
	Bind_DeleteWordPartRight = Key_DELETE | InputMod_AnyAlt,
	
    Bind_SaveBuffer = Key_S | InputMod_AnyCtrl,
};
