#include <reg52.h> //包含特殊功能寄存器定义的头文件

sbit ADDR0 = P1 ^ 0; // 位地址声明 38译码器U3控制端口
sbit ADDR1 = P1 ^ 1;
sbit ADDR2 = P1 ^ 2;
sbit ADDR3 = P1 ^ 3;
sbit ENLED = P1 ^ 4; // 38译码器使能控制

sbit BUZZ = P1 ^ 6; // 蜂鸣器端口

sbit KEY1 = P2 ^ 4; // 矩阵按键第一列
sbit KEY2 = P2 ^ 5; // 矩阵按键第二列
sbit KEY3 = P2 ^ 6;
sbit KEY4 = P2 ^ 7;

/**
 * @author achieveil,atnhaoyang,wensenyun
 * @date 2023/12/22
 */

unsigned char code LedChar[] = { // 数码管显示字符转换表，从0到f
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
    0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};

unsigned char LedBuff[6] = { // 数码管显示缓冲区
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char KeySta[4]  = { // 按键当前状态
    1, 1, 1, 1};

bit RunStopWatchFlag = 0; // 秒表运行标志
bit RunLowFlag       = 0; // 低二位运行标志

signed char Low  = 24; // 低2位显示24秒计时剩余
signed char Mid  = 0;  // 中间2位显示计时剩余秒数
signed char High = 12; // 6位数码管的高2位显示计时剩余分钟数
void RefreshOnce();
void Warning();
void Load();
void StartBuzz();
void DelayHalfSecond();
void StopWatchAction();
void StopWatchCount();
void StopWatchDisplay();
void StopWatchReset();
void LowCount();
void LowReset();

// 将数码管的值加载到缓冲区
void Load()
{
    // 24部分加载到低2位
    LedBuff[0] = LedChar[Low % 10];
    LedBuff[1] = LedChar[Low / 10];
    // 秒部分加载到中2位
    LedBuff[2] = LedChar[Mid % 10];
    LedBuff[3] = LedChar[Mid / 10];
    // 分部分加载到高2位
    LedBuff[4] = LedChar[High % 10];
    LedBuff[5] = LedChar[High / 10];
}
// 刷新
void RefreshOnce()
{
    Load();
    StopWatchDisplay();
}

// Low复位
void LowReset()
{
    Low = 24;
    RefreshOnce();
}
// 秒表复位
void StopWatchReset()
{
    RunStopWatchFlag = 0;
    Low              = 24;
    Mid              = 0;
    High             = 12;
    RefreshOnce();
}

// 秒表计数函数，每隔1s调用一次进行秒表计数累加
void StopWatchCount()
{

    Mid--;
    if (Mid <= 0) // 中间两位为0，重新计时，高两位减1
    {
        Mid = 60;
        High--;
    }
    if (High == 0 && Mid == 0) // 计时结束，全部点亮
    {
        RunStopWatchFlag = 0;
        RunLowFlag       = 0;
        Warning(); // 报警
    }
}
// Low计时
void LowCount()
{
    if (Low != 0) {
        Low--;
    } else if (!(High == 0 && Mid < 24)) {
        Low = 24;
        StartBuzz();
        DelayHalfSecond();
        BUZZ = 1; // 关闭蜂鸣器
    }// 当 Low 变为 0 时，启动蜂鸣器响0.5秒
   
}

// 按键驱动函数，检测按键动作
void KeyDriver()
{
    unsigned char i;
    static unsigned char backup[4] = {1, 1, 1, 1};

    for (i = 0; i < 4; i++) // 循环检测4个按键
    {
        if (backup[i] != KeySta[i]) // 检测按键动作
        {
            if (backup[i] != 0) // 按键按下时执行动作
            {
                if (i == 0) { // K1键启停秒表
                    StopWatchAction();
                } else if (i == 1 && (Mid >= 24 || High > 0)) // Esc键复位秒表,只有在计时大于24秒时有效
                    LowReset();
                else if (i == 2) {
                    StopWatchReset();
                }
            }
            backup[i] = KeySta[i]; // 刷新前一次的备份值
        }
    }
}

// 按键扫描函数
void KeyScan()
{
    unsigned char i;
    static unsigned char keybuf[4] = {// 按键扫描缓冲区
                                      0xFF, 0xFF, 0xFF, 0xFF};

    // 按键值移入缓冲区
    keybuf[0] = (keybuf[0] << 1) | KEY1;
    keybuf[1] = (keybuf[1] << 1) | KEY2;
    keybuf[2] = (keybuf[2] << 1) | KEY3;
    keybuf[3] = (keybuf[3] << 1) | KEY4;

    // 消抖后更新按键状态
    for (i = 0; i < 4; i++) {
        if (keybuf[i] == 0x00) { // 连续8次扫描值为0，即16ms内都是按下状态时，可认为按键已稳定的按下
            KeySta[i] = 0;
        } else if (keybuf[i] == 0xFF) { // 连续8次扫描值为1，即16ms内都是弹起状态时，可认为按键已稳定的弹起
            KeySta[i] = 1;
        }
    }
}

// 数码管动态扫描刷新函数，需在定时中断中调用
void StopWatchDisplay()
{
    static unsigned char i = 0; // 动态扫描索引

    P0 = 0xFF;
    P1 = (P1 & 0xF8) | i;
    if (i == 2) {
        P0 = LedBuff[i] & 0x7F;
    } else {
        P0 = LedBuff[i];
    }
    if (i < 5) {
        i++;
    } else {
        i = 0;
    }
}
// 报警
void Warning()
{
    unsigned char x = 0;
    unsigned char i = 0;
    while (1) {
        P0 = 0xFF;
        switch (x) {
            // 点亮数码管
            case 0:
                ADDR3 = 1;
                ADDR2 = 0;
                ADDR1 = 0;
                ADDR0 = 0;
                x++;
                P0 = LedChar[0];
                break;
            case 1:
                ADDR3 = 1;
                ADDR2 = 0;
                ADDR1 = 0;
                ADDR0 = 1;
                x++;
                P0 = LedChar[0];
                break;
            case 2:
                ADDR3 = 1;
                ADDR2 = 0;
                ADDR1 = 1;
                ADDR0 = 0;
                x++;
                P0 = LedChar[0] & 0x7F; // 点亮小数点;
                break;
            case 3:
                ADDR3 = 1;
                ADDR2 = 0;
                ADDR1 = 1;
                ADDR0 = 1;
                x++;
                P0 = LedChar[0];
                break;
            case 4:
                ADDR3 = 1;
                ADDR2 = 1;
                ADDR1 = 0;
                ADDR0 = 0;
                x++;
                P0 = LedChar[0];
                break;
            case 5:
                ADDR3 = 1;
                ADDR2 = 1;
                ADDR1 = 0;
                ADDR0 = 1;
                x++;
                P0 = LedChar[0];
                break;

            // 点亮小灯
            case 6:
                ADDR3 = 1;
                ADDR2 = 1;
                ADDR1 = 1;
                ADDR0 = 0;
                x++;
                P0 = 0x00;
                break;

            // 点亮点阵
            case 7:
                ADDR3 = 0;
                ADDR2 = 0;
                ADDR1 = 0;
                ADDR0 = 0;
                x++;
                P0 = 0x00;
                break;
            case 8:
                ADDR3 = 0;
                ADDR2 = 0;
                ADDR1 = 0;
                ADDR0 = 1;
                x++;
                P0 = 0x00;
                break;
            case 9:
                ADDR3 = 0;
                ADDR2 = 0;
                ADDR1 = 1;
                ADDR0 = 0;
                x++;
                P0 = 0x00;
                break;
            case 10:
                ADDR3 = 0;
                ADDR2 = 0;
                ADDR1 = 1;
                ADDR0 = 1;
                x++;
                P0 = 0x00;
                break;
            case 11:
                ADDR3 = 0;
                ADDR2 = 1;
                ADDR1 = 0;
                ADDR0 = 0;
                x++;
                P0 = 0x00;
                break;
            case 12:
                ADDR3 = 0;
                ADDR2 = 1;
                ADDR1 = 0;
                ADDR0 = 1;
                x++;
                P0 = 0x00;
                break;
            case 13:
                ADDR3 = 0;
                ADDR2 = 1;
                ADDR1 = 1;
                ADDR0 = 0;
                x++;
                P0 = 0x00;
                break;
            case 14:
                ADDR3 = 0;
                ADDR2 = 1;
                ADDR1 = 1;
                ADDR0 = 1;
                x     = 0;
                P0    = 0x00;
                break;
            default:
                break;
        }
        for (i = 0; i <= 200; i++)
            ;
        StartBuzz();
    }
}
// 蜂鸣器
void StartBuzz()
{
    BUZZ = ~BUZZ;
}
void DelayHalfSecond()
{
    unsigned int delayCount;
    
}

// 秒表启停函数
void StopWatchAction()
{
    if (RunStopWatchFlag) {
        RunStopWatchFlag = 0;
    } else {
        RunStopWatchFlag = 1;
    }
}
// T0中断服务函数，完成数码管、按键扫描与秒表计数
void InterruptTimer0() interrupt 1
{
    //static int count = 0; // 蜂鸣器循环计数

    static unsigned int tmr2ms = 0;

    TH0 = 0xF8; // 重新加载初值
    TL0 = 0xCD;
    KeyScan(); // 按键扫描
    
    // 定时2ms进行一次秒表计数
    tmr2ms++;
    if (tmr2ms >= 500) {
        tmr2ms = 0;
        if (RunStopWatchFlag) {
            StopWatchCount();
            LowCount();
        }
    }
    StopWatchDisplay(); // 数码管扫描显示
}

void main()
{
    EA    = 1; // 开总中断
    ENLED = 0; // 使控制数码管的38译码器使能
    ADDR3 = 1;
    TMOD  = 0x01;
    TH0   = 0xF8; // T0定时2ms
    TL0   = 0xCD;
    ET0   = 1;    // 使能t0中断
    TR0   = 1;    // 使能t0
    P2    = 0xF7; // P2.3置0，即KeyOut1输出低电平，KeyIn1-4置1，开启检测
    while (1) {
        Load();
        KeyDriver(); // 调用按键驱动函数
    }
}