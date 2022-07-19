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

enum  {
    HT_memory,
    HT_cpu,
    HT_bus,
};

struct battery{
    f32 output = 5; //volts
    u64 use = 0; //incremented by 1 everytime power is requested from this battery
    
    f32 power(){
        return 5;
    }
};

struct hardware{
    u64 id;
    str8 name;
    Type type;
};

struct wire;
struct pin{
    wire* w = 0;
    f32 V = 0;
};

struct wire{
    pin* p0;
    pin* p1;
    f32 load = 5; //how much power the wire can carry before burning up or breaking
    f32 voltage = 0; //how much force is travelling through the wire
    b32 burnt = 0; //true when the wire has burnt up
};

//volatile
//TODO(sushi) with very large ram chips, we may want to put clearing onto another thread
//            and just block the chips execution until we are done
/*
    nasau ram chip
    --------------

    this is a parallel in/out random access memory chip
    
    it has 18 pins
        1 for power
        1 for chip select
        16 for addressing
        8 for input and output

    
    the chip can read up to 65535 addresses


*/

enum{
    ram_op_write,
    ram_op_read,
};
struct ram {
    hardware hw;
    
    b32 powered = 0;

    u8* mem = 0;
    u64 size = 0;
    //set to true when the chip has been cleared because of no power
    //so we dont try and clear everytime this chip is clocked
    b32 cleared = 0;

    enum{
        read_input,
        write,
        read,
    };

    u32 stage = 0;

    struct{
        pin power;

        //if this pin is set to high, the chip will assume incoming signals
        //are meant for it and respond to them
        //otherwise the chip does nothing
        pin active;

        //addressing pins
        pin a[16];

        //input/output pins
        pin io[8];
    }pins;
    
    void init(u64 memsize){
        size = memsize;
        mem = (u8*)memalloc(memsize);
    }

    void clock(){
        Assert(mem, "attempt to use a ram chip without initializing it. (call init())");
        //if the power pin is set to low, then we clear the memory and do nothing else
        if(!pins.power.w || pins.power.w->voltage < 2){
            if(!cleared){
                memset(mem, 0, size);
                cleared = 1;
            }
            return;
        }
        cleared = 0;
        switch(stage){
            case read_input:{
                if(!pins.active.w || pins.active.w->voltage < 2) return;

            }break;
        }



    }    

};

struct rom {

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
    OP_ADD, 
    OP_SUB, 
    OP_MUL, 
    OP_SMUL, 
    OP_DIV, 
    OP_SDIV,
    OP_AND, 
    OP_OR, 
    OP_XOR, 
    OP_SHR, 
    OP_SHL, 
    OP_NOT, 
    OP_JMP, 
    OP_JZ, 
    OP_JNZ,
    OP_JP,
    OP_JN,
    OP_MOV,
    OP_ST,
    OP_LD,
    OP_COUNT,
    OP_UNKNOWN = npos,
};

static const str8 opnames[] = {
    STR8("nop"),
    STR8("add"), 
    STR8("sub"), 
    STR8("mul"), 
    STR8("smul"), 
    STR8("div"), 
    STR8("sdiv"),
    STR8("and"), 
    STR8("or"), 
    STR8("xor"), 
    STR8("shr"), 
    STR8("shl"), 
    STR8("not"), 
    STR8("jmp"), 
    STR8("jz"), 
    STR8("jnz"),
    STR8("jp"),
    STR8("jn"),
    STR8("mov"),
    STR8("st"),
    STR8("ld"),
};

