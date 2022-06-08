#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define BOARD_HEIGHT 9 //盤面の縦
#define BOARD_WIDTH 15 //盤面の横
//#define BOMB_N 3
#define BOMB_LIMIT 3*10 //爆弾の爆発するまでの時間*10
#define TRESURE_N 4 //宝物の数
#define TIME_LIMIT 60 //制限時間(秒)
#define STAGE_N 4 //盤面の種類

int BOMB_N=1; //爆弾の置ける個数初期値

//絵
char FIRE[3][6]={
    "(( )) ",
    ")@@@( ",
    "@@@@@",
};
char WALL[3][6]={
    "|:|:|",
    ":|:|:",
    "|:|:|",
};
char ROCK[3][6]={
    "* * *",
    " * * ",
    "* * *"
};
char ME[3][6]={
    "[^_^]",
    "/| |\\",
    " o o "
};
char BOMB[3][6]={
    "  ^* ",
    "/   \\",
    "\\___/"
};
char TREASURE[3][6]={
    "+  * ",
    "  /\\+",
    "* \\/ "
};
char SPACE[3][6]={
    "     ",
    "     ",
    "     "
};

char ENEMY[3][6]={
    "(x x)",
    "L| |7",
    " o o "
};

//座標
typedef struct point {
    int x;
    int y;
} Point;

//キーボードを押したか判定
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

//書式設定
void check_box(const char board[][BOARD_WIDTH + 1], int row, int column){
    char box[7]={'F','x','#','o','B','*','E'};
    int color[7]={31,37,47,36,31,33,32};
    for(int i=0; i<7; i++){
        if(board[row][column]==box[i]){
            if(i==2) printf("\x1b[30m");
            if(i==3 || i==5 || i==6) printf("\x1b[1m"); 
            printf("\x1b[%dm",color[i]);
        }
    }
}

//1行(ターミナル上では3行)ずつ表示
void print_row(char row[3][5*BOARD_WIDTH+1], int bold[BOARD_WIDTH], const char board[][BOARD_WIDTH + 1], int row_n){
    for(int i=0; i<3; i++){
        for(int j=0; j<5*BOARD_WIDTH; j++){
            if(j%5==0){//1マスごとに書式をチェック
                printf("\x1b[0m");
                if(bold[j/5]) printf("\x1b[1m");
                check_box(board, row_n, j/5);
            }
            printf("%c",row[i][j]);
        }
        printf("\x1b[0m");
        printf("\r\n");
    }
}

//絵柄を挿入
void set_box(char row[][5*BOARD_WIDTH+1], char box[][6], int n){
    for(int i=0; i<3; i++){
        for(int j=0; j<5; j++){
            row[i][5*n+j]=box[i][j];
        }
    }
}


//盤面を表示
void print_board(const char board[][BOARD_WIDTH + 1], Point bombs[BOMB_N], int bombsCount[BOMB_N]) {
    for(int i = 0; i < BOARD_HEIGHT; ++i) {
        char row[3][5*BOARD_WIDTH+1];
        int bold[BOARD_WIDTH]={0};
        

        for(int j = 0; j < BOARD_WIDTH; j++){
            if(board[i][j]=='F'){
                set_box(row,FIRE,j);
            } 
            if(board[i][j]=='x'){
                set_box(row,WALL,j);
            } 
            if(board[i][j]=='#'){
                set_box(row,ROCK,j);
            } 
            if(board[i][j]=='o'){
                set_box(row,ME,j);
            } 
            if(board[i][j]=='B'){
                for(int l=0; l<BOMB_N; l++){
                    if(i==bombs[l].y && j==bombs[l].x && (bombsCount[l]>=BOMB_LIMIT-10 || bombsCount[l]%10==0) && bombsCount[l] != 0){
                        bold[j]=1;
                    }
                }
                set_box(row,BOMB,j);
            }
            if(board[i][j]=='*'){
                set_box(row,TREASURE,j);
            } 

            if(board[i][j]=='-'){
                set_box(row,SPACE,j);
            }
            if(board[i][j]=='E'){
                set_box(row,ENEMY,j);
            }
        }

        print_row(row, bold, board, i);
    }
}

