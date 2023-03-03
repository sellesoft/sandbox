

struct{
    str8 buffer; // entire buffer
    str8 stream; // progressing stream
    u64 line;
    u64 column;
    Entity* working;
}parser;

#define error(...) LogE("ontology_parser", "ontology.data(", parser.line, ",", parser.column, "): ", __VA_ARGS__)
#define warning(...) LogW("ontology_parser", "ontology.data(", parser.line, ",", parser.column, "): ", __VA_ARGS__)
#define advance() {if(utf8codepoint(parser.stream.str)=='\n'){parser.line++;parser.column=1;} else parser.column++; str8_advance(&parser.stream); }
#define str8case(str) str8_static_hash64(str8_static_t(str))

// returns a slice of 'stream' up until the first non-word character, including _
// advances stream
str8 eat_word() {
    str8 out = parser.stream;
    while(1) {
        u32 cp = utf8codepoint(parser.stream.str);
        if(!isalnum(cp) && cp != '_') break;
        advance();
    }
    out.count = parser.stream.str - out.str;

    return out;
}

str8 eat_whitespace(){
    str8 out = parser.stream;
    while(1) {
        u64 cp = utf8codepoint(parser.stream.str);
        if(!is_whitespace(cp)) break;
        advance();
    }
    out.count = parser.stream.str - out.str;
    return out;
}

//assmues that this is called while stream is at a '"'
//advances the stream to the last '"'
str8 eat_string(){
    advance();
    b32 skip = 0;
    str8 out = parser.stream;
    while(*parser.stream.str != '"'){
        u64 cp = utf8codepoint(parser.stream.str);
        if(!skip && cp == '"') break;
        if(skip) skip = 0;
        if(cp == '\\') skip = 1;
        advance();
    }
    out.count = parser.stream.str - out.str;
    return out;
}

// highest level of parsing. any word found at global scope is the name of a new concept
void parse_concept(){
#define pass_key(key) eat_whitespace(); if(*parser.stream.str != ':'){ error("expected : after key '", STRINGIZE(key), "' while parsing concept '", parser.working->name, "'."); return; } advance(); eat_whitespace();
    advance();
    while(1){
        eat_whitespace();
        if(!isalnum(utf8codepoint(parser.stream.str)) && *parser.stream.str != '_'){
            error("expected a key in concept definition.");
            return;
        }
        str8 key = eat_word();
        switch(str8_hash64(key)){
            case str8case("desc"):{
                pass_key(desc);
                if(*parser.stream.str != '"'){ error("expected a string for 'desc' of concept '", parser.working->name, "'."); return; }
                parser.working->desc = eat_string();
                advance();
            }break;
            case str8case("instance_of"):{
                pass_key(instance_of);
                if(!isalnum(utf8codepoint(parser.stream.str)) && *parser.stream.str != '_') { error("expected a concept identifier for 'instance of' of concept '", parser.working->name, "'."); return; }
                str8 name = eat_word();
                Entity* ref = strmap_at(&storage.concepts, name);
                if(!ref) { error("the concept '", name, "' is not defined."); return; }
                
            }break;
            case str8case("plural"):{
                // TODO(sushi) decide if we are going to use this or not.
            }break; 
            case str8case("has_quality"):{

            }break;
        }

        eat_whitespace();
        if(*parser.stream.str != ';') { error("missing ';' after value for key."); return; }
        advance();
    }

    if(!parser.working->desc.str){
        warning("the concept '", parser.working->name, "' doesn't have a description defined for it. define one using the key 'desc'.");
    }
#undef pass_key
}

// parses ontology.data
void parse_ontology(){
    File* f = file_init(STR8("data/ontology.data"), FileAccess_Read);
    //NOTE(sushi) we DONT deallocate this buffer, because we use it to point str8's into, instead of making many copies of its contents
    parser.buffer = file_read_alloc(f, f->bytes, deshi_allocator);
    file_deinit(f);
    parser.stream = parser.buffer;
    parser.line = 1;
    parser.column = 1;

    // the first thing we need to do is gather all of the concept names
    // because concept definitions can reference other ones
    while(parser.stream){
        eat_whitespace();
        if(isalnum(utf8codepoint(parser.stream.str)) || *parser.stream.str == '_'){
            str8 name = eat_word();
            if(strmap_find(&storage.concepts, name) != -1) { error("the concept '", name, "' was defined twice."); return; }
            FixMe;
            // smval v;
            // v.type = smval_entity;
            // v.entity = storage.entities;
            // strmap_add(&storage.concepts, name, {smval_entity, Entity(name)});
            eat_whitespace();
            if(*parser.stream.str != ':'){ error("expected ':' after concept name defintion."); return; }
            advance();
            eat_whitespace();
            if(*parser.stream.str != '{'){ error("expected '{' after ':' for concept definition. concepts must be defined as a dictionary."); return; }
            u64 layers = 1;
            while(layers) {
                advance();
                if(*parser.stream.str == '{') layers++;
                else if(*parser.stream.str == '}') layers--;
                if(!parser.stream) { error("unexpected eof."); return; }
            }
            advance();
        }else { error("expected a name for a concept defintion."); return; }
    }

    parser.stream = parser.buffer;
    parser.line = 1;
    parser.column = 1;

    while(parser.stream){
        eat_whitespace();
        parser.working = concept_map_at(eat_word());
        eat_whitespace();
        advance(); // we already know there's a : here
        eat_whitespace();
        parse_concept();
    }
}

#undef error
#undef warning
#undef advance
#undef str8case