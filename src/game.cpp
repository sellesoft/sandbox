//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @board
void init_board(s32 width, s32 height){
	board        = (Tile*)memory_alloc((width*height)*sizeof(Tile));
	board_width  = width;
	board_height = height;
	turn_count   = 0;
	board_area   = width*height;
}

void deinit_board(){
	memzfree(board);
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
	UI::Text(str8l(  "(Escape) Open Menu"
				   "\n(Enter)  Skip Turn"));
	UI::SetWinCursor(UI::GetLastItemPos() + vec2{UI::GetLastItemSize().x + 2.f,0});
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
	UI::Text(str8l(  "(b) Place Bomb"
				   "\n(B) Detonate Bomb"
				   "\n(l) Build Ladder"
				   "\n(p) Build Pillar"));
	UI::SetWinCursor(UI::GetLastItemPos() + vec2{UI::GetLastItemSize().x + 2.f,0});
	UI::TextF(str8_lit("Turn Count: %d\nPlayer Turn: %s\nLast Action: %s at (%d,%d)"), turn_count, (turn_count % 2 == 0) ? "British" : "German", (char*)MessageStrings[last_action].str, last_action_x, last_action_y);
	
	//draw board
	forX(y, board_height){
		forX(x, board_width){
			vec2 tile_pos = vec2(x*tile_width, DeshWindow->cheight - (y+1)*tile_height);
			
			//draw tile background
			switch(TileAt(x,y).bg){
				case TileBG_Dirt:{
					UI::RectFilled(tile_pos, tile_dims, color(70,37,33));
					UI::Text(str8l("Dirt"), tile_pos+vec2::ONE, UITextFlags_NoWrap);
				}break;
				case TileBG_Tunnel:{
					UI::RectFilled(tile_pos, tile_dims, color(150,112,91));
					UI::Text(str8l("Tunnel"), tile_pos+vec2::ONE, UITextFlags_NoWrap);
				}break;
				/*case TileBG_Trench:{
					UI::Text(str8l("Trench"), tile_pos+vec2::ONE, UITextFlags_NoWrap);
				}break;*/
				case TileBG_BritishBase:{
					UI::RectFilled(tile_pos, tile_dims, Color_Grey);
					UI::RectFilled(tile_pos, tile_dims - tile_dims.yComp()*.75f, Color_DarkBlue);
					UI::Text(str8l("Base (B)"), tile_pos+vec2::ONE, UITextFlags_NoWrap);
				}break;
				case TileBG_GermanBase:{
					UI::RectFilled(tile_pos, tile_dims, Color_Grey);
					UI::RectFilled(tile_pos, tile_dims - tile_dims.yComp()*.75f, Color_DarkRed);
					UI::Text(str8l("Base (G)"), tile_pos+vec2::ONE, UITextFlags_NoWrap);
				}break;
				case TileBG_Sky:{
					UI::Text(str8l("Sky"), tile_pos+vec2::ONE, UITextFlags_NoWrap);
				}break;
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
	
	player0 = {1,             board_height-5, 3, TileFG_BritishPlayer, TileFG_BritishBomb,  array<u32>(deshi_allocator)};
	TileAt(0,board_height-1).bg = TileBG_BritishBase;
	for(int i = 1; i < board_width-1; ++i) TileAt(i,board_height-1).bg = TileBG_Sky;
	TileAt(0,board_height-2).bg = TileBG_Tunnel;
	TileAt(0,board_height-2).fg = TileFG_Ladder;
	TileAt(0,board_height-3).bg = TileBG_Tunnel;
	TileAt(0,board_height-3).fg = TileFG_Ladder;
	TileAt(0,board_height-4).bg = TileBG_Tunnel;
	TileAt(0,board_height-4).fg = TileFG_Ladder;
	TileAt(0,board_height-5).bg = TileBG_Tunnel;
	TileAt(0,board_height-5).fg = TileFG_Ladder;
	TileAt(player0.x,player0.y).bg = TileBG_Tunnel;
	TileAt(player0.x,player0.y).fg = TileFG_BritishPlayer;
	
	player1 = {board_width-2, board_height-5, 3, TileFG_GermanPlayer,  TileFG_GermanBomb, array<u32>(deshi_allocator)};
	TileAt(board_width-1,board_height-1).bg = TileBG_GermanBase;
	TileAt(board_width-1,board_height-2).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-2).fg = TileFG_Ladder;
	TileAt(board_width-1,board_height-3).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-3).fg = TileFG_Ladder;
	TileAt(board_width-1,board_height-4).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-4).fg = TileFG_Ladder;
	TileAt(board_width-1,board_height-5).bg = TileBG_Tunnel;
	TileAt(board_width-1,board_height-5).fg = TileFG_Ladder;
	TileAt(player1.x,player1.y).bg = TileBG_Tunnel;
	TileAt(player1.x,player1.y).fg = TileFG_GermanPlayer;
	
	last_action = Message_None;
	last_action_x = 0;
	last_action_y = 0;
}

