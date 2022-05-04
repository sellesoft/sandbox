#ifndef NASAU_HARDWARE_H
#define NASAU_HARDWARE_H

#include "kigu/common.h"
#include "kigu/array.h"
#include "kigu/unicode.h"
#include "core/logger.h"

//make these better later
template<int n>
void printbits(void* in){
    u8* inu8 = (u8*)in;
    char bits[n+1]; bits[n] = 0;
    forI(n){
        if((*(inu8 + i / 8)>>i)&0x1) bits[n-1-i] = '1';
        else bits[n-1-i] = '0';
    }
    Log("", bits);
}

template<int n>
string getbits(void* in){
    u8* inu8 = (u8*)in;
    char bits[n+1]; bits[n] = 0;
    forI(n){
        if((*(inu8 + i / 8)>>i)&0x1) bits[n-1-i] = '1';
        else bits[n-1-i] = '0';
    }
    return bits;
}

void printbits(u8 in){
    char bits[9]; bits[8] = 0;
    forI(16){
        if((in >> i) & 0x1) bits[7-i] = '1';
        else bits[7-i] = '0';
    }
    Log("", bits);
}

void printbits(u16 in){
    char bits[17]; bits[16] = 0;
    forI(16){
        if((in >> i) & 0x1) bits[15-i] = '1';
        else bits[15-i] = '0';
    }
    Log("", bits);
}

void printbits(u32 in){
    char bits[33]; bits[32] = 0;
    forI(32){
        if((in >> i) & 0x1) bits[31-i] = '1';
        else bits[31-i] = '0';
    }
    Log("", bits);
}

void printbits(u64 in){
    char bits[65]; bits[64] = 0;
    forI(64){
        if((in >> i) & 0x1) bits[63-i] = '1';
        else bits[63-i] = '0';
    }
    Log("", bits);
}

enum hardware_type : u32 {
    HT_memory,
    HT_cpu,
    HT_bus,
};

/*


*/

struct hardware{
    u64 id;
    cstring name;
    hardware_type type;

};

struct memory : public hardware {
    u8* mem = 0;
    u64 size=0;
};

union Reg {
    u64 u;
    s64 s;
    f64 f;
};
#define ureg(x) reg[x].u
#define sreg(x) reg[x].s
#define freg(x) reg[x].f
    

enum : u32{
    R_R0 = 0, //16 64 bit data registers
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_R8, 
    R_R9,
    R_R10,
    R_R11,
    R_R12,
    R_R13,
    R_R14,
    R_R15,

    R_PC,    // program counter
    R_S,     // stack pointer
    R_COND,  // condition flags
    
    R_COUNT, 

    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2,

    OP_NOP = 0,
    OP_ADD,     //
    OP_SUB,     //
    OP_MUL,     //
    OP_SMUL,    //
    OP_DIV,     //
    OP_SDIV,    //
    OP_AND,     //
    OP_OR,      //
    OP_XOR,     //
    OP_SHR,     //
    OP_SHL,     //
    OP_NOT,     //
    OP_JMP,     //
    OP_MOV,     //
    //OP_LD,      //
    //OP_ST,      //
    //OP_JSR,     //
    //OP_LDR,     //
    //OP_STR,     //
    //OP_RTI,     //
    //OP_LDI,     //
    //OP_STI,     //
    //OP_RES,     //
    //OP_LEA,     //
    OP_COUNT,

    Destination=0, 
    Source,
    ImmediateMode,
    OpcodeDependent, //bits left to the opcode to eval itself
    Unused,
};

static const str8 opnames[] = {
    str8l("nop"),
    str8l("add"), 
    str8l("sub"), 
    str8l("mul"), 
    str8l("smul"), 
    str8l("div"), 
    str8l("sdiv"), 
    str8l("and"), 
    str8l("or"), 
    str8l("xor"), 
    str8l("shr"), 
    str8l("shl"), 
    str8l("not"), 
    str8l("jmp"), 
    str8l("mov"), 
};

//come back to this later

struct oppart{
    u8 type = Unused;
    u8 nbits = 0;
}; 

