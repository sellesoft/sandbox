#ifndef TUNLLER_TYPES_H
#define TUNLLER_TYPES_H

enum MoveType{
    Move_Up,
    Move_Down,
    Move_Right,   
    Move_Left,   
};typedef u32 MoveType;

struct Tile{
    u32 texture;
    b32 has_bomb;
};

struct Player{
    u32 health;
    vec2 pos;
    u32 bombs;
};

Tile* board;
u32 board_width;
u32 board_height;

u32 turn_count;

#endif
