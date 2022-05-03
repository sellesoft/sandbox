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
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

#if BUILD_INTERNAL
#include "misc_testing.cpp"
#endif

//// nasau includes ////
#include "hardware.h"
//#include "machine.cpp"
#define nbits(n) ((u64(1) << u64(n)) - u64(1)) 
#define MchnInst(ptr, bits) *ptr=bits;ptr+=1
//helps write programs 
//pretty much ASM but in code form
struct progwrite{
	machine* mchn = 0;
	u64* mem = 0;

	void ADD(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000000;
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

	void MUL(u64 DR, u64 SR1, u64 immcond, u64 S){
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

	void SMUL(u64 DR, u64 SR1, u64 immcond, s64 S){
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

	void DIV(u64 QR, u64 RR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b000100;
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
		u64 instr = 0b000101;
		u64 Su = ConversionlessCast(u64, S);
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
		u64 instr = 0b000110;
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

	void XOR(u64 DR, u64 SR1, u64 immcond, u64 S){
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

	void SHR(u64 DR, u64 SR1, u64 immcond, u64 S){
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

	void SHL(u64 DR, u64 SR1, u64 immcond, u64 S){
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

	void NOT(u64 DR, u64 SR1, u64 immcond, u64 S){
		u64 instr = 0b001011;
		instr = (instr << 4) | (DR & nbits(4));
		instr = (instr << 4) | (SR1 & nbits(4));
		*mem = instr;
		mem++;
	}
};


int main(){
	//init deshi
	memory_init(Gigabytes(1), Gigabytes(1));
	logger_init();
	console_init();
	DeshWindow->Init("sandbox", 1280, 720);
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	DeshWindow->ShowWindow();
    render_use_default_camera();
	DeshThreadManager->init();

	machine pc;
	progwrite pw;
	pw.mchn = &pc;
	pw.mem = (u64*)pc.mem + pc.PC_START;
	pw.ADD(R_R0,R_R0,1,10);     	// add  10  to  r0 -> r0    | r0: 10
	pw.ADD(R_R1,R_R1,1,50);     	// add  50  to  r1 -> r1    | r0: 10, r1: 50
	pw.SUB(R_R2,R_R0,0,R_R1);   	// sub  r1 from r0 -> r2    | r0: 10, r1: 50, r2: -40
	pw.MUL(R_R3,R_R2,1,2);     		// mul  r2  by   2 -> r3    | r0: 10, r1: 50, r2: -40 r3: -80
	pw.SMUL(R_R4,R_R3,1,-1);        // imul r3  by  -1 -> r4    | r0: 10, r1: 50, r2: -40 r3: -80 r4: 80
	pw.DIV(R_R5,R_R6,R_R4,1,3);     // div  r4  by   3 -> r5,r6 | r0: 10, r1: 50, r2: -40 r3: -80 r4: 20 r5: 26, r6: 2
	pw.SDIV(R_R7,R_R8,R_R6,1,-1);   // idiv r6  by  -1 -> r7,r8 | r0: 10, r1: 50, r2: -40 r3: -80 r4: 20 r5: 26, r6: 2 r7: -2 r8: 0
	pw.OR(R_R0, R_R0, 1, 0xFF);

	pc.turnon();
    
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		DeshWindow->Update();
		{//update debug
			using namespace UI;
			SetNextWindowPos(0,0);
			SetNextWindowSize(DeshWinSize);
			Begin("MachineDebug");{
            #define regc "%"
				if(Button("tick")) pc.tick();
				if(Button("reset")) pc.ureg(R_PC) = pc.PC_START;
				
				static u64 offset = 0;
				if(key_pressed(Key_UP)) (offset ? offset-- : 0);
				if(key_pressed(Key_DOWN)) offset++;
				SetNextWindowSize(MAX_F32, MAX_F32);
				BeginChild("SourceMenu", vec2::ZERO);{
					for(u64 instr = pc.PC_START; instr < 30+offset; instr++){
						//Selectable(getbits<64>(&pc.mem[instr]).str, instr==pc.ureg(R_PC));
						BeginRow("instralign", 5, GetStyle().fontHeight, UIRowFlags_AutoSize); 
						RowSetupColumnAlignments({{0,1},{1,1},{1,1},{0,1},{0,1}});
						InstrRead read = ReadInstr(pc.mem[instr]);
						switch(read.OP){
							case OP_ADD:{
								Text("add "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2)));
                                Text(" ");
							}break;	
							case OP_SUB:{
								Text("sub "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
							case OP_MUL:{
								Text("mul "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
							case OP_SMUL:{
								Text("smul "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(ConversionlessCast(s64,read.immval))) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
							case OP_DIV:{
								Text("div "); 
                                Text(toStr(regc, read.QR, " ")); 
                                Text(toStr(regc, read.RR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2)));
							}break;
						}
						EndRow();
					}
				}EndChild();
			}End();
		}
		UI::ShowMetricsWindow();
		UI::Update();
		render_update();
		memory_clear_temp();
	}
}