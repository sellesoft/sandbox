
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

#define pass_key(key)                                                                                       \
do{                                                                                                              \
    eat_whitespace();                                                                                            \
    if(*parser.stream.str != ':'){                                                                               \
        error("expected : after key '", STRINGIZE(key), "' while parsing concept '", parser.working->name, "'.");\
        return;                                                                                                  \
    }                                                                                                            \
    advance();                                                                                                   \
    eat_whitespace();                                                                                            \
}while(0);

#define check_key_duplicate(key)                                                                                                      \
do{                                                                                                                                   \
    if(strmap_at(&parser.working->predicates, STR8(STRINGIZE(key)))){                                                                 \
        error("the key '", STRINGIZE(key), "' was already specified. To assign multiple values to a key, use an array (if allowed).");\
        return;                                                                                                                       \
    }                                                                                                                                 \
}while(0);

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

// eats a key and advances to the '{' following it.
// expects to be called just before the key
str8 eat_key(){
    str8 out = eat_word();
    eat_whitespace();
    if(*parser.stream.str != ':'){
        error("expected a key, but it is either missing, invalid, or missing a : after it. valid keys may only use ascii alphanumeric characters and '_'.");
        return {0};
    }

    advance();
    eat_whitespace();
    return out;
}

//assmues that this is called while stream is at a '"'
//advances the stream to the character following the last '"'
str8 eat_string(){
    advance();
    b32 skip = 0;
    str8 out = parser.stream;
    while(*parser.stream.str != '"' || skip){
        if(skip) skip = 0;
        if(*parser.stream.str == '\\') skip = 1;
        if(!parser.stream) { error("unexpected end of file while parsing string, missing \" ?"); return {0}; }
        advance();
    }
    out.count = parser.stream.str - out.str;
    advance();
    return out;
}

// eats an integer
// expects to be called at the beginning of the integer
str8 eat_int(){
    str8 out = parser.stream;
    while(isdigit(*parser.stream.str)) advance();
    out.count = parser.stream.str - out.str;
    return out;
}

// eats a float
// expects to be called at the beginning of the float
str8 eat_float(){
    str8 out = parser.stream;
    if(*parser.stream.str == '+' || *parser.stream.str == '-') advance();
    b32 dot_found = 0;
    while(1){
        if(*parser.stream.str == '.'){
            if(dot_found){
                error("invalid float string. found multiple '.'s.");
                return {0};
            } else dot_found = 1;
        }
        else if(!isdigit(*parser.stream.str)){
            break;
        }
        advance();
    }
    out.count = parser.stream.str - out.str;
    return out;
}

// returns stb array of strval, must be freed by user
// assumes that this is called while stream is at the '[' 
smval parse_array(){
    advance(); // skip [
    b32 skip = 0;
    smval out{smval_array};
    while(1){
        eat_whitespace();
        if(*parser.stream.str == '"'){
            smval v{smval_string};
            v.str = eat_string();
            if(!v.str.str) return {0};
            arrput(out.arr, v);
        }else if(*parser.stream.str == '['){
            TestMe;
            smval v = parse_array();
            arrput(out.arr, v);
        }else if(isalnum(*parser.stream.str) || *parser.stream.str == '_'){
            smval v{smval_entity};
            str8 name = eat_word(); if(!name.str) return {0};
            smval* ref = strmap_at(&storage.concepts, name); 
            if(!ref){ error("the concept '", name, "' is not defined."); return {0}; }
            v.entity = ref->entity;
            arrput(out.arr, v);
        }else{
            error("invalid value for array. expected an entity id, an array, or a string.");
            return {0};
        }
        eat_whitespace();
        if(*parser.stream.str == ']' && !skip) break;
        if(*parser.stream.str != ','){
            error("expected a , following array element or ] to end the array.");
            return {0};
        }else advance();
    }
    return out;
}

