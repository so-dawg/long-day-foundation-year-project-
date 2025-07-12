#pragma once
#include "miniaudio.h"

typedef struct {
    ma_engine engine;
    ma_sound bgm;
} AudioSystem;
typedef struct {
    ma_engine engine;
    ma_sound sfx;
} Audiosfx;

void init_audio(AudioSystem* audio);
void init_sfx(Audiosfx* audio,const char* filename);
void play_bgm(AudioSystem* audio, const char* filepath);
void play_sfx(Audiosfx* audio);
void stop_bgm(AudioSystem* audio);
void shutdown_audio(AudioSystem* audio);
void shutdown_sfx(Audiosfx* audio);