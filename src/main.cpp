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
    pw.MOV(R_R0, 1, 1);
    pw.MOV(R_R1, 1, 1);
    pw.MOV(R_R3, 1, 3);
    pw.ADD(R_R2, R_R1, 0, R_R0);
    pw.MOV(R_R0, 0, R_R1);
    pw.MOV(R_R1, 0, R_R2);
    pw.JMP(R_R3);

    pc.turnon();
    
	while(!DeshWindow->ShouldClose()){DPZoneScoped;
		DeshWindow->Update();
		{//update debug
			using namespace UI;
			SetNextWindowPos(0,0);
			SetNextWindowSize(DeshWinSize);
			Begin("MachineDebug");{
            #define regc "%"
				persist u32 running = false;
                persist f32 throttle = 500; //time between ticks when running 
                persist Stopwatch throttle_watch = start_stopwatch();
                if(running && peek_stopwatch(throttle_watch) > throttle) reset_stopwatch(&throttle_watch), pc.tick();
                if(!running && Button("tick")) pc.tick();
                if(Button("reset")){ pc.ureg(R_PC) = pc.PC_START; memset(pc.reg, 0, sizeof(Reg)*R_COUNT); }
                if(Button("run")) running = true;
                if(Button("stop")) running = false; 
                if(running) Slider("throttle", &throttle, 0, 1000);
				
				static u64 offset = 0;
				if(key_pressed(Key_UP)) (offset ? offset-- : 0);
				if(key_pressed(Key_DOWN)) offset++;
				SetNextWindowSize((GetMarginedRight()-GetMarginedLeft()) * 0.5, MAX_F32);
				BeginChild("SourceMenu", vec2::ZERO);{
					for(u64 instr = pc.PC_START+offset; instr < 30+offset; instr++){
						//Selectable(getbits<64>(&pc.mem[instr]).str, instr==pc.ureg(R_PC));
                        if(pc.ureg(R_PC) == instr){
                            RectFilled(vec2(0, GetWinCursor().y), vec2(MAX_F32, GetStyle().fontHeight), color(100,50,40));
                        }
						BeginRow("instralign", 5, GetStyle().fontHeight, UIRowFlags_AutoSize); 
						RowSetupColumnAlignments({{0,1},{1,1},{1,1},{0,1},{0,1}});
						InstrRead read = ReadInstr(pc.mem[instr]);
						switch(read.OP){
                            case OP_NOP:{
                                Text("nop ");
                                Text(" ");
                                Text(" ");
                                Text(" ");
                                Text(" ");
                            }break;
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
                            case OP_SDIV:{
                                Text("sdiv "); 
                                Text(toStr(regc, read.QR, " ")); 
                                Text(toStr(regc, read.RR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2)));
                            }break;
                            case OP_AND:{
								Text("and "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
                            case OP_OR:{
								Text("or "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
                            case OP_XOR:{
								Text("xor "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
                            case OP_SHR:{
								Text("shr "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
                            case OP_SHL:{
								Text("shl "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR2))); 
                                Text(" ");
							}break;
                            case OP_NOT:{
								Text("not "); 
                                Text(toStr(regc, read.DR, " ")); 
                                Text(toStr(regc, read.SR1, " ")); 
                                Text(" ");
                                Text(" ");
							}break;
                            case OP_JMP:{
								Text("jmp "); 
                                Text(toStr(regc, read.SR1, " ")); 
                                Text(" ");
                                Text(" ");
                                Text(" ");
							}break;
                            case OP_MOV:{
                                Text("mov ");
                                Text(toStr(regc, read.DR, " "));
                                (read.immcond ? Text(toStr(read.immval)) : Text(toStr(regc, read.SR1)));
                                Text(" ");
                                Text(" ");
                            }break;
						}
						EndRow();
					}
				}EndChild();
                vec2 lpos = GetLastItem()->position;
                vec2 lsiz = GetLastItem()->size;
                SetNextWindowSize(MAX_F32, MAX_F32);
                SetWinCursor({lpos.x+lsiz.x,lpos.y});
                BeginChild("registers", vec2::ZERO);{
                    BeginRow("regbuttonalign", 4, 0, UIRowFlags_AutoSize);
                    RowSetupColumnAlignments({{0,0},{0,0},{0,0},{0,0}});
                    static u64 state[R_COUNT]={0};//0 u64; 1 s64; 2 f64;
                    forI(R_COUNT){
                        switch(state[i]){
                            case 0:{Text(toStr(regc, i, ": ", pc.ureg(i)));}break;
                            case 1:{Text(toStr(regc, i, ": ", pc.sreg(i)));}break;
                            case 2:{Text(toStr(regc, i, ": ", pc.freg(i)));}break;
                        }
                        if(Button((state[i] == 0 ? "o unsigned" : "unsigned"))) state[i] = 0;
                        if(Button((state[i] == 1 ? "o signed" : "signed")))   state[i] = 1;
                        if(Button((state[i] == 2 ? "o float" : "float")))    state[i] = 2;
                    }
                    EndRow();
                }EndChild();
			}End();
		}
		UI::ShowMetricsWindow();
		UI::Update();
		render_update();
		memory_clear_temp();
	}
}