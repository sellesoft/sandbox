//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @board
#define LinearRow(pos) ((pos)/board_width)
#define LinearCol(pos) ((pos)%board_width)
#define ToLinear(row,col) ((board_width*(row))+(col))
#define TileAt(row,col) board[ToLinear(row,col)]
#define TileAtLinear(pos) board[pos]

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
	UI::PushLayer(3);
	UI::PushColor(UIStyleCol_WindowBg, Color_LightBlue);
	UI::SetNextWindowSize(DeshWindow->dimensions);
	UI::Begin(str8l("tunller_board"), vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_NoInteract);
	UIStyle style = UI::GetStyle();
	
	//draw controls
	UI::SetWinCursor(vec2{2.f,2.f});
	UI::Text(str8l(  "(Up)     Move Up"
				   "\n(Down)   Move Down"
				   "\n(Right)  Move Right"
				   "\n(Left)   Move Left"));
	UI::SetWinCursor(UI::GetLastItemPos() + vec2{UI::GetLastItemSize().x + 2.f,0});
	UI::Text(str8l(  "(Shift Up)     Dig Up"
				   "\n(Shift Down)   Dig Down"
				   "\n(Shift Right)  Dig Right"
				   "\n(Shift Left)   Dig Left"));
	UI::SetWinCursor(UI::GetLastItemPos() + vec2{UI::GetLastItemSize().x + 2.f,0});
	UI::Text(str8l(  "(b)  Place Bomb"
				   "\n(B)  Detonate Bomb"
				   "\n(l)  Place Ladder"));
	
	//draw board
	forX(row, board_height){
		forX(col, board_width){
			vec2 tile_pos = vec2(col*tile_width, DeshWindow->cheight - (row+1)*tile_height);
			
			//draw tile background
			switch(TileAt(row,col).bg){
				case TextureBG_Dirt:    UI::Text(str8l("Dirt"),    tile_pos, UITextFlags_NoWrap); break;
				case TextureBG_Surface: UI::Text(str8l("Surface"), tile_pos, UITextFlags_NoWrap); break;
				case TextureBG_Sky:     UI::Text(str8l("Sky"),     tile_pos, UITextFlags_NoWrap); break;
				case TextureBG_Trench:  UI::Text(str8l("Trench"),  tile_pos, UITextFlags_NoWrap); break;
				case TextureBG_Fort:    UI::Text(str8l("Fort"),    tile_pos, UITextFlags_NoWrap); break;
				case TextureBG_Tunnel:  UI::Text(str8l("Tunnel"),  tile_pos, UITextFlags_NoWrap); break;
			}
			
			//draw tile foreground
			switch(TileAt(row,col).fg){
				case TextureFG_BritishPlayer: UI::Text(str8l("British"),   tile_pos + vec2{0,style.fontHeight}, UITextFlags_NoWrap); break;
				case TextureFG_GermanPlayer:  UI::Text(str8l("German"),    tile_pos + vec2{0,style.fontHeight}, UITextFlags_NoWrap); break;
				case TextureFG_Pillar:        UI::Text(str8l("Pillar"),    tile_pos + vec2{0,style.fontHeight}, UITextFlags_NoWrap); break;
				case TextureFG_Explosive:     UI::Text(str8l("Explosive"), tile_pos + vec2{0,style.fontHeight}, UITextFlags_NoWrap); break;
				case TextureFG_Sandbags:      UI::Text(str8l("Sandbag"),   tile_pos + vec2{0,style.fontHeight}, UITextFlags_NoWrap); break;
				case TextureFG_Ladder:      UI::Text(str8l("Ladder"),    tile_pos + vec2{0,style.fontHeight}, UITextFlags_NoWrap); break;
			}
			
			//draw tile border
			UI::Rect(tile_pos, tile_dims, Color_Black);
		}
	}
	UI::End();
	UI::PopColor(1);
	UI::PopLayer(1);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @game
void init_game(){
	init_board(20, 10);
	player_idx = 0;
	turn_count = 1;
	player0.pos    = vec2{0,(f32)(board_height-1)};
	player0.health = 100;
	player0.bombs  = 3;
	player1.pos    = vec2{(f32)(board_width-1),(f32)(board_height-1)};
	player1.health = 100;
	player1.bombs  = 3;
}

void update_game(){
	b32 our_turn = (turn_count % 2 == player_idx);
	u32 action_performed = Action_None;
	
	//player input
	if(our_turn){
		if      (key_pressed(Key_UP    | InputMod_None)){ //move up
			action_performed = Action_MoveUp;
		}else if(key_pressed(Key_DOWN  | InputMod_None)){ //move down
			action_performed = Action_MoveDown;
		}else if(key_pressed(Key_RIGHT | InputMod_None)){ //move right
			action_performed = Action_MoveRight;
		}else if(key_pressed(Key_LEFT  | InputMod_None)){ //move left
			action_performed = Action_MoveLeft;
		}
	}
	
	if(action_performed != Action_None){
		turn_count += 1;
	}
	
	//debug
	if(key_pressed(Key_ENTER)){
		turn_count += 1;
	}
	
	//draw things
	draw_board();
}