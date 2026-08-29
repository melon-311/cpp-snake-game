#include <iostream>
#include <conio.h>
#include <windows.h>
#include <cstdlib> //rand（）随机数
#include <ctime>   //time()设置随机种子
#include <cstdio>
using namespace std;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 获取控制台句柄，用于设置控制台颜色
void setColor(int color)                           // 设置控制台颜色
{
    SetConsoleTextAttribute(hConsole, color);
}
// 窗口尺寸
const int ROWS = 20;
const int COLS = 20;

// 移动方向
enum Direction
{ // enum定义枚举类型，代表方向，每次调用时，枚举类型会自动赋值
    STOP = 0,
    UP,
    DOWN,
    LEFT,
    RIGHT
}; // enum代表这是枚举，direction是枚举类型名，STOP=0代表第一个枚举值是0，UP=1代表第二个枚举值是1，以此类推，他会自动按顺序赋值，相当于
// Direction STOP=0,UP=1,DOWN=2,LEFT=3,RIGHT=4;

// 蛇的链表节点结构体，即用链表表示蛇
// 单向链表，每个节点包含蛇的坐标和指向下一个节点的指针
struct Node
{
    int row, col; // 蛇的坐标
    Node *next;   // 指向下一个节点的指针
    // 构造函数
    Node(int r, int c, Node *n = nullptr) : row(r), col(c), next(n) {} // 构造函数的初始化列表，初始化row，col，next
};

// 全局游戏数据：
int *grid;                            // 用一维数组表示二维数组，grid[i*COLS+j]表示第i行第j列的元素,用顺序表来表示游戏地图
Node *head;                           // 蛇的头部节点
Node *tail;                           // 蛇的尾部节点
int snake_length;                     // 蛇的初始长度
Direction dir = STOP, nextDir = STOP; // 蛇的当前移动方向,nextDir是下一次移动的方向
bool gameOver = false;                // 游戏是否结束
int score = 0;                        // 得分
int max_score = 0;                    // 最高分
int food_row, food_col;               // 食物的坐标
bool inMenu = true;                   // 是否在菜单界面
bool pause = false;                   // 暂停标记
void freeSnake();
void gotoxy(int x, int y) // 用于在控制台输出时设置光标位置，可以让光标跳转到指定位置开始输出字符
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    // GetStdHandle(STD_OUTPUT_HANDLE)获取标准输出句柄，SetConsoleCursorPosition设置光标位置，coord是要设置的位置
} // 可以防止光标在输出时乱跳，与cls相比，cls会刷新整个屏幕，而gotoxy只会刷新光标所在位置，所以gotoxy更高效
// 让每次的地图输出直接覆盖掉上一次的地图输出，而不会出现闪烁的现象，所以xy设置为0，0，让光标回到左上角

void initGrid() // 初始化地图且初始创建蛇
{
    if (grid != nullptr)
        delete[] grid;
    freeSnake();
    grid = new int[ROWS * COLS]{}; // 初始化地图，用一维数组表示二维数组，grid[i*COLS+j]表示第i行第j列的元素,{}表示初始化为0
                                   // 初始化蛇的三条身体
    head = new Node(5, 7);
    head->next = new Node(5, 6);
    head->next->next = new Node(5, 5);
    snake_length = 3;        // 初始化蛇的长度
    tail = head->next->next; // 尾指针固定最后一个节点

    Node *cur = head;
    while (cur)
    {                                         // 遍历蛇的每个节点，将蛇的坐标在地图上标记为1
        grid[cur->row * COLS + cur->col] = 1; // grid[i*COLS+j]表示第i行第j列的元素
        cur = cur->next;
    }
    gameOver = false;
    score = 0;
    pause = false;
}

void spawnFood() // 随机生成食物
{
    int total = ROWS * COLS;
    int free = total - snake_length;
    if (free <= 0)
    {
        gameOver = true;
        return;
    }
    int target = rand() % free; // 随机生成一个0到free-1的整数,即生成一个是空地的值，用于放置食物
    for (int i = 0; i < total; i++)
    {
        if (grid[i] == 0)
        {
            if (target == 0)
            {
                food_row = i / COLS;
                food_col = i % COLS;
                grid[i] = 2; // 将食物的坐标在地图上标记为2
                return;
            }
            target--;
        }
    }
}

