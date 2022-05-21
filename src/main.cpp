#ifdef TRACY_ENABLE
#  include "Tracy.hpp"
#endif

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
#include "core/file.h"
#include "core/graphing.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/platform.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/threading.h"
#include "core/time.h"
#include "core/ui.h"
#include "core/window.h"
#include "math/math.h"

#if BUILD_INTERNAL
#include "misc_testing.cpp"
#endif

#include "pdbparse.cpp"

#include <windows.h>
#include <tlhelp32.h>

void init_deshi(){
    Stopwatch deshi_watch = start_stopwatch();
	memory_init(Gigabytes(1), Gigabytes(3));
	platform_init();
	logger_init();
	window_create(str8l("suugu"));
	console_init();
	render_init();
	Storage::Init();
	UI::Init();
	cmd_init();
	window_show(DeshWindow);
	render_use_default_camera();
	DeshThreadManager->init();
	LogS("deshi","Finished deshi initialization in ",peek_stopwatch(deshi_watch),"ms");
}

void memdbg(){
    

}

void log_error(const char* func_name, b32 crash_on_error = false){
    LPVOID msg_buffer;
	DWORD error = ::GetLastError();
	::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
					 0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)&msg_buffer, 0, 0);
	LogE("win32",func_name," failed with error ",(u32)error,": ",(const char*)msg_buffer);
	::LocalFree(msg_buffer);
	if(crash_on_error){
		Assert(!"assert before exit so we can stack trace in debug mode");
		::ExitProcess(error);
	}
}

HANDLE get_process(str8 name, b32 log_names = 0){
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(snapshot==INVALID_HANDLE_VALUE){
        log_error("CreateToolhelp32Snapshot");
        return 0;
    }

    PROCESSENTRY32W process = {0};
    process.dwSize = sizeof(process);
    while(Process32NextW(snapshot, &process)){
        str8 pname = str8_from_wchar(process.szExeFile);
		if(log_names) Log("get_process", pname);
		if(str8_equal(name, pname)){
			CloseHandle(snapshot);
			return OpenProcess(PROCESS_ALL_ACCESS, 0, process.th32ProcessID);
		}
    }
	return 0;
}

struct Process{
	str8 name;
	DWORD processID; //use with OpenProcess
	HANDLE handle = 0;
};

array<Process> get_processes(){
	array<Process> processes;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(snapshot==INVALID_HANDLE_VALUE){
        log_error("CreateToolhelp32Snapshot");
        return processes;
    }
    PROCESSENTRY32W process = {0};
    process.dwSize = sizeof(process);
	while(Process32NextW(snapshot, &process)) processes.add({str8_from_wchar(process.szExeFile), process.th32ProcessID});		
	return processes;
}


//returns number of bytes read
u64 read_memory(HANDLE phandle, u64 address, void* out, u64 size){
	u64 br;
	if(ReadProcessMemory(phandle, (LPCVOID)address, out, size, 0)) return 1;
	//log_error("ReadProcessMemory");
	return 0;
}
array<Process> processes;
b32 process_open = 0;
Process open_process;
void draw_process_menu(){
	using namespace UI;

	SetNextWindowSize(DeshWindow->dimensions);
	SetNextWindowPos(vec2::ZERO);
	Begin(STR8("processmenu"), UIWindowFlags_NoInteract);{
		if(!process_open){
			if(Button(STR8("scan for processes"))){
				processes = get_processes();
				bubble_sort(processes, [](Process p1, Process p2){
					str8 p1n = p1.name;
					str8 p2n = p2.name;
					u32 count = Min(str8_length(p1n), str8_length(p2n));
					forI(count){
						u32 c1 = str8_index(p1n,0).codepoint; 
						u32 c2 = str8_index(p2n,0).codepoint;
						if(c1 != c2){
							return c2 < c1;
						}
						str8_advance(&p1n);
						str8_advance(&p2n);
					}
					return false;
				});
			}
			Text(STR8("Choose process:"));
			SetNextWindowSize(MAX_F32, MAX_F32);
			PushVar(UIStyleVar_ScrollAmount, vec2(10,50));
			BeginChild(STR8("processlist"), vec2(MAX_F32, MAX_F32));{
				for(Process p : processes){
					SetNextItemSize(MAX_F32, 0);
					if(Button(p.name)){
						open_process = p;
						open_process.handle = OpenProcess(PROCESS_ALL_ACCESS, 0, p.processID);
						process_open = 1;
					}
				}
			}EndChild();
			PopVar();
		}
		else{
			if(Button(STR8("return"))) process_open = 0;
			SameLine(); 
			string fps = toStr(1000/DeshTime->deltaTime);
			Text({(u8*)fps.str, fps.count});
			BeginTabBar(STR8("processtabs"));{
				if(BeginTab(STR8("Memory"))){
					persist u64 start_address = 0x20004195b90;
					persist u8 address_buffer[32] = u8"0x20004195b90";
					persist f32 bytes_read = 1024;
					persist u8* memory_read = (u8*)memalloc(Kilobytes(100));
					persist f32 cell_size = 5;
					
					Slider(STR8("thesliderforthe"), &bytes_read, 64, Kilobytes(100));SameLine();Text(STR8("n bytes read"));
					Slider(STR8("theothersliderforthe"), &cell_size, 5, 100);SameLine();Text(STR8("cell size"));
					if(InputText(STR8("address_input"), address_buffer, 32, str8{}, UIInputTextFlags_EnterReturnsTrue)){
						start_address = strtoll((char*)address_buffer, 0, 16);
					}
					u64 sz = snprintf(0, 0, "%llx", start_address)+1;
					str8 str = {(u8*)malloc(sz), sz};
					sprintf((char*)str.str, "%llx", start_address);
					Text(str);
					SetNextWindowSize(MAX_F32, MAX_F32);
					BeginChild(STR8("memorydisp"), vec2::ZERO);{
						if(read_memory(open_process.handle, start_address, memory_read, bytes_read)){
							u64 cursor = GetMarginedLeft();
							u64 rows = 0;
							forI(bytes_read){
								if(cursor+cell_size>GetMarginedRight()){
									cursor = GetMarginedLeft(); rows++;
								}
								RectFilled(vec2(cursor, rows*cell_size), vec2::ONE*cell_size, color(memory_read[i],memory_read[i],memory_read[i]));
								cursor+=cell_size;
							}
						}
						else{
							Text(STR8("no bytes read"));
						}
					}EndChild();	
					
					EndTab();
				}
			}EndTabBar();
		}
	}End();
}


int main(){
	init_deshi();
	parse_pdb(STR8("../basicguy/build/debug/pogram.pdb"));
	array<Process> processes = get_processes();
    //HANDLE phandle = get_process(STR8("uiredesign.exe"), 1);
	//if(!phandle) LogE("", "unable to find process");
	bubble_sort(processes, [](Process p1, Process p2){
		str8 p1n = p1.name;
		str8 p2n = p2.name;
		u32 count = Min(str8_length(p1n), str8_length(p2n));
		forI(count){
			u32 c1 = str8_index(p1n,0).codepoint; 
			u32 c2 = str8_index(p2n,0).codepoint;
			if(c1 != c2){
				return c2 < c1;
			}
			str8_advance(&p1n);
			str8_advance(&p2n);
		}
		return false;
	});
	b32 process_open = 0;
	Process open_process;
	//start main loop
	while(platform_update()){DPZoneScoped;
		console_update();
		UI::Update();
		render_update();
		logger_update();
		memory_clear_temp();
	}
	
	//cleanup deshi
	render_cleanup();
	logger_cleanup();
	memory_cleanup();
}