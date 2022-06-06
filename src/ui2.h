/*  NOTES

    Everything in this new system is a uiItem.

    Retained UI is stored in arenas while immediate UI is temp allocated

    Any time an item is modified or removed we find it's parent window and recalculate it entirely
        this can probably be optimized
    
    For the reason above, initial generation of a uiItem and it's actual drawinfo generation
        are in separate functions

    UI macros always automatically apply the str8_lit macro to arguments that ask for text

    uiWindow cursors never take into account styling such as padding (or well, not sure yet)

    Everything in the interface is prefixed with "ui" (always lowercase)
        type and macro names follow the prefix and are UpperCamelCase
        function names have a _ after the prefix and are lower_snake_case

    Everything in this system is designed to be as dynamic as possible to enable
    minimal rewriting when it comes to tweaking code. Basically, there should be almost
    no hardcoding of anything and everything that can be abstracted out to a single function
    should be.

*/



typedef void (*Action)(void*);

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

enum uiItemType_{
    uiItemType_Window, 
    uiItemType_ChildWindow,
    uiItemType_Button,
    uiItemType_Text,
    uiItemType_COUNT
};typedef u32 uiItemType;

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

struct primcount{
    u32 n;
    vec2i* counts;
    vec2i* sums;
};

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

//@Button

enum {
    uiButtonFlags_ActOnPressed,  // button performs its action as soon as its clicked
    uiButtonFlags_ActOnReleased, // button waits for the mouse to be released before performing its action
    uiButtonFlags_ActOnDown,     // button performs its action for the duration the mouse is held down over it
};

struct uiButton{
    uiItem item;
    Action action;
    void*  action_data;
    b32    clicked;
};
#define uiButtonFromNode(x) CastFromMember(uiButton, item, CastFromMember(uiItem, node, x))

struct uiText{
    uiItem item;
    str8 text; 
};

//@Context

#define SetNextPos(x,y) uiContext.nextPos = {x,y};
#define SetNextSize(x,y) uiContext.nextSize = {x,y};
struct Context{
    vec2i nextPos;  // default: -MAx_F32, -MAX_F32; MAX_F32 does nothing
    vec2i nextSize; // default: -MAx_F32, -MAX_F32; MAX_F32 in either indiciates to stretch item to the edge of the window in that direction
    TNode base;
    uiWindow* curwin; //current working window in immediate contexts 
    uiStyle style;
}uiContext;

//we cant grow the arena because it will move the memory, so we must chunk 
struct ArenaList{
    Node node;   
    Arena* arena;
};

#define ArenaListFromNode(x) CastFromMember(ArenaList, node, x);

//@Functionality

void ui_push_var(Type idx, f32 nu);

void ui_push_var(Type idx, vec2 nu);

// makes a ui window that stores ui items 
// this cannot be a child of anything
//  name: Name for the window.
// flags: a collection of uiWindowFlags to be applied to the window
//  

uiWindow* ui_make_window(str8 name, Flags flags, str8 file, upt line);
uiWindow* ui_begin_window(str8 name, Flags flags, str8 file, upt line);
uiWindow* ui_end_window();
#define uiWindowM (name, pos, size, flags) ui_make_window(STR8(name),pos,size,flags,STR8(__FILE__),__LINE__)
#define uiWindowMF(name, pos, size) uiWindowM(name,pos,size,0)
#define uiWindowB (name, pos, size, flags) ui_begin_window(STR8(name),pos,size,flags,,STR8(__FILE__),__LINE__)
#define uiWindowBF(name, pos, size) uiWIndowB(name,pos,size,0)
#define uiWindowE() ui_end_window()


// makes a child window inside another window
//
// window: uiWindow to emplace this window in
//   name: name of the window
//    pos: initial position of the window
//   size: initial size of the window
uiWindow* ui_make_child_window(uiWindow* window, str8 name, vec2i size, Flags flags, str8 file, upt line);


//      window: uiWindow to emplace the button in
//        text: text to be displayed in the button
//      action: function pointer of type Action ( void name(void*) ) that is called when the button is clicked
//              if 0 is passed, the button will just set it's clicked flag
// action_data: data passed to the action function 
//       flags: uiButtonFlags to be given to the button
#define uiMakeButton(window, text, action, action_data, flags) ui_make_button(window, STR8(text), action, action_data, flags, STR8(__FILE__), __LINE__)
uiButton* ui_make_button(uiWindow* window, Action action, void* action_data, Flags flags, str8 file, upt line);

//same as a div in HTML, just a section that items will place themselves in
//TODO(sushi) void ui_make_section(vec2i pos, vec2i size);

void ui_init();

void ui_update();