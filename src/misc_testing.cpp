#include "kigu/common.h"
#include "core/render.h"
#include "core/window.h"
#include "kigu/array.h"
#include "math/edge.h"

u64 rng(){
	persist u64 rngseed = 0xa0a0a0a0a0;
	static u64 x = rngseed;
	x = 
	((0x00000000FF & x) % 50  + 
	 (0x000000FF00 & x) % 100 +
	 (0x0000FF0000 & x) % 150 +
	 (0x00FF000000 & x) % 200 +
	 (0xFF00000000 & x) % 250);
	return x;
}

struct RandDrawObj {
	vec2 pos;
	vec2 target;
	array<pair<vec2, color>> hist;
};

void random_draw(u32 count) {
	persist array<RandDrawObj> rdobjs;
	persist b32 init = 0;
	persist Stopwatch walk = start_stopwatch();
	if (!init) {
		init = 1;
		forI(count) {
			rdobjs.add(RandDrawObj{
						   vec2(rand() % (u32)DeshWindow->dimensions.x, rand() % (u32)DeshWindow->dimensions.y),
						   vec2(rand() % (u32)DeshWindow->dimensions.x, rand() % (u32)DeshWindow->dimensions.y) });
			rdobjs.last->hist.add({ rdobjs.last->pos, randcolor });
		}
	}
	else {
		render_start_cmd2(4, 0, vec2::ZERO, DeshWindow->dimensions);
		
		for (RandDrawObj& rdo : rdobjs) {
			vec2& pos = rdo.pos;
			vec2& target = rdo.target;
			array<pair<vec2, color>>& hist = rdo.hist;
			
			
			//pos += sinf(rand() % 100) * randvec2(10);
			
			vec2 tdis = (vec2(DeshWindow->dimensions) / 2 - pos).normalized() * 2;
			//pos += tdis * 2;
			pos += vec2(tdis.y, -sin(tdis.x)) * 0.5;
			
			
			//pos = Nudge(pos, target, randvec2(10) / 10);
			
			//pos = Clamp(pos, vec2::ZERO, DeshWindow->dimensions);
			
			
			
			if (pos == target) {
				target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
				hist.clear();
			}
			else if (hist.count) {
				color nu = hist.last->second;
				//nu.r = (nu.r + rand() % 10) % 255;
				nu.g = Nudge((u32)nu.g, (u32)((u32)target.x % (u32)255), (u32)1);
				nu.b = 144;
				hist.add({ pos, nu });
			}
			
			if (hist.count) {
				for (int i = 0; i < hist.count - 1; i++) {
					render_line_thick2(hist[i].first, hist[i + 1].first, 2, hist[i].second);
				}
			}
		}
		if (peek_stopwatch(walk) > 5000) { 
			reset_stopwatch(&walk); 
			f32 i = 0;
			for (RandDrawObj& rdo : rdobjs) {
				vec2& pos = rdo.pos;
				vec2& target = rdo.target;
				array<pair<vec2, color>>& hist = rdo.hist;
				
				hist.clear();
				hist.add({ vec2(rand() % (u32)DeshWindow->dimensions.x, rand() & (u32)DeshWindow->dimensions.y), randcolor });
				
			}
			
		}
	}
}



