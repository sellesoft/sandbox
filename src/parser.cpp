struct EntityBlock{
    Entity* entity;
    str8 stream;
    u64 line, column;
    b32 parsed;
};

struct{
    str8 buffer; // entire buffer
    str8 stream; // progressing stream
    u64 line;
    u64 column;
    Entity* working;
    EntityBlock* blocks; // array of entity blocks, the entire definition of an entity
    EntityBlock** parsing_stack;
}parser;


#define error(block, ...) LogE("ontology_parser", "ontology.data(", (block)->line, ",", (block)->column, "): ", __VA_ARGS__);  DebugBreakpoint
#define warning(block, ...) LogW("ontology_parser", "ontology.data(", (block)->line, ",", (block)->column, "): ", __VA_ARGS__)
#define str8case(str) str8_static_hash64(str8_static_t(str))

#define advance(block)                   \
do{                                      \
    if(*(block)->stream.str=='\n'){      \
        (block)->line++;                 \
        (block)->column=1;               \
    } else (block)->column++;            \
    str8_advance(&(block)->stream);      \
}while(0);                                    


#define pass_key(block, key)                                                                                    \
do{                                                                                                             \
    eat_whitespace(block);                                                                                           \
    if(*block->stream.str != ':'){                                                                               \
        error(block, "expected : after key '", STRINGIZE(key), "' while parsing concept '", (block)->entity->name, "'.");\
        return;                                                                                                 \
    }                                                                                                           \
    advance(block);                                                                                                  \
    eat_whitespace(block);                                                                                           \
}while(0);

#define check_key_duplicate(block, key)                                                                                                      \
do{                                                                                                                                   \
    if(strmap_at(&block->entity->predicates, STR8(STRINGIZE(key)))){                                                                 \
        error(block, "the key '", STRINGIZE(key), "' was already specified. To assign multiple values to a key, use an array (if allowed).");\
        return;                                                                                                                       \
    }                                                                                                                                 \
}while(0);

str8 eat_whitespace(EntityBlock* block){
    str8 out = block->stream;
    while(is_whitespace(*block->stream.str)) advance(block);
    out.count = block->stream.str - out.str;
    return out;
}

// returns a slice of 'stream' up until the first non-word character, including _
// advances stream
str8 eat_word(EntityBlock* block) {
    str8 out = block->stream;
    while(isalnum(*block->stream.str) || *block->stream.str == '_') advance(block);
    out.count = block->stream.str - out.str;
    return out;
}

// eats a key and advances to the '{' following it.
// expects to be called just before the key
str8 eat_key(EntityBlock* block){
    str8 out = eat_word(block);
    eat_whitespace(block);
    if(*block->stream.str != ':'){
        error(block, block,"expected a key, but it is either missing, invalid, or missing a : after it. valid keys may only use ascii alphanumeric characters and '_'.");
        return {0};
    }

    advance(block);
    eat_whitespace(block);
    return out;
}

//assmues that this is called while stream is at a '"'
//advances the stream to the character following the last '"'
str8 eat_string(EntityBlock* block){
    advance(block);
    b32 skip = 0;
    str8 out = block->stream;
    while(*block->stream.str != '"' || skip){
        if(skip) skip = 0;
        if(*block->stream.str == '\\') skip = 1;
        if(!block->stream) { error(block, "unexpected end of file while parsing string, missing \" ?"); return {0}; }
        advance(block);
    }
    out.count = block->stream.str - out.str;
    advance(block);
    return out;
}

// eats an integer
// expects to be called at the beginning of the integer
str8 eat_int(EntityBlock* block){
    str8 out = block->stream;
    while(isdigit(*block->stream.str)) advance(block);
    out.count = parser.stream.str - out.str;
    return out;
}