/*

    nasau cpu opcode reference

    nasau's current cpu opcodes are fixed length at 64 bits

    the opcodes for nasau's cpus should follow some guidelines

        1. the order of operands follows traditional writing of operations
            so the destination is always the first operand, followed by sources
            for example, add would be
                add dr, sr1, sr2
            which is equivalent to
                destination register = source register 1 + source register 2
        2. not sure of anymore yet :)

    



    |  ****  |
    |  bits  |
    |54    34|
     ^      ^
     |      |
     |      starting bit
     end bit 
    bits = readbits(34, 20);

---------------------------------------------------------------------------------------------------------------
nop:
  no operation

  does nothing
  
  encoding:
    normal:
        000000 | ... |
        nop    | nu  |
        63   58|57  0|
---------------------------------------------------------------------------------------------------------------
binary arithmetic:

  binary operators all follow the pattern
    dest = src1 <op> src2 
  they also set the condition flags

  opcodes:
    add  | 0b000001 | 0x01 | 1  | addition  
    sub  | 0b000010 | 0x02 | 2  | subtraction
    mul  | 0b000011 | 0x03 | 3  | multiplication
    smul | 0b000100 | 0x04 | 4  | signed multiplication 
    div  | 0b000101 | 0x05 | 5  | division
    sdiv | 0b000110 | 0x06 | 6  | signed division
    and  | 0b000111 | 0x07 | 7  | bitwise and
    or   | 0b001000 | 0x08 | 8  | bitwise or
    xor  | 0b001001 | 0x09 | 9  | bitwise xor
    shr  | 0b001010 | 0x0A | 10 | bitwise shift right
    shl  | 0b001011 | 0x0B | 11 | bitwise shift left

  encodings:
    add,sub,mul,smul,and,or,xor,shr,shl:

      if the 49th bit is 0 then the second operand is a register, if it is 1 then it is a sign extended 
      49 bit value.

      normal:
        ****** | **** | **** | 0 | **** | ... |
        opcode |  dr  |  sr1 |   |  sr2 | nu  |
        63   58|57  54|53  50| 49|48  45|44  0|

      immediate:
        ****** | **** | **** | 1 |  ...  |
        opcode |  dr  |  sr1 |   | imm49 |
        63   58|57  54|53  50| 49|48    0|

    div, sdiv:

      if the 45th bit is 0 sr1 is divided by sr2 and the quotient and remainder are stored in qr and rr respectively
      if it's 1 then sr1 is divided by imm45 and the quotient and remainder are stored in qr and rr respectively

      normal:
        ****** | **** | **** | **** | 0 | **** | ... |
        opcode |  qr  |  rr  |  sr1 |   |  sr2 | nu  |
        63   58|57  54|53  50|49  46| 45|44  41|40  0|

      immediate:
        ****** | **** | **** | **** | 1 |  ...  |
        opcode |  qr  |  rr  |  sr1 |   | imm45 |
        63   58|57  54|53  50|49  46| 45|44    0|

---------------------------------------------------------------------------------------------------------------
not:
  bitwise not

  todo desc
  
  encoding:
    normal:
      001100 | **** | **** | ... |
      not    |  dr  |  sr1 | nu  |
      63   58|57  54|53  50|49  0|

---------------------------------------------------------------------------------------------------------------
jumps:
  
  jumps move the stack pointer to a new location relative to where it was at the jump
  they can either be passed a register or an immediate value

  opcodes:
    jmp | 0b001101 | 0x0D | 13 | jumps unconditionally 
    jz  | 0b001110 | 0x0E | 14 | jumps if the last operation's result was not zero
    jnz | 0b001111 | 0x0F | 15 | jumps if the last operation's result was not zero
    jp  | 0b010000 | 0x10 | 16 | jumps if the last operation's result was positive
    jn  | 0b010001 | 0x11 | 17 | jumps if the last operation's result was negative

  encoding:
    normal:
      ****** | 0 | **** | ... |
      opcode |   |  sr1 | nu  |
      63   58| 57|56  53|52  0|
    
    immediate:
      ****** | 1 |  ...  |
      opcode |   | imm57 |
      63   58| 57|56    0|

---------------------------------------------------------------------------------------------------------------
mov:
  if 53rd bit is 0, copies data from sr1 to dr
  if 53rd bit is 1, copies data from imm53 to dr
  
  encoding:
    normal:
      010010 | **** | 0 | **** | ... |
      mov    |  dr  |   | sr1  | nu  |
      63   58|57  54| 53|52  49|48  0|

    immediate:
      010010 | **** | 1 |  ...  |
      mov    |  dr  |   | imm53 |
      63   58|57  54| 53|52    0|

---------------------------------------------------------------------------------------------------------------
memory:
  
  st and ld are used for manipulating memory connected to the cpu
  they both have a width operand which determines the size of memory to be manipulated

  widths:
    0: 8 bits
    1: 16 bits
    2: 32 bits
    3: 64 bits

  opcodes:
    st | 0b010011 | 0x13 | 19 | stores the value in sr1 at the memory location pointed to by dr
    ld | 0b010100 | 0x14 | 20 | loads the value stored at the memory location pointed to by sr1 into dr

  encoding:
    normal:
      ****** |  **   | **** | **** | ... |
      opcode | width |  dr  | sr1  | nu  |
      63   58|57   56|55  52|51  48|47  0|

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
        case OP_JZ:
        case OP_JNZ:
        case OP_JP:
        case OP_JN:
        case OP_JMP:{
            if(readbits(57,1)) read.immval = sign_extend(readbits(0, 56), 56), read.immcond = 1;
            else               read.SR1 = readbits(53, 4);
        }break;
        case OP_MOV:{
            read.DR = readbits(54, 4);
            if(readbits(53, 1)) read.immval = sign_extend(readbits(0, 52), 52), read.immcond = 1;
            else                read.SR1 = readbits(49,4);
        }break;
        case OP_LD:
        case OP_ST:{
            read.DR = readbits(54, 4);
            read.SR1 = readbits(50, 4);
        }break;
    }
    return read;
}



struct machine{

    Reg reg[R_COUNT];

    b32 powered = false;
    u64 PC_START = 0x0000; //the address the machine starts executing from.
    u8* mem = (u8*)memalloc(Kilobytes(1)); // the machines memory
    
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
        auto mem_read_u8 = [&](u64 address){
            return *(u8*)(mem + address);
        };
        auto mem_read_u16 = [&](u64 address){
            return *(u16*)(mem + address);
        };
        auto mem_read_u32 = [&](u64 address){
            return *(u32*)(mem + address);
        };
        auto mem_read_u64 = [&](u64 address){
            return *(u64*)(mem + address);
        };
        auto update_flags = [&](u64 r){
            if     (ureg(r)==0)  ureg(R_COND)=FL_ZRO;
            else if(ureg(r)>>63) ureg(R_COND)=FL_NEG;
            else                 ureg(R_COND)=FL_POS;
        };
    
        u64 instr = mem_read_u64(reg[R_PC].u++ * sizeof(u64));  //get current instruction
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
                update_flags(read.DR);
            }break;
            case OP_SUB:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) - read.immval;
                else             ureg(read.DR) = ureg(read.SR1) - ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_MUL:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) * read.immval;
                else             ureg(read.DR) = ureg(read.SR1) * ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_SMUL:{
                if(read.immcond) sreg(read.DR) = sreg(read.SR1) * *(s64*)&read.immval;
                else             sreg(read.DR) = sreg(read.SR1) * sreg(read.SR2);
                update_flags(read.DR);
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
                update_flags(read.DR);
            }break;
            case OP_SDIV:{
                if(read.immcond){
                    sreg(read.QR) = sreg(read.SR1) / *(s64*)read.immval;
                    sreg(read.RR) = sreg(read.SR1) % *(s64*)read.immval;
                }
                else{
                    sreg(read.QR) = sreg(read.SR1) / sreg(read.SR2);
                    sreg(read.RR) = sreg(read.SR1) % sreg(read.SR2);
                }
                update_flags(read.DR);
            }break;
            case OP_AND:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) & read.immval;
                else             ureg(read.DR) = ureg(read.SR1) & ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_OR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) | read.immval;
                else             ureg(read.DR) = ureg(read.SR1) | ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_XOR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) ^ read.immval;
                else             ureg(read.DR) = ureg(read.SR1) ^ ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_SHR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) >> read.immval;
                else             ureg(read.DR) = ureg(read.SR1) >> ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_SHL:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) << read.immval;
                else             ureg(read.DR) = ureg(read.SR1) << ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_NOT:{
                ureg(read.DR) = !ureg(read.SR1);
                update_flags(read.DR);
            }break;
            case OP_JMP:{
                if(read.immcond) ureg(R_PC) = read.immval;
                else             ureg(R_PC) = ureg(read.SR1);
            }break;
            case OP_JZ:{
                if(ureg(R_COND) & FL_ZRO){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_JNZ:{
                if(!(ureg(R_COND) & FL_ZRO)){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_JP:{
                if(ureg(R_COND) & FL_POS){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_JN:{
                if(ureg(R_COND) & FL_NEG){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_MOV:{
                if(read.immcond) ureg(read.DR) = read.immval;
                else             ureg(read.DR) = ureg(read.SR1);
            }break;
            case OP_ST:{
                mem_write(ureg(read.DR), ureg(read.SR1));
            }break; 
            case OP_LD:{
                ureg(read.DR) = mem_read_u64(ureg(read.SR1));
            }break;
        }//switch(read.OP)
    }//tick()
};//machine

/*
    nasau cpu
    ---------

    this is a much-more-complex-than-it-needs-to-be implementation of a nasau cpu
    this implementation uses the pin struct to simulate connects between it and other chips
    in practical use it would be much better to just have chips communicate by calling each other

*/
struct cpu{
    Reg reg[R_COUNT];

