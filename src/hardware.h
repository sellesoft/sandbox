#ifndef NASAU_HARDWARE_H
#define NASAU_HARDWARE_H

#include "kigu/common.h"
#include "kigu/array.h"

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

    OP_ADD = 0, //
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
    //OP_LD,     //
    //OP_ST,     //
    //OP_JSR,    //
    //OP_LDR,    //
    //OP_STR,    //
    //OP_RTI,    //
    //OP_LDI,    //
    //OP_STI,    //
    //OP_JMP,    //
    //OP_RES,    //
    //OP_LEA,    //
    OP_COUNT,

    Destination=0, 
    Source,
    ImmediateMode,
    OpcodeDependent, //bits left to the opcode to eval itself
    Unused,
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

void addop(Reg* reg, u64 DR, u64 SR1, u64 immcond, u64 S){ureg(DR) = ureg(SR1) + (immcond ? S : ureg(S>>45) & (((u64)1<<4)-1));}
struct opcode{
    cstring name;
    operation op;
    oppart preimm[8] = {0};
    oppart immtparts[8] = {0};

}opcodes[OP_COUNT]{
    {cstr("add"), {&addop, 4}, {{Destination, 4},{Source, 4},{ImmediateMode, 1},{OpcodeDependent, 49}}}
};


/*
---------------------------------------------------------------------------------------------------------------
ADD:
  addition

  if the 49th bit is 0 then you add SR1 and SR2 and store the result in DR  
  if it's 1 then you add a 49 bit number to SR1 and store the result in DR

  sets condition flags

  encoding:
    normal:
        000000 | **** | **** | 0 | **** | ... |
        ADD    |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000000 | **** | **** | 1 |  ...  |
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
        000001 | **** | **** | 0 | **** | ... |
        SUB    |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000001 | **** | **** | 1 |  ...  |
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
        000010 | **** | **** | 0 | **** | ... |
        MUL    |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000010 | **** | **** | 1 |  ...  |
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
        000011 | **** | **** | 0 | **** | ... |
        SMUL   |  DR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
        000011 | **** | **** | 1 |  ...  |
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
        000100 | **** | **** | **** | 0 | **** | ... |
        DIV    |  QR  |  RR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50|49  46| 45|44  41|40  0|

    immediate:
        000100 | **** | **** | **** | 1 |  ...  |
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
        000101 | **** | **** | **** | 0 | **** | ... |
        SDIV   |  QR  |  RR  |  SR1 |   |  SR2 | NU  |
        63   58|57  54|53  50|49  46| 45|44  41|40  0|

    immediate:
        000101 | **** | **** | **** | 1 |  ...  |
        SDIV   |  QR  |  RR  |  SR1 |   | imm45 |
        63   58|57  54|53  50|49  46| 45|44    0|
---------------------------------------------------------------------------------------------------------------
AND:
  bitwise and

    TODO desc
  
  encoding:
    normal:
      000110 | **** | **** | 0 | **** | ... |
      AND    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      000110 | **** | **** | 1 |  ...  |
      AND    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
OR:
  bitwise or

    TODO desc
  
  encoding:
    normal:
      000111 | **** | **** | 0 | **** | ... |
      OR     |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      000111 | **** | **** | 1 |  ...  |
      OR     |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
XOR:
  bitwise xor

  TODO desc
  
  encoding:
    normal:
      001000 | **** | **** | 0 | **** | ... |
      XOR    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001000 | **** | **** | 1 |  ...  |
      XOR    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
SHR:
  bitwise shift right

  TODO desc
  
  encoding:
    normal:
      001001 | **** | **** | 0 | **** | ... |
      SHR    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001001 | **** | **** | 1 |  ...  |
      SHR    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
SHL:
  bitwise shift left

  TODO desc
  
  encoding:
    normal:
      001010 | **** | **** | 0 | **** | ... |
      SHL    |  DR  |  SR1 |   |  SR2 | NU  |
      63   58|57  54|53  50| 49|48  45|44  0|

    immediate:
      001010 | **** | **** | 1 |  ...  |
      SHL    |  DR  |  SR1 |   | imm49 |
      63   58|57  54|53  50| 49|48    0|
---------------------------------------------------------------------------------------------------------------
NOT:
  bitwise NOT

  TODO desc
  
  encoding:
    normal:
      001011 | **** | **** | ... |
      NOT    |  DR  |  SR1 | NU  |
      63   58|57  54|53  50|49  0|


*/

