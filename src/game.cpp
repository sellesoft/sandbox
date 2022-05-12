//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @board
void init_board(s32 width, s32 height){
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
	forX(y, board_height){
		forX(x, board_width){
			vec2 tile_pos = vec2(x*tile_width, DeshWindow->cheight - (y+1)*tile_height);
			
			//draw tile background
			switch(TileAt(x,y).bg){
				case TileBG_Dirt:    UI::Text(str8l("Dirt"),    tile_pos, UITextFlags_NoWrap); break;
				case TileBG_Surface: UI::Text(str8l("Surface"), tile_pos, UITextFlags_NoWrap); break;
				case TileBG_Sky:     UI::Text(str8l("Sky"),     tile_pos, UITextFlags_NoWrap); break;
				case TileBG_Trench:  UI::Text(str8l("Trench"),  tile_pos, UITextFlags_NoWrap); break;
				case TileBG_Fort:    UI::Text(str8l("Fort"),    tile_pos, UITextFlags_NoWrap); break;
				case TileBG_Tunnel:  UI::Text(str8l("Tunnel"),  tile_pos, UITextFlags_NoWrap); break;
			}
			
			//draw foreground structures
			if      (HasFlag(TileAt(x,y).fg, TileFG_Ladder)){
				UI::Text(str8l("Ladder"), UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
			}else if(HasFlag(TileAt(x,y).fg, TileFG_Pillar)){
				UI::Text(str8l("Pillar"), UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
			}
			
			//draw foreground bombs
			if      (HasFlag(TileAt(x,y).fg, TileFG_BritishBomb)){
				UI::Text(str8l("Bomb (B)"), UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
			}else if(HasFlag(TileAt(x,y).fg, TileFG_GermanBomb)){
				UI::Text(str8l("Bomb (G)"), UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
			}else if(HasFlag(TileAt(x,y).fg, TileFG_BombWire)){
				UI::Text(str8l("Wire"),     UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
			}
			
			//draw foreground players
			if      (HasFlag(TileAt(x,y).fg, TileFG_BritishPlayer)){
				UI::Text(str8l("Player (B)"), UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
			}else if(HasFlag(TileAt(x,y).fg, TileFG_GermanPlayer)){
				UI::Text(str8l("Player (G)"), UI::GetLastItemPos()+UI::GetLastItemSize().yComp(), UITextFlags_NoWrap);
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
	turn_count = 0;
	player0 = {0,             board_height-1, 3};
	TileAt(player0.x,player0.y).bg = TileBG_Trench;
	TileAt(player0.x,player0.y).fg = TileFG_BritishPlayer;
	player1 = {board_width-1, board_height-1, 3};
	TileAt(player1.x,player1.y).bg = TileBG_Trench;
	TileAt(player1.x,player1.y).fg = TileFG_GermanPlayer;
}

void update_game(){
	b32 our_turn = (turn_count % 2 == player_idx);
	u32 action_performed = Message_None;
	
	//player input
	if(our_turn){
		if      (key_pressed(Key_UP    | InputMod_None)){ //move up
			if(player0.y < board_height-1){
				action_performed = Message_MoveUp;
				RemoveFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
				player0.y += 1;
				AddFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
			}
		}else if(key_pressed(Key_DOWN  | InputMod_None)){ //move down
			if(player0.y > 0){
				action_performed = Message_MoveDown;
				RemoveFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
				player0.y -= 1;
				AddFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
			}
		}else if(key_pressed(Key_RIGHT | InputMod_None)){ //move right
			if(player0.y < board_width-1){
				action_performed = Message_MoveRight;
				RemoveFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
				player0.x += 1;
				AddFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
			}
		}else if(key_pressed(Key_LEFT  | InputMod_None)){ //move left
			if(player0.y > 0){
				action_performed = Message_MoveLeft;
				RemoveFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
				player0.x -= 1;
				AddFlag(TileAt(player0.x,player0.y).fg, TileFG_BritishPlayer);
			}
		}
	}
	
	if(action_performed != Message_None){
		turn_count += 1;
		turn_info.uid = player_idx;
		turn_info.x = player0.x;
		turn_info.y = player0.y;
		turn_info.message = action_performed;
		net_client_send(turn_info);
	}
	
	//debug
	if(key_pressed(Key_ENTER)){
		turn_count += 1;
	}
	
	//draw things
	draw_board();
}