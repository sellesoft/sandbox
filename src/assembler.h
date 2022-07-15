
//NOTE this file is temporarily a header because vscode's cpp intellisense doesnt like unity build !!
#include "kigu/common.h"
#include "kigu/unicode.h"
#include "core/file.h"
#include "core/memory.h"
#include "hardware.h"


/*
    <prog> = { <opcode> | <label> | <var> }
    
    <var>    = <id> "=" ( <string> | <num> )
    <label>  = <id> ":"
    <opcode> = "nop"
            | "add"  <reg> "," <operand>
            | "sub"  <reg> "," <operand>
            | "mul"  <reg> "," <operand>
            | "smul" <reg> "," <operand>
            | "div"  <reg> "," <operand>
            | "sdiv" <reg> "," <operand>
            | "and"  <reg> "," <operand>
            | "or"   <reg> "," <operand>
            | "xor"  <reg> "," <operand>
            | "shr"  <reg> "," <operand>
            | "shl"  <reg> "," <operand>
            | "not"  <reg> 
            | "jmp"  ( <operand> | <label> ) 
            | "jz"   ( <operand> | <label> )
            | "jnz"  ( <operand> | <label> )
            | "jp"   ( <operand> | <label> )
            | "jn"   ( <operand> | <label> )
            | "mov"  <operand> "," <operand>

    <operand> = <reg> | <num>
    <reg> = "%" <int>
    <num> = <int> | <float>

*/

#define stream_next { str8_advance(&stream); linecol++; } 

#define str8case(str) str8_static_hash64(str8_static_t(str))

#define asmerr(...)  Log("", filename, "(", linenum, ",", linecol, "): ", ErrorFormat("error: "),     __VA_ARGS__)
#define asmwarn(...) Log("", filename, "(", linenum, ",", linecol, "): ", WarningFormat("warning: "), __VA_ARGS__)

#define nbits(n) ((u64(1) << u64(n)) - u64(1)) 

b32 is_white_space(u32 codepoint){
	switch(codepoint){
		case '\t': case '\n': case '\v': case '\f':  case '\r':
		case ' ': case 133: case 160: case 5760: case 8192:
		case 8193: case 8194: case 8195: case 8196: case 8197:
		case 8198: case 8199: case 8200: case 8201: case 8202:
		case 8232: case 8239: case 8287: case 12288: return true;
		default: return false;
	}
}

b32 is_digit(u32 codepoint){
	switch(codepoint){
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			return true;
		default:
			return false;
	}
}

//any unicode codepoint that is not within the ascii range, is not whitespace, or is an alphanumeric character from 
//ascii is an identifier character
b32 is_identifier_char(u32 codepoint){
	if(isalnum(codepoint))        return true;
	if(is_white_space(codepoint)) return false;
	if(codepoint == '_')          return true;
	if(codepoint > 127)           return true;
	return false;
}

u8 lowerspace[256] = {0};

b32 is_opcode(str8 in){
    if(in.count > 256) return OP_UNKNOWN;
    //not necessary, so remove if unwanted later
    forI(in.count) lowerspace[i] = tolower(in.str[i]);
    switch(str8_hash64({lowerspace, in.count})){
        case str8case("nop"):   return OP_NOP;
        case str8case("add"):   return OP_ADD;
        case str8case("sub"):   return OP_SUB;
        case str8case("mul"):   return OP_MUL;
        case str8case("smul"):  return OP_SMUL;
        case str8case("div"):   return OP_DIV;
        case str8case("sdiv"):  return OP_SDIV;
        case str8case("and"):   return OP_AND;
        case str8case("or"):    return OP_OR;
        case str8case("xor"):   return OP_XOR;
        case str8case("shr"):   return OP_SHR;
        case str8case("shl"):   return OP_SHL;
        case str8case("not"):   return OP_NOT;
        case str8case("jmp"):   return OP_JMP;
        case str8case("jz"):    return OP_JZ;
        case str8case("jnz"):   return OP_JNZ;
        case str8case("jp"):    return OP_JP;
        case str8case("jn"):    return OP_JN;
        case str8case("mov"):   return OP_MOV;
    }
    return OP_UNKNOWN;
}

void print_instr(u64 instr){
    InstrRead ir = ReadInstr(instr);
    Log("", 
        "instr: ", opnames[ir.OP], "\n",
        "      dr:", ir.DR, "\n",
        "      qr:", ir.QR, "\n"
        "      rr:", ir.RR, "\n",
        "     sr1:", ir.SR1, "\n",
        "     sr2:", ir.SR2, "\n",
        " immcond:", ir.immcond, "\n",
        "  immval:", ir.immval
    );
}

struct assembler{
    str8 filename;

