#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define ESC "\x1B["

#define HIGHT 10       // y i
#define WIDTH 25       // x j
int map[HIGHT][WIDTH]; // map[y][x] 表示坐标点 (x, y)

// 新增：游戏状态变量
int score = 0;
int level = 1;
int paused = 0;
unsigned long tick_speed = 1000 * 1000; // 初始速度

typedef int16_t Coordinate;
typedef struct COORD {
    Coordinate Y;
    Coordinate X;
} COORD;

COORD faller[4];
COORD next_faller[4];  // 添加这行以修复错误

short o_shape[8] = {4, 3, 5, 3, 4, 4, 5, 4};
short j_shape[8] = {4, 4, 5, 4, 6, 4, 4, 3};
short l_shape[8] = {5, 4, 4, 4, 3, 4, 5, 3};
short t_shape[8] = {5, 4, 4, 4, 6, 4, 5, 3};
short i_shape[8] = {4, 4, 5, 4, 3, 4, 6, 4};
short s_shape[8] = {4, 3, 5, 3, 4, 2, 5, 4};
short z_shape[8] = {4, 3, 5, 3, 5, 2, 4, 4};

short shape[7][8] = {
    {4, 3, 5, 3, 4, 4, 5, 4}, {4, 4, 5, 4, 6, 4, 4, 3},
    {5, 4, 4, 4, 3, 4, 5, 3}, {5, 4, 4, 4, 6, 4, 5, 3},
    {4, 4, 5, 4, 3, 4, 6, 4}, {4, 3, 5, 3, 4, 2, 5, 4},
    {4, 3, 5, 3, 5, 2, 4, 4},
};

typedef enum SHAPE_TYPE {
    O_SHAPE,
    J_SHAPE,
    L_SHAPE,
    T_SHAPE,
    I_SHAPE,
    S_SHAPE,
    Z_SHAPE
} SHAPE_TYPE;

SHAPE_TYPE now_shape;

typedef enum ELEMENT { AIR, BLOCK, MOVING } ELEMENT;

#define TICK 1000 * 1000    // 1 seconds
#define COOLDOWN 200 * 1000 // 0.2 seconds
int keyboard_flag;
unsigned long start_time;

static int old_fcntl;
static struct termios term;
// static struct winsize console_size;

int is_legal(COORD test[4]) // 1为合法 0为不合法
{
    for (int i = 0; i < 4; i++) {
        if (test[i].Y < 0 || test[i].Y >= HIGHT)
            return 0;
        if (test[i].X >= WIDTH)
            return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (map[test[i].Y][test[i].X] == BLOCK)
            return 0;
    }
    return 1;
}

// 修复 generate 函数，正确创建方块形状
void generate() {
    // 直接生成当前方块，不再使用预览
    now_shape = rand() % 7;
    // 正确转换形状数组到坐标结构
    for (int i = 0; i < 4; i++) {
        faller[i].X = shape[now_shape][i*2];
        faller[i].Y = shape[now_shape][i*2+1];
    }
    
    for (int i = 0; i < 4; i++)
        map[faller[i].Y][faller[i].X] = MOVING;
}

void set_cursor_absolute_position(Coordinate x, Coordinate y) {
    // POSIX控制台坐标从1开始
    printf(ESC "%d;%dH", y + 1, x + 1);
}

