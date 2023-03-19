//// kigu includes ////
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"

//// deshi includes ////
#define DESHI_DISABLE_IMGUI
#include "core/commands.h"
#include "core/console.h"
#include "core/graphing.h"
#include "core/file.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/assets.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/ui2.h"
#include "core/window.h"
#include "core/file.h"
#include "math/math.h"

#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>



enum NodeType : u32 {
	NodeType_Program,
	NodeType_Structure,
	NodeType_Function,
	NodeType_Variable,
	NodeType_Scope,
	NodeType_Statement,
	NodeType_Expression,
};

// node struct used in amu
struct amuNode{
	Type  type;
	Flags flags;
	
	amuNode* next = 0;
	amuNode* prev = 0;
	amuNode* parent = 0;
	amuNode* first_child = 0;
	amuNode* last_child = 0;
	u32     child_count = 0;
	
	mutex lock;

	str8 debug;
};

struct{
	HANDLE amu_process;
	array<amuNode> nodes;
}globals;

HANDLE get_amu_process(){
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if(Process32First(snapshot, &entry) == TRUE){
		while(Process32Next(snapshot, &entry) == TRUE){
			if(stricmp(entry.szExeFile, "amu.exe") == 0){
				return OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
			}
		}
	}
	return 0;
}

#include "graphviz/gvc.h"

Agraph_t* gvgraph = 0;
GVC_t* gvc = 0;
u32 colidx = 1;
u32 groupid = 0;

void make_ast_graph(amuNode* node, Agnode_t* parent, amuNode* align_to){
    static u32 i = 0;
	i++;
	u32 colsave = colidx;
	
	Agnode_t* me = agnode(gvgraph, to_string(i).str, 1);

	//DEBUG
	//if(node->type == NodeType_Expression)agset(me, "label", dataTypeStrs[ExpressionFromNode(node)->datatype]);
	//else agset(me, "label", node->comment.str);
	str8 out = str8_copy(node->debug);
	agset(me, "label", (char*)out.str);
	agset(me, "color", to_string(colsave).str);
	free(out.str);
	
	
	for_node(node->first_child){
		if (node->first_child->next) {
			align_to = node;
			make_ast_graph(it, me, node);
		}
		else {
			make_ast_graph(node->first_child, me, align_to);
		}
		colidx = (colidx + 1) % 11 + 1;
	}
	agset(me, "group", to_string(align_to).str);
	
	Agedge_t* edge = agedge(gvgraph, parent, me, "", 1);
	agset(edge, "color", to_string(colsave).str);
}

void generate_ast_graph_svg(const char* filename, amuNode* start){
    gvc = gvContext();
	gvgraph = agopen("ast tree", {1,0,0,1}, 0);
	agattr(gvgraph, AGNODE,"fontcolor",   "white");
	agattr(gvgraph, AGNODE,"color",       "1");
	agattr(gvgraph, AGNODE,"shape",       "box");
	agattr(gvgraph, AGNODE,"margins",     "0.08");
	agattr(gvgraph, AGNODE,"group",       "0");
	agattr(gvgraph, AGNODE,"width",       "0");
	agattr(gvgraph, AGNODE,"height",      "0");
	agattr(gvgraph, AGNODE,"colorscheme", "rdylbu11");
	agattr(gvgraph, AGEDGE,"color",       "white");
	agattr(gvgraph, AGEDGE,"colorscheme", "rdylbu11");
	agattr(gvgraph, AGEDGE,"style",       "");
	agattr(gvgraph, AGEDGE,"arrowhead",   "none");
	agattr(gvgraph, AGEDGE,"penwidth",    "0.5");
	agattr(gvgraph, AGEDGE,"constraint",  "true");
	agattr(gvgraph, AGRAPH,"bgcolor",     "grey12");
	agattr(gvgraph, AGRAPH,"concentrate", "true");
	agattr(gvgraph, AGRAPH,"splines",     "true");

    Agnode_t* prog = agnode(gvgraph, "program", 1);
	
	for_node(start->first_child){
		make_ast_graph(it, prog, it);
	}
	
	gvLayout(gvc, gvgraph, "dot");
	gvRenderFilename(gvc, gvgraph, "svg", filename);

}

void extract(amuNode* node){
	if(node->next){
		globals.nodes.add(amuNode{0});
		amuNode* n = globals.nodes.last;
		ReadProcessMemory(globals.amu_process, node->next, n, sizeof(amuNode), 0);
		if(n->debug.str){
			void* loc = n->debug.str;
			n->debug.str = (u8*)memalloc(n->debug.count);
			ReadProcessMemory(globals.amu_process, loc, n->debug.str, n->debug.count, 0);
		}
		node->next = n;
		n->prev = node;
		extract(node->next);
	}
	if(node->first_child){
		globals.nodes.add(amuNode{0});
		amuNode* n = globals.nodes.last;
		ReadProcessMemory(globals.amu_process, node->first_child, n, sizeof(amuNode), 0);
		if(n->debug.str){
			void* loc = n->debug.str;
			n->debug.str = (u8*)memalloc(n->debug.count);
			ReadProcessMemory(globals.amu_process, loc, n->debug.str, n->debug.count, 0);
		}
		node->first_child = n;
		n->parent = node;
		extract(node->first_child);
	}
}

int main(){
	//init deshi
	Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(1));
	platform_init();
	logger_init();

	globals.amu_process = get_amu_process();
	if(!globals.amu_process){
		LogE("", "unable to get amu process");
		return 0;
	}
	Log("", "Enter memory address of node:\n");
	u64 addr = 0;
	scanf("%llx", &addr);

	globals.nodes.add(amuNode{0});
	amuNode* n = globals.nodes.last;
	u64 success = 0;
	ReadProcessMemory(globals.amu_process, (void*)addr, n, sizeof(amuNode), &success);
	extract(n);

	generate_ast_graph_svg("out.svg", n);

	//cleanup deshi
	logger_cleanup();
	memory_cleanup();
	return 0;
}