    str8 stream;
	u32 linenum;
	u32 linecol;
	u8* line_start;

    str8 word;

    u8* chunk_start;

    Type opcode;

    u32 operand_count;
    u32 immsize;
    u64 instr;

    u64 operand1;
    u64 operand2;
    u64 operand3;

    array<u64> progout;
    
    str8 eat_whitespace(){
        str8 out = {stream.str, 0};
        while(is_white_space(decoded_codepoint_from_utf8(stream.str, 4).codepoint)){
            out.count += str8_advance(&stream).advance;
            linecol++;
            if(*stream.str == '\n'){
                line_start = stream.str;
                linenum++;
                linecol = 0;
            };
        }
        return out;
    }

    str8 eat_word(){
        str8 out = {stream.str, 0};
        while(is_identifier_char(decoded_codepoint_from_utf8(stream.str, 4).codepoint)){
            out.count += str8_advance(&stream).advance;
            linecol++;
        }   
        return out;
    }

    str8 eat_int(){
        str8 out = {stream.str, 0};
        while(is_digit(decoded_codepoint_from_utf8(stream.str, 4).codepoint)){
            out.count += str8_advance(&stream).advance;
            linecol++;
        } 
        return out;
    }

    u64 parse_reg(){
        u64 reg = stolli(eat_int());
        if(reg > R_COUNT){
            asmerr("there are only ", R_COUNT, " registers, but tried to access register ", reg);
            return npos;
        } 
        instr = (instr << 4) | (reg & nbits(4));
        return reg;
    }