// 打印完整界面（包括说明部分）- 只在游戏开始和从暂停恢复时调用
void print_full_map() {
    // 完全清屏，重新开始绘制
    printf(ESC "2J");
    set_cursor_absolute_position(0, 0);
    
    printf("\t\t\t  俄罗斯方蟹\n"
           "\t  ______________________________________________\n");
    
    // 打印游戏主区域
    for (int i = 0; i < HIGHT; i++) {
        printf("\t<<|");
        for (int j = 3; j < WIDTH; j++) {
            if (map[i][j] == AIR) {
                printf(". ");
            } else {
                printf("██");
            }
        }
        printf("|>>\n");
    }
    
    // 蟹的下半身
    printf("\t <----------------①----------①----------------->\n");
    printf("\t    /                                        \\\n"
           "\t   /                                          \\\n"
           "\t   \\                                          /\n"
           "\t    \\                                        /\n"
           "\t    /\\                                      /\\\n");
    
    // 在操作说明上方居中显示分数和等级
    // printf("\n\t\t\t分数: %d  等级: %d\n", score, level);
    
    // 操作说明部分
    printf("\t  ================= 操作说明 =================\n\n");
    
    // 完整的操作提示信息
    printf("\t  h/r/R/H: 旋转方块(↻↺)      P: 暂停/继续游戏\n"
           "\t  SPACE: 快速到最右(→→)      Ctrl+C: 退出游戏\n"
           "\t  l/L: 向右移动(→)          \n"
           "\t  k/K: 向上移动(↑)          \n"
           "\t  j/J: 向下移动(↓)          蟹如人生矣，戏T!\n");
}

// 添加一个专门用于更新分数显示的函数
void update_score_display() {
    // 保存当前光标位置
    printf(ESC "s");
    
    // 移动到显示分数的位置（居中显示在操作说明上方）
    set_cursor_absolute_position(0, HIGHT + 6);
    printf("\t\t\t分数: %d  等级: %d", score, level);
    
    // 恢复光标位置
    printf(ESC "u");
}

// 只更新游戏区域和分数 - 在游戏循环中调用
void print_map() {
    // 不清屏，只移动光标到顶部
    set_cursor_absolute_position(0, 0);
    
    printf("\t\t\t  俄罗斯方蟹\n"
           "\t  ______________________________________________\n");
    
    // 只打印游戏主区域
    for (int i = 0; i < HIGHT; i++) {
        printf("\t<<|");
        for (int j = 3; j < WIDTH; j++) {
            if (map[i][j] == AIR) {
                printf(". ");
            } else {
                printf("██");
            }
        }
        printf("|>>\n");
    }
}

void restore_console(void) {
    fcntl(STDIN_FILENO, F_SETFL, old_fcntl);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    printf(ESC "?25h");
    // 到窗口左下角去，但要必须要输出一个换行才能恢复输出文本属性
    printf(ESC "0m\n");
}

// 被杀死前，先恢复控制台
void signal_kill(int sig) {
    unsigned long now = clock();
    printf("\t\t\t游戏结束!\n"
           "\t\t\t最终分数: %d  等级: %d\n"
           "\t\t\t游戏时间: %f 秒\n",
           score, level, (now - start_time) / 1000.0 / 1000.0);
    restore_console();
    memset(map, 0, WIDTH * HIGHT * sizeof(int));
    exit(0);
}

int try_move_right() {
    memcpy(next_faller, faller, sizeof(faller));
    for (int i = 0; i < 4; i++) {
        next_faller[i].X++;
        if (next_faller[i].X >= WIDTH)
            return -1;
        if (map[next_faller[i].Y][next_faller[i].X] == BLOCK)
            return -1;
    }
    for (int i = 0; i < 4; i++)
        map[faller[i].Y][faller[i].X] = AIR;
    memcpy(faller, next_faller, sizeof(faller));
    for (int i = 0; i < 4; i++)
        map[faller[i].Y][faller[i].X] = MOVING;
    return 0;
}

void clear_row() {
    int flag;
    int rows_cleared = 0;
    for (int i = WIDTH - 1; i >= 3; i--) {
        flag = 1;
        for (int j = 0; j < HIGHT; j++) {
            if (map[j][i] != BLOCK) {
                flag = 0;
                break;
            }
        }
        if (flag) {
            rows_cleared++;
            for (int y = 0; y < HIGHT; y++) {
                for (int x = i; x > 0; x--) {
                    // 当前列等于前一列，即清行
                    map[y][x] = map[y][x - 1];
                }
            }
            i++;
        }
    }
    
    // 根据消除的行数增加分数
    if (rows_cleared > 0) {
        int points = 0;
        switch (rows_cleared) {
            case 1: points = 40 * level; break;
            case 2: points = 100 * level; break;
            case 3: points = 300 * level; break;
            case 4: points = 1200 * level; break;
        }
        score += points;
        
        // 每增加1000分升一级，速度加快
        if (score / 1000 + 1 > level) {
            level = score / 1000 + 1;
            tick_speed = 1000 * 1000 / level;
            if (tick_speed < 100 * 1000) tick_speed = 100 * 1000; // 限制最高速度
        }
        
        // 添加：更新分数显示
        update_score_display();
    }
}