    struct{
        pin power;

        pin active;

        //addressing pins
        //64 bits provides access to 18,446,744,073,709,551,615 bytes of memory 
        pin a[64];

        //data pins
        //64 bits is the fixed length of an opcode for this cpu
        pin d[64];
    }pins;

    void tick(){
        if(!pins.power.w || pins.power.w->voltage < 2 || !pins.active.w || pins.active.w->voltage < 2){
            //reset the cpu and return immediatly
            reg = {0};
        }
        auto update_flags = [&](u64 r){
            if     (ureg(r)==0)  ureg(R_COND)=FL_ZRO;
            else if(ureg(r)>>63) ureg(R_COND)=FL_NEG;
            else                 ureg(R_COND)=FL_POS;
        };
    
        u64 instr = mem_read_u64(reg[R_PC].u++ * sizeof(u64));  //get current instruction
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
                update_flags(read.DR);
            }break;
            case OP_SUB:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) - read.immval;
                else             ureg(read.DR) = ureg(read.SR1) - ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_MUL:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) * read.immval;
                else             ureg(read.DR) = ureg(read.SR1) * ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_SMUL:{
                if(read.immcond) sreg(read.DR) = sreg(read.SR1) * *(s64*)&read.immval;
                else             sreg(read.DR) = sreg(read.SR1) * sreg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_DIV:{
                if(read.immcond){
                    ureg(read.QR) = ureg(read.SR1) / read.immval;
                    ureg(read.RR) = ureg(read.SR1) % read.immval;
                }else{
                    ureg(read.QR) = ureg(read.SR1) / ureg(read.SR2);
                    ureg(read.RR) = ureg(read.SR1) % ureg(read.SR2);
                }
                update_flags(read.DR);
            }break;
            case OP_SDIV:{
                if(read.immcond){
                    sreg(read.QR) = sreg(read.SR1) / *(s64*)read.immval;
                    sreg(read.RR) = sreg(read.SR1) % *(s64*)read.immval;
                }
                else{
                    sreg(read.QR) = sreg(read.SR1) / sreg(read.SR2);
                    sreg(read.RR) = sreg(read.SR1) % sreg(read.SR2);
                }
                update_flags(read.DR);
            }break;
            case OP_AND:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) & read.immval;
                else             ureg(read.DR) = ureg(read.SR1) & ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_OR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) | read.immval;
                else             ureg(read.DR) = ureg(read.SR1) | ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_XOR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) ^ read.immval;
                else             ureg(read.DR) = ureg(read.SR1) ^ ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_SHR:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) >> read.immval;
                else             ureg(read.DR) = ureg(read.SR1) >> ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_SHL:{
                if(read.immcond) ureg(read.DR) = ureg(read.SR1) << read.immval;
                else             ureg(read.DR) = ureg(read.SR1) << ureg(read.SR2);
                update_flags(read.DR);
            }break;
            case OP_NOT:{
                ureg(read.DR) = !ureg(read.SR1);
                update_flags(read.DR);
            }break;
            case OP_JMP:{
                if(read.immcond) ureg(R_PC) = read.immval;
                else             ureg(R_PC) = ureg(read.SR1);
            }break;
            case OP_JZ:{
                if(ureg(R_COND) & FL_ZRO){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_JNZ:{
                if(!(ureg(R_COND) & FL_ZRO)){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_JP:{
                if(ureg(R_COND) & FL_POS){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_JN:{
                if(ureg(R_COND) & FL_NEG){
                    if(read.immcond) ureg(R_PC) = read.immval;
                    else             ureg(R_PC) = ureg(read.SR1);
                } 
            }break;
            case OP_MOV:{
                if(read.immcond) ureg(read.DR) = read.immval;
                else             ureg(read.DR) = ureg(read.SR1);
            }break;
            case OP_ST:{
                mem_write(ureg(read.DR), ureg(read.SR1));
            }break; 
            case OP_LD:{
                ureg(read.DR) = mem_read_u64(ureg(read.SR1));
            }break;
        }//switch(read.OP)
    }//tick()
};

#endif