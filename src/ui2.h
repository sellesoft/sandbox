/* deshi UI Module
Notes
-----
  Everything in this new system is a uiItem.

  Retained UI is stored in arenas while immediate UI is temp allocated

  Any time an item is modified or removed we find it's parent window and recalculate it entirely
    this can probably be optimized

  For the reason above, initial generation of a uiItem and it's actual drawinfo generation are in separate functions

  UI macros always automatically apply the str8_lit macro to arguments that ask for text

  uiWindow cursors never take into account styling such as padding (or well, not sure yet)

  Everything in the interface is prefixed with "ui" (always lowercase)
    type and macro names follow the prefix and are UpperCamelCase
    function names have a _ after the prefix and are lower_snake_case

  Everything in this system is designed to be as dynamic as possible to enable
    minimal rewriting when it comes to tweaking code. Basically, there should be almost
    no hardcoding of anything and everything that can be abstracted out to a single function
    should be.

Index
-----
@ui_style
@ui_item
@ui_window
@ui_button
@ui_text
@ui_section
@ui_context
@ui_shared_vars
@ui_shared_funcs

TODOs
-----
move ui2 to deshi so DLLs can work
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
*   top,left,bottom,right
    ---
    Determines where a uiItem is positioned according to it's 'positioning' value. Percents are only supported
    in string styling, by suffixing the literal with a %. Note that if bottom or right are used, they overrule
    top or left respectively.

-   Inherited: no

-   Defaults:
        top and left default to 0, while bottom and right default to MAX_U32, indicating to use top or left instead.

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
    Determines the size of the item.

-   Inherited: no

-   Special:
        There are 2 special values for sizing an item. Negative values are invalid for sizing are so some are
        reserved to indicate to the renderer how to size the item.

        -1: equivalent to auto
            this tells the renderer to automatically size the item based on its content.
            an enum 'size_auto' is specified
        
        -2: fill remaining space
            this tells the renderer to automatically fill the parent's remaining space after
            its initial size is determined by its content
            an enum 'size_fill' is specified

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
        margin_top and margin_left default to 0, while margin_bottom and margin_right default to MAX_U32

-   Shorthands:
        margin sets margin_left and margin_top

-   Example:
        in code:
            uiStyle style;
            style.margin_top = 10;
        in string:
            margin: 10px 10%;

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
        padding_top and padding_left default to 0, while padding_bottom and padding_right default to MAX_U32

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
    Determines how to align an item's contents. This only works when positioning is set to static, otherwise its
    value will be ignored. This value is a vec2 whose valid values range from 0 to 1.

-   Inherited: no

-   Defaults:
        Defaults to {0,0}

-   Example:
        in code:
            uiStyle style;
            style.content_align = vec2{0.5,0.5}; //aligns the item's content to be centered over x and y
        in string:
            content_align: 0.5 0; //aligns the item's content to be centered over x and top aligned over y
        visual:
            {0, 0.5}
            ┌----------------------------┑
            |                            |
            |┌------------------┑        |
            ||                  |        |
            ||                  |        |
            ||                  |        |
            ||                  |        |
            ||                  |        |
            |└------------------┙        |
            |                            |
            └----------------------------┙

            {1, 1}
            ┌----------------------------┑
            |                            |
            |                            |
            |        ┌------------------┑|
            |        |                  ||
            |        |                  ||
            |        |                  ||
            |        |                  ||
            |        |                  ||
            |        └------------------┙|
            └----------------------------┙

------------------------------------------------------------------------------------------------------------
*   font
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
    Determines the background color of a uiItem. Note that this property does not apply to all items.

-   Inherited: no

-   Defaults:
        Defaults to (14,14,14,255)

------------------------------------------------------------------------------------------------------------
*   border_style
    Determines the style of border a uiItem has


*/


#pragma once
#ifndef DESHI_UI2_H
#define DESHI_UI2_H

#include "kigu/color.h"
#include "kigu/common.h"
#include "kigu/node.h"
#include "kigu/unicode.h"
#include "math/vector.h"


#if DESHI_RELOADABLE_UI
//#  define UI_FUNCTION __declspec(dllexport)
//ref: http://www.codinglabs.net/tutorial_CppRuntimeCodeReload.aspx
#  define UI_FUNC_API(sig__return_type, sig__name, ...) \
    __declspec(dllexport) sig__return_type sig__name(__VA_ARGS__); \
    typedef sig__return_type GLUE(sig__name,__sig)(__VA_ARGS__); \
    sig__return_type GLUE(sig__name,__stub)(__VA_ARGS__){return (sig__return_type)0;}

#  define UI_DEF(x) GLUE(g_ui->, x)
#else
#  define UI_FUNC_API(sig__return_type, sig__name, ...) external sig__return_type sig__name(__VA_ARGS__)
#  define UI_DEF(x) GLUE(ui_, x) 
#endif




//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_item
enum uiItemType_{
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
    Vertex2* vertices;
    u32*     indicies;
    vec2i    counts;
};

