#include "kigu/common.h"
#include "kigu/unicode.h"
#include "core/memory.h"
#include "math/vector.h"

struct Entity;

// represents keys in strmap
// NOTE(sushi): the key_string is NOT copied and is expected to be kept allocated by the user 
struct smkey{
	str8 key_string;
	u64 key;
};

struct smval;
struct strmap;
smval* strmap_at(strmap* map, str8 key);

// maps a string to a value
// the value may be a string, and Entity*, or an array of smvals
// this is used for the concept map and the predicate maps in Entities.
struct strmap{
	smkey* keys;
	smval* values;
	s64 count;
	s64 space;

	template<u64 N> smval* operator[](char const (&key)[N]){ str8 guy; guy.str = (u8*)key; guy.count = N-1; return strmap_at(this, guy); }
	smval* operator[](str8 key){ return strmap_at(this, key); }
};

// indicates what type an smval is
enum{
	smval_null,         // nothing, likely an error
	smval_string,       // holds a str8
	smval_entity,       // holds an Entity*
	smval_array,        // holds an array of smvals
	smval_map,          // holds a strmap
	smval_signed_int,   // holds a signed int
	smval_unsigned_int, // holds a unsigned int
	smval_float,        // holds a float
};

// represents a value in a strmap
struct smval{
	Type type;
	union{
		str8 str;
		Entity* entity;
		smval* arr;
		strmap map;
		s64 signed_int;
		u64 unsigned_int;
		f64 floating;
	};
};

// returns the index of a key
// returns -1 if it doesn't exist
u64 strmap_find(strmap* map, str8 name){DPZoneScoped;
    u64 hashed = str8_hash64(name);
	s64 index = -1;
	s64 middle = 0;
	if(map->count){
		s64 left = 0;
		s64 right = map->count-1;
		while(left <= right){
			middle = left+(right-left)/2;
			if(map->keys[middle].key == hashed){
				index = middle; break;
			}
			if(map->keys[middle].key < hashed){
				left = middle+1;
				middle = left+(right-left)/2;
			}else{
				right = middle-1;
			}
		}
	}
	return index;
}

void strmap_grow_if_needed(strmap* map){DPZoneScoped;
	if(map->count == map->space){
		map->space += 16;
		map->values = (smval*)memrealloc(map->values, sizeof(smval)*map->space);
		map->keys = (smkey*)memrealloc(map->keys, sizeof(smkey)*map->space);
	}
}

void strmap_insert(strmap* map, s64 idx, str8 key, u64 hashed, smval val){DPZoneScoped;
	strmap_grow_if_needed(map);
	if(idx != map->count){
		MoveMemory(map->keys+idx+1, map->keys+idx, (map->count-idx)*sizeof(smkey));
		MoveMemory(map->values+idx+1, map->values+idx, (map->count-idx)*sizeof(smval));
	}
	(map->keys+idx)->key_string = key;
	(map->keys+idx)->key = hashed;
	*(map->values+idx) = val;
	map->count++;
}

// adds a COPY of the given smval to the strmap
u64 strmap_add(strmap* map, str8 key, smval val){DPZoneScoped;
	u64 hashed = str8_hash64(key);
	s64 index = -1;
	s64 middle = 0;
	if(map->count){
		s64 left = 0;
		s64 right = map->count-1;
		while(left <= right){
			middle = left+(right-left)/2;
			if(map->keys[middle].key == hashed){
				index = middle; break;
			}
			if(map->keys[middle].key < hashed){
				left = middle+1;
				middle = left+(right-left)/2;
			}else{
				right = middle-1;
			}
		}
	}

	// if the index was found and this isn't a collision, then we can just return the index
	// however, if the second check fails, then this is a collision and we insert it as a neighbor
	if(index != -1 && str8_equal_lazy(key, map->keys[index].key_string)){
		return index;
	}else{
		strmap_insert(map, middle, key, hashed, val);
		return middle;
	}
}

