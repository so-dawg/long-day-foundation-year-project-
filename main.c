#include "tigr.h"
#include "miniaudio.h"
#include "audio.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//configure
#define BOARD_SIZE 8
#define TILES_TYPE 5
#define TILE_SIZE 18
#define FONT_SIZE 20
#define MAX_SCORE_POP 32

typedef struct {
    int x;
    int y;
}Vector2;
Vector2 grid_origin;

//game state
typedef enum{
    MENU,
    PLAYING,
    GAME_OVER,
    WIN_GAME
}GameState;
GameState gameState = MENU;

//button also difficulty
typedef struct{
    int x,y,width,height;
    const char* label;
    int swapCount;
    int goal_score;
}Button;

//score pop up animation
typedef struct {
    Vector2 position;
    int amount;
    float lifetime;
    float  alpha;
    bool active;
}Scorepopup;
Scorepopup score_popups[MAX_SCORE_POP]={0};

//struct of data
typedef struct {
    int highest_score;
    int last_score;
}Store;

//all global variable
int score=0;
int swap_count=1;
char board[BOARD_SIZE][BOARD_SIZE]={0};
char matched[BOARD_SIZE][BOARD_SIZE];
float fall_offset[BOARD_SIZE][BOARD_SIZE]={0};
const char tile[TILES_TYPE]={'#','*','^','+','$'};
float fall_speed = 2.0f;
float score_scale=1.0f;
float score_scale_velocity=0.0f;
bool score_animating=false;
Audiosfx coin;


//all function
char random_tile(){
    return tile[rand()%TILES_TYPE];
}
void saveData(const char *filename,Store *data){
    FILE *file = fopen(filename,"w");
    if (!file){
        printf("error opening file for writting.\n");
        return;
    }
    fprintf(file, "%d %d\n",data->highest_score,data->last_score);
    fclose(file);
}
Store *loadStore(const char *filename){
    FILE *file = fopen(filename,"r");
    Store *data = (Store *)malloc(sizeof(Store));
    if (data == NULL){
        printf("memory allocation failed.\n");
        return NULL;
    }
    
    fscanf(file,"%d %d",&data->highest_score,&data->last_score);
    fclose(file);
    return data;
}
void swap_tiles(int* x1,int* y1,int* x2,int* y2){
    char temp = board[*y1][*x1];
    board[*y1][*x1]= board[*y2][*x2];
    board[*y2][*x2]= temp;
}
bool are_tile_adjacent(Vector2* a, Vector2* b) {
    return abs(a->x - b->x) + abs(a->y - b->y) == 1;
}
void add_score_popup(int x,int y,int amount,Vector2 grid_origin){
    for (int i=0;i<MAX_SCORE_POP;i++){
        if (!score_popups[i].active){
            score_popups[i].position=(Vector2){grid_origin.x+x*TILE_SIZE+TILE_SIZE/2,grid_origin.y+y*TILE_SIZE+TILE_SIZE/2};
            score_popups[i].amount=amount;
            score_popups[i].lifetime=3.0f;
            score_popups[i].alpha=1.0f;
            score_popups[i].active=true;
            break;
        }
    }
}

