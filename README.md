# long-day-foundation-year-project-
This is my foundation year final project. This is Candy Crush Saga like game match of 3 tile in gameplay to score.

![background](https://github.com/user-attachments/assets/a322728f-f2af-482b-b5a8-e4538bd7238d)



Library graphic used: https://github.com/erkkah/tigr

library audio used: https://github.com/mackron/miniaudio


*to run a game:

    #Window:
    
    gcc main.c tigr.c audio.c miniaudio.c -o game.exe -I. -lgdi32 -lwinmm -lm 
    
    and after .\game.exe
  
    #Linux:
    
    gcc main.c tigr.c audio.c miniaudio.c -o main -lGLU -lGL -lX11 -lm && ./main

Thank you so much!

@so_dawg
