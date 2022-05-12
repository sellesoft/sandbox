ma_engine sound_engine;

void sound_init(){
   if(ma_engine_init(0, &sound_engine) != MA_SUCCESS){
       LogE("sound", "Sound engine failed to initialize");
   }
}

void sound_update(){
    
}