//爆発
void explode(int k, Point p, char board[][BOARD_WIDTH + 1], int *gameover, Point bombs[BOMB_N], int bombsCount[BOMB_N], int combo, int *pt, int *e_live){
    board[bombs[k].y][bombs[k].x]='F'; 

    if(bombs[k].y==p.y && bombs[k].x==p.x){
        *gameover=1;
    }

    int d[4][2] = {{0,1},{0,-1},{-1,0},{1,0}};

    for(int i=0; i<4; i++){
        for(int j=1; j<=2; j++){
            if(0<=bombs[k].y+d[i][0]*j && bombs[k].y+d[i][0]*j<=BOARD_HEIGHT-1 && 0<=bombs[k].x+d[i][1]*j && bombs[k].x+d[i][1]*j<=BOARD_WIDTH-1){
                if(board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] == '#' || board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] == '*'){//障害物と宝物は燃えない
                    break;
                }
                if(!combo){
                    if(board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] == 'B'){
                        for(int bomb=0; bomb<BOMB_N; bomb++){
                            if(bombs[bomb].x != -1){
                                explode(bomb, p, board, gameover, bombs, bombsCount, 1, pt, e_live);
                            }
                        }
                        break;
                    }
                }
                if(board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] == 'o'){
                    board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] = 'F';
                    *gameover = 1;
                    break;
                }
                if(board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] == 'x'){
                    board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] = 'F';
                    break;
                }
                if(board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] == 'E'){
                    board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] = 'F';
                    *e_live=0;
                    *pt+=10;
                    break;
                }
                board[bombs[k].y+d[i][0]*j][bombs[k].x+d[i][1]*j] = 'F';
            }
        }
    }
    bombsCount[k]=0;
    bombs[k].x=-1;
    bombs[k].y=-1;

}

void move(int c, Point *p, Point bombs[BOMB_N], char board[][BOARD_WIDTH + 1], int *treasure, int *pt){
    int key[4] = {'d', 'a', 'w', 's'};
    int d[4][2] = {{0,1},{0,-1},{-1,0},{1,0}};
    for(int i=0; i<4; i++){
        if(c == key[i]){
            if(0<=p->y+d[i][0] && p->y+d[i][0]<=BOARD_HEIGHT-1 && 0<=p->x+d[i][1] && p->x+d[i][1]<=BOARD_WIDTH-1){
                if(board[p->y+d[i][0]][p->x+d[i][1]] != '#' && board[p->y+d[i][0]][p->x+d[i][1]] != 'x' && board[p->y+d[i][0]][p->x+d[i][1]] != 'B' && board[p->y+d[i][0]][p->x+d[i][1]] != 'E'){
                    if(board[p->y+d[i][0]][p->x+d[i][1]]=='*'){
                        board[p->y+d[i][0]][p->x+d[i][1]] = '-';
                        *treasure += 1;
                        *pt += 1;
                        BOMB_N+=1;
                    }else{
                        p->y += d[i][0];
                        p->x += d[i][1];
                    }
                }
            }
            break;
        }
    }   
    if(board[p->y][p->x] != 'B'){
        board[p->y][p->x] = 'o';
    }
}


void setBomb(Point p, Point bombs[BOMB_N], int bombsCount[BOMB_N], char board[][BOARD_WIDTH + 1]){
    for(int i=0; i<BOMB_N; i++){
        if(bombs[i].x==p.x && bombs[i].y==p.y){
            break;
        }
        if(bombs[i].x==-1){
            for(int j=0; j<BOMB_N; j++){
                if(bombs[j].x != -1){
                    bombsCount[j]++;
                }
            }
            board[p.y][p.x]='B';
            bombs[i].x=p.x;
            bombs[i].y=p.y;
            break;
        }
        
    }
}