// 向下输入1，向上输入-1
int try_move_vertical(int direction) {
    memcpy(next_faller, faller, sizeof(faller));
    for (int i = 0; i < 4; i++) {
        next_faller[i].Y += direction;
        if (next_faller[i].Y >= HIGHT || next_faller[i].Y < 0)
            return -1;
        if (map[next_faller[i].Y][next_faller[i].X] == BLOCK)
            return -1;
    }
    keyboard_flag = 1;
    for (int i = 0; i < 4; i++)
        map[faller[i].Y][faller[i].X] = AIR;
    memcpy(faller, next_faller, sizeof(faller));
    for (int i = 0; i < 4; i++)
        map[faller[i].Y][faller[i].X] = MOVING;
    return 0;
}

int try_goto_last_right() {
    int result = -1;
    while (try_move_right() == 0)
        result = 0;
    return result;
}

// 以中心点旋转
// 以所有点旋转后向下
// 以其他点旋转
int t_spin(int direction) {
    for (int j = 0; j < 4; j++) {
        next_faller[j].X =
            faller[0].X + direction * (faller[0].Y - faller[j].Y);
        next_faller[j].Y =
            faller[0].Y + direction * (faller[j].X - faller[0].X);
    }
    if (is_legal(next_faller)) {
        for (int i = 0; i < 4; i++)
            map[faller[i].Y][faller[i].X] = AIR;
        memcpy(faller, next_faller, sizeof(faller));
        for (int i = 0; i < 4; i++)
            map[faller[i].Y][faller[i].X] = MOVING;
        return 0;
    }

    for (int round = 1; round >= 0; round--) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                next_faller[j].X =
                    faller[i].X + direction * (faller[i].Y - faller[j].Y);
                next_faller[j].Y = faller[i].Y +
                                   direction * (faller[j].X - faller[i].X) +
                                   round;
            }
            if (is_legal(next_faller)) {
                for (int i = 0; i < 4; i++)
                    map[faller[i].Y][faller[i].X] = AIR;
                memcpy(faller, next_faller, sizeof(faller));
                for (int i = 0; i < 4; i++)
                    map[faller[i].Y][faller[i].X] = MOVING;
                return 0;
            }
        }
    }
    return -1;
}

// 以中心点旋转
// 以其他点旋转
// 检查以中心点旋转后能否向下移动一格
// 检查以其他点旋转后能否向下移动一格
int try_rotate(int direction) {
    if (now_shape == O_SHAPE)
        return -1;
    if (now_shape == T_SHAPE)
        return t_spin(direction);
    for (int round = 0; round <= 1; round++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                next_faller[j].X =
                    faller[i].X + direction * (faller[i].Y - faller[j].Y);
                next_faller[j].Y = faller[i].Y +
                                   direction * (faller[j].X - faller[i].X) +
                                   round;
            }
            if (is_legal(next_faller)) {
                for (int i = 0; i < 4; i++)
                    map[faller[i].Y][faller[i].X] = AIR;
                memcpy(faller, next_faller, sizeof(faller));
                for (int i = 0; i < 4; i++)
                    map[faller[i].Y][faller[i].X] = MOVING;
                return 0;
            }
        }
    }
    return -1;
}