struct operation{
    void* ptr;
    u32  args; //number of args after the normal reg 
};

struct opcode{
    cstring name;
    operation op;
    


};


/*


    |  ****  |
    |  bits  |
    |54    34|
     ^      ^
     |      |
     |      starting bit
     end bit 
    bits = readbits(34, 20);

---------------------------------------------------------------------------------------------------------------
NOP:
  no operation

  does nothing
  
  encoding:
    normal:
        000000 | ... |
        NOP    | NU  |
        63   58|57  0|
---------------------------------------------------------------------------------------------------------------
ADD:
  addition

  if the 49th bit is 0 then you add SR1 and SR2 and store the result in DR  
  if it's 1 then you add a 49 bit number to SR1 and store the result in DR

  sets condition flags

  encoding:
    normal:
        000001 | **** | **** | 0 | **** | ... |
        ADD    |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000001 | **** | **** | 1 |  ...  |
        ADD    |  DR  |  SR1 |   | imm49 |
        63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
SUB:
  subtraction

  if the 49th bit is 0 then you subtract SR2 from SR1 and store the result in DR  
  if it's 1 then you subtract a 49 bit number from SR1 and store the result in DR
  
  sets condition flags

  encoding:
    normal:
        000010 | **** | **** | 0 | **** | ... |
        SUB    |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000010 | **** | **** | 1 |  ...  |
        SUB    |  DR  |  SR1 |   | imm49 |
        63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
MUL:
  unsigned multiplication

  if the 49th bit is 0 then you multiply SR1 and SR2 and store the result in DR  
  if it's 1 then you multiply SR1 by a 47 bit number and store the result in DR

  sets condition flags

  encoding:
    normal:
        000011 | **** | **** | 0 | **** | ... |
        MUL    |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000011 | **** | **** | 1 |  ...  |
        MUL    |  DR  |  SR1 |   | imm49 |
        63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
SMUL:
  signed multiplication

  if the 49th bit is 0 then you multiply SR1 and SR2 and store the result in DR  
  if it's 1 then you multiply SR1 by a 47 bit number and store the result in DR

  sets condition flags

  encoding:
    normal:
        000100 | **** | **** | 0 | **** | ... |
        SMUL   |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000100 | **** | **** | 1 |  ...  |
        SMUL   |  DR  |  SR1 |   | imm49 |
        63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
DIV:
  unsigned division

  if the 45th bit is 0 SR1 is divided by SR2 and the quotient and remainder are stored in QR and RR respectively
  if it's 1 then SR1 is divided by imm45 and the quotient and remainder are stored in QR and RR respectively

  sets condition flags

  encoding:
    normal:
        000101 | **** | **** | **** | 0 | **** | ... |
        DIV    |  QR  |  RR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50|49  46| 45|44  41|40  0|

    immediate:
        000101 | **** | **** | **** | 1 |  ...  |
        DIV    |  QR  |  RR  |  SR1 |   | imm45 |
        63   58|57  54|53  50|49  46| 45|44    0|
---------------------------------------------------------------------------------------------------------------
SDIV:
  signed division

  if the 45th bit is 0 SR1 is divided by SR2 and the quotient and remainder are stored in QR and RR respectively
  if it's 1 then SR1 is divided by imm45 and the quotient and remainder are stored in QR and RR respectively

  sets condition flags

  encoding:
    normal:
        000110 | **** | **** | **** | 0 | **** | ... |
        SDIV   |  QR  |  RR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50|49  46| 45|44  41|40  0|

    immediate:
        000110 | **** | **** | **** | 1 |  ...  |
        SDIV   |  QR  |  RR  |  SR1 |   | imm45 |
        63   58|57  54|53  50|49  46| 45|44    0|
---------------------------------------------------------------------------------------------------------------
AND:
  bitwise and

    TODO desc
  
  encoding:
    normal:
      000111 | **** | **** | 0 | **** | ... |
      AND    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      000111 | **** | **** | 1 |  ...  |
      AND    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
OR:
  bitwise or

    TODO desc
  
  encoding:
    normal:
      001000 | **** | **** | 0 | **** | ... |
      OR     |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001000 | **** | **** | 1 |  ...  |
      OR     |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
XOR:
  bitwise xor

  TODO desc
  
  encoding:
    normal:
      001001 | **** | **** | 0 | **** | ... |
      XOR    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001001 | **** | **** | 1 |  ...  |
      XOR    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
SHR:
  bitwise shift right

  TODO desc
  
  encoding:
    normal:
      001010 | **** | **** | 0 | **** | ... |
      SHR    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001010 | **** | **** | 1 |  ...  |
      SHR    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
SHL:
  bitwise shift left

  TODO desc
  
  encoding:
    normal:
      001011 | **** | **** | 0 | **** | ... |
      SHL    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001011 | **** | **** | 1 |  ...  |
      SHL    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
NOT:
  bitwise NOT

  TODO desc
  
  encoding:
    normal:
      001100 | **** | **** | ... |
      NOT    |  DR  |  SR1 | NU  |
      63   58|57  54|53  50|49  0|
---------------------------------------------------------------------------------------------------------------
JMP:
  jump unconditionally to the location specified in the register
  
  encoding:
    normal:
      001101 | **** | ... |
      JMP    |  SR1 | NU  |
      63   58|57  54|53  0|
---------------------------------------------------------------------------------------------------------------
MOV:
  if 53rd bit is 0, copies data from SR1 to DR
  if 53rd bit is 1, copies data from imm53 to DR
  
  encoding:
    normal:
      001110 | **** | 0 | **** | ... |
      MOV    |  DR  |   | SR1  | NU  |
      63   58|57  54| 53|52  49|48  0|

    immediate:
      001110 | **** | 1 |  ...  |
      MOV    |  DR  |   | imm53 |
      63   58|57  54| 53|52    0|



*/