enum{
    pos_static=0,
    pos_relative,
    pos_fixed,
	// pos_absolute, TODO(sushi) decide if this is useful to implement
    pos_sticky,
    pos_draggable_relative,
    pos_draggable_fixed,

    border_none = 0,
    border_solid,

    size_auto = -1,
    size_fill = -2,
};

struct Font;
struct uiStyle{
    Type positioning;  
    union{
        struct{s32 left, top;};
        vec2i tl;
    };
    union{
        struct{s32 right, bottom;};
        vec2i br;
    };
    union{
        struct{s32 width, height;};
        vec2i size;
    };
    union{
        struct{s32 margin_left, margin_top;};
        vec2i margintl;
    };
    union{
        struct{s32 margin_bottom, margin_right;};
        vec2i marginbr;        
    };
    union{
        struct{s32 padding_left, padding_top;};
        vec2i paddingtl;
    };
    union{
        struct{s32 padding_bottom, padding_right;};
        vec2i paddingbr;        
    };
    vec2 content_align; 
    Font* font;
    u32 font_height;
    color background_color;
    Texture* backgound_image;
    Type border_style;
    color border_color;

    void operator=(const uiStyle& rhs){ memcpy(this, &rhs, sizeof(this)); }

} ui_initial_style {
/*          positioning */ pos_static,
/*                 left */ 0,
/*                  top */ 0,
/*                right */ MAX_U32,
/*               bottom */ MAX_U32,
/*                width */ 0,
/*               height */ 0,
/*          margin_left */ 0,
/*           margin_top */ 0,
/*         margin_right */ MAX_U32,
/*        margin_bottom */ MAX_U32,
/*         padding_left */ 0,
/*          padding_top */ 0,
/*        padding_right */ MAX_U32,
/*       padding_bottom */ MAX_U32,
/*        content_align */ vec2{0,0},
/*                 font */ Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second,
/*          font_height */ 11,
/*     background_color */ color{0,0,0,0},
/*     background_image */ 0,
/*         border_style */ border_none,
/*         border_color */ color{180,180,180,255},
};

struct uiItem{
    TNode node;
    Flags flags;
    uiStyle style;
    //the following position and size vars are internal and unrelated to actually determining 
    //the item's position and size at creation. uiStyle is used for that.
    union{
        struct{ // position relative to parent
            s32 lx; 
            s32 ly;
        };
        vec2i lpos;
    };
    union{
        struct{ // position relative to screen
            s32 sx;
            s32 sy;
        };
        vec2i spos;
    };
    union{
        struct{
            s32 width;
            s32 height;
        };
        vec2i size;
    };
	
    u64 draw_cmd_count;
    uiDrawCmd* drawcmds;

    str8 file_created;
    upt  line_created;

    void operator=(const uiItem& rhs){memcpy(this, &rhs, sizeof(this));}
} base;



#define uiItemFromNode(x) CastFromMember(uiItem, node, x)


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_window
struct uiWindow{
    uiItem item;
    str8 name;
    union{
        struct{
            s32 curx;
            s32 cury;
        };
        vec2i cursor;
    };
};
#define uiWindowFromNode(x) CastFromMember(uiWindow, item, CastFromMember(uiItem, node, x))

// makes a uiItem that by default can be dragged and has a border
UI_FUNC_API(uiWindow*, ui_make_window, str8 name, Flags flags, str8 file, upt line);
#define uiWindowM(name, flags, style) UI_DEF(make_window(STR8(name),(flags),STR8(__FILE__),__LINE__))

UI_FUNC_API(uiWindow*, ui_begin_window, str8 name, Flags flags, uiStyle style, str8 file, upt line);
#define uiWindowB(name,  flags)        UI_DEF(begin_window(STR8(name),(flags),(style),STR8(__FILE__),__LINE__))
#define uiWindowBS(name, flags, style) UI_DEF(begin_window(STR8(name),(flags),(style),STR8(__FILE__),__LINE__))

UI_FUNC_API(uiWindow*, ui_end_window);
#define uiWindowE() UI_DEF(window_end())


// makes a child window inside another window
//
// window: uiWindow to emplace this window in
//   name: name of the window
//    pos: initial position of the window
//   size: initial size of the window
UI_FUNC_API(uiWindow*, ui_make_child_window, uiWindow* window, str8 name, vec2i size, Flags flags, str8 file, upt line);


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
UI_FUNC_API(uiButton*, ui_make_button, uiWindow* window, Action action, void* action_data, Flags flags, str8 file, upt line);
#if DESHI_RELOADABLE_UI
#  define uiMakeButton(window, text, action, action_data, flags) g_ui->make_button((window),STR8(text),(action),(action_data),(flags),STR8(__FILE__),__LINE__)
#else
#  define uiMakeButton(window, text, action, action_data, flags) ui_make_button((window),STR8(text),(action),(action_data),(flags),STR8(__FILE__),__LINE__)
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_text
struct uiText{
    uiItem item;
    str8 text; 
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_section
//same as a div in HTML, just a section that items will place themselves in
//TODO(sushi) void ui_make_section(vec2i pos, vec2i size);


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context
//we cant grow the arena because it will move the memory, so we must chunk 
struct Arena;
struct ArenaList{
    Node node;   
    Arena* arena;
};
#define ArenaListFromNode(x) CastFromMember(ArenaList, node, x);

struct uiContext{
#if DESHI_RELOADABLE_UI
	//// functions ////
	void* module;
	b32   module_valid;
    ui_update__sig            update;
	ui_make_window__sig       make_window;
	ui_begin_window__sig      begin_window;
	ui_end_window__sig        end_window;
	ui_make_child_window__sig make_child_window;
	ui_make_button__sig       make_button;
#endif //#if DESHI_RELOADABLE_UI
	
