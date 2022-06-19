#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/color.h"
#include "core/input.h"


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
	
	f32 repeat_hold;     //ms time to wait before allowing an input to repeat
	f32 repeat_throttle; //ms time between repeating a held input
	
    u32   font_height; //visual height of font when rendered
    Font* font;
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
    Bind_SelectWordLeft  = Key_LEFT  | InputMod_AnyShift | InputMod_AnyCtrl,
    Bind_SelectWordPartLeft  = Key_LEFT  | InputMod_AnyShift | InputMod_AnyAlt,
	
    Bind_SelectRight = Key_RIGHT | InputMod_AnyShift,
    Bind_SelectWordRight = Key_RIGHT | InputMod_AnyShift | InputMod_AnyCtrl,
    Bind_SelectWordPartRight = Key_RIGHT | InputMod_AnyShift | InputMod_AnyAlt,
	
    Bind_SelectUp    = Key_UP    | InputMod_AnyShift,
    Bind_SelectDown  = Key_DOWN  | InputMod_AnyShift,
	
	Bind_DeleteLeft         = Key_BACKSPACE | InputMod_None,
	Bind_DeleteWordLeft     = Key_BACKSPACE | InputMod_AnyCtrl,
	Bind_DeleteWordPartLeft = Key_BACKSPACE | InputMod_AnyAlt,
	
	Bind_DeleteRight         = Key_DELETE | InputMod_None,
	Bind_DeleteWordRight     = Key_DELETE | InputMod_AnyCtrl,
	Bind_DeleteWordPartRight = Key_DELETE | InputMod_AnyAlt,
	
    Bind_SaveBuffer = Key_S | InputMod_AnyCtrl,
	
    Bind_ReloadConfig = Key_F5 | InputMod_AnyCtrl,
};

struct KeyBinds{
	KeyCode cursorLeft;
	KeyCode cursorWordLeft;
	KeyCode cursorWordPartLeft;
    
	KeyCode cursorRight;
    KeyCode cursorWordRight;
    KeyCode cursorWordPartRight;
    
	KeyCode cursorUp;
    KeyCode cursorDown;
	
    KeyCode cursorAnchor;
	
    KeyCode selectLeft;
    KeyCode selectWordLeft;
    KeyCode selectWordPartLeft;
	
    KeyCode selectRight;
    KeyCode selectWordRight;
    KeyCode selectWordPartRight;
	
    KeyCode selectUp;
    KeyCode selectDown;
	
	KeyCode deleteLeft;
	KeyCode deleteWordLeft;
	KeyCode deleteWordPartLeft;
	
	KeyCode deleteRight;
	KeyCode deleteWordRight;
	KeyCode deleteWordPartRight;
	
    KeyCode saveBuffer;
	
    KeyCode reloadConfig;
};

struct Buffer{
	str8 upperbloc;
	str8 lowerbloc;
	u64  gap_size;
	u64  capacity;

	u64* line_starts;
	u64  line_starts_count;
};

#define UserToGapSpace(buffer, idx) ((idx)>(buffer)->upperbloc.count?(idx)+(buffer)->gap_size:(idx))
//NOTE(sushi) this macro only works if you know you are working with just single byte chars or if your idx is guaranteed to be aligned to a character
#define UserToMemSpace(buffer, idx) ((idx)>(buffer)->upperbloc.count?(buffer)->lowerbloc.str+((idx)-(buffer)->upperbloc.count):buffer->upperbloc.str+(idx))
#define GapToUserSpace(buffer, idx) ((idx)>(buffer)->upperbloc.count+(buffer)->gap_size?(idx)-(buffer)->gap_size:idx)
#define LineLength(buffer, idx) (((idx==(buffer)->line_starts_count?(buffer)->upperbloc.count+(buffer)->lowerbloc.count:(buffer)->line_starts[idx+1]))-((buffer)->line_starts[idx]))


struct TextChunk;
struct Line{
	Node node;
	str8 raw;
	u64  index;
	u64  count; //count of codepoints in line
};	
#define LineFromNode(x) CastFromMember(Line, node, x)
#define NextLine(x) LineFromNode((x)->node.next)
#define PrevLine(x) LineFromNode((x)->node.prev)

struct TextChunk{
	Node  node;
	str8  raw;
	Line* line; //the line the text chunk starts in
	u64   offset; //byte offset from beginning of file
};
#define TextChunkFromNode(x) CastFromMember(TextChunk, node, x)
#define NextTextChunk(x) TextChunkFromNode((x)->node.next)
#define PrevTextChunk(x) TextChunkFromNode((x)->node.prev)


struct Cursor{
	s64   pos; //position in user space in bytes
	s64   count;  //selection size, signed for selections in either direction
};

enum{
	CursorShape_VerticalLine,
	CursorShape_VerticalLineThick,
	CursorShape_Underline,
	CursorShape_Rectangle,
	CursorShape_FilledRectangle,
};

global str8 CursorShapeStrs[] = {
	STR8("VerticalLine"),
	STR8("VerticalLineThick"),
	STR8("Underline"),
	STR8("Rectangle"),
	STR8("FilledRectangle"),
};