void enemy_move(char board[][BOARD_WIDTH + 1], Point *e, int e_live){
    if(e_live){
        int d[9][2] = {{0,1},{0,-1},{-1,0},{1,0},{0,1},{0,-1},{-1,0},{1,0},{0,0}};
        srand((unsigned int)time(NULL));
        board[e->y][e->x]='-'; 
        int move=1;
        for(int i=0; i<4; i++){
            if(0<=e->y+d[i][0] && e->y+d[i][0]<=BOARD_HEIGHT-1 && 0<=e->x+d[i][1] && e->x+d[i][1]<=BOARD_WIDTH-1){
                if(board[e->y+d[i][0]][e->x+d[i][1]]=='B'&& 0<=e->y-d[i][0] && e->y-d[i][0]<=BOARD_HEIGHT-1 && 0<=e->x-d[i][1] && e->x-d[i][1]<=BOARD_WIDTH-1){
                    if(board[e->y-d[i][0]][e->x-d[i][1]] != '#' && board[e->y-d[i][0]][e->x-d[i][1]] != 'x' && board[e->y-d[i][0]][e->x-d[i][1]] != 'B' && board[e->y-d[i][0]][e->x-d[i][1]] != '*' && board[e->y-d[i][0]][e->x-d[i][1]] != 'o'){
                        e->y -= d[i][0];
                        e->x -= d[i][1];
                        move=0;
                    }
                }
            }
        }
        while(move){
            int i = rand()%9;   
            
            if(0<=e->y+d[i][0] && e->y+d[i][0]<=BOARD_HEIGHT-1 && 0<=e->x+d[i][1] && e->x+d[i][1]<=BOARD_WIDTH-1){
                if(board[e->y+d[i][0]][e->x+d[i][1]] != '#' && board[e->y+d[i][0]][e->x+d[i][1]] != 'x' && board[e->y+d[i][0]][e->x+d[i][1]] != 'B' && board[e->y+d[i][0]][e->x+d[i][1]] != '*' && board[e->y+d[i][0]][e->x+d[i][1]] != 'o'){
                    e->y += d[i][0];
                    e->x += d[i][1];
                    move=0;
                    if(0<=e->y+d[i][0] && e->y+d[i][0]<=BOARD_HEIGHT-1 && 0<=e->x+d[i][1] && e->x+d[i][1]<=BOARD_WIDTH-1 && board[e->y+d[i][0]][e->x+d[i][1]] == 'B'){//二個先が爆弾ならその方には進まない。
                        e->y -= d[i][0];
                        e->x -= d[i][1];
                        move=1;
                    }        
                }
            }
        }
        board[e->y][e->x]='E';
    }        
}



void display(int c, int pt, double remain){
    //表示
    system("clear");
    printf("\x1b[0m");
    printf("Press '.' to close\r\n");        
    printf("\x1b[1m");
    for(int i=0; i<BOARD_WIDTH; i++){
        printf("-----");
    }
    printf("\r\n\r\n");
    
    char bomberman[5][58]={
        " ####  ##### #   # ####  ##### ####     #   #   #   #   #",
        " #  ## #   # ## ## #  ## #     #   #    ## ##  # #  ##  #",
        " ##### #   # # # # ##### ##### ####     # # #  ###  # # #",
        " #   # #   # # # # #   # #     #  ##    # # # #   # #  ##",
        " ####  ##### #   # ####  ##### #   #    #   # #   # #   #" 

    };
    
    for(int i=0; i<5; i++){
        printf("\x1b[31m");
        printf("%s", bomberman[i]);
        if(i==0) printf("\x1b[36m        w");
        if(i==1) printf("\x1b[39m        ^");
        if(i==2) printf("\x1b[36m      a\x1b[39m< >\x1b[36md");
        if(i==3) printf("\x1b[39m        v");
        if(i==4) printf("\x1b[36m        s");

        printf("\r\n");
    }

    printf("\x1b[33m");
    for(int i=0; i<5*BOARD_WIDTH-16; i++){
        printf(" ");
    }
    printf("%2d ", pt);
    printf("\x1b[39m");
    printf("pt ");
    if(remain<10){
        printf("\x1b[31m"); 
        if(remain<10) printf(" ");
    }   
    printf(" %.1f", remain);
    printf("\x1b[39m");
    printf(" sec. \r\n");
    for(int i=0; i<BOARD_WIDTH; i++){
        printf("-----");
    }
    printf("\r\n");
    printf("\x1b[49m");
}

