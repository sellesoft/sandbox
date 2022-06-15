/* deshi UI Module
Notes
-----
  Everything in this new system is a uiItem.

  Retained UI is stored in arenas while immediate UI is temp allocated

  Everything in the interface is prefixed with "ui" (always lowercase)
	type and macro names follow the prefix and are UpperCamelCase
	function names have a _ after the prefix and are lower_snake_case

  Everything in this system is designed to be as dynamic as possible to enable
	minimal rewriting when it comes to tweaking code. Basically, there should be almost
	no hardcoding of anything and everything that can be abstracted out to a single function
	should be.

  

  Differences from HTML/CSS:
	Margins do not collapse

Index
-----
@ui_style
@ui_item
@ui_window
@ui_button
@ui_text
@ui_section
@ui_context

TODOs
-----
move ui2 to deshi
*/

/*
	Item Style Documentation

	uiItems may be passed a uiStyle object or a css-style string to determine
	the style an item takes on. Following is docs about each property. Each property
	lists it's valid vaules in both programmatic and string form followed by an example.

	//TODO(sushi) eventually convert this to HTML or markdown for easier viewing

------------------------------------------------------------------------------------------------------------
*   positioning 
	---
	Determines how a uiItem is positioned relative to it's parent.

-   Inherited: no

-   Example:
		in code:
			uiStyle style;
			style.positioning = pos_fixed;
		in string:
			positioning: static;

-   Values:
		pos_static  |  static
			Default value.
			The item will be positioned normally. Position values will do nothing.

		pos_relative  |  relative
			The position values will position the item relative to where it would have 
			normally been placed. This does not remove the item from the flow.

        pos_absolute  |  absolute
			The position values will position the item relative to where its parent will
            be placed.

		pos_fixed  |  fixed
			The item is positioned relative to the window it is in and does not move.

		pos_sticky  |  sticy
			The item is positioned just as it would be in relative, but if the item
			were to go out of view by the user scrolling the item will stick to the edge

		pos_draggable_relative | draggable_relative
			The item is positioned the same as relative, but its position may be changed
			by dragging it with the mouse

		pos_draggable_fixed | draggable_fixed
			The item is positioned the same as fixed, but its position may be changed by 
			dragging it with the mouse

------------------------------------------------------------------------------------------------------------
*   sizing 
	---
    Determines how an item is sized. This is a flagged value, meaning it can take on multiple of these
	properties. For example setting 'size_fill_x | size_percent_x' will tell the renderer to interpret
	the width as a percentage of the remaining space of the item's parent's width.

	Fill only applies when the 

-   Inherited: no

-   Values:
		size_percent_x 

		size_percent_y 

		size_percent   

		size_fill_x

		size_fill_y   

		size_fill

		size_square
			Keeps the item at a 1:1 aspect ratio. This requires that either height or width are set to auto, while
			the other has a specified value. If both values are specified, then this value is ignored

------------------------------------------------------------------------------------------------------------
*   top,left,bottom,right
	---
	Determines where a uiItem is positioned according to it's 'positioning' value. Percents are only supported
	in string styling, by suffixing the literal with a %. Note that if bottom or right are used, they overrule
	top or left respectively.

-   Inherited: no

-   Defaults:
		top and left default to 0, while bottom and right default to MAX_S32, indicating to use top or left instead.

-   Shorthands:
		tl - a vec2i representing the x and y coords of the top left corner of the item
		br - a vec2i representing the x and y coords of the bottom right corner of the item
	
-   Example:
		in code:
			uiStyle style;
			style.positioning = pos_relative; 
			style.top = 10; //sets the item's position to be 10 pixels below 
		in string:
			positioning: static;
			top: 10; 
			////
			positioning: relative;
			lt: 10% 15%; //sets the item's left edge to be 10% of its parents width from its parent left edge and 15% of its parent's height from its parent's top edge

------------------------------------------------------------------------------------------------------------
*   size, width, height
	Determines the size of the item. Note that text is not affected by this property, and its size is always
	as it appears on the screen. If you want to change text size use font_height.

    This is the size of the area in which items can be placed minus padding. Note that if margins or borders are 
    specified, this increases the total area an item takes up visually.

-   Inherited: no

-   Special:
		There are 2 special values for sizing an item. Negative values are invalid for sizing are so some are
		reserved to indicate to the renderer how to size the item.

		-1: equivalent to auto
			this tells the renderer to automatically size the item based on its content.
			an enum 'size_auto' is specified

-   Defaults:
		width and height both default to -1. See above for why.

-   Example:
		in code:
			uiStyle style;
			style.width = size_fill; //will fill remaining width of parent
			style.height = 20; //height 20 pixels
		in string:
			width: auto; //size based on content
			height: 20%; //size 20% of parent's width
			width: fill; //fills remaining width of parent.


------------------------------------------------------------------------------------------------------------
*   margin, margin_top, margin_bottom, margin_left, margin_right
	---
	Determines the spacing between a child's edges and its parents edges. Percents are only supported in
	string styling. Note that if bottom or right are used, they overrule top or left respectively.

	 ┌----------------------------┑
	 |     |─────────────────────────── margin_top
	 |    ┌------------------┑    |
	 |━━━━|                  |    |
	 |  | |    child item    |    |
	 |  | |                  |    |
	 |  | |                  |    |
	 |  | |                  |    |
	 |  | └------------------┙    |
	 |  |                         |
	 └--|-------------------------┙
		|
	   margin_left

-   Inherited: no

-   Defaults:
		margin_top and margin_left default to 0, while margin_bottom and margin_right default to MAX_S32
		MAXP_S32 just indicates to the renderer that it should use the same value as the other side

-   Shorthands:
		margin sets margin_left and margin_top

-   Example:
		in code:
			uiStyle style;
			style.margin_top = 10;
		in string:
			margin: 10px 10%;

-   Behavoir:
		Margins do not cause items to push away their siblings, a margin for an item just 

------------------------------------------------------------------------------------------------------------
*   padding, padding_top, padding_bottom, padding_left, padding_right
	---
	Determines the spacing between an item's edges and its content's edges. Percents are only supported in
	string styling. The value is interpretted as a 

	 ┌----------------------------┑
	 |                            |
	 |    ┌------------------┑    |
	 |    |    |─────────────────────────── padding_top
	 |    |----text in item  |    |
	 |    | |                |    |
	 |    | |                |    |
	 |    | |                |    |
	 |    └-|----------------┙    |
	 |      |                     |
	 └------|---------------------┙
			|
			padding_left

-   Inherited: no

-   Defaults:
		padding_top and padding_left default to 0, while padding_bottom and padding_right default to MAX_S32
		MAX_S32 just indicates to the renderer that it should use the same value as the other side

-   Shorthands:
		padding sets padding_left and padding_top

-   Example:
		in code:
			uiStyle style;
			style.padding_left = 10;
			style.padding_bottom = 15;
		in string:
			padding: 20; //sets both padding_left and padding_top to 20 pixels;

------------------------------------------------------------------------------------------------------------
*   content_align
	---
	Determines how to align an item's contents over the x axis. 
	This only works on children who's positioning is static or relative.
	This respects margins and padding, content will only be aligned within their valid space. 
	Negative values are not valid.
	Values larger than 1 are not valid.
	This does not apply when alignment is set to inline.

-   Performance:
		In order to properly do this (as far as I can tell), we must reevaluate all children of an item
		after having already evaluated them because the size of the group of items is not known until
		all of them have been evaluated. So this more or less doubles the evaluation time of every
		item that uses it.

-   Inherited: no

-   Defaults:
		Defaults to 0

TODO(sushi) example

------------------------------------------------------------------------------------------------------------
*   font
	---
	Determines the font to use when rendering text. In code this takes a Font* while in string this takes the
	name of a font file stored in data/fonts.

-   Inherited: yes

-   Defaults:
		By default the font used is gohudont-11.bdf which should ship with deshi.

-   Example:
		in code:
			uiStyle style;
			style.font = Storage::CreateFontFromFile("Terminus.ttf").second;
		in string:
			font: Terminus.ttf;

------------------------------------------------------------------------------------------------------------
*   font_height
	---
	Determines the visible height in pixels of rendered text. Note that this does not have to be the same height
	the font was loaded at, though it may not render as pretty if it isnt.

-   Inherited: yes

-   Defaults:
		Defaults to 11, the height of the default loaded font.

-   Example:
		in code:
			uiStyle style; 
			style.font_height = 200;
		in string:
			font_height: 200;
------------------------------------------------------------------------------------------------------------
*   background_color
	---
	Determines the background color of a uiItem. Note that this property does not apply to all items.

-   Inherited: no

-   Defaults:
		Defaults to (14,14,14,255)

------------------------------------------------------------------------------------------------------------
*   border_style
	---
	Determines the style of border a uiItem has

	//TODO(sushi) border_style docs

------------------------------------------------------------------------------------------------------------
*   border_width
	---
	Determines the width of border a uiItem has

------------------------------------------------------------------------------------------------------------
*   text_color
	---
	Determines the color of text

	//TODO(sushi) text_color docs

------------------------------------------------------------------------------------------------------------
*   overflow
	---
	Determines how items that go out of their parent's bounds are displayed.

-   Inherited: no

-   Defaults:
		This value defaults to scroll.

-  Values:
	
	overflow_scroll | scroll
		When items extend beyond the parents borders, the items are cut off and scroll bars are shown.
	
	overflow_scroll_hidden | scroll_hidden
		Same as overflow_scroll, but scrollbars are not shown
	
	overflow_hidden | hidden
		The overflowing items are cut off, but the user cannot scroll.
		NOTE(sushi): in this case you can still manually change the scroll value on the item
					 this just prevents the user from scrolling the item. Good for scrolling things you
					 dont want the user to mess with.
	
	overflow_visible | visible
		The overflowing items are shown.

*/