// 新增：开始界面
void show_start_screen() {
    printf(ESC "2J"); // 清屏
    set_cursor_absolute_position(0, 0);
    printf("\n\n\n\n"
           "\t\t       俄罗斯方蟹\n\n"
           "\t\t  Crab + Tetris = Crabtris\n\n\n"
           "\t\t     按任意键开始游戏\n\n"
           "\t\t     按 Q 键退出游戏\n");
    
    // 等待按键
    fcntl(STDIN_FILENO, F_SETFL, old_fcntl); // 暂时恢复阻塞模式
    char key;
    read(STDIN_FILENO, &key, 1);
    fcntl(STDIN_FILENO, F_SETFL, old_fcntl | O_NONBLOCK); // 恢复非阻塞
    
    if (key == 'q' || key == 'Q') {
        restore_console();
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    // 信号处理
    signal(SIGABRT, signal_kill);
    signal(SIGINT, signal_kill);
    signal(SIGTERM, signal_kill);

    tcgetattr(STDIN_FILENO, &term);
    struct termios t = term;
    t.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    // 无阻塞
    old_fcntl = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, old_fcntl | O_NONBLOCK);

    // 无光标
    printf(ESC "?25l");
    // POSIX清屏
    printf(ESC "2J");
    // 获取控制台大小
    // ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
    set_cursor_absolute_position(0, 0);

    memset(map, 0, WIDTH * HIGHT * sizeof(int));
    srand(time(NULL));
    
    // 显示开始界面
    show_start_screen();
    
    // 在开始游戏前再次清屏
    printf(ESC "2J");
    set_cursor_absolute_position(0, 0);
    
    generate();
    // 游戏开始时显示完整界面
    print_full_map();

    unsigned long last = clock();
    start_time = last;
    char c;
    int fd = fileno(stdin);
    keyboard_flag = 0;
    while (1) {
        unsigned long now = clock();
        
        // 暂停状态下减少CPU使用
        if (paused) {
            usleep(10 * 1000);
            
            // 非阻塞读取输入 - 仅处理暂停/继续按键
            ssize_t bytesRead = read(fd, &c, sizeof(c));
            if (bytesRead > 0 && (c == 'p' || c == 'P')) {
                paused = 0;
                // 从暂停恢复时显示完整界面
                print_full_map();
            }
            continue;
        }
        
        // 说明该向下移动了
        if (keyboard_flag) {
            keyboard_flag = 0;
            last = clock() + COOLDOWN;
        }
        if (now > last) {
            last += tick_speed;
            if (try_move_right() == -1) {
                for (int i = 0; i < 4; i++)
                    map[faller[i].Y][faller[i].X] = BLOCK;
                clear_row();
                generate();
            }
            // 只更新游戏区域
            print_map();
        }
        
        // 非阻塞读取输入
        ssize_t bytesRead = read(fd, &c, sizeof(c));
        if (bytesRead > 0) {
            // 在这里添加其他按键的处理逻辑
            switch (c) {
            case 'R':
            case 'r':
                if (try_rotate(1) == 0)
                    keyboard_flag = 1;
                break;
            case 'H':
            case 'h':
                if (try_rotate(-1) == 0)
                    keyboard_flag = 1;
                break;
            case 'L':
            case 'l':
                if (try_move_right() == 0)
                    keyboard_flag = 1;
                break;
            case ' ':
                if (try_goto_last_right() == 0)
                    keyboard_flag = 1;
                break;
            case 'J':
            case 'j':
                if (try_move_vertical(1) == 0)
                    keyboard_flag = 1;
                break;
            case 'K':
            case 'k':
                if (try_move_vertical(-1) == 0)
                    keyboard_flag = 1;
                break;
            case 'P':
            case 'p':
                paused = !paused;
                if (paused) {
                    // 保存当前光标位置，移动到特定位置显示暂停信息，然后恢复位置
                    printf(ESC "s");  // 保存光标位置
                    set_cursor_absolute_position(0, HIGHT + 5);
                    printf("\t\t\t游戏已暂停 - 按P继续\n");
                    printf(ESC "u");  // 恢复光标位置
                } else {
                    // 从暂停恢复时重新显示完整界面
                    print_full_map();
                }
                keyboard_flag = 1;
                break;
            }
            // 按键后只更新游戏区域
            print_map();
        }
    }

    return EXIT_SUCCESS;
}