//line that interacts with itself
#include <ctime>
void random_walk_avoid() {
	persist b32 rwainit = 0;
	persist RandDrawObj rwa;
	persist Stopwatch rwawalk = start_stopwatch();
	persist Stopwatch histreset = start_stopwatch();
	
	if (!rwainit) {
		srand(time(0));
		rwainit = 1;
		rwa.pos = vec2(rand() % (u32)DeshWindow->dimensions.x, rand() % (u32)DeshWindow->dimensions.y);
		rwa.target = vec2(rand() % (u32)DeshWindow->dimensions.x, rand() % (u32)DeshWindow->dimensions.y);
		rwa.hist.add({ rwa.pos, randcolor });
	}
	else {
		render_start_cmd2(4, 0, vec2::ZERO, DeshWindow->dimensions);
		
		vec2& pos = rwa.pos;
		vec2& target = rwa.target;
		array<pair<vec2, color>>& hist = rwa.hist;
		
		vec2 tdis = (target - pos).normalized();
		vec2 step = tdis + vec2(-tdis.y, tdis.x);
		
		step *= 4;
		
		for (int i = 0; i < (s32)hist.count - 200; i++) {
			vec2 histpos = hist[i].first;
			if ((histpos - pos).mag() < 100 && step.dot(histpos - pos) < 0) {
				vec2 left = hist[Max(i - 1, 0)].first;
				vec2 right = hist[i + 1].first;
				Edge edge(left, right);
				f32 m = 1;
				while (edge.edge_intersect(Edge(pos, pos + step)) || step.dot(histpos - (pos + step)) < -0.1){
					step = Math::vec2RotateByAngle(m, step);
					m += 1;
					if (m > 270) {
						break;
					}
				}
			}
		}
		
		pos += step;
		
		//pos = Clamp(pos, vec2::ZERO, DeshWindow->dimensions);
		
		if (pos == target) {
			target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
			hist.clear();
		}
		else {
			color nu = hist.last->second;
			//nu.r = (nu.r + rand() % 10) % 255;
			nu.g = Nudge((u32)nu.g, (u32)((u32)target.x % (u32)255), (u32)1);
			nu.b = 144;
			hist.add({ pos, nu });
		}
		
		if (peek_stopwatch(rwawalk) > 100) {
			reset_stopwatch(&rwawalk);
			target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
			
		}
		
		if (peek_stopwatch(histreset) > 5000) {
			reset_stopwatch(&histreset);
			color last = hist.last->second;
			hist.clear();
			hist.add({ rwa.pos, last });
		}
		
		if (hist.count) {
			for (int i = 0; i < hist.count - 1; i++) {
				render_line_thick2(hist[i].first, hist[i + 1].first, 2, hist[i].second);
			}
		}
		
	}
}

void vector_field() {
	f32 precision = 100;
	static f32 zoom = 1;
	zoom += 10 / zoom * DeshInput->scrollY;
	static vec2 postrack(0, 0);
	static array<vec2> hist;
	static vec2 mpl(0,0);
	static Stopwatch vft = start_stopwatch();
	
	vec2 mp = input_mouse_position();
	
	static b32 ringed = 0;
	static u32 idx = 0;
	static b32 tracking = 1;
	if (input_lmouse_down() && mpl != input_mouse_position()) {
		postrack = input_mouse_position();
		mpl = input_mouse_position();
		ringed = 0;
		idx = 0;
		hist.clear();
	}
	
	f32 time = DeshTotalTime/1000;
	
	static vec2 target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
	
	auto step = [&](vec2 pos) {
		vec2 tc = (vec2(DeshWindow->dimensions) / 2 - pos) * zoom;
		f32 x = tc.x, y = tc.y;
		//vec2 tm = mp - pos;// - ;
		Log("", rng());
		if(x>0)
			return vec2(rng()%50-25,rng()%50-25);
		else
			return vec2(rand()%50-25,rand()%50-25);
		
	};
	
	if (peek_stopwatch(vft) > 1000) {
		reset_stopwatch(&vft);
		target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
	}
	
	static f32 maxmag = 0;
	
	render_start_cmd2(4, 0, vec2::ZERO, DeshWindow->dimensions);
	for (u32 i = 0; i < precision; i++) {
		for (u32 o = 0; o < precision; o++) {
			vec2 pos(i / precision * DeshWindow->dimensions.x+precision/DeshWindow->dimensions.x , o / precision * DeshWindow->dimensions.y+precision/ DeshWindow->dimensions.y);
			vec2 s = step(pos);
			vec2 pos2 = pos + s.normalized() * 10;
			
			maxmag = Max(maxmag, s.mag());
			
			//render_circle2(pos, 0, 4, 10, Color_Red);
			render_line_thick2(pos, pos2, 2, color(25,  Clamp(255 * s.mag() / maxmag, 0.f, 255.f), 125));
		}
	}
	
	if (tracking) {
		if (Math::PointInRectangle(postrack, vec2::ZERO, DeshWindow->dimensions)) {
			postrack += step(postrack);
			hist.add(postrack);
		}
		for (int i = 0; i < hist.count - 1; i++) {
			render_line_thick2(hist[i], hist[i + 1], 2, Color_Red);
		}
	}
	
	
}