void freeSnake()
{ // 释放蛇的内存，避免内存浪费
    Node *cur = head;
    while (cur)
    {
        Node *tmp = cur; // 用临时指针保存当前节点
        cur = cur->next; // 移动到下一个节点
        delete tmp;      // 释放当前节点
    }
    head = tail = nullptr;
    snake_length = 0;
}

void loadMaxScore()
{ // 加载最高分
    FILE *file = fopen("max_score.txt", "r");
    if (file != nullptr)
    {
        fscanf(file, "%d", &max_score); // 从文件中读取最高分，从file指针指向的文件中读取一个整数，并将其存储在max_score变量中
        fclose(file);
    }
    else
    {
        max_score = 0;
    }
}

void saveMaxScore()
{ // 保存最高分
    if (score > max_score)
    {
        max_score = score;
        FILE *file = fopen("max_score.txt", "w");
        if (file != nullptr)
        {
            fprintf(file, "%d", max_score); // 将最高分写入文件中
            fclose(file);
        }
        setColor(12);
        cout << "\n!!!恭喜创造历史最高分!!!" << endl;
        setColor(7);
    }
}

void moveSnake() // 蛇移动核心逻辑
{
    if (gameOver)
        return; // 如果游戏结束，则直接返回,不需要再移动
    if (pause)
        return; // 如果游戏暂停，则直接返回,不需要再移动
    // 1.禁止蛇反向掉头
    bool can_change = true;
    if ((dir == UP && nextDir == DOWN) || (dir == DOWN && nextDir == UP) || (dir == LEFT && nextDir == RIGHT) || (dir == RIGHT && nextDir == LEFT))
    {
        can_change = false; // 如果蛇当前方向和下一次方向相反，则不能改变方向(防止自杀)
    }
    if (can_change)
        dir = nextDir; // 如果可以改变方向，则将当前方向设置为下一次方向

    // 2.计算新蛇头坐标
    int new_row = head->row;
    int new_col = head->col;
    switch (dir)
    {
    case UP:
        new_row--;
        break;
    case DOWN:
        new_row++;
        break;
    case LEFT:
        new_col--;
        break;
    case RIGHT:
        new_col++;
        break;
    default:
        return;
    }

    // 3.撞墙判断
    if (new_row < 0 || new_row >= ROWS || new_col < 0 || new_col >= COLS)
    {
        gameOver = true;
        return;
    }
    int idx = new_row * COLS + new_col;
    // 4.判断是否撞到身体
    if (grid[idx] == 1)
    {
        gameOver = true;
        return;
    }

    // 5.判断是否吃到食物
    bool eat = (grid[idx] == 2);
    Node *newHead = new Node(new_row, new_col, head); // 新建头节点，指向原来的头节点（即头插法）
    head = newHead;
    grid[idx] = 1;

    if (eat)
    {
        snake_length++;
        score += 10;
        spawnFood();

        if (gameOver)
        {
            return;
        }
    }
    else
    {
        // 没吃到，就新建头然后删尾巴，模拟正常前进一格
        Node *cur = head;
        while (cur->next != tail)
            cur = cur->next;                    // cur记录tail的前一个节点
        grid[tail->row * COLS + tail->col] = 0; // 将尾巴的坐标在地图上标记为0，即空地
        delete tail;
        tail = cur;
        tail->next = nullptr;
    }
}

void showMenu()
{ // 显示菜单界面

    system("cls");
    setColor(14);
    cout << " ===============================================\n";
    cout << "||               贪吃蛇小游戏                  ||\n";
    cout << "||                                             ||\n";
    cout << "||                1. 开始游戏                  ||\n";
    cout << "||                2. 退出游戏                  ||\n";
    cout << " ===============================================\n";
    setColor(7);
    cout << "请输入选项（1或2）：";
}