void result(int gameclear, int gameover, int *finish){
    char clear[5][60]={
        " ####   #   #   # #####     #### #     #####   #   ####   #", 
        "#      # #  ## ## #        #     #     #      # #  #   #  #",
        "#  ##  ###  # # # #####    #     #     #####  ###  ####   #",
        "#   # #   # # # # #        #     #     #     #   # #  ##   ",
        "##### #   # #   # #####    ##### ##### ##### #   # #   #  #"

    };
    char over[5][54]={
        " ####   #   #   # #####     #### #   # ##### ####   #",
        "#      # #  ## ## #        #   # #   # #     #   #  #",
        "#  ##  ###  # # # #####    #   #  # #  ##### ####   #",
        "#   # #   # # # # #        #   #  # #  #     #  ##   ",
        "##### #   # #   # #####    ####    #   ##### #   #  #"
    };


    //ゲームクリアー
    if(!gameover && gameclear){
        usleep(1.5*1000*1000);
        system("clear");
        for(int i=0; i<6+3*(int)(BOARD_HEIGHT/2)-2; i++){
            printf("\r\n");
        }
        printf("\x1b[33m\x1b[1m");
        for(int i=0; i<5; i++){
            for(int j=0; j<5*(int)(BOARD_WIDTH/2)-28; j++){
                printf(" ");
            }
            printf("%s\r\n",clear[i]);
        }
        for(int i=0; i<6+3*(int)(BOARD_HEIGHT/2)-2; i++){
            printf("\r\n");
        }
        printf("\x1b[0m");
        *finish=1;
    }
    //ゲームオーバー
    if(gameover){
        usleep(1.5*1000*1000);
        system("clear");
        for(int i=0; i<6+3*(int)(BOARD_HEIGHT/2)-2; i++){
            printf("\r\n");
        }
        printf("\x1b[31m\x1b[1m");

        for(int i=0; i<5; i++){
            for(int j=0; j<5*(int)(BOARD_WIDTH/2)-26; j++){
                printf(" ");
            }
            printf("%s\r\n",over[i]);
        }
         for(int i=0; i<6+3*(int)(BOARD_HEIGHT/2)-2; i++){
            printf("\r\n");
        }
        printf("\x1b[0m");
        *finish=1;
    }
}

void select_display(){
    printf("\x1b[1m");
    for(int i=0; i<BOARD_WIDTH; i++){
        printf("-----");
    }
    printf("\r\n\r\n");
    
    char select[5][47]={
        "      #### ##### #     #####  #### #####   #  ",
        "     #     #     #     #     #       #     #  ",
        "     ##### ##### #     ##### #       #     #  ",
        "         # #     #     #     #       #        ",
        "     ####  ##### ##### #####  ####   #     #  " 
    };
    
    for(int i=0; i<5; i++){
        printf("\x1b[31m");
        printf("%s", select[i]);
        if(i==2) printf("    \x1b[36m      a\x1b[39m< >\x1b[36md");
        if(i==4) printf("   \x1b[39mpress \x1b[36m[space] \x1b[39mto start");
        printf("\r\n");
    }
    printf("\r\n\x1b[39m");
    for(int i=0; i<BOARD_WIDTH; i++){
        printf("-----");
    }
    printf("\r\n");
    printf("\x1b[49m");
}

