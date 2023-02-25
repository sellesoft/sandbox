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
	Predicate* predicates;
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
	cmap concepts;
}storage;

// returns the index of a concept
// returns -1 if it doesn't exist
u64 concept_map_find(str8 name){DPZoneScoped;
    u64 hashed = str8_hash64(name);
	s64 index = -1;
	s64 middle = 0;
	if(storage.concepts.count){
		s64 left = 0;
		s64 right = storage.concepts.count-1;
		while(left <= right){
			middle = left+(right-left)/2;
			if(storage.concepts.keys[middle].key == hashed){
				index = middle; break;
			}
			if(storage.concepts.keys[middle].key < hashed){
				left = middle+1;
				middle = left+(right-left)/2;
			}else{
				right = middle-1;
			}
		}
	}
	return index;
}

void __concept_map_grow_if_needed(){DPZoneScoped;
	if(storage.concepts.count == storage.concepts.space){
		storage.concepts.space += 16;
		storage.concepts.values = (Entity*)memrealloc(storage.concepts.values, sizeof(Entity)*storage.concepts.space);
		storage.concepts.keys = (cmkey*)memrealloc(storage.concepts.keys, sizeof(cmkey)*storage.concepts.space);
	}
}

void __concept_map_insert(s64 idx, str8 key, u64 hashed, Entity val){DPZoneScoped;
	__concept_map_grow_if_needed();
	if(idx != storage.concepts.count){
		MoveMemory(storage.concepts.keys+idx+1, storage.concepts.keys+idx, (storage.concepts.count-idx)*sizeof(cmkey));
		MoveMemory(storage.concepts.values+idx+1, storage.concepts.values+idx, (storage.concepts.count-idx)*sizeof(Entity));
	}
	CopyMemory(&(storage.concepts.keys+idx)->name, &key, sizeof(str8));
	CopyMemory(&(storage.concepts.keys+idx)->key, &hashed, sizeof(u64));
	CopyMemory(storage.concepts.values+idx, &val, sizeof(Entity));
	storage.concepts.count++;
}

// adds a !copy! of the entity to the concept map
u64 concept_map_add(str8 key, Entity val){DPZoneScoped;
	u64 hashed = str8_hash64(key);
	s64 index = -1;
	s64 middle = 0;
	if(storage.concepts.count){
		s64 left = 0;
		s64 right = storage.concepts.count-1;
		while(left <= right){
			middle = left+(right-left)/2;
			if(storage.concepts.keys[middle].key == hashed){
				index = middle; break;
			}
			if(storage.concepts.keys[middle].key < hashed){
				left = middle+1;
				middle = left+(right-left)/2;
			}else{
				right = middle-1;
			}
		}
	}

	// if the index was found and this isn't a collision, then we can just return the index
	// however, if the second check fails, then this is a collision and we insert it as a neighbor
	if(index != -1 && str8_equal_lazy(key, storage.concepts.keys[index].name)){
		return index;
	}else{
		__concept_map_insert(middle, key, hashed, val);
		return middle;
	}
}

// get entity at key
Entity* concept_map_at(str8 key){DPZoneScoped;
	u64 idx = concept_map_find(key);
	cmkey* keys = storage.concepts.keys;
	Entity* values = storage.concepts.values;
	if(idx != -1){
		if(storage.concepts.count == 1) return &storage.concepts.values[idx];
		else if( // check if any neighbors match
			 idx && idx < storage.concepts.count - 1 && keys[idx-1].key != keys[idx].key && keys[idx].key != keys[idx+1].key ||
			!idx && storage.concepts.count > 1 && keys[1].key != keys[0].key ||
			 idx == storage.concepts.count - 1 && keys[idx-1].key != keys[idx].key 
		){ return &values[idx]; }
		else{
			Log("concept map", "a collision has happened in the concept map, but this isn't tested yet");
			TestMe;
			//a collision happened so we must find the right neighbor
			u32 index = idx;
			//iterate backwards as far as we can
			while(index!=0 && storage.concepts.keys[index-1].key == storage.concepts.keys[index].key){
				index--;
				if(str8_equal_lazy(storage.concepts.keys[index].name, key)){
					return &storage.concepts.values[index];
				}
			}
			index = idx;
			//iterate forwards as far as we can
			while(index!=storage.concepts.count-1 && storage.concepts.keys[index+1].key == storage.concepts.keys[index].key){
				index++;
				if(str8_equal_lazy(storage.concepts.keys[index].name, key)){
					return &storage.concepts.values[index];
				}
			}
			return 0;
		}
	}
	return 0;
}

// get entity at index
Entity* concept_map_at_idx(s64 idx){
	Assert(idx < storage.concepts.count);
	return &storage.concepts.values[idx];
}