#pragma once
#ifndef DESHI_UI2_H
#define DESHI_UI2_H

#include "kigu/color.h"
#include "kigu/common.h"
#include "kigu/node.h"
#include "kigu/unicode.h"
#include "math/vector.h"
#include "core/render.h"
#include "kigu/hash.h"

#if DESHI_RELOADABLE_UI
#  if DESHI_DLL
#    define UI_FUNC_API(sig__return_type, sig__name, ...) \
external __declspec(dllexport) sig__return_type sig__name(__VA_ARGS__); \
typedef sig__return_type GLUE(sig__name,__sig)(__VA_ARGS__); \
sig__return_type GLUE(sig__name,__stub)(__VA_ARGS__);
#  else
#    define UI_FUNC_API(sig__return_type, sig__name, ...) \
external __declspec(dllexport) sig__return_type sig__name(__VA_ARGS__); \
typedef sig__return_type GLUE(sig__name,__sig)(__VA_ARGS__); \
sig__return_type GLUE(sig__name,__stub)(__VA_ARGS__){return (sig__return_type)0;}
#  endif //DESHI_DLL
#  define UI_DEF(x) GLUE(g_ui->, x)
#else
#  define UI_FUNC_API(sig__return_type, sig__name, ...) external sig__return_type sig__name(__VA_ARGS__)
#  define UI_DEF(x) GLUE(ui_, x)
#endif //DESHI_RELOADABLE_UI

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_item
enum uiItemType_{
	uiItemType_Item = 0, //a normal item, doesnt have any special data associated with it
	uiItemType_Section,
	uiItemType_Window, 
	uiItemType_ChildWindow,
	uiItemType_Button,
	uiItemType_Text,
	uiItemType_COUNT
};typedef u32 uiItemType;