#define readbits(start, numbits) ReadBits64(instr, start, numbits)
 
inline u64 sign_extend(u64 x, s64 bit_count){
    if((x >> (bit_count-1)) & 1){
        x |= (0xFFFFFFFFFFFFFFFF << bit_count);
    }
    return x;
}

//this is probably inefficient
struct InstrRead{
    u64 OP;

    u64 DR;
    u64 QR;
    u64 RR;
    u64 SR1;
    u64 SR2;

    u64 immcond;
    u64 immval;
};

InstrRead ReadInstr(u64 instr){
    InstrRead read = {0};
    read.OP = instr >> 58;
    switch(read.OP){
        case OP_ADD:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_SUB:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_MUL:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1))  read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                 read.SR2 = readbits(45, 4);
        }break;
        case OP_SMUL:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1))  read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                 read.SR2 = readbits(45, 4);
        }break;
        case OP_DIV:{
            read.QR = readbits(54, 4);
            read.RR = readbits(50, 4);
            read.SR1 = readbits(46, 4);
            if(readbits(45, 1))  read.immval = sign_extend(readbits(0, 44), 44), read.immcond = 1;
            else                 read.SR2 = readbits(41, 4);
        }break;
        case OP_SDIV:{
            read.QR = readbits(54, 4);
            read.RR = readbits(50, 4);
            read.SR1 = readbits(46, 4);
            if(readbits(45, 1)) read.immval = sign_extend(readbits(0, 44), 44), read.immcond = 1;
            else                read.SR2 = readbits(41, 4);
        }break;
        case OP_AND:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_OR:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_XOR:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_SHR:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_SHL:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48), read.immcond = 1;
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_NOT:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
        }break;
        case OP_JMP:{
            read.SR1 = readbits(54, 4);
        }break;
        case OP_MOV:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(53, 1)) read.immval = sign_extend(readbits(0, 42), 42), read.immcond = 1;
            else                read.SR1 = readbits(49,4);
        }break;
    }
    return read;
}



struct machine{

    Reg reg[R_COUNT];

    b32 powered = false;
    u64 PC_START = 0x0000; //the address the machine starts executing from.
    u64 MEM_START = 0x7fff;
    u64 mem[MAX_U16]; // the machines memory
    
