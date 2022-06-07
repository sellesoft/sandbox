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

TODOs
-----
move ui2 to deshi so DLLs can work
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
#else
#  define UI_FUNC_API(sig__return_type, sig__name, ...) external sig__return_type sig__name(__VA_ARGS__)
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_style
enum{
    uiColor_Text,
    uiColor_Separator,
    uiColor_WindowBg,
    uiColor_WindowBorder,
    uiColor_COUNT
};

enum{
    uiStyle_WindowMargins,
    uiStyle_ItemSpacing,
    uiStyle_WindowBorderSize,
    uiStyle_ButtonBorderSize,
};

struct Font;
struct uiStyle{
    vec2i window_margins;
    vec2i item_spacing;
    f32   window_border_size;
    f32   button_border_size;
    color colors[uiColor_COUNT];
    Font* font;
};

struct uiStyleColorMod{
    Type  col;
    color old;
};

struct uiStyleVarMod{
    Type var;
    f32 old[2];
};

struct uiStyleVarType{
    u32 count;
    u32 offset;
};

local const uiStyleVarType uiStyleTypes[] = {
    {2, offsetof(uiStyle, window_margins)},
    {2, offsetof(uiStyle, item_spacing)},
    {1, offsetof(uiStyle, window_border_size)},
    {1, offsetof(uiStyle, button_border_size)},
};

UI_FUNC_API(void, ui_push_f32, Type idx, f32 nu);

UI_FUNC_API(void, ui_push_vec2, Type idx, vec2 nu);


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_item
enum uiItemType_{
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

struct uiItem{
    TNode node;
    Flags flags;
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
    
    uiStyle style; // style at time of making this item. eventually this should be optimized to not store the entire style
	
    uiItem(){} // C++ is retarded
	
    str8 file_created;
    upt  line_created;
};


/*
    Item Style Documentation

    uiItems may be passed a uiItemStyle object or a css-style string to determine
    the style an item takes on. Following is docs about each property. Each property
    lists it's valid vaules in both programmatic and string form followed by an example.


*   positioning 
    ---
    Determines how a uiItem is positioned relative to it's parent.

-   Example:
        in code:
            uiItemStyle style;
            style.positioning = pos_fixed;
        in string:
            positioning: static

-   Values:
        pos_static    |  static
            Default value.
            The item will be positioned normally. Position values will do nothing.

        pos_relative  |  relative
            The position values will position the item relative to where it would have 
            normally been placed. This does not remove the item from the flow.

        pos_fixed     |  fixed
            The item is positioned relative to the window it is in and does not move.

        pos_sticky    |  sticy
            The item is positioned just as it would be in relative, but if the item
            were to go out of view by the user scrolling the item will stick to the edge


*   top,left,bottom,right
    ---
    Determines where a uiItem is positioned according to it's 'positioning' value
    
-   Shorthands:
        tl - a vec2i representing top and left
        br - a vec2i representing bottom and right
    
-   Example:
        in code:
            uiItemStyle style;
            style.top = 10;

*/

enum{
    pos_static=0,
    pos_relative,
    pos_fixed,
	// pos_absolute, TODO(sushi) decide if this is useful to implement
    pos_sticky,
};

struct uiItemStyle{
    Type  positioning;  
    union{
        struct{u32 top, left;};
        vec2i tl;
        struct{f32 ptop, pleft;};
        vec2 ptl;
    };
    union{
        struct{u32 bottom, right;};
        vec2i br;
    };
    vec2i margin;        
    vec2i padding;       
    vec2  content_align; 
	
};

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
	
    uiWindow(){} //C++ is retarded
};
#define uiWindowFromNode(x) CastFromMember(uiWindow, item, CastFromMember(uiItem, node, x))

// makes a ui window that stores ui items 
// this cannot be a child of anything
//  name: Name for the window.
// flags: a collection of uiWindowFlags to be applied to the window
//  

UI_FUNC_API(uiWindow*, ui_make_window, str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line);
#if DESHI_RELOADABLE_UI
#  define uiWindowM(name, pos, size, flags) g_ui->make_window(STR8(name),(pos),(size),(flags),STR8(__FILE__),__LINE__)
#else
#  define uiWindowM(name, pos, size, flags) ui_make_window(STR8(name),(pos),(size),(flags),STR8(__FILE__),__LINE__)
#endif
#define uiWindowMF(name, pos, size) uiWindowM(name,pos,size,0)

UI_FUNC_API(uiWindow*, ui_begin_window, str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line);
#if DESHI_RELOADABLE_UI
#  define uiWindowB(name, pos, size, flags) g_ui->begin_window(STR8(name),(pos),(size),(flags),STR8(__FILE__),__LINE__)
#else
#  define uiWindowB(name, pos, size, flags) ui_begin_window(STR8(name),(pos),(size),(flags),STR8(__FILE__),__LINE__)
#endif
#define uiWindowBF(name, pos, size) uiWIndowB(name,pos,size,0)

UI_FUNC_API(uiWindow*, ui_end_window);
#if DESHI_RELOADABLE_UI
#  define uiWindowE() g_ui->end_window()
#else
#  define uiWindowE() ui_end_window()
#endif


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
UI_FUNC_API(void, ui_init);
#if DESHI_RELOADABLE_UI
#  define uiInit() g_ui->init()
#else
#  define uiInit() ui_init()
#endif

UI_FUNC_API(void, ui_update);
#if DESHI_RELOADABLE_UI
#  define uiUpdate() g_ui->update()
#else
#  define uiUpdate() ui_update()
#endif

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
	ui_push_f32__sig*          push_f32;
	ui_push_vec2__sig*         push_vec2;
	ui_make_window__sig*       make_window;
	ui_begin_window__sig*      begin_window;
	ui_end_window__sig*        end_window;
	ui_make_child_window__sig* make_child_window;
	ui_make_button__sig*       make_button;
	ui_init__sig*             init;
	ui_update__sig*           update;
#endif //#if DESHI_RELOADABLE_UI
	
	//// state ////
    vec2i nextPos;  // default: -MAx_F32, -MAX_F32; MAX_F32 does nothing
    vec2i nextSize; // default: -MAx_F32, -MAX_F32; MAX_F32 in either indiciates to stretch item to the edge of the window in that direction
    TNode base;
    uiWindow* curwin; //current working window in immediate contexts 
    uiStyle style;
	
	//// memory ////
	ArenaList* item_list;
	ArenaList* drawcmd_list;
	array<uiStyleVarMod>   stack_var;
	array<uiStyleColorMod> stack_color;
};

//global UI pointer
extern uiContext* g_ui;

#endif //DESHI_UI2_H