void electric_field() {
	struct charge{
		vec2 r;
		f64 q;
	};
	array<charge> pcharges{
		{{ 1.,  0.}, 1.},
		{{ 1.,  1.},-1.},
		{{ 0.,  1.}, 1.},
		{{-1.,  1.},-1.},
		{{-1.,  0.}, 1.},
		{{-1., -1.},-1.},
		{{ 0., -1.}, 1.},
		{{ 1., -1.},-1.},
	};
	f32 scale = 30;
	render_start_cmd2(5, 0, vec2::ZERO, DeshWindow->dimensions);
	vec2 coff = vec2(DeshWindow->dimensions)/2;
	
	u32 res = 10;
	f64 ar = DeshWindow->dimensions.y/DeshWindow->dimensions.x;
	for(u32 i = 0; i < res; i++){
		for(u32 j = 0; j < res; j++){
			vec2 pos = vec2((i/f32(res)+1/(2*f32(res)))*DeshWindow->dimensions.x, (j/f32(res)+1/(2*f32(res)))*DeshWindow->dimensions.y);
			vec2 e;
			for(charge c : pcharges){
				e+=1000*c.q/((pos-c.r).magSq())*(pos-c.r).normalized();
			}
			render_line2(pos, pos+e, Color_White);
		}
	}
	
	render_start_cmd2(0, 0, vec2::ZERO, DeshWindow->dimensions);
	for(charge c : pcharges){
		vec2 pos = c.r * scale + coff;
		color col = color(155, 155 * Remap(c.q, 0., 1., -1., 1.), 0); 
		render_circle_filled2(pos, 0, 4, 30, col);
	}
}

void draw_pixels(){DPZoneScoped;
	render_start_cmd2(0, 0, vec2::ZERO, DeshWindow->dimensions);
	u32 size = 4;
	static u64 frames = 0;
	static s64 chance = 1;
	forX(i, 200){
		forX(j, 200){ //y,x
			
			
			//flashing growing thing
			f64 lhs = 0;
			f64 rhs = sin(DeshTotalTime/100000*i*(j)-2*(i-(j+DeshTotalTime/100000))+1);
			
			
			if(lhs < rhs){
				render_quad_filled2(vec2(i*size,j*size), vec2::ONE*size, color(100,75,14));
			}
		}
	}
}

void sim_pixels(){DPZoneScoped;
	render_start_cmd2(0, 0, vec2::ZERO, DeshWindow->dimensions);
	u32 size = 4;
	static u64 frames = 0;
	static s64 chance = 1;
	static b32 table[100][100] = {0};
	
	
}



void repulsion(){DPZoneScoped;
	render_start_cmd2(0,0,vec2::ZERO, DeshWindow->dimensions);
	const u32 nparticles = 100;
	struct particle{
		vec2 pos;
		vec2 vel;
		vec2 acc;
	};
	static particle particles[nparticles];
	static b32 init = 0;
	if(!init){
		forI(nparticles){
			particles[i].pos = vec2(sin(rng()), cos(rng()));
			particles[i].vel = vec2::ZERO;
		}
		init = true;
	}
	//calc forces
	
	for(u32 i = 0; i < nparticles; i++){
		particle* a = &particles[i];
		for(u32 j = 0; j < nparticles; j++){
			if(j==i) continue;
			particle* b = &particles[j];
			a->acc = 0.001/(a->pos - b->pos).magSq() * (a->pos - b->pos).normalized(); 
			//a->acc = 0.005*vec2(a->pos.y, a->pos.x);
			//a->acc = a->acc.compOn(vec2(a->pos.y, -a->pos.x));
			//b->acc = 0.001 / ((b->pos - a->pos).magSq()) * (b->pos - a->pos).normalized(); 
			//b->acc = 0.005*vec2(b->pos.y, b->pos.x);
			//b->acc = b->acc.compOn(vec2(b->pos.y, -b->pos.x));
		}
	}
	
	forI(nparticles){
		particle* p0 = &particles[i];
		f64 steps = 1000;
		p0->vel += p0->acc * DeshTime->deltaTime;
		forI(steps){
			p0->vel = p0->vel.compOn(vec2(p0->pos.y, -p0->pos.x)).normalized() * p0->vel.mag();
			p0->pos += p0->vel * DeshTime->deltaTime / steps;
		}
		//p0->pos.normalize();
		p0->acc = vec2::ZERO;
		p0->vel *= 0.1;
		render_circle_filled2(p0->pos * 100 + vec2(DeshWindow->dimensions) / 2, 0, 5, 20, color(50, 80, f32(i) / nparticles * 170));
	}
	
	if(key_pressed(Key_SPACE)){
		init = false;
	}
}