void countdown(){
    char num[4][5][16]={
        {
            "     #####     ",
            "         #     ",
            "     #####     ",
            "         #     ",
            "     #####     ",

        },
        {
            "     #####     ",
            "         #     ",
            "     #####     ",
            "     #         ",
            "     #####     "
        },
        {
            "      ##       ",
            "       #       ",
            "       #       ",
            "       #       ",
            "     #####     "
        },
        {
            " ####  #####  #",
            "#      #   #  #",
            "#  ##  #   #  #",
            "#   #  #   #   ",
            "#####  #####  #"
        }
    };
    
    printf("\x1b[1m\x1b[31m");
    
    for(int i=0; i<4; i++){
        system("clear");
        usleep(0.4*1000*1000);
        for(int j=0; j<6+3*(int)(BOARD_HEIGHT/2)-2; j++){
            printf("\r\n");
        }
        for(int j=0; j<5; j++){
            for(int k=0; k<5*(int)(BOARD_WIDTH/2)-8; k++){
                printf(" ");
            }
            printf("%s\r\n",num[i][j]);
        }
        for(int j=0; j<6+3*(int)(BOARD_HEIGHT/2)-2; j++){
            printf("\r\n");
        }
        usleep(0.6*1000*1000);
    }
    usleep(0.4*1000*1000);
    printf("\x1b[0m");
}

void title(int light){
    char bomberman[5][58]={
        " ####  ##### #   # ####  ##### ####     #   #   #   #   #",
        " #  ## #   # ## ## #  ## #     #   #    ## ##  # #  ##  #",
        " ##### #   # # # # ##### ##### ####     # # #  ###  # # #",
        " #   # #   # # # # #   # #     #  ##    # # # #   # #  ##",
        " ####  ##### #   # ####  ##### #   #    #   # #   # #   #" 

    };
    for(int i=0; i<6+3*(int)(BOARD_HEIGHT/2)-5; i++){
        printf("\r\n");
    }
    printf("\x1b[31m\x1b[1m");
    for(int i=0; i<5; i++){
        for(int j=0; j<5*(int)(BOARD_WIDTH/2)-28; j++){
            printf(" ");
        }
        printf("%s\r\n",bomberman[i]);
    }
    for(int i=0; i<2; i++){
        printf("\r\n");
    }
    if(light==1){
        for(int i=0; i<5*(int)(BOARD_WIDTH/2)-12; i++){
            printf(" ");
        }
        printf("\x1b[39m");
        printf("Press \x1b[36m[space]\x1b[39m to continue\r\n");
    }else{
        printf("\r\n");
    }
    for(int i=0; i<6+3*(int)(BOARD_HEIGHT/2)-3; i++){
        printf("\r\n");
    }
    printf("\x1b[0m");
}