// eats a float
// expects to be called at the beginning of the float
str8 eat_float(EntityBlock* block){
    str8 out = block->stream;
    if(*block->stream.str == '+' || *block->stream.str == '-') advance(block);
    b32 dot_found = 0;
    while(1){
        if(*block->stream.str == '.'){
            if(dot_found){
                error(block, "invalid float string. found multiple '.'s.");
                return {0};
            } else dot_found = 1;
        }
        else if(!isdigit(*block->stream.str)){
            break;
        }
        advance(block);
    }
    out.count = block->stream.str - out.str;
    return out;
}

// returns stb array of strval, must be freed by user
// assumes that this is called while stream is at the '[' 
smval parse_array(EntityBlock* block){
    advance(block); // skip [
    b32 skip = 0;
    smval out{smval_array};
    while(1){
        eat_whitespace(block);
        if(*block->stream.str == '"'){
            smval v{smval_string};
            v.str = eat_string(block);
            if(!v.str.str) return {0};
            arrput(out.arr, v);
        }else if(*block->stream.str == '['){
            TestMe;
            smval v = parse_array(block);
            arrput(out.arr, v);
        }else if(isalnum(*block->stream.str) || *block->stream.str == '_'){
            smval v{smval_entity};
            str8 name = eat_word(block); if(!name.str) return {0};
            smval* ref = strmap_at(&storage.concepts, name); 
            if(!ref){ error(block, "the concept '", name, "' is not defined."); return {0}; }
            v.entity = ref->entity;
            arrput(out.arr, v);
        }else{
            error(block, "invalid value for array. expected an entity id, an array, or a string.");
            return {0};
        }
        eat_whitespace(block);
        if(*block->stream.str == ']' && !skip) break;
        if(*block->stream.str != ','){
            error(block, "expected a , following array element or ] to end the array.");
            return {0};
        }else advance(block);
    }
    return out;
}

