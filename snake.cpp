#include <iostream>
#include <conio.h>
#include <windows.h>
#include <cstdlib> //rand（）随机数
#include <ctime>   //time()设置随机种子
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
int food_row, food_col;               // 食物的坐标
bool pause = false;                   // 暂停标记

void gotoxy(int x, int y) // 用于在控制台输出时设置光标位置，可以让光标跳转到指定位置开始输出字符
{
    COORD coord{x, y}; // COORD是windows.h中定义的一个结构体，用于表示控制台中的坐标
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    // GetStdHandle(STD_OUTPUT_HANDLE)获取标准输出句柄，SetConsoleCursorPosition设置光标位置，coord是要设置的位置
} // 可以防止光标在输出时乱跳，与cls相比，cls会刷新整个屏幕，而gotoxy只会刷新光标所在位置，所以gotoxy更高效
// 让每次的地图输出直接覆盖掉上一次的地图输出，而不会出现闪烁的现象，所以xy设置为0，0，让光标回到左上角

void initGrid() // 初始化地图且初始创建蛇
{
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
    srand((unsigned)time(NULL)); // 设置随机种子，食物位置随机
    initGrid();                  // 初始化地图
    spawnFood();                 // 随机生成食物

    // 隐藏控制台的光标，减少闪烁
    CONSOLE_CURSOR_INFO curInfo;
    curInfo.dwSize = 1;
    curInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);

    while (!gameOver)
    {
        draw();      // 绘制地图
        handInput(); // 处理键盘输入
        moveSnake(); // 移动蛇
        Sleep(100);  // 暂停100毫秒，控制游戏速度
    }
    freeSnake();   // 释放蛇的内存
    delete[] grid; // 释放地图的内存
    return 0;
}