struct primcount{
	u32 n;
	vec2i* counts;
	vec2i* sums;
};

struct Texture;
struct Vertex2;
struct uiDrawCmd{
	Texture* texture;
	//offsets into the vertex and index arenas
	// these must be offsets now that the arenas must be contiguous and therefore
	// must move if overfilled
	u32 vertex_offset; 
	u32 index_offset;
	RenderDrawCounts counts;
	vec2i scissorOffset;
	vec2i scissorExtent;
};

enum{
	pos_static=0,
	pos_relative,
	pos_fixed,
	pos_absolute,
	pos_sticky,
	pos_draggable_relative,
	pos_draggable_fixed,

	size_auto = -1,
    size_percent_x = 1 << 0,
    size_percent_y = 1 << 1,
    size_percent   = size_percent_x | size_percent_y,
    size_fill_x    = 1 << 2,
    size_fill_y    = 1 << 3,
    size_fill      = size_fill_x | size_fill_y,
	size_square    = 1 << 4,
	
	border_none = 0,
	border_solid,
	
	overflow_scroll = 0,
	overflow_scroll_hidden,
	overflow_hidden,
	overflow_visible,
};

struct Font;
external struct uiStyle{
	Type positioning;
    Type sizing;
	union{
		struct{f32 left, top;};
		vec2 tl;
	};
	union{
		struct{f32 right, bottom;};
		vec2 br;
	};
	union{
		struct{f32 width, height;};
		vec2 size;
	};
	union{
		struct{f32 margin_left, margin_top;};
		vec2 margintl;
	};
	union{
		struct{f32 margin_bottom, margin_right;};
		vec2 marginbr;        
	};
	union{
		struct{f32 padding_left, padding_top;};
		vec2 paddingtl;
	};
	union{
		struct{f32 padding_bottom, padding_right;};
		vec2 paddingbr;        
	};
	vec2 content_align; 
	Font* font;
	u32 font_height;
	color background_color;
	Texture* background_image;
	Type border_style;
	color border_color;
	f32 border_width;
	color text_color;
	Type overflow;
	
	void operator=(const uiStyle& rhs){ memcpy(this, &rhs, sizeof(uiStyle)); }
	
	
};
extern uiStyle* ui_initial_style;