    array<u64> assemble(str8 filename_in){
        filename = filename_in;

        File* file = file_init(filename, FileAccess_Read);
        str8 buffer = file_read_alloc(file, file->bytes, deshi_allocator);

        stream = buffer;
        linenum = 1;
        linecol = 1;
        line_start = stream.str;

        chunk_start = stream.str;

        opcode = 0;

        operand_count;
        immsize;
        instr = 0;

        enum{
            expect_reg = 1 << 0,
            expect_digit = 1 << 1,
            expect_id = 1 << 2,
            expect_operand = 1 << 3,
        }state = expect_id;

        while(stream){
            switch(decoded_codepoint_from_utf8(stream.str, 4).codepoint){
                case '\t': case '\n': case '\v': case '\f':  case '\r':
                case ' ': case 133: case 160: case 5760: case 8192:
                case 8193: case 8194: case 8195: case 8196: case 8197:
                case 8198: case 8199: case 8200: case 8201: case 8202:
                case 8232: case 8239: case 8287: case 12288:{
                    eat_whitespace();
                }continue;

                case '0': case '1': case '2': case '3': case '4': 
                case '5': case '6': case '7': case '8': case '9':{
                    // if(HasFlag(state, expect_operand)){
                    //     //we must be parsing an immediate value

                    // }
                    // if(!(HasFlag(state, expect_digit) || HasFlag(state, expect_operand))) asmerr("unexpected digit.");
                    // else{

                    // }
                }break;

                case '%':{
                    if(!(HasFlag(state, expect_reg) || HasFlag(state, expect_operand))) asmerr("unexpected register.");
                    stream_next;
                    
                }break;
                
                default:{
                    if(is_identifier_char(decoded_codepoint_from_utf8(stream.str, 4).codepoint)){
                        if(!HasFlag(state, expect_id)) asmerr("unexpected identifier.");
                        
                        word = eat_word();
                        opcode = is_opcode(word);
                        if(opcode == OP_UNKNOWN) {
                            asmerr("unknown word '", word, "'");
                            break;
                        }
                        switch(opcode){
                            case OP_NOP:{
                                progout.add(0);
                            }break;
                            case OP_ADD: 
                            case OP_SUB:
                            case OP_MUL:
                            case OP_SMUL:
                            case OP_AND:
                            case OP_OR:
                            case OP_XOR:
                            case OP_SHR:
                            case OP_SHL:{
                                u64 maximm = 49;
                                instr = opcode;
                                eat_whitespace();
                                if(*stream.str == '%'){
                                    stream_next;
                                    operand1 = parse_reg();
                                }else{
                                    asmerr("first operand of ", CyanFormatDyn(word), " must be a register, did you forget '%'?");
                                    break;
                                } 
                                eat_whitespace();
                                if(*stream.str != ','){
                                    asmerr("unexpected token after first operand, expected ','");
                                    break;
                                } 
                                stream_next;
                                eat_whitespace();
                                if(*stream.str == '%'){
                                    stream_next;
                                    operand2 = parse_reg();
                                    if(*stream.str != ','){
                                        //binop so set operand 2 to 1 and 3 to 2 and early out
                                        instr |= operand1 & nbits(4);
                                        instr <<= 5;
                                        instr |= operand2;
                                        instr <<= 45;
                                        progout.add(instr);
                                        break;
                                    }
                                }else if(is_digit(*stream.str)){
                                    // binop overload
                                    instr = (instr << 4) | (operand1 & nbits(4));
                                    operand2 = stolli(eat_int());
                                    instr = (instr << 1) | 1; //set imm bit
                                    if(operand2 > nbits(maximm)){
                                        asmerr("attempt to use immediate value ", operand2, " which is larger than the CPU's max immediate value for ", CyanFormatDyn(word), " (", maximm, " bits, which is ", nbits(maximm), "). TODO(sushi) implement an instruction for mov'ing a 64 bit value.");
                                        break;
                                    }else{
                                        instr = (instr << maximm) | operand2;
                                        progout.add(instr);
                                    }
                                    break;
                                }
                                eat_whitespace();
                                if(*stream.str != ','){
                                    break;
                                } 
                                stream_next;
                                eat_whitespace();
                                if(*stream.str == '%'){
                                    stream_next;
                                    instr <<= 1;
                                    operand3 = parse_reg();
                                    instr <<= 45;
                                    progout.add(instr);
                                }else if(is_digit(*stream.str)){
                                    operand3 = stolli(eat_int());
                                    instr = (instr << 1) | 1; //set imm bit
                                    if(operand3 > nbits(maximm)){
                                        asmerr("attempt to use immediate value ", operand2, " which is larger than the CPU's max immediate value for ", CyanFormatDyn(word), " (", maximm, " bits, which is ", nbits(maximm), "). TODO(sushi) implement an instruction for mov'ing a 64 bit value.");
                                    }else{
                                        instr = (instr << maximm) | operand3;
                                        progout.add(instr);
                                    }
                                }
                            }break;
                            case OP_MOV:{
                                u64 maximm = 53;
                                instr = opcode;
                                eat_whitespace();
                                if(*stream.str == '%'){
                                    stream_next;
                                    operand1 = parse_reg();
                                }else{
                                    asmerr("first operand of ", CyanFormatDyn(word), " must be a register, did you forget '%'?");
                                    break;
                                } 
                                eat_whitespace();
                                if(*stream.str != ','){
                                    asmerr("unexpected token after first operand, expected ','");
                                    break;
                                } 
                                stream_next;
                                eat_whitespace();
                                if(*stream.str == '%'){
                                    instr = (instr << 1);
                                    stream_next;
                                    operand2 = parse_reg();
                                    instr <<= 49;
                                    progout.add(instr);
                                }else if(is_digit(*stream.str)){
                                    operand2 = stolli(eat_int());
                                    instr = (instr << 1) | 1; //set imm bit
                                    if(operand2 > nbits(maximm)){
                                        asmerr("attempt to use immediate value ", operand2, " which is larger than the CPU's max immediate value for ", CyanFormatDyn(word), " (", maximm, " bits, which is ", nbits(maximm), "). \nYou must put this immediate value into a register then use it as an operand. \nTODO(sushi) implement an instruction for mov'ing a 64 bit value.");
                                    }else{
                                        instr = (instr << maximm) | operand2;
                                        progout.add(instr);
                                    }
                                }
                                if(*stream.str == ','){
                                    asmerr(word, " only takes two operands.");
                                }
                            }break;
                            case OP_JMP:  
                            case OP_JZ:  
                            case OP_JNZ: 
                            case OP_JP: 
                            case OP_JN:{
                                instr = opcode;
                                eat_whitespace();
                                if(*stream.str == '%'){
                                    stream_next;
                                    operand1 = parse_reg();
                                    instr <<= 54;
                                    progout.add(instr);
                                }else{
                                    asmerr("operand of ", CyanFormatDyn(word), " must be a register, did you forget '%'?");
                                    break;
                                }
                                if(*stream.str == ','){
                                    asmerr(CyanFormatDyn(word), " only takes one operand");
                                    break;
                                }
                            }break;
                            case OP_NOT:{
                                instr = opcode;
                                if(*stream.str == '%'){
                                    stream_next;
                                    parse_reg();
                                }else asmerr("first operand of ", CyanFormatDyn(word), " must be a register, did you forget '%'?");
                                eat_whitespace();
                                if(*stream.str != ','){
                                    asmerr(word, " expects 2 operands.");
                                    break;
                                }
                                if(*stream.str == '%'){
                                    stream_next;
                                    parse_reg();
                                }else asmerr("second operand of ", CyanFormatDyn(word), " must be a register, did you forget '%'?");
                                instr <<= 50;
                                progout.add(instr);
                            }break;
                        }
                        
                    }
                }break;
            }
            str8_advance(&stream);
        }

        return progout;
    }
};

