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
    Bind_Cursor_Left         = Key_LEFT | InputMod_None,
	Bind_Cursor_WordLeft     = Key_LEFT | InputMod_AnyCtrl,
	Bind_Cursor_WordPartLeft = Key_LEFT | InputMod_AnyAlt,
    Bind_Cursor_Right         = Key_RIGHT | InputMod_None,
    Bind_Cursor_WordRight     = Key_RIGHT | InputMod_AnyCtrl,
    Bind_Cursor_WordPartRight = Key_RIGHT | InputMod_AnyAlt,
    
	Bind_Cursor_Up    = Key_UP    | InputMod_None,
    Bind_Cursor_Down  = Key_DOWN  | InputMod_None,
	
    Bind_Cursor_Anchor = Key_SPACE | InputMod_AnyCtrl,
	
    Bind_Select_Left  = Key_LEFT  | InputMod_AnyShift,
    Bind_Select_Right = Key_RIGHT | InputMod_AnyShift,
    Bind_Select_Up    = Key_UP    | InputMod_AnyShift,
    Bind_Select_Down  = Key_DOWN  | InputMod_AnyShift,

    Bind_Save_Buffer = Key_S | InputMod_AnyCtrl,
};