template<>
struct hash<uiStyle> {
	inline u32 operator()(const uiStyle& s){DPZoneScoped;
		u32 seed = 2166136261;
		seed ^= *(u32*)&s.positioning;     seed *= 16777619;
		seed ^= *(u32*)&s.left;            seed *= 16777619;
		seed ^= *(u32*)&s.top;             seed *= 16777619;
		seed ^= *(u32*)&s.right;           seed *= 16777619;
		seed ^= *(u32*)&s.bottom;          seed *= 16777619;
		seed ^= *(u32*)&s.width;           seed *= 16777619;
		seed ^= *(u32*)&s.height;          seed *= 16777619;
		seed ^= *(u32*)&s.margin_left;     seed *= 16777619;
		seed ^= *(u32*)&s.margin_top;      seed *= 16777619;
		seed ^= *(u32*)&s.margin_bottom;   seed *= 16777619;
		seed ^= *(u32*)&s.margin_right;    seed *= 16777619;
		seed ^= *(u32*)&s.padding_left;    seed *= 16777619;
		seed ^= *(u32*)&s.padding_top;     seed *= 16777619;
		seed ^= *(u32*)&s.padding_bottom;  seed *= 16777619;
		seed ^= *(u32*)&s.padding_right;   seed *= 16777619;
		seed ^= *(u32*)&s.content_align.x; seed *= 16777619;
		seed ^= *(u32*)&s.content_align.y; seed *= 16777619;
		seed ^= (u64)s.font;               seed *= 16777619;
		seed ^= s.font_height;             seed *= 16777619;
		seed ^= s.background_color.rgba;   seed *= 16777619;
		seed ^= (u64)s.background_image;   seed *= 16777619;
		seed ^= s.border_style;            seed *= 16777619;
		seed ^= s.border_color.rgba;       seed *= 16777619;
		seed ^= *(u32*)&s.border_width;    seed *= 16777619;
		seed ^= s.text_color.rgba;         seed *= 16777619;
		seed ^= s.overflow;                seed *= 16777619;
		return seed;
	}
};