// get entity at key
smval* strmap_at(strmap* map, str8 key){DPZoneScoped;
	u64 idx = strmap_find(map, key);
	smkey* keys = map->keys;
	smval* values = map->values;
	if(idx != -1){
		if(map->count == 1) return &map->values[idx];
		else if( // check if any neighbors match
			 idx && idx < map->count - 1 && keys[idx-1].key != keys[idx].key && keys[idx].key != keys[idx+1].key ||
			!idx && map->count > 1 && keys[1].key != keys[0].key ||
			 idx == map->count - 1 && keys[idx-1].key != keys[idx].key 
		){ return &values[idx]; }
		else{
			Log("strmap", "a collision has happened in a strmap, but this isn't tested yet");
			TestMe;
			//a collision happened so we must find the right neighbor
			u32 index = idx;
			//iterate backwards as far as we can
			while(index!=0 && map->keys[index-1].key == map->keys[index].key){
				index--;
				if(str8_equal_lazy(map->keys[index].key_string, key)){
					return &map->values[index];
				}
			}
			index = idx;
			//iterate forwards as far as we can
			while(index!=map->count-1 && map->keys[index+1].key == map->keys[index].key){
				index++;
				if(str8_equal_lazy(map->keys[index].key_string, key)){
					return &map->values[index];
				}
			}
			return 0;
		}
	}
	return 0;
}

// get smval at index
smval* strmap_at_idx(strmap* map, s64 idx){
	Assert(idx < map->count);
	return &map->values[idx];
}

//TODO(sushi) we could just store this in the actual strmap struct, but that may be tedious
struct smpair{
	smkey* key;
	smval* val;
};

// returns the key and value at the given index
smpair strmap_pair_at_idx(strmap* map, s64 idx){
	Assert(idx < map->count);
	return {&map->keys[idx], &map->values[idx]};
}

enum{
	need_hunger,
	need_sleep,
	need_bladder,
	need_mood,
	need_food,
	need_COUNT,
};

struct Needs{
	union{
		f32 arr[5];
		struct{
			f32 hunger;
			f32 sleep;
			f32 bladder;
			f32 mood;
			f32 food;
		};
	};
};

// an event caused by an Agent that causes some change in the world
struct Action{
	str8 name;
	u64 time;
	Needs costs;
	strmap reqs; // a list of requirements in the form of predicates needed to perform this action
};

Action idle;

// an advertisement of a collection of actions that an agent may take
struct Advert{
	str8 name;
	Action* actions;
};

Needs advert_costs(Advert* advert){
	Needs out = {0};
	forI(arrlen(advert->actions)){
		Action* action = advert->actions + i;
		forX(j, need_COUNT){
			out.arr[j] += action->costs.arr[j];
		}
	}
	return out;
}

enum{
	entity_null, // indicates that an entity has not been parsed or that an error has occured
	entity_entity,
	entity_object,
	entity_agent,
};

// anything that exists, whether it be physical, abstract, tangible, intangible... so on.
// all things have a name and a set of predicates describing qualities about that thing
struct Entity{
	Type type;
	str8 raw;  // the data that defined this entity
	str8 name; 
	str8 desc;
	strmap predicates;
};

// something tangible that may be acted upon. has a position in reality, an age, mass, and a 
// list of adverts animate objects may take against it
struct Object{
	Entity entity;
	u32 age;
	vec2 pos;
	f32 mass;
	Advert* adverts; // stb array
};

struct ActionQueue{
	Action** actions;
	u64* times;
};

// an animate object that can make decisions to take actions on other objects based on 
// a list of needs and is able to store memories about entities.
struct Agent{
	Object object;
	ActionQueue action_queue;
	Needs needs;
	Entity* memories; // TODO(sushi)
};

// walks up from a given entity to find if it is a subclass of the other entity.
b32 is_subclass_of(Entity* entity, Entity* class_entity){
	smval* val = strmap_at(&entity->predicates, STR8("subclass_of"));
	if(!val) return 0;
	switch(val->type){
		case smval_entity:{
			if(val->entity == class_entity) return 1;
			else return is_subclass_of(val->entity, class_entity);
		}break;
		case smval_array:{
			forI(arrlen(val->arr)){
				Entity* e = val->arr[i].entity;
				if(is_subclass_of(e, class_entity)) return 1;
			}
		}break;
	}
	return 0;
}

struct{
	Arena* entities;
	Arena* objects;
	Arena* agents;
	// Arena* adverts; // normally allocated, for now
	strmap concepts;
}storage;

struct{
	struct{
		Action idle;
	}actions;
	struct{
		Entity* entity;
		Entity* object;
		Entity* agent;

	}concepts;
}singletons;