void spring(){
	using namespace UI;
	render_start_cmd2(0,0,vec2::ZERO, DeshWindow->dimensions);
	struct particle{
		vec2 pos;
		vec2 vel;
		vec2 acc;
		f64 mass;
	};
	
	static f32 k = 0.2; //spring constant
	static f32 g = 0.5;
	persist f32 B=1;
	persist f32 w=1;
	f32 h = 0.1;
	persist particle p0{{0,0},{0,0},{0,0},1};
	persist particle p1{{0,0},{0,0},{0,0},1};
	persist particle p2{{0,0},{0,0},{0,0},1};
	persist particle p3{{0,0},{0,0},{0,0},1};
	persist particle p4{{0,0},{0,0},{0,0},1};
	if(isnan(p0.vel.y)) p0.vel = vec2::ZERO;
	if(isnan(p0.pos.y)) p0.pos = vec2::ZERO;
	f32 denom = 1 / (1 + 2*h*B);
	f64 t = DeshTotalTime;
	f64 d = 5; //distance of second spring from origin
	f64 split = 1 - w*w;
	//if(!input_lmouse_down()){
	//p0.acc = -2*B*p0.vel-(25-w*w)*(p0.pos-vec2(0,100))-(w*w)*(p0.pos-vec2(100,0))-g*100*vec2(0,1); //damped single spring with gravity
	//p0.acc = -k*(p0.pos-vec2(0,10)); //double spring 
	//p0.acc = -2*B*p0.vel-w*w*(p0.pos-vec2(0,-100))-g*100*vec2(0,1);
	
	//spring dampening
	p0.acc = -2*B*p0.vel;
	p1.acc = -2*B*p1.vel;
	p2.acc = -2*B*p2.vel;
	p3.acc = -2*B*p3.vel;
	p4.acc = -2*B*p4.vel;
	
	//spring
	p0.acc += -w*w*(p0.pos-vec2(0,-10)) - w*w*(p0.pos - p1.pos);
	p1.acc += -w*w*(p1.pos-p0.pos-vec2(0,-10)) - w*w*(p1.pos - p2.pos);
	p2.acc += -w*w*(p2.pos-p1.pos-vec2(0,-10)) - w*w*(p2.pos - p3.pos);
	p3.acc += -w*w*(p3.pos-p2.pos-vec2(0,-10)) - w*w*(p3.pos - p4.pos);
	p4.acc += -w*w*(p4.pos-p3.pos-vec2(0,-10));
	
	//gravity
	p0.acc += -g*100*vec2(0,1);
	p1.acc += -g*100*vec2(0,1);
	p2.acc += -g*100*vec2(0,1);
	p3.acc += -g*100*vec2(0,1);
	p4.acc += -g*100*vec2(0,1);
	
	p0.vel += p0.acc * DeshTime->deltaTime / 1000;
	p1.vel += p1.acc * DeshTime->deltaTime / 1000;
	p2.vel += p2.acc * DeshTime->deltaTime / 1000;
	p3.vel += p3.acc * DeshTime->deltaTime / 1000;
	p4.vel += p4.acc * DeshTime->deltaTime / 1000;
	
	p0.pos += p0.vel * DeshTime->deltaTime / 1000;
	p1.pos += p1.vel * DeshTime->deltaTime / 1000;
	p2.pos += p2.vel * DeshTime->deltaTime / 1000;
	p3.pos += p3.vel * DeshTime->deltaTime / 1000;
	if(!input_lmouse_down()){
		p4.pos += p4.vel * DeshTime->deltaTime / 1000;
	} 
	else{
		p4.vel = vec2::ZERO;
	}
	
	//}
	
	persist array<vec2> ppos0;
	persist array<vec2> ppos1;
	persist array<vec2> ppos2;
	persist array<vec2> ppos3;
	persist array<vec2> ppos4;
	ppos0.add(p0.pos);
	ppos1.add(p1.pos);
	ppos2.add(p2.pos);
	ppos3.add(p3.pos);
	ppos4.add(p4.pos);
	if(input_lmouse_pressed()) ppos0.clear(), ppos1.clear(), ppos2.clear(), ppos3.clear(), ppos4.clear();
	
	SetNextWindowPos(vec2::ZERO);
	SetNextWindowSize(DeshWindow->dimensions);
	Begin(str8_lit("springwin"), UIWindowFlags_NoScroll | UIWindowFlags_NoInteract);{
#define Sldr(var, min, max) Slider(str8_lit(STRINGIZE(var)), &var, min, max); SameLine(); Text(str8_lit(STRINGIZE(var)));
		Sldr(B, 0, 5);
		Sldr(w, 0, 5);
		Sldr(g, 0, 10);
		Sldr(k, 0, 3);
		SetNextWindowSize(MAX_F32, MAX_F32);
		BeginChild(str8_lit("springwinchld"), vec2::ZERO, UIWindowFlags_NoInteract);{
			if(IsWinHovered() && input_lmouse_down()){
				p4.pos = ((input_mouse_position()- vec2(GetWindow()->dimensions.x/2,0))).yInvert();
				
			}
			forI(ppos0.count){
				if(i>0){
					Line(ppos0[i-1].yInvert()+vec2(GetWindow()->dimensions.x/2,0), ppos0[i].yInvert()+vec2(GetWindow()->dimensions.x/2,0),2,color(50,70,51));
					Line(ppos1[i-1].yInvert()+vec2(GetWindow()->dimensions.x/2,0), ppos1[i].yInvert()+vec2(GetWindow()->dimensions.x/2,0),2,color(50,70,102));
					Line(ppos2[i-1].yInvert()+vec2(GetWindow()->dimensions.x/2,0), ppos2[i].yInvert()+vec2(GetWindow()->dimensions.x/2,0),2,color(50,70,153));
					Line(ppos3[i-1].yInvert()+vec2(GetWindow()->dimensions.x/2,0), ppos3[i].yInvert()+vec2(GetWindow()->dimensions.x/2,0),2,color(50,70,204));
					Line(ppos4[i-1].yInvert()+vec2(GetWindow()->dimensions.x/2,0), ppos4[i].yInvert()+vec2(GetWindow()->dimensions.x/2,0),2,color(50,70,255));
				}
			}
			
			vec2 sp0 = p0.pos.yInvert() + vec2(GetWindow()->dimensions.x / 2, 0);
			vec2 sp1 = p1.pos.yInvert() + vec2(GetWindow()->dimensions.x / 2, 0);
			vec2 sp2 = p2.pos.yInvert() + vec2(GetWindow()->dimensions.x / 2, 0);
			vec2 sp3 = p3.pos.yInvert() + vec2(GetWindow()->dimensions.x / 2, 0);
			vec2 sp4 = p4.pos.yInvert() + vec2(GetWindow()->dimensions.x / 2, 0);
			
			//Line(GetWindow()->dimensions/2 + vec2(0,-100),sp, 2, Color_Cyan);
			//Line(GetWindow()->dimensions/2 + vec2(100,0),sp, 2, Color_Cyan);
			Line(vec2(GetWindow()->dimensions.x / 2, 0),sp0,2, Color_Cyan);
			Line(sp0,sp1,2,Color_Cyan);
			Line(sp1,sp2,2,Color_Cyan);
			Line(sp2,sp3,2,Color_Cyan);
			Line(sp3,sp4,2,Color_Cyan);
			
			CircleFilled(sp0, 5);
			CircleFilled(sp1, 5);
			CircleFilled(sp2, 5);
			CircleFilled(sp3, 5);
			CircleFilled(sp4, 5);
			
			
			//Log("", sp, " ", p0.pos);  
		}EndChild();
	}End();
	if(key_pressed(Key_SPACE)){
		p0.pos = vec2(100*sin(2*M_PI*0/6),100*cos(2*M_PI*0/6));
		p1.pos = vec2(100*sin(2*M_PI*1/6),100*cos(2*M_PI*1/6));
		p2.pos = vec2(100*sin(2*M_PI*2/6),100*cos(2*M_PI*2/6));
		p3.pos = vec2(100*sin(2*M_PI*3/6),100*cos(2*M_PI*3/6));
		p4.pos = vec2(100*sin(2*M_PI*4/6),100*cos(2*M_PI*4/6));
		p0.vel=vec2::ZERO;
		p1.vel=vec2::ZERO;
		p2.vel=vec2::ZERO;
		p3.vel=vec2::ZERO;
		p4.vel=vec2::ZERO;
	}
}