void showGameOver()
{
    system("cls");
    setColor(12);
    cout << "================================================\n";
    cout << "                 游戏结束                    \n";
    cout << "                                             \n";
    cout << "                本局得分：" << score << "                  \n";
    cout << "                蛇身长度：" << snake_length << "               \n";
    cout << "                最高分数：" << max_score << "                \n";
    cout << "================================================\n";
    if (score > max_score)
    {
        setColor(12);
        cout << "恭喜你创造了新的最高分！" << endl;
    }
    setColor(7);
    cout << "   按Y重新对局，或按X退出游戏" << endl;
}
void draw()
{
    gotoxy(0, 0);
    // 上边框
    setColor(15);
    cout << "+";
    for (int i = 0; i < COLS; i++)
        cout << "--";
    cout << "+\n";

    // 地图每行
    for (int i = 0; i < ROWS; i++)
    {
        setColor(15);
        cout << "|";
        for (int j = 0; j < COLS; j++)
        {
            int val = grid[i * COLS + j];
            if (val == 0) // 即空地
                cout << "  ";
            else if (val == 1) // 即蛇身
            {
                setColor(10), cout << "■ ";
            }
            else
            {
                setColor(14), cout << "★ ";
            } // 即食物
        }
        setColor(15);
        cout << "|\n";
    }

    // 下边框
    setColor(15);
    cout << "+";
    for (int i = 0; i < COLS; i++)
        cout << "--";
    cout << "+\n";

    setColor(12);
    cout << "分数：" << score << " 长度：" << snake_length << " ";
    cout << "最高分：" << max_score << " " << endl;
    if (gameOver)
        cout << "游戏结束，请关闭窗口\n";
    else if (pause)
        cout << "=====游戏暂停，按空格键继续=====\n";
    else
        cout << "按WASD键控制方向，按空格键暂停/继续，按X键退出\n";
    setColor(7);
}

void handInput()
{
    if (_kbhit()) // 判断是否有键盘输入
    {
        int key = _getch(); // 获取键盘输入
        switch (key)        // 判断输入的键
        {
        case 'w':
        case 'W':
            nextDir = UP;
            break;
        case 's':
        case 'S':
            nextDir = DOWN;
            break;
        case 'a':
        case 'A':
            nextDir = LEFT;
            break;
        case 'd':
        case 'D':
            nextDir = RIGHT;
            break; // 按WASD键改变方向
        case 'x':
        case 'X':
            gameOver = true;
            break; // 按x键退出游戏
        case ' ':
            pause = !pause; // 按空格键暂停/继续游戏
            break;
        }
    }
}

int main()
{
    srand((unsigned int)time(nullptr)); // 设置随机种子，使用当前时间作为种子，保证每次运行程序时生成的随机数不同
    loadMaxScore();                     // 加载最高分
    while (true)                        // 外层循环，控制整个游戏
    {
        showMenu();             // 显示菜单界面
        char choice = _getch(); // 获取用户输入的选项
        if (choice == '2')
        {
            cout << "退出游戏，欢迎下次再来！" << endl;
            return 0; // 退出游戏
        }
        if (choice != '1')
        {
            continue; // 如果输入的选项不是1或2，则重新显示菜单界面
        }
        system("cls");                  // 清屏
        initGrid();                     // 初始化地图和蛇
        spawnFood();                    // 生成食物
        CONSOLE_CURSOR_INFO cursorInfo; // 设置光标不可见
        cursorInfo.bVisible = false;
        cursorInfo.dwSize = 1;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
        while (!gameOver) // 内层循环，控制游戏进行
        {
            draw();      // 绘制地图
            handInput(); // 处理用户输入
            moveSnake(); // 移动蛇
            Sleep(100);  // 延时，控制游戏速度
        }
        cursorInfo.bVisible = true; // 设置光标可见
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
        saveMaxScore(); // 保存最高分
        showGameOver(); // 显示游戏结束界面
        while (true)    // 等待用户输入重新开始或退出
        {
            char choice = _getch();
            if (choice == 'y' || choice == 'Y')
            {
                freeSnake(); // 释放蛇的内存
                break;       // 重新开始游戏
            }
            else if (choice == 'x' || choice == 'X')
            {
                freeSnake();    // 释放蛇的内存
                delete[] grid;  // 释放地图的内存
                grid = nullptr; // 将地图指针置为空，避免悬空指针
                cout << "退出游戏，欢迎下次再来！" << endl;
                return 0; // 退出游戏
            }
        }
    }
    return 0;
}