// parses the action key, found in adverts
// expects to be called at the opening '{'
smval parse_action(){
    advance(); // skip {
    smval out{smval_map};
    while(1){
        eat_whitespace();
        if(*parser.stream.str == '}'){
            if(!out.map.count){
                error("nothing defined for action.");
                return {0};
            }
            advance();
            break;
        }

        str8 key = eat_key(); if(!key.str) return {0};
        if(strmap_at(&out.map, key)){
            error("key '", key, "' was defined twice.");
            return {0};
        }

        switch(str8_hash64(key)){
            case str8case("time"):{
                if(*parser.stream.str != '{'){
                    error("expected a map for 'time' key.");
                    return {0};
                }
                advance();
                
                smval t{smval_unsigned_int};
                while(1){
                    eat_whitespace();
                    if(*parser.stream.str == '}'){
                        advance();
                        break;
                    }
                    str8 key = eat_key(); if(!key.str) return {0};
                    #define check_valid_time() if(!isdigit(*parser.stream.str)){ error("expected a numeric value for time."); return {0}; }
                    switch(str8_hash64(key)){
                        case str8case("seconds"):{check_valid_time();
                            t.unsigned_int += stolli(eat_int());
                        }break;
                        case str8case("minutes"):{check_valid_time();
                            t.unsigned_int += 60*stolli(eat_int());
                        }break;
                        case str8case("hours"):{check_valid_time();
                            t.unsigned_int += 60*60*stolli(eat_int());
                        }break;
                        case str8case("days"): {check_valid_time();
                            t.unsigned_int += 24*60*60*stolli(eat_int());
                        }break;
                        case str8case("months"): {check_valid_time();
                            t.unsigned_int += 31*24*60*60*stolli(eat_int());
                        }break;
                        case str8case("years"): {check_valid_time();
                            t.unsigned_int += 12*31*24*60*60*stolli(eat_int());
                        }break;
                        default:{
                            error("unknown time key '", key, "'. valid keys are 'seconds', 'minutes', 'hours', 'days', 'months', and 'years'");
                        }break;
                    }
                    #undef check_valid_time
                    eat_whitespace();
                    if(*parser.stream.str != ';'){
                        error("expected ; after value.");
                        return {0};
                    }
                    advance();
                }
                strmap_add(&out.map, key, t);
            }break;
            case str8case("costs"):{
                if(*parser.stream.str != '{'){
                    error("expected a map for 'costs' key.");
                    return {0};
                }
                advance();

                smval v{smval_map};
                while(1){
                    eat_whitespace();
                    if(*parser.stream.str == '}'){
                        advance();
                        break;
                    }
                    str8 key = eat_key(); if(!key.str) return {0};
                    if(strmap_at(&v.map, key)){
                        error("key '", key, "' defined more than once.");
                        return {0};
                    }
                    if(!isdigit(*parser.stream.str) && *parser.stream.str != '+' && *parser.stream.str != '-' && *parser.stream.str != '.') { 
                        error("expected a float for cost value."); 
                        return {0}; 
                    }
                    smval f{smval_float};
                    str8 s = eat_float(); if(!s.str) return {0};
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
                            error("unknown key '", key, "'. valid keys are 'food', 'hunger', 'bladder', 'mood', and 'sleep'.");
                            return {0};
                        }break;
                    }
                    eat_whitespace();
                    if(*parser.stream.str != ';'){
                        error("expected ; after value.");
                        return {0};
                    }
                    advance();
                }
                strmap_add(&out.map, key, v);
            }break;
            case str8case("reqs"):{
                //TODO(sushi) implement reqs, what we have here just skips over all of it.
                if(*parser.stream.str != '{'){
                    error("expected a map for 'costs' key.");
                    return {0};
                }
                advance();
                u32 layers = 1;
                while(layers){
                    if(!parser.stream){
                        error("unexpected end of file, missing } or extra {");
                        return {0};
                    }
                    if     (*parser.stream.str == '{') layers++;
                    else if(*parser.stream.str == '}') layers--;
                    advance();
                }
            }break;
            default:{
                error("unknown key '", key, "'.");
                return {0};
            }break;
        }

    } 
    return out;

}

// parses the adverts key, found in templates
// expects to be called at the opening '{'
smval parse_adverts(){
    advance(); // skip {
    smval out{smval_map};
    while(1){
        eat_whitespace();
        if(*parser.stream.str == '}'){
            advance();
            break;
        }

        str8 key = eat_key(); if(!key.str) return {0};
        if(strmap_at(&out.map, key)){
            error("key '", key, "' was defined twice.");
            return {0};
		}

        if(*parser.stream.str != '{'){
            error("expected a map for advert defintion of '", key, "'.");
            return {0};
        }
        advance();
        
        smval advert{smval_map};
        while(1){
            eat_whitespace();
            if(*parser.stream.str == '}'){
                advance();
                if(!advert.map.count){
                    error("no actions were defined for the advert '", key, "'.");
                    return {0};
                }
                break;
            }

            str8 key = eat_key(); if(!key.str) return {0};
            switch(str8_hash64(key)){
                case str8case("action"):{
                    smval v = parse_action(); if(!v.type) return {0};
                    strmap_add(&advert.map, key, v);
                }break;
                default:{
                    error("unknown key '", key, "'.");
                    return {0};
                }break;
            }
        }
    }
    return out;
}