typedef Flags uiFlags; enum{

	uiFlag_ActOnMouseHover    =1<<0, // call action when the mouse is positioned over the item
	uiFlag_ActOnMousePressed  =1<<1, // call action when the mouse is pressed over the item
	uiFlag_ActOnMouseReleased =1<<2, // call action when the mouse is released over the item
	uiFlag_ActOnMouseDown     =1<<3, // call action when the mouse is down over the item
	uiFlag_ActAlways          =1<<4, // call action every frame

};

struct uiItem{
	TNode node;
	str8 id; //NOTE(sushi) mostly for debugging, not sure if this will have any other use in the interface
	uiStyle style;
	u64 style_hash;
	
	//an items action call back function 
	//this function is called in situations defined by the flags in the uiFlags enum
	//and is always called before anything happens to the item in ui_update
	//NOTE(sushi) remember that to actually affect the item, you must change its style NOT the variables below
	s32 (*action)(uiItem*);

	//// state ////
	b32 hovered;
	
	//// INTERNAL ////
	union{ // position relative to parent
		struct{ f32 lx, ly; };
		vec2 lpos;
	};
	union{ // position relative to screen
		struct{ f32 sx, sy; };
		vec2 spos;
	};
	union{
		struct{ f32 width, height; };
		vec2 size;
	};
	union{//TODO(sushi) this should probably be on uiStyle, so you can programatically change scroll.
		struct{f32 scrx, scry;};
		vec2 scroll;
	};

    //screen position and size of the bounding box containing all of an items
    //children, this is used to optimize things later, such as finding the hovered item
    vec2 children_bbx_pos;
    vec2 children_bbx_size;

	//the visible size of the item after being cut by overflow
	vec2 visible_start;
	vec2 visible_size; 
	
	u64 draw_cmd_count;
	uiDrawCmd* drawcmds;
	
	str8 file_created;
	upt  line_created;
	
	void operator=(const uiItem& rhs){memcpy(this, &rhs, sizeof(uiItem));}
};
#define uiItemFromNode(x) CastFromMember(uiItem, node, x)

UI_FUNC_API(uiItem*, ui_make_item, str8 id, uiStyle* style, str8 file, upt line);
#define uiItemM() UI_DEF(make_item({0}, 0,STR8(__FILE__),__LINE__)))
#define uiItemMS(style) UI_DEF(make_item({0},(style),STR8(__FILE__),__LINE__))
#define uiItemMSI(id, style) UI_DEF(make_item({0},(style),STR8(__FILE__),__LINE__))

UI_FUNC_API(uiItem*, ui_begin_item, str8 id, uiStyle* style, str8 file, upt line);
#define uiItemB() UI_DEF(begin_item({0,0},0,STR8(__FILE__),__LINE__))
#define uiItemBS(style) UI_DEF(begin_item({0,0},(style),STR8(__FILE__),__LINE__))
#define uiItemBSI(id,style) UI_DEF(begin_item((id),(style),STR8(__FILE__),__LINE__))

UI_FUNC_API(void, ui_end_item, str8 file, upt line);
#define uiItemE() UI_DEF(end_item(STR8(__FILE__),__LINE__))


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_window
// makes a uiItem that by default can be dragged and has a border
// eventually make flags for automatically having a title bar and buttons, etc. 
UI_FUNC_API(uiItem*, ui_make_window, str8 name, Flags flags, uiStyle* style, str8 file, upt line);
#define uiWindowM(name, flags)         UI_DEF(make_window(STR8(name),(flags),STR8(__FILE__),__LINE__))
#define uiWindowMS(name, flags, style) UI_DEF(make_window(STR8(name),(flags),STR8(__FILE__),__LINE__))

UI_FUNC_API(uiItem*, ui_begin_window, str8 name, Flags flags, uiStyle* style, str8 file, upt line);
#define uiWindowB(name,  flags)        UI_DEF(begin_window(STR8(name),(flags),0,STR8(__FILE__),__LINE__))
#define uiWindowBS(name, flags, style) UI_DEF(begin_window(STR8(name),(flags),(style),STR8(__FILE__),__LINE__))