	//// state ////
    uiItem base;
    uiItem* curitem;
	
	//// memory ////
	ArenaList* item_list;
	ArenaList* drawcmd_list;
    array<uiItem*> item_stack; //TODO(sushi) eventually put this in it's own arena since we can do a stack more efficiently in it
};

//global UI pointer
extern uiContext* g_ui;

external void ui_init();

UI_FUNC_API(void, ui_update);
#define uiUpdate UI_DEF(update())

#if DESHI_RELOADABLE_UI
external void ui_reload_functions();
#endif //#if DESHI_RELOADABLE_UI

#endif //DESHI_UI2_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef DESHI_IMPLEMENTATION_UI2 //TODO rename this define to DESHI_IMPLEMENTATION when we move this to deshi
#include "core/input.h"
#include "core/platform.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_shared_vars
local uiContext deshi_ui{};
uiContext* g_ui = &deshi_ui;


//-//////////////////////////////////
//////////////////////////////////////////////////////////////
// @ui_shared_funcs
ArenaList* create_arena_list(ArenaList* old){
    ArenaList* nual = (ArenaList*)memalloc(sizeof(ArenaList));
    if(old) NodeInsertNext(&old->node, &nual->node);
    nual->arena = memory_create_arena(Megabytes(1));
    return nual;
}

void ui_init(){
#if DESHI_RELOADABLE_UI
	g_ui->module = platform_load_module(STR8("deshi.dll"));
	if(g_ui->module){
		g_ui->update            = platform_get_module_function(g_ui->module, "ui_update", ui_update);
        g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
		g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
		g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
		g_ui->make_child_window = platform_get_module_function(g_ui->module, "ui_make_child_window", ui_make_child_window);
		g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
		g_ui->module_valid = (g_ui->push_f32 && g_ui->push_vec2 && g_ui->make_window && g_ui->begin_window && g_ui->end_window && g_ui->make_child_window && g_ui->make_button);
	}
	if(!g_ui->module_valid){
		g_ui->make_window       = ui_make_window__stub;
		g_ui->begin_window      = ui_begin_window__stub;
		g_ui->end_window        = ui_end_window__stub;
		g_ui->make_child_window = ui_make_child_window__stub;
		g_ui->make_button       = ui_make_button__stub;
	}
#endif //#if DESHI_RELOADABLE_UI
	
	g_ui->item_list    = create_arena_list(0);
    g_ui->drawcmd_list = create_arena_list(0);

    //initialize base item
    g_ui->base = uiItem{0};
    g_ui->base.style = ui_initial_style;
    g_ui->base.node = TNode{0};
    g_ui->base.node.type = uiItemType_Section;
    g_ui->base.node.flags = 0;
        
    
}



#if DESHI_RELOADABLE_UI
void ui_reload_functions(){
	//unload the module
	g_ui->module_valid = false;
	if(g_ui->module){
		platform_free_module(g_ui->module);
		g_ui->module = 0;
	}
	
	//load the module
	g_ui->module = platform_load_module(STR8("deshi.dll"));
	if(g_ui->module){
		g_ui->push_f32          = platform_get_module_function(g_ui->module, "ui_push_f32", ui_push_f32);
		g_ui->push_vec2         = platform_get_module_function(g_ui->module, "ui_push_vec2", ui_push_vec2);
		g_ui->make_window       = platform_get_module_function(g_ui->module, "ui_make_window", ui_make_window);
		g_ui->begin_window      = platform_get_module_function(g_ui->module, "ui_begin_window", ui_begin_window);
		g_ui->end_window        = platform_get_module_function(g_ui->module, "ui_end_window", ui_end_window);
		g_ui->make_child_window = platform_get_module_function(g_ui->module, "ui_make_child_window", ui_make_child_window);
		g_ui->make_button       = platform_get_module_function(g_ui->module, "ui_make_button", ui_make_button);
		g_ui->module_valid = (g_ui->push_f32 && g_ui->push_vec2 && g_ui->make_window && g_ui->begin_window && g_ui->end_window && g_ui->make_child_window && g_ui->make_button);
	}
	if(!g_ui->module_valid){
		g_ui->make_window       = ui_make_window__stub;
		g_ui->begin_window      = ui_begin_window__stub;
		g_ui->end_window        = ui_end_window__stub;
		g_ui->make_child_window = ui_make_child_window__stub;
		g_ui->make_button       = ui_make_button__stub;
	}
}
#endif //#if DESHI_RELOADABLE_UI


#endif //#idef DESHI_IMPLEMENTATION_UI2