void update_game(){
	Player* player;
	Player* other_player;
#define TileAtPlayer() TileAt(player->x,player->y)
#define TileNearPlayer(_x,_y) TileAt(player->x+_x,player->y+_y)
#define TileAtOtherPlayer() TileAt(player->x,player->y)
#define TileNearOtherPlayer(_x,_y) TileAt(player->x+_x,player->y+_y)
	
	//debug
	if(key_pressed(Key_ENTER | InputMod_AnyShift)){
		turn_count += 1;
	}
	
	u32 action_performed = Message_None;
	if(turn_count % 2 == player_idx){
		player = (player_idx) ? &player1 : &player0;
		other_player = (player_idx) ? &player0 : &player1;
		
		//// moving input ////
		if      (   key_pressed(Key_UP    | InputMod_None)
				 && player->y < board_height-1 && TileNearPlayer(0, 1).bg == TileBG_Tunnel
				 && HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
			action_performed = Message_MoveUp;
		}else if(   key_pressed(Key_DOWN  | InputMod_None)
				 && player->y > 0              && TileNearPlayer(0,-1).bg == TileBG_Tunnel
				 && HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
			action_performed = Message_MoveDown;
		}else if(   key_pressed(Key_RIGHT | InputMod_None)
				 && player->x < board_width-1  && TileNearPlayer( 1,0).bg == TileBG_Tunnel){
			action_performed = Message_MoveRight;
		}else if(   key_pressed(Key_LEFT  | InputMod_None)
				 && player->x > 0              && TileNearPlayer(-1,0).bg == TileBG_Tunnel){
			action_performed = Message_MoveLeft;
		}
		
		//// digging input ////
		if      (   key_pressed(Key_UP    | InputMod_AnyShift)
				 && player->y < board_height-1 && TileNearPlayer(0, 1).bg == TileBG_Dirt){
			action_performed = Message_DigUp;
		}else if(   key_pressed(Key_DOWN  | InputMod_AnyShift)
				 && player->y > 0              && TileNearPlayer(0,-1).bg == TileBG_Dirt){
			action_performed = Message_DigDown;
		}else if(   key_pressed(Key_RIGHT | InputMod_AnyShift)
				 && player->x < board_width-1  && TileNearPlayer( 1,0).bg == TileBG_Dirt){
			action_performed = Message_DigRight;
		}else if(   key_pressed(Key_LEFT  | InputMod_AnyShift)
				 && player->x > 0              && TileNearPlayer(-1,0).bg == TileBG_Dirt){
			action_performed = Message_DigLeft;
		}
		
		//// building input ////
		if      (   key_pressed(Key_L | InputMod_None)
				 && TileAtPlayer().bg == TileBG_Tunnel
				 && !HasFlag(TileAtPlayer().fg, TileFG_Pillar)){
			action_performed = Message_BuildLadder;
		}else if(   key_pressed(Key_P | InputMod_None)
				 && TileAtPlayer().bg == TileBG_Tunnel
				 && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
			action_performed = Message_BuildPillar;
		}
		
		//// bombing input ////
		if      (   key_pressed(Key_B | InputMod_None)
				 && !HasFlag(TileAtPlayer().fg, TileFG_BritishBomb|TileFG_GermanBomb)
				 && player->bombs > 0){
			action_performed = Message_PlaceBomb;
		}else if(   key_pressed(Key_B | InputMod_AnyShift)
				 && player->placed_bombs.count > 0){
			action_performed = Message_DetonateBomb;
		}
		
		if(action_performed <= Message_MOVES_END && action_performed >= Message_MOVES_START){
			last_action = action_performed;
			last_action_x = player->x;
			last_action_y = player->y;
			
			turn_count += 1;
			turn_info.uid     = player_idx;
			turn_info.x       = player->x;
			turn_info.y       = player->y;
			turn_info.message = action_performed;
			net_client_send(turn_info);
		}
	}else{
		player = (player_idx) ? &player0 : &player1;
		other_player = (player_idx) ? &player1 : &player0;
		
		NetInfo info = listener_latch;
		if(info.magic[0] && info.uid != player_idx){
			action_performed = info.message;
			if(info.message <= Message_MOVES_END && info.message >= Message_MOVES_START){
				last_action = action_performed;
				last_action_x = player->x;
				last_action_y = player->y;
				
				turn_count += 1;
			}
			DeshThreadManager->add_job({&net_worker, 0});
			DeshThreadManager->wake_threads();
		}
	}
	
	//perform action for current player
	switch(action_performed){
		//// moving actions ////
		case Message_MoveUp:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->y += 1;
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_MoveDown:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->y -= 1;
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_MoveRight:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->x += 1;
			while(   TileNearPlayer(0,-1).bg == TileBG_Tunnel
				  && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
				player->y -= 1;
			}
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_MoveLeft:{
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			player->x -= 1;
			while(   TileNearPlayer(0,-1).bg == TileBG_Tunnel
				  && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
				player->y -= 1;
			}
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		
		//// digging actions ////
		case Message_DigUp:{
			TileNearPlayer(0, 1).bg = TileBG_Tunnel;
		}break;
		case Message_DigDown:{
			TileNearPlayer(0,-1).bg = TileBG_Tunnel;
			RemoveFlag(TileAtPlayer().fg, player->player_flag);
			while(   TileNearPlayer(0,-1).bg == TileBG_Tunnel
				  && !HasFlag(TileAtPlayer().fg, TileFG_Ladder)){
				player->y -= 1;
			}
			AddFlag(TileAtPlayer().fg, player->player_flag);
		}break;
		case Message_DigRight:{
			TileNearPlayer( 1,0).bg = TileBG_Tunnel;
		}break;
		case Message_DigLeft:{
			TileNearPlayer(-1,0).bg = TileBG_Tunnel;
		}break;
		
		//// building actions ////
		case Message_BuildLadder:{
			AddFlag(TileAtPlayer().fg, TileFG_Ladder);
		}break;
		case Message_BuildPillar:{
			AddFlag(TileAtPlayer().fg, TileFG_Pillar);
		}break;
		
		//// bombing actions ////
		case Message_PlaceBomb:{
			player->placed_bombs.add(ToLinear(player->x,player->y));
			player->bombs -= 1;
			AddFlag(TileAtPlayer().fg, player->bomb_flag);
		}break;
		case Message_DetonateBomb:{
			forI(player->placed_bombs.count){
				//explode up
				s32 up_tile = player->placed_bombs[i] - board_width;
				if(up_tile >= 0 && up_tile < board_height){
					TileAtLinear(up_tile).bg = TileBG_Tunnel;
					TileAtLinear(up_tile).fg = TileFG_None;
					if(ToLinear(player->x,player->y) == up_tile){
						//TODO player dies
					}
					if(ToLinear(other_player->x,other_player->y) == up_tile){
						//TODO other player dies
					}
				}
				
				//explode down
				s32 down_tile = player->placed_bombs[i] + board_width;
				if(down_tile >= 0 && down_tile < board_height){
					TileAtLinear(down_tile).bg = TileBG_Tunnel;
					TileAtLinear(down_tile).fg = TileFG_None;
					if(ToLinear(player->x,player->y) == down_tile){
						//TODO player dies
					}
					if(ToLinear(other_player->x,other_player->y) == down_tile){
						//TODO other player dies
					}
				}
				
				//explode right
				s32 right_tile = player->placed_bombs[i] + 1;
				if(right_tile >= 0 && right_tile < board_height){
					TileAtLinear(right_tile).bg = TileBG_Tunnel;
					TileAtLinear(right_tile).fg = TileFG_None;
					if(ToLinear(player->x,player->y) == right_tile){
						//TODO player dies
					}
					if(ToLinear(other_player->x,other_player->y) == right_tile){
						//TODO other player dies
					}
				}
				
				//explode left
				s32 left_tile = player->placed_bombs[i] - 1;
				if(left_tile >= 0 && left_tile < board_height){
					TileAtLinear(left_tile).bg = TileBG_Tunnel;
					TileAtLinear(left_tile).fg = TileFG_None;
					if(ToLinear(player->x,player->y) == left_tile){
						//TODO player dies
					}
					if(ToLinear(other_player->x,other_player->y) == left_tile){
						//TODO other player dies
					}
				}
			}
		}break;
		
		//// mode ////
		case Message_QuitGame:{
			deinit_board();
			game_active = 0;
		}break;
	}
	
	//draw things
	draw_board();
}