#define readbits(start, numbits) ReadBits64(instr, start, numbits)

inline u64 sign_extend(u64 x, s64 bit_count){
    printbits(x);
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
    InstrRead read;
    read.OP = instr >> 58;
    switch(read.OP){
        case OP_ADD:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_SUB:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_MUL:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1))  read.immval = sign_extend(readbits(0, 48), 48);
            else                 read.SR2 = readbits(45, 4);
        }break;
        case OP_SMUL:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
            if(readbits(49, 1))  read.immval = sign_extend(readbits(0, 48), 48);
            else                 read.SR2 = readbits(45, 4);
        }break;
        case OP_DIV:{
            read.QR = readbits(54, 4);
            read.RR = readbits(50, 4);
            read.SR1 = readbits(46, 4);
            if(readbits(45, 1))  read.immval = sign_extend(readbits(0, 44), 44);
            else                 read.SR2 = readbits(41, 4);
        }break;
        case OP_SDIV:{
            read.QR = readbits(54, 4);
            read.RR = readbits(50, 4);
            read.SR1 = readbits(46, 4);
            if(readbits(45, 1)) read.immval = sign_extend(readbits(0, 44), 44);
            else                read.SR2 = readbits(41, 4);
        }break;
        case OP_AND:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_OR:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_XOR:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_SHR:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_SHL:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
            if(readbits(49, 1)) read.immval = sign_extend(readbits(0, 48), 48);
            else                read.SR2 = readbits(45, 4);
        }break;
        case OP_NOT:{
            read.DR   = readbits(54, 4);
            read.SR1  = readbits(50, 4);
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
                if(readbits(49, 1)) ureg(read.DR) = ureg(read.SR1) * read.immval;
                else                ureg(read.DR) = ureg(read.SR1) * ureg(read.SR2);
            }break;
            case OP_SMUL:{
                if(readbits(49, 1)) sreg(read.DR) = sreg(read.SR1) * ConversionlessCast(s64,read.immval);
                else                sreg(read.DR) = sreg(read.SR1) * sreg(read.SR2);
            }break;
            case OP_DIV:{
                if(readbits(45, 1)){
                    ureg(read.QR) = ureg(read.SR1) / read.immval;
                    ureg(read.RR) = ureg(read.SR1) % read.immval;
                }
                else{
                    ureg(read.QR) = ureg(read.SR1) / ureg(read.SR2);
                    ureg(read.RR) = ureg(read.SR1) % ureg(read.SR2);
                }
            }break;
            case OP_SDIV:{
                if(readbits(45, 1)){
                    sreg(read.QR) = sreg(read.SR1) / ConversionlessCast(s64, read.immval);
                    sreg(read.RR) = sreg(read.SR1) % ConversionlessCast(s64, read.immval);
                }
                else{
                    sreg(read.QR) = sreg(read.SR1) / sreg(read.SR2);
                    sreg(read.RR) = sreg(read.SR1) % sreg(read.SR2);
                }
            }break;
            case OP_AND:{
                if(readbits(49, 1)) ureg(read.DR) = ureg(read.SR1) & read.immval;
                else                ureg(read.DR) = ureg(read.SR1) & ureg(read.SR2);
            }break;
            case OP_OR:{
                if(readbits(49, 1)) ureg(read.DR) = ureg(read.SR1) | read.immval;
                else                ureg(read.DR) = ureg(read.SR1) | ureg(read.SR2);
            }break;
            case OP_XOR:{
                if(readbits(49, 1)) ureg(read.DR) = ureg(read.SR1) ^ read.immval;
                else                ureg(read.DR) = ureg(read.SR1) ^ ureg(read.SR2);
            }break;
            case OP_SHR:{
                if(readbits(49, 1)) ureg(read.DR) = ureg(read.SR1) >> read.immval;
                else                ureg(read.DR) = ureg(read.SR1) >> ureg(read.SR2);
            }break;
            case OP_SHL:{
                if(readbits(49, 1)) ureg(read.DR) = ureg(read.SR1) << read.immval;
                else                ureg(read.DR) = ureg(read.SR1) << ureg(read.SR2);
            }break;
            case OP_NOT:{
                ureg(read.DR) = !ureg(read.SR1);
            }break;
        }