// returns -1 if no more room
Entity* storage_add_entity() {
	if(storage.entities->used + sizeof(Entity) > storage.entities->size){
		LogE("agent_storage", "The max amount of entities have been allocated. Increase size of storage.entities.");
		return 0;
	}
	Entity* out = (Entity*)storage.entities->cursor;
	storage.entities->used += sizeof(Entity);
	storage.entities->cursor += sizeof(Entity);
	return out;
}

Object* storage_add_object() {
	if(storage.objects->used + sizeof(Object) > storage.objects->size){
		LogE("agent_storage", "The max amount of objects have been allocated. Increase size of storage.objects.");
		return 0;
	}
	Object* out = (Object*)storage.objects->cursor;
	storage.objects->used += sizeof(Object);
	storage.objects->cursor += sizeof(Object);
	return out;
}

Agent* storage_add_agent() {
	if(storage.agents->used + sizeof(Agent) > storage.agents->size){
		LogE("agent_storage", "The max amount of agents have been allocated. Increase size of storage.agents.");
		return 0;
	}
	Agent* out = (Agent*)storage.agents->cursor;
	storage.agents->used += sizeof(Agent);
	storage.agents->cursor += sizeof(Agent);
	return out;
}

// Advert* storage_add_advert() {
// 	if(storage.entities->used + sizeof(Entity) > storage.entities->size){
// 		LogE("agent_storage", "The max amount of entities have been allocated. Increase size of storage.entities.");
// 		return -1;
// 	}
// 	Entity* out = (Entity*)storage.entities->cursor;
// 	storage.entities->used += sizeof(Entity);
// 	storage.entities->cursor += sizeof(Entity);
// 	return out;
// }




//TODO(sushi) printing stuff nicely
// struct{
// 	u32 indent_level;	
// }sts = {0};
// #define do_indent()\
// forI(sts.indent_level){\
// 	str8_builder_append(builder, "\t");\
// }

// void strmap_to_str(strmap* map, str8b* builder);

// // void entity_to_str(Entity* e, str8b builder, b32 show_predicates = 0){
// // 	str8_builder_append(builder, "Entity[", e->name);
// // 	if(show_predicates){
// // 		str8_builder_append(builder, ";\n");
// // 		sts.indent_level++;
// // 		do_indent();
// // 		str8_builder_append(builder, "desc: \"", e->desc, "\";\n");
// // 		strmap_to_str(&e->predicates, builder);
// // 		sts.indent_level--;
// // 		str8_builder_append(builder, "desc: \"", e->desc, "\";\n");

// // 	}else{
// // 		str8_builder_append(builder, "];\n");
// // 		do_indent();
// // 	}
// // }

// void element(smval val, str8b* builder){
// 	switch(val.type){
// 		case smval_entity:{
// 			Entity* e = val.entity;
// 			str8_builder_append(builder, "Entity[", e->name, "];\n");
// 			do_indent();
// 		}break;
// 		case smval_float:{
// 			str8_builder_append(builder, val.floating, ";\n");
// 			do_indent();
// 		}break;
// 		case smval_signed_int:{
// 			str8_builder_append(builder, val.signed_int, ";\n");
// 			do_indent();
// 		}break;
// 		case smval_unsigned_int:{
// 			str8_builder_append(builder, val.unsigned_int, ";\n");
// 			do_indent();
// 		}break;
// 		case smval_string:{
// 			str8_builder_append(builder, "\"", val.str, "\";\n");
// 			do_indent();
// 		}break;
// 		case smval_array:{
// 			forI(arrlen(val.arr)){
// 				element(val.arr[i], builder);
// 			}
// 		}break;
// 		case smval_map:{
// 			str8_builder_append(builder, ": {\n");
// 			sts.indent_level++;
// 			strmap_to_str(&val.map, builder);
// 			sts.indent_level--;
// 			do_indent();
// 		}break;
// 	}	
// }

// // pretty prints a strmap to a str8_builder
// // expects an allocated str8_builder
// void strmap_to_str(strmap* map, str8b* builder){
// 	forI(map->count){
// 		smkey key = map->keys[i];
// 		smval val = map->values[i];
// 		str8_builder_append(builder, key.key_string, ": ");
// 		element(val, builder);
// 	}
// }