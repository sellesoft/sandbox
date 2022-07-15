#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif

//// kigu includes ////
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/file.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

//// nasau includes ////
#include "hardware.h"
#include "assembler.h"
#define nbits(n) ((u64(1) << u64(n)) - u64(1)) 
#define MchnInst(ptr, bits) *ptr=bits;ptr+=1
//helps write programs 
//pretty much ASM but in code form
struct progwrite{
	machine* mchn = 0;
	u64* mem = 0;

    void NOP(){
		u64 instr = 0b000000;
		*mem = instr;
		mem++;
	}

	void ADD(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000001;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void SUB(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000010;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void MUL(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000011;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void SMUL(u64 DR, u64 SR1, u64 immcond, s64 S){
		u64 instr = 0b000100;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void DIV(u64 QR, u64 RR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000101;
		instr = (instr << 4) | (QR & nbits(4));
		instr = (instr << 4) | (RR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));

		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 45) | (S & nbits(45));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 41;
		}
		*mem = instr;
		mem++;
	}

	void SDIV(u64 QR, u64 RR, u64 SR1, u64 immcond, s64 S){
		u64 instr = 0b000110;
		u64 Su = *(u64*)S;
		instr = (instr << 4) | (QR & nbits(4));
		instr = (instr << 4) | (RR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 45) | (Su & nbits(45));

		}
		else{
			instr = (instr << 5) | (Su & nbits(4));
			instr <<= 41;
		}
		*mem = instr;
		mem++;
	}

	void AND(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000111;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void OR(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b001000;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void XOR(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b001001;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void SHR(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b001010;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void SHL(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b001011;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		if(immcond){
			instr = (instr << 1) | 1;
			instr = (instr << 49) | (S & nbits(49));
		}
		else{
			instr = (instr << 5) | (S & nbits(4));
			instr <<= 45;
		}
		*mem = instr;
		mem++;
	}

	void NOT(u64 DR, u64 SR1){
		u64 instr = 0b001100;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		*mem = instr << 50;
		mem++;
	}

    void JMP(u64 SR1){
        u64 instr = 0b001101;
        instr = (instr << 4) | (SR1 & nbits(4));
        *mem = instr << 54;
        mem++;
    }

    void MOV(u64 DR, u64 immcond, u64 S){
        u64 instr = 0b001110;
        instr = (instr << 4) | (DR & nbits(4));
        if(immcond) instr = (((instr << 1) | 1) << 53) | (S & nbits(53));
        else instr = ((instr << 5) | (S & nbits(4))) << 49;
        *mem = instr;
        mem++;
    }
};


int main(){
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();
	window_create(str8l("suugu"));
	console_init();
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	window_show(DeshWindow);
	render_use_default_camera();
	uiInit(0,0);
	DeshThreadManager->init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
	
	assembler asmblr;
	array<u64> program = asmblr.assemble(STR8("asm/set_half_kilobyte.a"));
	
	machine mchn;
	mchn.PC_START = 0;
	memcpy(mchn.mem + mchn.PC_START, program.data, program.count * sizeof(u64));
	mchn.turnon();

	uiItem* blocks[1024] = {0};
	u64 inc = 2;
#define viewsize 
	uiItem* win = uiItemB();
		win->style.background_color = color(14,14,14);
		win->style.border_style = border_solid;
		win->style.border_color = color(188,188,188);
		win->style.border_width = 1;
		win->style.padding = {5,5,5,5};
		win->style.sizing = size_percent;
		win->style.size = {100,100};
		forI(1024){
			blocks[i] = uiItemM();
			blocks[i]->style.positioning = pos_relative;
			blocks[i]->style.size = {10,10};
		}
	uiItemE();

	//start main loop
	while(platform_update()){DPZoneScoped;
		if(mchn.ureg(0)){
			mchn.tick();
		}else if(key_pressed(Key_SPACE)){
			mchn.ureg(0) = inc++;
		}

		vec2 cursor = {0,0};
		forI(1024){
			blocks[i]->style.pos = cursor;
			u64 val = *(u64*)mchn.mem + i;
			blocks[i]->style.background_color = color(val/5.f*255,val/5.f*255,val/5.f*255);
			cursor.x += 10;
			if(cursor.x > PaddedWidth(win)){
				cursor.x = 0;
				cursor.y += 10;
			}
			if(cursor.y > PaddedHeight(win)) break;
		}

		uiUpdate();
		console_update();
		UI::Update();
		render_update();
		memory_clear_temp();
	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}