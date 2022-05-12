#define LinearRow(pos) ((pos)/board_width)
#define LinearCol(pos) ((pos)%board_width)
#define ToLinear(row,col) ((board_width*(row))+(col))
#define TileAt(row,col) board[ToLinear(row,col)]
#define TileAtLinear(pos) board[pos]

u32 board_area;

void init_board(u32 width, u32 height){
	board        = (Tile*)memory_alloc((width*height)*sizeof(Tile));
	board_width  = width;
	board_height = height;
	turn_count   = 0;
	board_area   = width*height;
}

void draw_board(){
	f32 tile_width  = (f32)DeshWindow->width / (f32)board_width;
	f32 tile_height = tile_width;
	vec2 tile_dims(tile_width, tile_height);
	UI::PushColor(UIStyleCol_WindowBg, Color_LightBlue);
	UI::Begin(str8_lit("tunller_board"), vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_NoInteract);
	forX(row, board_height){
		forX(col, board_width){
			vec2 tile_pos = vec2(col*tile_width, DeshWindow->cheight - (row+1)*tile_height);
			//UI::RectFilled();
			UI::Rect(tile_pos, tile_dims, Color_Black);
		}
	}
	UI::End();
	UI::PopColor();
}