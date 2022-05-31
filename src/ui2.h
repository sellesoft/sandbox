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

#define uiMakeWindow(name, pos, size, flags) ui_make_window(STR8(name),pos,size,flags,STR8(__FILE__),__LINE__)
uiWindow* ui_make_window(str8 name, vec2i pos, vec2i size, Flags flags, str8 file, upt line);

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