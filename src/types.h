#include "kigu/common.h"
#include "kigu/unicode.h"
#include "core/memory.h"
#include "math/vector.h"

struct Entity;
struct Predicate{
	Entity* subject;
	str8 predicate;
	Entity* object;
};

// represents keys in strmap
struct smkey{
	str8 key_string;
	u64 key;
};

// indicates what type an smval is
enum{
	smval_string, // an smval that holds a str8
	smval_entity, // an smval that holds an Entity*
	smval_array,  // an smval that holds an array of smvals
};

// represents a value in a strmap
struct smval{
	Type type;
	union{
		str8 str;
		Entity* entity;
		smval* arr;
	};
};

// maps a string to a value
// the value may be a string, and Entity*, or an array of smvals
// this is used for the concept map and the predicate maps in Entities.
struct strmap{
	smkey* keys;
	smval* values;
	s64 count;
	s64 space;
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
	CopyMemory(&(map->keys+idx)->key_string, &key, sizeof(str8));
	CopyMemory(&(map->keys+idx)->key, &hashed, sizeof(u64));
	CopyMemory(map->values+idx, &val, sizeof(smval));
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

// an event caused by an Agent that causes some change in the world
struct Action{
	u64  time;
	f32* costs; 
	Predicate* reqs; // a list of requirements in the form of predicates needed to perform this action
};

// an advertisement of a collection of actions that an agent may take
struct Advert{
	str8 name;
	Action* actions;
};

// anything that exists, whether it be physical, abstract, tangible, intangible... so on.
// all things have a name and a set of predicates describing qualities about that thing
struct Entity{
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
	Advert* adverts;
};

// an animate object that can make decisions to take actions on other objects based on 
// a list of needs and is able to store memories about entities.
struct Agent{
	Object object;
	Action* action_queue;
	f32* needs;
	Entity* memories; // TODO(sushi)
};

struct cmkey{
	str8 name;
	u64 key;
};

// a sorted map for concepts; unique entities representing classes of entities
// this handles collisions by storing the original key string
struct cmap{
	cmkey* keys;
	Entity* values;
	s64 count;
	s64 space;
};

struct{
	Arena* entities;
	Arena* objects;
	Arena* agents;
	strmap concepts;
}storage;
