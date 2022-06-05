#include "core/commands.h"

void init_editor_commands(){
#define CMD_START(name,desc) last_cmd_desc = str8_lit(desc); auto cmd__##name = [](str8* args, u32 arg_count) -> void
#define CMD_END_NO_ARGS(name) ; cmd_add(cmd__##name, str8_lit(#name), last_cmd_desc, 0, 0)
#define CMD_END(name, ...) ; local Type cmd__##name##args[] = {__VA_ARGS__}; cmd_add(cmd__##name, str8_lit(#name), last_cmd_desc, cmd__##name##args, ArrayCount(cmd__##name##args))
#define temp_str8_cstr(s) (const char*)str8_copy(s, deshi_temp_allocator).str //NOTE(delle) this ensures the str8 is null-terminated
	
	str8 last_cmd_desc;
	
	CMD_START(editor_cursor_shape, "Changes the cursor shape to vertical line(0), thick vertical line(1), underline(2), rectangle(3), or filled rectangle(4)"){
		config.cursor_shape = atoi(temp_str8_cstr(args[0]));
	}CMD_END(editor_cursor_shape, CmdArgument_S32);
	
#undef CMD_START
#undef CMD_END_NO_ARGS
#undef CMD_END
}
