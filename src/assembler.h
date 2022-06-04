
//NOTE this file is temporarily a header because vscode's cpp intellisense doesnt like unity build !!
#include "kigu/common.h"
#include "kigu/unicode.h"
#include "core/file.h"
#include "core/memory.h"

void assemble(str8 filename){
    File* file = file_init(filename, FileAccess_Read);
    str8 read = file_read_alloc(file, file->bytes, deshi_allocator);
    str8 cursor = read;

    str8 word; //a word consisting of [A-Za-z0-9]
    b32 regsym = 0; //true if % was the previous symbol

    u64 instr = 0; //current instruction being constructed

    while(cursor.count){
        DecodedCodepoint curchr = str8_index(cursor, 0);
        switch(curchr.codepoint){
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
            case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': 
            case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': 
            case 'V': case 'W': case 'X': case 'Y': case 'Z': 
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
            case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': 
            case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': 
            case 'v': case 'w': case 'x': case 'y': case 'z': {
                if(!word.str) word.str = cursor.str-1;
                else          word.count += 1;
            }break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':{
                if(word.str) word.count += 1;
                else if(regsym){

                }
            }break;
            case '{': case '}':{
                //always skip to allow for arbitrary styling of code 
            }break;
            case '\t':
            case ' ':{
                
            }break;

        }
    }
    str8_advance(&cursor);
}