bool find_matched(){
    bool found = false;
    for (int y=0;y<BOARD_SIZE;y++){
        for (int x=0;x<BOARD_SIZE;x++){
            matched[y][x] = false;
        }
    }

    for (int y=0;y<BOARD_SIZE;y++){
        for (int x=0;x<BOARD_SIZE-2;x++){
            char t = board[y][x];
            if (t== board[y][x+1]&&t==board[y][x+2]){
                matched[y][x]=matched[y][x+1]=matched[y][x+2]=true;
                score+=10;
                play_sfx(&coin);
                found =true;
                add_score_popup(x,y,10,grid_origin);
                score_animating=true;
                score_scale=2.0f;
                score_scale_velocity=2.5f;
            }
        }
    }

    for (int y=0;y<BOARD_SIZE-2;y++){
        for (int x=0;x<BOARD_SIZE;x++){
            char t = board[y][x];
            if (t==board[y+1][x]&&t==board[y+2][x]){
                matched[y][x]=matched[y+1][x]=matched[y+2][x]=true;
                score+=10;
                play_sfx(&coin);
                found =true;
                add_score_popup(x,y,10,grid_origin);
                score_animating=true;
                score_scale=5.0f;
                score_scale_velocity=2.5f;
            }
        }
    }
   

    return found;
}
void resolve_matches(){
    for (int x=0;x<BOARD_SIZE;x++){
        int write_y = BOARD_SIZE-1;
        for (int y=BOARD_SIZE-1;y>=0;y--){
            if (!matched[y][x]){
                if (y!=write_y){
                    board[write_y][x]=board[y][x];
                    fall_offset[write_y][x] = (write_y-y)*TILE_SIZE;
                    board[y][x]=' ';
                }             
                write_y--;
            }
        }
        while (write_y>=0){
            board[write_y][x]=random_tile();
            fall_offset[write_y][x]=(write_y+1)*TILE_SIZE;
            write_y--;
        }
    }

}
void init_board(){
    for (int y=0;y<BOARD_SIZE;y++){
        for (int x=0;x<BOARD_SIZE;x++){
            board[y][x]=random_tile();
        }
    }
    int grid_width = BOARD_SIZE * TILE_SIZE;
    int grid_height = BOARD_SIZE * TILE_SIZE;

    grid_origin.x =(320-grid_width)/2;
    grid_origin.y =(240-grid_height)/2;

}