#if 0
        u64 op = instr >> 58; //get current opcode from instruction
        printbits(instr);
        switch(op){
            case OP_ADD:{
                u64 DR = readbits(54, 4);
                u64 SR1 = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) + imm49;
                }else{
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) + ureg(SR2);
                }
            }break;
            case OP_SUB:{
                u64 DR = readbits(54, 4);
                u64 SR1 = readbits(50, 4);
                if(readbits(49, 1)){
                     u64 imm49 = sign_extend(readbits(0, 48), 48);
                     ureg(DR) = ureg(SR1) - imm49;
                }
                else {
                     u64 SR2 = readbits(45, 4);
                     ureg(DR) = ureg(SR1) - ureg(SR2);
               }
            }break;
            case OP_MUL:{
                u64 DR = readbits(54, 4);
                u64 SR1 = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) * imm49;
                }
                else {
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) * ureg(SR2);
                }
            }break;
            case OP_SMUL:{
                u64 DR = readbits(54, 4);
                u64 SR1 = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    sreg(DR) = sreg(SR1) * ConversionlessCast(s64,imm49);
                }
                else {
                    u64 SR2 = readbits(45, 4);
                    sreg(DR) = sreg(SR1) * sreg(SR2);
                }
            }break;
            case OP_DIV:{
                u64 QR = readbits(54, 4);
                u64 RR = readbits(50, 4);
                u64 SR1 = readbits(46, 4);
                if(readbits(45, 1)){
                    u64 imm45 = sign_extend(readbits(0, 44), 44);
                    ureg(QR) = ureg(SR1) / imm45;
                    ureg(RR) = ureg(SR1) % imm45;
                }
                else{
                    u64 SR2 = readbits(41, 4);
                    ureg(QR) = ureg(SR1) / ureg(SR2);
                    ureg(RR) = ureg(SR1) % ureg(SR2);
                }
            }break;
            case OP_SDIV:{
                u64 QR = readbits(54, 4);
                u64 RR = readbits(50, 4);
                u64 SR1 = readbits(46, 4);
                if(readbits(45, 1)){
                    u64 imm45 = sign_extend(readbits(0, 44), 44);
                    sreg(QR) = sreg(SR1) / ConversionlessCast(s64, imm45);
                    sreg(RR) = sreg(SR1) % ConversionlessCast(s64, imm45);
                }
                else{
                    u64 SR2 = readbits(41, 4);
                    sreg(QR) = sreg(SR1) / sreg(SR2);
                    sreg(RR) = sreg(SR1) % sreg(SR2);
                }
            }break;
            case OP_AND:{
                u64 DR   = readbits(54, 4);
                u64 SR1  = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) & imm49;
                }else{
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) & ureg(SR2);
                }
            }break;
            case OP_OR:{
                u64 DR   = readbits(54, 4);
                u64 SR1  = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) | imm49;
                }else{
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) | ureg(SR2);
                }
            }break;
            case OP_XOR:{
                u64 DR   = readbits(54, 4);
                u64 SR1  = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) ^ imm49;
                }else{
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) ^ ureg(SR2);
                }
            }break;
            case OP_SHR:{
                u64 DR   = readbits(54, 4);
                u64 SR1  = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) >> imm49;
                }else{
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) >> ureg(SR2);
                }
            }break;
            case OP_SHL:{
                u64 DR   = readbits(54, 4);
                u64 SR1  = readbits(50, 4);
                if(readbits(49, 1)){
                    u64 imm49 = sign_extend(readbits(0, 48), 48);
                    ureg(DR) = ureg(SR1) << imm49;
                }else{
                    u64 SR2 = readbits(45, 4);
                    ureg(DR) = ureg(SR1) << ureg(SR2);
                }
            }break;
            case OP_NOT:{
                u64 DR   = readbits(54, 4);
                u64 SR1  = readbits(50, 4);
                ureg(DR) = !ureg(SR1);
            }break;
        }
    }
#endif
};

};

#endif