UI_FUNC_API(uiItem*, ui_end_window);
#define uiWindowE() UI_DEF(window_end())

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_button
enum {
	uiButtonFlags_ActOnPressed,  // button performs its action as soon as its clicked
	uiButtonFlags_ActOnReleased, // button waits for the mouse to be released before performing its action
	uiButtonFlags_ActOnDown,     // button performs its action for the duration the mouse is held down over it
};

typedef void (*Action)(void*);
struct uiButton{
	uiItem item;
	Action action;
	void*  action_data;
	b32    clicked;
};
#define uiButtonFromNode(x) CastFromMember(uiButton, item, CastFromMember(uiItem, node, x))

//      window: uiWindow to emplace the button in
//        text: text to be displayed in the button
//      action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//              if 0 is passed, the button will just set it's clicked flag
// action_data: data passed to the action function 
//       flags: uiButtonFlags to be given to the button
UI_FUNC_API(uiButton*, ui_make_button, Action action, void* action_data, Flags flags, str8 file, upt line);
#define uiMakeButton(window, text, action, action_data, flags) UI_DEF(make_button(STR8(text),(action),(action_data),(flags),STR8(__FILE__),__LINE__))


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_text
struct uiText{
	uiItem item;
	str8 text; 
};
#define uiTextFromItem(x) CastFromMember(uiText, item, x)

UI_FUNC_API(uiText*, ui_make_text, str8 text, str8 id, uiStyle* style, str8 file, upt line);
//NOTE(sushi) does not automatically make a str8, use uiTextML for that.
#define uiTextM(text)         UI_DEF(make_text((text),     {0},       0, STR8(__FILE__),__LINE__))
#define uiTextMS(text, style) UI_DEF(make_text((text),     {0}, (style), STR8(__FILE__),__LINE__))
//NOTE(sushi) this automatically applies STR8() to text, so you can only use literals with this macro
#define uiTextML(text)        UI_DEF(make_text(STR8(text), {0},       0, STR8(__FILE__),__LINE__))
#define uiTextMLS(text,style) UI_DEF(make_text(STR8(text), {0}, (style), STR8(__FILE__),__LINE__))

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context
struct MemoryContext;
struct uiContext;
UI_FUNC_API(void, ui_init, MemoryContext* memctx, uiContext* uictx);
#define uiInit(memctx,uictx) UI_DEF(init((memctx),(uictx)))

UI_FUNC_API(void, ui_update);
#define uiUpdate() UI_DEF(update())

//we cant grow the arena because it will move the memory, so we must chunk 
struct Arena;
struct ArenaList{
	Node node;   
	Arena* arena;
};
#define ArenaListFromNode(x) CastFromMember(ArenaList, node, x);

typedef u32 uiInputState; enum{
	uiISNone,
	uiISScrolling,
	uiISDragging,
	uiISResizing,
	uiISPreventInputs,
	uiISExternalPreventInputs,
};


struct uiContext{
#if DESHI_RELOADABLE_UI
	//// functions ////
	void* module;
	b32   module_valid;
	ui_make_item__sig*         make_item;
	ui_begin_item__sig*        begin_item;
	ui_end_item__sig*          end_item;
	ui_make_window__sig*       make_window;
	ui_begin_window__sig*      begin_window;
	ui_end_window__sig*        end_window;
	ui_make_button__sig*       make_button;
	ui_make_text__sig*         make_text;
	ui_init__sig*              init;
	ui_update__sig*            update;
#endif //#if DESHI_RELOADABLE_UI
	
	//// state ////
	uiItem base;
	uiItem* hovered; //item currently hovered by the mouse
	uiInputState istate;
	
	//// memory ////
	//TODO(sushi) convert these 2 to Heaps when its implemented
	ArenaList* item_list;
	ArenaList* drawcmd_list;
	//we cannot chunk these 2 arenas, and they cannot be mixed either
	Arena* vertex_arena;
	Arena* index_arena;
	RenderTwodBuffer render_buffer;
	array<uiItem*> update_this_frame;
	array<uiItem*> item_stack; //TODO(sushi) eventually put this in it's own arena since we can do a stack more efficiently in it
};

//global UI pointer
extern uiContext* g_ui;

#endif //DESHI_UI2_H
