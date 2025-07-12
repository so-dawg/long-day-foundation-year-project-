#include "audio.h"
#include <stdio.h>

void init_audio(AudioSystem* audio) {
    ma_result result;
    
    result = ma_engine_init(NULL, &audio->engine);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize audio engine\n");
        return;
    }
}

void init_sfx(Audiosfx *audio,const char* filename) {
    if (!audio) return;
    
    // Initialize audio engine
    ma_result result = ma_engine_init(NULL, &audio->engine);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize audio engine\n");
        return;
    }
    
    // Pre-load coin sound effect
    result = ma_sound_init_from_file(&audio->engine, filename, 0, NULL, NULL, &audio->sfx);
    if (result != MA_SUCCESS) {
        printf("Failed to load coin sound\n");
    }
}

void play_bgm(AudioSystem* audio, const char* filepath) {
    ma_result result;
    
    // Load and start BGM (loops automatically)
    result = ma_sound_init_from_file(
        &audio->engine,
        filepath,
        0,
        NULL,  
        NULL,  
        &audio->bgm
    );
    
    if (result != MA_SUCCESS) {
        printf("Failed to load BGM: %s\n", filepath);
        return;
    }
    
    ma_sound_start(&audio->bgm);
}

void play_sfx(Audiosfx *audio) {
    if (!audio) return;
    
    // Restart the sound if it's already playing
    ma_sound_seek_to_pcm_frame(&audio->sfx, 0);
    ma_sound_start(&audio->sfx);
}

void stop_bgm(AudioSystem* audio) {
    ma_sound_stop(&audio->bgm);
    ma_sound_uninit(&audio->bgm);
}

void shutdown_audio(AudioSystem* audio) {
    ma_engine_uninit(&audio->engine);
}

void shutdown_sfx(Audiosfx *audio) {
    if (audio) {
        ma_sound_uninit(&audio->sfx);
        ma_engine_uninit(&audio->engine);
    }
}