// parses the action key, found in adverts
// expects to be called at the opening '{'
smval parse_action(EntityBlock* block){
    advance(block); // skip {
    smval out{smval_map};
    while(1){
        eat_whitespace(block);
        if(*block->stream.str == '}'){
            if(!out.map.count){
                error(block, "nothing defined for action.");
                return {0};
            }
            advance(block);
            break;
        }

        str8 key = eat_key(block); if(!key.str) return {0};
        if(strmap_at(&out.map, key)){
            error(block, "key '", key, "' was defined twice.");
            return {0};
        }

        switch(str8_hash64(key)){
            case str8case("time"):{
                if(*block->stream.str != '{'){
                    error(block, "expected a map for 'time' key.");
                    return {0};
                }
                advance(block);
                
                smval t{smval_unsigned_int};
                while(1){
                    eat_whitespace(block);
                    if(*block->stream.str == '}'){
                        advance(block);
                        break;
                    }
                    str8 key = eat_key(block); if(!key.str) return {0};
                    #define check_valid_time() if(!isdigit(*block->stream.str)){ error(block, "expected a numeric value for time."); return {0}; }
                    switch(str8_hash64(key)){
                        case str8case("seconds"):{check_valid_time();
                            t.unsigned_int += stolli(eat_int(block));
                        }break;
                        case str8case("minutes"):{check_valid_time();
                            t.unsigned_int += 60*stolli(eat_int(block));
                        }break;
                        case str8case("hours"):{check_valid_time();
                            t.unsigned_int += 60*60*stolli(eat_int(block));
                        }break;
                        case str8case("days"): {check_valid_time();
                            t.unsigned_int += 24*60*60*stolli(eat_int(block));
                        }break;
                        case str8case("months"): {check_valid_time();
                            t.unsigned_int += 31*24*60*60*stolli(eat_int(block));
                        }break;
                        case str8case("years"): {check_valid_time();
                            t.unsigned_int += 12*31*24*60*60*stolli(eat_int(block));
                        }break;
                        default:{
                            error(block, "unknown time key '", key, "'. valid keys are 'seconds', 'minutes', 'hours', 'days', 'months', and 'years'");
                        }break;
                    }
                    #undef check_valid_time
                    eat_whitespace(block);
                    if(*block->stream.str != ';'){
                        error(block, "expected ; after value.");
                        return {0};
                    }
                    advance(block);
                }
                strmap_add(&out.map, key, t);
            }break;
            case str8case("costs"):{
                if(*block->stream.str != '{'){
                    error(block, "expected a map for 'costs' key.");
                    return {0};
                }
                advance(block);

                smval v{smval_map};
                while(1){
                    eat_whitespace(block);
                    if(*block->stream.str == '}'){
                        advance(block);
                        break;
                    }
                    str8 key = eat_key(block); if(!key.str) return {0};
                    if(strmap_at(&v.map, key)){
                        error(block, "key '", key, "' defined more than once.");
                        return {0};
                    }
                    if(!isdigit(*block->stream.str) && *block->stream.str != '+' && *block->stream.str != '-' && *block->stream.str != '.') { 
                        error(block, "expected a float for cost value."); 
                        return {0}; 
                    }
                    smval f{smval_float};
                    str8 s = eat_float(block); if(!s.str) return {0};
                    f.floating = stod(s);
                    switch(str8_hash64(key)){
                        case str8case("food"):{ 
                            strmap_add(&v.map, STR8("food"), f);
                        }break;
                        case str8case("hunger"):{ 
                            strmap_add(&v.map, STR8("hunger"), f);
                        }break;
                        case str8case("sleep"):{ 
                            strmap_add(&v.map, STR8("sleep"), f);
                        }break;
                        case str8case("bladder"):{ 
                            strmap_add(&v.map, STR8("bladder"), f);
                        }break;
                        case str8case("mood"):{ 
                            strmap_add(&v.map, STR8("mood"), f);
                        }break;
                        default:{
                            error(block, "unknown key '", key, "'. valid keys are 'food', 'hunger', 'bladder', 'mood', and 'sleep'.");
                            return {0};
                        }break;
                    }
                    eat_whitespace(block);
                    if(*block->stream.str != ';'){
                        error(block, "expected ; after value.");
                        return {0};
                    }
                    advance(block);
                }
                strmap_add(&out.map, key, v);
            }break;
            case str8case("reqs"):{
                //TODO(sushi) implement reqs, what we have here just skips over all of it.
                if(*block->stream.str != '{'){
                    error(block, "expected a map for 'costs' key.");
                    return {0};
                }
                advance(block);
                u32 layers = 1;
                while(layers){
                    if(!block->stream){
                        error(block, "unexpected end of file, missing } or extra {");
                        return {0};
                    }
                    if     (*block->stream.str == '{') layers++;
                    else if(*block->stream.str == '}') layers--;
                    advance(block);
                }
            }break;
            default:{
                error(block, "unknown key '", key, "'.");
                return {0};
            }break;
        }

    } 
    return out;

}

// parses the adverts key, found in templates
// expects to be called at the opening '{'
smval parse_adverts(EntityBlock* block){
    advance(block); // skip {
    smval out{smval_map};
    while(1){
        eat_whitespace(block);
        if(*block->stream.str == '}'){
            advance(block);
            break;
        }

        str8 key = eat_key(block); if(!key.str) return {0};
        if(strmap_at(&out.map, key)){
            error(block, "key '", key, "' was defined twice.");
            return {0};
		}

        if(*block->stream.str != '{'){
            error(block, "expected a map for advert defintion of '", key, "'.");
            return {0};
        }
        advance(block);
        
        smval advert{smval_map};
        while(1){
            eat_whitespace(block);
            if(*block->stream.str == '}'){
                advance(block);
                if(!advert.map.count){
                    error(block, "no actions were defined for the advert '", key, "'.");
                    return {0};
                }
                break;
            }

            str8 key = eat_key(block); if(!key.str) return {0};
            switch(str8_hash64(key)){
                case str8case("action"):{
                    smval v = parse_action(block); if(!v.type) return {0};
                    strmap_add(&advert.map, key, v);
                }break;
                default:{
                    error(block, "unknown key '", key, "'.");
                    return {0};
                }break;
            }
        }
    }
    return out;
}