int main (int argc, char *argv[]) {

    int stage=0;

    char board[STAGE_N][BOARD_HEIGHT][BOARD_WIDTH + 1] = {
        {
            "-------x-------",
            "-#-#-#-x-#-#-#-",
            "--#*#--x--#*#--",
            "-------x-------",
            "xxxxxxxxxxxxxxx",
            "-------x-------",
            "--#*#--x--#*#--",
            "-#-#-#-x-#-#-#-",
            "-------x------E"
        },
        {
            "-------x-------",
            "-----#xxx#-----",
            "---#x--x--x#---",
            "-#x---*x*---x#-",
            "xxxxxxxxxxxxxxx",
            "-#x---*x*---x#-",
            "---#x--x--x#---",
            "-----#xxx#-----",
            "-------x------E"
        },
        {
            "-------x-------",
            "-x#x#x-x-x#x#x-",
            "--x*x--x--x*x--",
            "-x#x#x-x-x#x#x-",
            "xxxxxxxxxxxxxxx",
            "-x#x#x-x-x#x#x-",
            "--x*x--x--x*x--",
            "-x#x#x-x-x#x#x-",
            "-------x------E"
        },
        {
            "-------*-------",
            "xxxxxxxxxxxxxxx",
            "-------*-------",
            "xxxxxxxxxxxxxxx",
            "-------*-------",
            "xxxxxxxxxxxxxxx",
            "-------*-------",
            "xxxxxxxxxxxxxxx",
            "--------------E"
        }
    };

    Point p = {.x = 0, .y = 0};  // 自分の位置
    Point e = {.x = BOARD_WIDTH-1, .y=BOARD_HEIGHT-1};
    int e_live = 1;
    int e_move=1;

    int c;

    int gameover = 0;
    int gameclear = 0;
    int finish=0;
    int treasure = 0;
    int pt=0;

    Point bombs[BOMB_N+TRESURE_N];
    int bombsCount[BOMB_N+TRESURE_N];
    
    for(int i=0; i<BOMB_N+TRESURE_N; i++){
        bombs[i].x=-1;
        bombs[i].y=-1;
        bombsCount[i]=0;
    }

    struct timespec start;
    struct timespec now;
    double remain=TIME_LIMIT;
    double old_remain=TIME_LIMIT+1;
    int first = 1;
  
    int light=1;
 
    system("/bin/stty raw onlcr");  // enterを押さなくてもキー入力を受け付けるようになる
  
    while(1){
        system("clear");
        title(light);
        usleep(0.5*1000*1000);
        light*=-1;
        if(kbhit()){
            if((c=getchar())==' '){
                break;
            }
        }
    }
    while(1){
        system("clear");
        select_display();
        print_board(board[stage],bombs,bombsCount);
        c=getchar();
        if(c==' '){
            break;
        }
        if(c=='d' && stage<STAGE_N-1){
            stage++;
        }else if(c=='a' && stage>0){
            stage--;
        }
    }
    
    countdown();
    
    timespec_get(&start, TIME_UTC);
    c='q';
    while(old_remain != remain) {   // '.' を押すと抜ける
        
        if(kbhit()){
            if((c=getchar())=='.'){
                break;
            }
            //火がついてたら消す
            for(int i=0; i<BOARD_HEIGHT; i++){
                for(int j=0; j<BOARD_WIDTH; j++){
                    if(board[stage][i][j]=='F'){
                        board[stage][i][j]='-';
                    }
                }
            }

        }
        
        for(int j=0; j<BOMB_N; j++){
            if(bombs[j].x != -1){
                bombsCount[j]++;
            }
        }

        if(c == 'd' || c == 'a' | c == 'w' || c == 's'){   //移動
            move(c,&p, bombs, board[stage], &treasure, &pt);
        }else if(c == ' '){ //爆弾設置
            setBomb(p,bombs,bombsCount,board[stage]);
        }else{
            if(board[stage][p.y][p.x]!='B'){
                board[stage][p.y][p.x] = 'o';
            }
        }

        //カウントが溜まったら爆発
        for(int i=0; i<BOMB_N; i++){
            if(bombsCount[i]==BOMB_LIMIT){
                explode(i, p, board[stage], &gameover, bombs, bombsCount, 0, &pt, &e_live); 
            }
        }

        e_move++;
        if(e_move % 3==0){
            enemy_move(board[stage], &e, e_live);
        }

 
        
        display(c,pt, remain);

        print_board(board[stage], bombs, bombsCount);
        
        if(board[stage][p.y][p.x] == 'o'){
            board[stage][p.y][p.x] = '-';  
        }

        if(treasure==TRESURE_N && !e_live) gameclear=1;
        if(remain<=0) gameover=1;

        
        result(gameclear, gameover, &finish);
        if(finish) break;

        usleep(1000*100);
    
        timespec_get(&now, TIME_UTC);
        old_remain=remain;
        remain=floor((TIME_LIMIT-(now.tv_sec-start.tv_sec)-(double)((now.tv_nsec-start.tv_nsec))/(1000*1000*1000))*10)/10;
        c='q';
        
    }
    system("/bin/stty cooked");  // 後始末
    return 0;
}