//game loop
int main(void)
{
    const int screenwidth = 320;
    const int screenheight = 240;

    //all button and difficulty load up 
    Button difficultyButton[]={
        {85,100,150,30,"Easy (10 Swaps)",10,200},
        {85,130,150,30,"Medium (5 Swaps)",5,350},
        {85,160,150,30,"Hard (3 Swaps)",3,400}
    };
    Button Menu={110,130,97,30,"Menu",0,0};
    Button resstartButton = {110,130,97,30,"restart",0,0};
    int selectedSwapCount=5;
    int selectedscore=350;

    //all of image load up ready to go
    bool tile_selected = false;
    bool current_selected=false;
    Vector2 selected_tile = {-1,-1};
    Vector2 current_tile={-1,-1};
    Tigr *tiles[5];
    Tigr *background;
    Tigr *background1;
    Tigr *background2;
    Tigr *background3;
    AudioSystem audio;
    Audiosfx button_sfx;
    Audiosfx selected_sfx;
    Tigr *screen = tigrWindow(screenwidth, screenheight, "LONG DAY!", 0);
    background = tigrLoadImage("image/bck.png");
    background1 = tigrLoadImage("image/3.png");
    background2 = tigrLoadImage("image/2.png");
    background3 = tigrLoadImage("image/1.png");
    tiles[0] = tigrLoadImage("image/full-moon.png");
    tiles[1] = tigrLoadImage("image/lucky-cat.png");
    tiles[2] = tigrLoadImage("image/sakura.png");
    tiles[3] = tigrLoadImage("image/snowflake.png");
    tiles[4] = tigrLoadImage("image/star.png");

    srand(time(NULL));

    //all of audio load up ready to go
    init_board();
    init_audio(&audio);
    init_sfx(&button_sfx,"sound/pop.mp3");
    init_sfx(&selected_sfx,"sound/button.mp3");
    init_sfx(&coin,"sound/coin.mp3");
    play_bgm(&audio,"sound/music.mp3");
    const char *filename="file.txt";

    //load up data of score
    Store *gameData=loadStore(filename);
    if (gameData==NULL){
        return 1;
    }

    //init all of mouse position to click gameplay
    int mouse_x;
    int mouse_y;
    int lastMousePressed = 0;
    int button; 

    //init all of mouse button to click button
    int mx;
    int my;
    int mb;
    int lmp=0;
    

    while (!tigrClosed(screen))
    {
        //logic game
        
        //Menu page of the game
        if (gameState==MENU){
        
            tigrClear(screen,tigrRGB(255,255,255));
            tigrBlit(screen,background,0,0,0,0,screenwidth,screenheight);

            tigrPrint(screen,tfont,130,60,tigrRGB(255,0,0),"LONG DAY!");
            tigrPrint(screen, tfont,5,5,tigrRGB(155,155,155),"Highest score: ");
            tigrPrint(screen,tfont,5,15,tigrRGB(155,155,155),"last score: ");
            tigrPrint(screen,tfont,100,5,tigrRGB(255,0,0),"%d\n",gameData->highest_score);
            tigrPrint(screen,tfont,85,15,tigrRGB(255,0,0),"%d\n",gameData->last_score);

            for (int i=0;i<3;i++){
                Button btn = difficultyButton[i];
                int hover = (mx>= btn.x && mx <= btn.x + btn.width && my >= btn.y && my <= btn.y + btn.height);
                tigrRect(screen,btn.x,btn.y,btn.width,btn.height,hover? tigrRGB(0,0,0):tigrRGB(255,0,0));
                tigrFillRect(screen,btn.x,btn.y,btn.width,btn.height,hover? tigrRGBA(255,0,0,190):tigrRGBA(255,255,255,190));
                tigrPrint(screen,tfont,btn.x+20,btn.y+10,hover? tigrRGB(255,255,255):tigrRGB(255,0,0),btn.label);
            } 

           
            tigrMouse(screen,&mx,&my,&mb);
            if (mb&&!lmp){
                for (int i=0;i<3;i++){
                    Button btn=difficultyButton[i];
                    if (mx>=btn.x&&mx<=btn.x+btn.width&&my>=btn.y&&my<=btn.y+btn.height){
                        play_sfx(&button_sfx);
                        selectedSwapCount = btn.swapCount;
                        selectedscore=btn.goal_score;
                        gameState = PLAYING;
                        init_board(); // Reset board
                        swap_count = selectedSwapCount;
                        score = 0;
                        break;
                    }
                }
            }
            lmp=mb;
        }

        //gameplay
        tigrMouse(screen, &mouse_x,&mouse_y,&button);

        if (button && !lastMousePressed && gameState==PLAYING){
            int grid_x = (mouse_x - grid_origin.x)/TILE_SIZE;
            int grid_y = (mouse_y - grid_origin.y)/TILE_SIZE;

            if (grid_x >= 0 && grid_x < BOARD_SIZE && grid_y >= 0 && grid_y < BOARD_SIZE){
                play_sfx(&selected_sfx);
                if (!tile_selected){
                    selected_tile=(Vector2){grid_x,grid_y};
                    tile_selected=true;
                }else if (!current_selected){
                    current_tile=(Vector2){grid_x,grid_y};
                    current_selected=true;
                   
                    if (are_tile_adjacent(&selected_tile,&current_tile)){
                        swap_tiles(&selected_tile.x, &selected_tile.y, &current_tile.x, &current_tile.y);

                        if (!find_matched()) {
                            swap_tiles(&selected_tile.x, &selected_tile.y, &current_tile.x, &current_tile.y);
                        } else {
                            swap_count--;
                        }


                        tile_selected=false;
                        current_selected=false;
                        selected_tile=(Vector2){-1,-1};
                        current_tile=(Vector2){-1,-1};
                        
                    }else{
                        selected_tile=(Vector2){-1,-1};
                        current_tile=(Vector2){-1,-1};
                        tile_selected=false;
                        current_selected=false;

                    }
                }
            }else{
                tile_selected=false;
                current_selected=false;
                selected_tile=(Vector2){-1,-1};
                current_tile=(Vector2){-1,-1};
            }
        }
        lastMousePressed=button;


        if (find_matched()){
            resolve_matches();
          
        }

        if (score>=selectedscore){
            gameState=WIN_GAME;
        }

        if (swap_count<=0){
            gameState=GAME_OVER;
        }

        //animation 
        for (int y=0;y<BOARD_SIZE;y++){
            for (int x=0;x<BOARD_SIZE;x++){
                if (fall_offset[y][x]>0){
                    fall_offset[y][x]-= fall_speed;
                    if (fall_offset[y][x]<0){
                        fall_offset[y][x]=0;
                    }
                }
            }
        }

        for (int i=0;i<MAX_SCORE_POP;i++){
            if (score_popups[i].active){
                score_popups[i].lifetime-= 0.1f;
                score_popups[i].position.y-=1;
                if (score_popups[i].lifetime<=0){
                    score_popups[i].alpha=score_popups[i].lifetime;

                    if (score_popups[i].lifetime<=0.0f){
                        score_popups[i].active=false;
                    }
                }
            }
        }

        if (score_animating){
            score_scale-=score_scale_velocity*0.1f;
            if (score_scale<=1.0f){
                score_scale=1.0f;
                score_animating=false;
            }
        }

        //graphic game

        //gameplay page
        if (gameState==PLAYING){

            tigrClear(screen, tigrRGB(215, 215, 215));
            tigrBlit(screen,background1,0,0,0,0,screenwidth,screenheight);
            for (int y=0;y<BOARD_SIZE;y++){
                for (int x=0;x<BOARD_SIZE;x++){
                    tigrRect(screen,grid_origin.x+(x*TILE_SIZE),grid_origin.y+(y*TILE_SIZE),TILE_SIZE,TILE_SIZE,tigrRGB(100,100,100));
                    tigrFillRect(screen,grid_origin.x+(x*TILE_SIZE),grid_origin.y+(y*TILE_SIZE),TILE_SIZE,TILE_SIZE,tigrRGBA(100,100,100,160));
                    if (board[y][x]!=' '){

                        int tile_index = -1;
                        for (int i = 0; i < TILES_TYPE;i++) {
                            if (tile[i] == board[y][x]) {
                                tile_index = i;
                                break;
                            }
                        }
                        // tigrPrint(screen, tfont,grid_origin.x+(x*TILE_SIZE+3),(grid_origin.y+ (y*TILE_SIZE+1))-fall_offset[y][x],tigrRGB(235,235,235), "%c",board[y][x]);
                        tigrBlitAlpha(screen,tiles[tile_index],grid_origin.x+(x*TILE_SIZE),grid_origin.y+(y*TILE_SIZE)-fall_offset[y][x],0,0,16,16,1.0f);                    
                    }

                }

            }

            if (score_animating){
                score_scale-=score_scale_velocity*0.05f;
                if (score_scale<=1.0f){
                    score_scale=1.0f;
                    score_animating=false;
                    }
            }

            if (tile_selected&&selected_tile.x>=0){
                tigrRect(screen, grid_origin.x+(selected_tile.x*TILE_SIZE) , grid_origin.y+(selected_tile.y*TILE_SIZE),TILE_SIZE , TILE_SIZE ,tigrRGB(255,0,0));
            }

            tigrPrint(screen,tfont,5,5,tigrRGB(155,155,155),"Score: ");
            tigrPrint(screen,tfont,48-(score_scale*2),5-score_scale,tigrRGB(255,0,0),"%d",score);
            // tigrPrint(screen, tfont,5,15,tigrRGB(255,255,255),"Highest score: %d\n",gameData->highest_score);
            tigrPrint(screen,tfont,5,15,tigrRGB(155,155,155),"Goal score: %d\n",selectedscore);
            tigrPrint(screen,tfont,230,5,tigrRGB(255,0,0),"swap left: %d",swap_count);

            for (int i=0;i<MAX_SCORE_POP;i++){
                if (score_popups[i].active){
                    tigrPrint(screen,tfont,score_popups[i].position.x,score_popups[i].position.y,tigrRGB(255,0,0),"%d",score_popups[i].amount);
                }
            }
        }
        //game win page
        else if (gameState==WIN_GAME){
           gameData->last_score=score;
            if (score>gameData->highest_score){
                gameData->highest_score=score;
            }
            saveData(filename,gameData);


            tigrClear(screen,tigrRGB(255,255,255));
            tigrBlit(screen,background3,0,0,0,0,screenwidth,screenheight);
            tigrPrint(screen,tfont,(screenwidth/2)-(tigrTextWidth(tfont,"U win!")/2),100,tigrRGB(255,0,0),"U win!");

            tigrPrint(screen, tfont,5,5,tigrRGB(155,155,155),"Highest score: ");
            tigrPrint(screen,tfont,5,15,tigrRGB(155,155,155),"last score: ");
            tigrPrint(screen,tfont,100,5,tigrRGB(255,0,0),"%d\n",gameData->highest_score);
            tigrPrint(screen,tfont,85,15,tigrRGB(255,0,0),"%d\n",gameData->last_score);


            int hover2 = (mx >=Menu.x && mx <= Menu.x + Menu.width && my >= Menu.y && my <= Menu.y + Menu.height);

            tigrRect(screen,Menu.x,Menu.y,Menu.width,Menu.height,hover2? tigrRGB(125,125,125):tigrRGB(255,0,0));
            tigrFillRect(screen,Menu.x,Menu.y,Menu.width,Menu.height,hover2? tigrRGBA(125,125,125,190):tigrRGBA(255,0,0,190));
            tigrPrint(screen,tfont,Menu.x+35,Menu.y+10,hover2? tigrRGB(125,125,125):tigrRGB(255,0,0),"Menu");
            tigrMouse(screen,&mx,&my,&mb);
            if (mb && !lmp){
                if (mx >= Menu.x && mx <= Menu.x + Menu.width &&my >= Menu.y && my <= Menu.y + Menu.height){
                    play_sfx(&button_sfx);
                    gameState=MENU;
                    swap_count=1;
                    init_board();
                    score=0;
                    selectedSwapCount=5;
                    selectedscore=350;
                }    
            }
            lmp=mb;
        }

        //game lose page
        else if (gameState==GAME_OVER){
            gameData->last_score=score;
            if (score>gameData->highest_score){
                gameData->highest_score=score;
            }
            saveData(filename,gameData);


            tigrClear(screen,tigrRGB(255,0,0));
            tigrBlit(screen,background2,0,0,0,0,screenwidth,screenheight);
            tigrPrint(screen,tfont,(screenwidth/2)-(tigrTextWidth(tfont,"u lose!")/2),100,tigrRGB(255,255,255),"u lose!");

            tigrPrint(screen, tfont,5,5,tigrRGB(155,155,155),"Highest score: ");
            tigrPrint(screen,tfont,5,15,tigrRGB(155,155,155),"last score: ");
            tigrPrint(screen,tfont,100,5,tigrRGB(255,0,0),"%d\n",gameData->highest_score);
            tigrPrint(screen,tfont,85,15,tigrRGB(255,0,0),"%d\n",gameData->last_score);


            int hover1 = (mx>= resstartButton.x && mx <= resstartButton.x + resstartButton.width && my >= resstartButton.y && my <= resstartButton.y + resstartButton.height);
            tigrRect(screen,resstartButton.x,resstartButton.y,resstartButton.width,resstartButton.height,hover1? tigrRGB(125,125,125):tigrRGB(255,255,255));
            tigrFillRect(screen,resstartButton.x,resstartButton.y,resstartButton.width,resstartButton.height,hover1? tigrRGBA(125,125,125,190):tigrRGBA(255,0,0,190));
            tigrPrint(screen,tfont,resstartButton.x+25,resstartButton.y+10,hover1? tigrRGB(125,125,125):tigrRGB(255,255,255),"Restart");

            tigrMouse(screen,&mx,&my,&mb);
            if (mb && !lmp){
                if (mx >= resstartButton.x && mx <= resstartButton.x + resstartButton.width &&my >= resstartButton.y && my <= resstartButton.y + resstartButton.height){
                    play_sfx(&button_sfx);
                    gameState=PLAYING;
                    score=0;
                    init_board();
                    swap_count=selectedSwapCount;
                }    
            }
            lmp=mb;
        }

        tigrUpdate(screen);
    }
    stop_bgm(&audio);

    //save data
    gameData->last_score=score;
    if (score>gameData->highest_score){
        gameData->highest_score=score;
    }
    saveData(filename,gameData);

    //clean up
    shutdown_sfx(&coin);
    shutdown_sfx(&button_sfx);
    shutdown_sfx(&selected_sfx);
    shutdown_audio(&audio);
    tigrFree(background);
    tigrFree(background1);
    tigrFree(background2);
    tigrFree(screen);
    for (int i=0;i<5;i++){
        tigrFree(tiles[i]);
    }
    free(gameData);

    return 0;
}