// parses the template key, which is a nested map
// expects to be called at the opening '{'
smval parse_template(EntityBlock* block){
    advance(block); // skip {
    smval out{smval_map};
    while(1){
        eat_whitespace(block);
        if(*block->stream.str == '}'){
            advance(block);
            break;
        }
        if(!isalnum(*block->stream.str) && *block->stream.str != '_'){
            error(block, "expected a key in template definition.");
            return {0};
        }
        str8 key = eat_key(block); if(!key.str) return {0};
        switch(str8_hash64(key)){
            case str8case("adverts"):{
                smval v = parse_adverts(block); if(!v.type) return {0};
            }break;
            default:{
                error(block, "unknown key '", key, "'.");
                return {0};
            }break;
        }
    }
    return out;
}

// highest level of parsing. any word found at global scope is the name of a new concept
b32 parse_entity(EntityBlock* block){
    if(block->parsed) return 1; 
    if(!(block)->stream) DebugBreakpoint;\
    arrpush(parser.parsing_stack, block);
    defer{ arrpop(parser.parsing_stack); };
    eat_word(block);
    advance(block);
    eat_whitespace(block);
    advance(block);
    while(1){
        eat_whitespace(block);
        if(*block->stream.str == '}'){
            advance(block);
            break;
        }
        if(!isalnum(utf8codepoint(block->stream.str)) && *block->stream.str != '_'){
            error(block, "expected a key in concept definition.");
            return 0;
        }

        #define check_for_semicolon() eat_whitespace(block); if(*block->stream.str != ';') { error(block, "missing ';' after value for key."); return 0; }
        
        str8 key = eat_key(block); if(!key.str) return 0;
        if(strmap_at(&block->entity->predicates, key)){
            error(block, "the key '", key, "' was already specified. To assign multiple values to a key, use an array (if allowed).");
            return 0;       
        }

        switch(str8_hash64(key)){
            case str8case("desc"):{
                if(*block->stream.str != '"'){ error(block, "expected a string for 'desc' of concept '", block->entity->name, "'."); return 0; }
                block->entity->desc = eat_string(block);
            }break;
            case str8case("instance_of"):{
                if(*block->stream.str == '['){
                    smval v = parse_array(block); if(!v.type) return 0;
                    forI(arrlen(v.arr)){ // check that each element is an entity reference.
                        if(v.arr[i].type != smval_entity){ error(block, "invalid value at index ", i, " of 'instance_of' value. valid values for instance_of are an entity id or an array of only entities ids."); return 0; }
                    }
                    strmap_add(&block->entity->predicates, STR8("instance_of"), v);
                    advance(block);
                }else if(isalnum(*block->stream.str) || *block->stream.str == '_'){
                    str8 name = eat_word(block);
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error(block, "the concept '", name, "' is not defined."); return 0; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&block->entity->predicates, STR8("instance_of"), v);
                }else{
                    error(block, "invalid value for 'instance_of' key, valid values are entity ids or an array of only entity ids.");
                    return 0;
                }
                check_for_semicolon();
            }break;
            case str8case("subclass_of"):{
                if(*block->stream.str == '['){
                    smval v = parse_array(block); if(!v.type) return 0;
                    forI(arrlen(v.arr)){ // check that each element is an entity reference.
                        if(v.arr[i].type != smval_entity){ error(block, "invalid value at index ", i, " of 'subclass_of' value. valid values for subclass_of are an entity id or an array of only entities ids."); return 0; }
                        Entity* e = v.arr[i].entity;
                        // using find here kind of sucks, find a better way to do it 
                        if(!parse_entity(parser.blocks + strmap_find(&storage.concepts, e->name))) return 0;
                        if(e->type > block->entity->type) block->entity->type = e->type;
                    }
                    strmap_add(&block->entity->predicates, STR8("subclass_of"), v);
                    advance(block);
                }else if(isalnum(*block->stream.str) || *block->stream.str == '_'){
                    str8 name = eat_word(block);
                    u64 idx = strmap_find(&storage.concepts, name);
                    if(idx == -1) { error(block, "the concept '", name, "' is not defined."); return 0; }
                    Entity* e = (storage.concepts.values + idx)->entity;
                    if(!e->type){ // the entity we are referencing has not been parsed yet, but we need it to be
                        forI_reverse(arrlen(parser.parsing_stack)){
                            if(parser.parsing_stack[i] == parser.blocks+idx){
                                error(block, "circular subclassing between entity '", block->entity->name, "' and '", name, "'.");
                                return 0;
                            }
                        }
                        if(!parse_entity(parser.blocks + idx)) return 0;
                    }
                    block->entity->type = e->type; // inherit type of parent class
                    smval v{smval_entity};
                    v.entity = e;
                    strmap_add(&block->entity->predicates, STR8("subclass_of"), v);
                }else{
                    error(block, "invalid value for 'subclass_of' key, valid values are entity ids or an array of only entity ids.");
                    return 0;
                }
                check_for_semicolon();
            }break;
            case str8case("plural"):{
                // TODO(sushi) decide if we are going to use this or not.
                if(*block->stream.str != '"'){
                    error(block, "invalid value for 'plural' key. expected a string.");
                    return 0;
                }
                str8 str = eat_string(block);
                check_for_semicolon();
            }break; 
            case str8case("has_quality"):{
                if(*block->stream.str == '['){
                    smval v = parse_array(block);
                    forI(arrlen(v.arr)){
                        if(v.arr[i].type != smval_string){
                            error(block, "invalid value at index ", i, " in array value of 'has_quality'. valid values are strings.");
                            return 0;
                        }
                    }
                    strmap_add(&block->entity->predicates, STR8("has_quality"), v);
                    advance(block);
                }else if(*block->stream.str == '"'){
                    smval v{smval_string};
                    str8 str = eat_string(block); if(!str.str) return 0;
                    v.str = str;
                    strmap_add(&block->entity->predicates, STR8("has_quality"), v);
                    advance(block);
                }else{
                    error(block, "invalid value for key 'has_quality'. valid values are strings or an array of only strings.");
                    return 0;
                }
                check_for_semicolon();
            }break;
            case str8case("template"):{
                smval v = parse_template(block); if(!v.type) return 0;
                strmap_add(&block->entity->predicates, STR8("template"), v);
            }break;
            case str8case("part_of"):{
                if(*block->stream.str == '['){
                    smval v = parse_array(block);
                    forI(arrlen(v.arr)){
                        if(v.arr[i].type != smval_entity){
                            error(block, "invalid value at index ", i, " in array value of 'part_of'. 'part_of' values may only be an entity id or an array of only entity ids.");
                            return 0;
                        }
                    }
                    strmap_add(&block->entity->predicates, STR8("part_of"), v);
                    advance(block);
                }else if(isalnum(*block->stream.str) || *block->stream.str == '_'){
                    str8 name = eat_word(block);
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error(block, "the concept '", name, "' is not defined."); return 0; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&block->entity->predicates, STR8("part_of"), v);
                }else{
                    error(block, "invalid value for 'part_of'. 'part_of' only accepts an entity id, or an array of only entity ids.");
                    return 0;
                }
                check_for_semicolon();
            }break;
            case str8case("has_parts"):{
                if(*block->stream.str == '['){
                    smval v = parse_array(block);
                    forI(arrlen(v.arr)){
                        if(v.arr[i].type != smval_entity){
                            error(block, "invalid value at index ", i, " in array value of 'has_parts'. 'has_parts' values may only be an entity id or an array of only entity ids.");
                            return 0;
                        }
                    }
                    strmap_add(&block->entity->predicates, STR8("has_parts"), v);
                    advance(block);
                }else if(isalnum(*block->stream.str) || *block->stream.str == '_'){
                    str8 name = eat_word(block);
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error(block, "the concept '", name, "' is not defined."); return 0; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&block->entity->predicates, STR8("has_parts"), v);
                }else{
                    error(block, "invalid value for 'has_parts'. 'has_parts' only accepts an entity id, or an array of only entity ids.");
                    return 0;
                }
                check_for_semicolon();
            }break;
            default:{
                error(block, "unknown key '", key, "'.");
                return 0;
            }break;
        }
        advance(block);
    }

    if(!block->entity->desc.str){
        warning(block, "the concept '", parser.working->name, "' doesn't have a description defined for it. define one using the key 'desc'.");
    }
    block->parsed = 1;
    return 1;

    #undef check_for_semicolon
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

    str8 stream = parser.buffer;

    EntityBlock* block = (EntityBlock*)memalloc(sizeof(EntityBlock));
    defer { memzfree(block); };
    block->stream = parser.buffer;
    block->line = 1;
    block->column = 1;

    // the first thing we need to do is gather all of the concept names
    // because concept definitions can reference other ones
    while(block->stream){
        eat_whitespace(block);
        if(isalnum(*block->stream.str) || *block->stream.str == '_'){
            u64 start_line = block->line, start_column = block->column;
           
            
            str8 curr = block->stream;
            str8 name = eat_word(block);
            if(strmap_at(&storage.concepts, name)) { error(block, "the concept '", name, "' was defined twice."); return; }
           
            eat_whitespace(block);
            if(*block->stream.str != ':'){ error(block, "expected ':' after concept name defintion."); return; }
            advance(block);
            eat_whitespace(block);
            if(*block->stream.str != '{'){ error(block, "expected '{' after ':' for concept definition. concepts must be defined as a map."); return; }
            u64 layers = 1;
            while(layers) {
                advance(block);
                if     (*block->stream.str == '{') layers++;
                else if(*block->stream.str == '}') layers--;
                if(!block->stream) { error(block, "unexpected eof."); return; }
            }
            advance(block);
            curr.count = block->stream.str - curr.str;
            smval v{smval_entity}; 
            v.entity = ((Entity*)storage.entities->start) + storage_add_entity();
            v.entity->name = name; // TODO(sushi) maybe automatically replace underscores with spaces
            v.entity->raw = curr;
            u64 idx = strmap_add(&storage.concepts, name, v);

            // we make the EntityBlock and insert it into the same idx the Entity is
            // inserted into the concept map. this way they are parallel arrays.
            EntityBlock nu;
            nu.line = start_line;
            nu.column = start_column;
            nu.stream = curr;
            nu.entity = v.entity;
            nu.parsed = 0;
            arrins(parser.blocks, idx, nu);

        }else { error(block, "expected a name for a concept defintion."); return; }
    }

    smval* entity = strmap_at(&storage.concepts, STR8("entity"));
    smval* object = strmap_at(&storage.concepts, STR8("object"));
    smval* agent = strmap_at(&storage.concepts, STR8("agent"));

    if(!entity){ error(block, "the ontology is required to define 'entity'."); return; }
    entity->entity->type = entity_entity;

    if(!object){ error(block, "the ontology is required to define 'object', whose 'subclass_of' key is set to 'entity'."); return; }
    object->entity->type = entity_object;

    if(!agent){ error(block, "the ontology is required to define 'agent', whose 'subclass_of' key is set to 'object'."); return; }
    agent->entity->type = entity_agent;

    forI(arrlen(parser.blocks)){
        parse_entity(parser.blocks + i);
    }
}

#undef error
#undef warning
#undef advance
#undef str8case
#undef pass_key
#undef check_key_duplicate