// parses the template key, which is a nested map
// expects to be called at the opening '{'
smval parse_template(){
    advance(); // skip {
    smval out{smval_map};
    while(1){
        eat_whitespace();
        if(*parser.stream.str == '}'){
            advance();
            break;
        }
        if(!isalnum(*parser.stream.str) && *parser.stream.str != '_'){
            error("expected a key in template definition.");
            return {0};
        }
        str8 key = eat_key(); if(!key.str) return {0};
        switch(str8_hash64(key)){
            case str8case("adverts"):{
                smval v = parse_adverts(); if(!v.type) return {0};
            }break;
            default:{
                error("unknown key '", key, "'.");
                return {0};
            }break;
        }
    }
    return out;
}

// highest level of parsing. any word found at global scope is the name of a new concept
void parse_concept(){
    advance();
    while(1){
        eat_whitespace();
        if(*parser.stream.str == '}'){
            advance();
            break;
        }
        if(!isalnum(utf8codepoint(parser.stream.str)) && *parser.stream.str != '_'){
            error("expected a key in concept definition.");
            return;
        }

        #define check_for_semicolon() eat_whitespace(); if(*parser.stream.str != ';') { error("missing ';' after value for key."); return; }
        str8 key = eat_word();
        switch(str8_hash64(key)){
            case str8case("desc"):{
                check_key_duplicate(desc);
                pass_key(desc);
                if(*parser.stream.str != '"'){ error("expected a string for 'desc' of concept '", parser.working->name, "'."); return; }
                parser.working->desc = eat_string();
            }break;
            case str8case("instance_of"):{
                check_key_duplicate(instance_of);
                pass_key(instance_of);

                if(*parser.stream.str == '['){
                    smval v = parse_array(); if(!v.type) return;
                    forI(arrlen(v.arr)){ // check that each element is an entity reference.
                        if(v.arr[i].type != smval_entity){ error("invalid value at index ", i, " of 'instance_of' value. valid values for instance_of are an entity id or an array of only entities ids."); return; }
                    }
                    strmap_add(&parser.working->predicates, STR8("instance_of"), v);
                    advance();
                }else if(isalnum(*parser.stream.str) || *parser.stream.str == '_'){
                    str8 name = eat_word();
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error("the concept '", name, "' is not defined."); return; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&parser.working->predicates, STR8("instance_of"), v);
                }else{
                    error("invalid value for 'instance_of' key, valid values are entity ids or an array of only entity ids.");
                    return;
                }
                check_for_semicolon();
            }break;
            case str8case("subclass_of"):{
                check_key_duplicate(subclass_of);
                pass_key(subclass_of);

                if(*parser.stream.str == '['){
                    smval v = parse_array(); if(!v.type) return;
                    forI(arrlen(v.arr)){ // check that each element is an entity reference.
                        if(v.arr[i].type != smval_entity){ error("invalid value at index ", i, " of 'subclass_of' value. valid values for subclass_of are an entity id or an array of only entities ids."); return; }
                    }
                    strmap_add(&parser.working->predicates, STR8("subclass_of"), v);
                    advance();
                }else if(isalnum(*parser.stream.str) || *parser.stream.str == '_'){
                    str8 name = eat_word();
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error("the concept '", name, "' is not defined."); return; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&parser.working->predicates, STR8("subclass_of"), v);
                }else{
                    error("invalid value for 'subclass_of' key, valid values are entity ids or an array of only entity ids.");
                    return;
                }
                check_for_semicolon();
            }break;
            case str8case("plural"):{
                check_key_duplicate(plural);
                pass_key(plural);
                // TODO(sushi) decide if we are going to use this or not.
                if(*parser.stream.str != '"'){
                    error("invalid value for 'plural' key. expected a string.");
                    return;
                }
                str8 str = eat_string();
                check_for_semicolon();
            }break; 
            case str8case("has_quality"):{
                check_key_duplicate(has_quality);
                pass_key(has_quality);
                if(*parser.stream.str == '['){
                    smval v = parse_array();
                    forI(arrlen(v.arr)){
                        if(v.arr[i].type != smval_string){
                            error("invalid value at index ", i, " in array value of 'has_quality'. valid values are strings.");
                            return;
                        }
                    }
                    strmap_add(&parser.working->predicates, STR8("has_quality"), v);
                    advance();
                }else if(*parser.stream.str == '"'){
                    smval v{smval_string};
                    str8 str = eat_string(); if(!str.str) return;
                    v.str = str;
                    strmap_add(&parser.working->predicates, STR8("has_quality"), v);
                    advance();
                }else{
                    error("invalid value for key 'has_quality'. valid values are strings or an array of only strings.");
                    return;
                }
                check_for_semicolon();
            }break;
            case str8case("template"):{
                check_key_duplicate(template);
                pass_key(template);
                smval v = parse_template(); if(!v.type) return;
                strmap_add(&parser.working->predicates, STR8("template"), v);
            }break;
            case str8case("part_of"):{
                check_key_duplicate(part_of);
                pass_key(part_of);
                if(*parser.stream.str == '['){
                    smval v = parse_array();
                    forI(arrlen(v.arr)){
                        if(v.arr[i].type != smval_entity){
                            error("invalid value at index ", i, " in array value of 'part_of'. 'part_of' values may only be an entity id or an array of only entity ids.");
                            return;
                        }
                    }
                    strmap_add(&parser.working->predicates, STR8("part_of"), v);
                    advance();
                }else if(isalnum(*parser.stream.str) || *parser.stream.str == '_'){
                    str8 name = eat_word();
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error("the concept '", name, "' is not defined."); return; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&parser.working->predicates, STR8("part_of"), v);
                }else{
                    error("invalid value for 'part_of'. 'part_of' only accepts an entity id, or an array of only entity ids.");
                    return;
                }
                check_for_semicolon();
            }break;
            case str8case("has_parts"):{
                check_key_duplicate(has_parts);
                pass_key(has_parts);
                if(*parser.stream.str == '['){
                    smval v = parse_array();
                    forI(arrlen(v.arr)){
                        if(v.arr[i].type != smval_entity){
                            error("invalid value at index ", i, " in array value of 'has_parts'. 'has_parts' values may only be an entity id or an array of only entity ids.");
                            return;
                        }
                    }
                    strmap_add(&parser.working->predicates, STR8("has_parts"), v);
                    advance();
                }else if(isalnum(*parser.stream.str) || *parser.stream.str == '_'){
                    str8 name = eat_word();
                    smval* ref = strmap_at(&storage.concepts, name);
                    if(!ref) { error("the concept '", name, "' is not defined."); return; }
                    smval v{smval_entity};
                    v.entity = ref->entity;
                    strmap_add(&parser.working->predicates, STR8("has_parts"), v);
                }else{
                    error("invalid value for 'has_parts'. 'has_parts' only accepts an entity id, or an array of only entity ids.");
                    return;
                }
                check_for_semicolon();
            }break;
            default:{
                error("unknown key '", key, "'.");
                return;
            }break;
        }
        advance();
    }

    if(!parser.working->desc.str){
        warning("the concept '", parser.working->name, "' doesn't have a description defined for it. define one using the key 'desc'.");
    }

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

    // the first thing we need to do is gather all of the concept names
    // because concept definitions can reference other ones
    while(parser.stream){
        eat_whitespace();
        if(isalnum(utf8codepoint(parser.stream.str)) || *parser.stream.str == '_'){
            str8 name = eat_word();
            u64 idx = strmap_find(&storage.concepts, name);
            if(idx != -1) { error("the concept '", name, "' was defined twice."); return; }
            smval v{smval_entity}; 
            v.entity = ((Entity*)storage.entities->start) + storage_add_entity();
            v.entity->name = name; // TODO(sushi) maybe automatically replace underscores with spaces
            strmap_add(&storage.concepts, name, v);
            eat_whitespace();
            if(*parser.stream.str != ':'){ error("expected ':' after concept name defintion."); return; }
            advance();
            eat_whitespace();
            if(*parser.stream.str != '{'){ error("expected '{' after ':' for concept definition. concepts must be defined as a dictionary."); return; }
            u64 layers = 1;
            while(layers) {
                advance();
                if     (*parser.stream.str == '{') layers++;
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
        smval* v = strmap_at(&storage.concepts, eat_word());
        if(!v) return;
        parser.working = v->entity;
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
#undef pass_key
#undef check_key_duplicate