    void turnon(){
        powered = true;
        memset(reg, 0, R_COUNT);
        ureg(R_COND)=FL_ZRO;
        ureg(R_PC) = PC_START;
    }

    void turnoff(){
        powered = false;
        memset(mem, 0, MAX_U16);
    }

    void tick(){
        if(!powered) return;

        auto mem_write = [&](u64 address, u64 val){
            mem[address] = val;
        };
        auto mem_read = [&](u64 address){
            return mem[address];
        };
        auto update_flags = [&](u64 r){
            if     (ureg(r)==0)  ureg(R_COND)=FL_ZRO;
            else if(ureg(r)>>63) ureg(R_COND)=FL_NEG;
            else                 ureg(R_COND)=FL_POS;
        };
    
        u64 instr = mem_read(reg[R_PC].u++);  //get current instruction
        InstrRead read = ReadInstr(instr);
        
#if 0
        Log("",
            "OP:  ", opnames[read.OP], "\n",
            "DR:  %", read.DR, "\n",
            "QR:  %", read.QR, "\n",
            "RR:  %", read.RR, "\n",
            "SR1: %", read.SR1, "\n",
            "SR2: %", read.SR2, "\n",
            "imc: ", read.immcond, "\n",
            "imv: ", read.immval, "\n"
        );

#endif

        switch(read.OP){
            case OP_ADD:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) + read.immval;
                else             ureg(read.DR) = ureg(read.SR1) + ureg(read.SR2);
            }break;
            case OP_SUB:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) - read.immval;
                else             ureg(read.DR) = ureg(read.SR1) - ureg(read.SR2);
            }break;
            case OP_MUL:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) * read.immval;
                else                ureg(read.DR) = ureg(read.SR1) * ureg(read.SR2);
            }break;
            case OP_SMUL:{
                if(read.immcond) sreg(read.DR) = sreg(read.SR1) * ConversionlessCast(s64,read.immval);
                else                sreg(read.DR) = sreg(read.SR1) * sreg(read.SR2);
            }break;
            case OP_DIV:{
                if(read.immcond){
                    ureg(read.QR) = ureg(read.SR1) / read.immval;
                    ureg(read.RR) = ureg(read.SR1) % read.immval;
                }
                else{
                    ureg(read.QR) = ureg(read.SR1) / ureg(read.SR2);
                    ureg(read.RR) = ureg(read.SR1) % ureg(read.SR2);
                }
            }break;
            case OP_SDIV:{
                if(read.immcond){
                    sreg(read.QR) = sreg(read.SR1) / ConversionlessCast(s64, read.immval);
                    sreg(read.RR) = sreg(read.SR1) % ConversionlessCast(s64, read.immval);
                }
                else{
                    sreg(read.QR) = sreg(read.SR1) / sreg(read.SR2);
                    sreg(read.RR) = sreg(read.SR1) % sreg(read.SR2);
                }
            }break;
            case OP_AND:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) & read.immval;
                else                ureg(read.DR) = ureg(read.SR1) & ureg(read.SR2);
            }break;
            case OP_OR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) | read.immval;
                else                ureg(read.DR) = ureg(read.SR1) | ureg(read.SR2);
            }break;
            case OP_XOR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) ^ read.immval;
                else                ureg(read.DR) = ureg(read.SR1) ^ ureg(read.SR2);
            }break;
            case OP_SHR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) >> read.immval;
                else                ureg(read.DR) = ureg(read.SR1) >> ureg(read.SR2);
            }break;
            case OP_SHL:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) << read.immval;
                else                ureg(read.DR) = ureg(read.SR1) << ureg(read.SR2);
            }break;
            case OP_NOT:{
                ureg(read.DR) = !ureg(read.SR1);
            }break;
            case OP_JMP:{
                ureg(R_PC) = ureg(read.SR1);
            }break;
            case OP_MOV:{
                if(read.immcond) ureg(read.DR) = read.immval;
                else             ureg(read.DR) = ureg(read.SR1);
            }break;
        }//switch(read.OP)
    }//tick()
};//machine

#endif