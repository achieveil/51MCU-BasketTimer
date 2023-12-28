#include <reg52.h>

sbit ADDR0 = P1 ^ 0;
sbit ADDR1 = P1 ^ 1;
sbit ADDR2 = P1 ^ 2;
sbit ADDR3 = P1 ^ 3;
sbit ENLED = P1 ^ 4;

sbit BUZZ = P1 ^ 6;

sbit KEY1 = P2 ^ 4;
sbit KEY2 = P2 ^ 5;
sbit KEY3 = P2 ^ 6;
sbit KEY4 = P2 ^ 7;

/**
 * @author achieveil,atnhaoyang,wensenyun
 * @date 2023/12/22
 */

unsigned char code LedChar[] = { // �������ʾ�ַ�ת����
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
    0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
unsigned char LedBuff[6]     = { // �������ʾ������
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char KeySta[4]      = { // ������ǰ״̬
    1, 1, 1, 1};

bit RunFLag      = 0;  // ������б�־
bit Refresh      = 1;  // ������ˢ�±�־
signed char Low  = 24; // ��2λ��ʾ24���ʱʣ��
signed char Mid  = 0;  // �м�2λ��ʾ��ʱʣ������
signed char High = 12; // 6λ����ܵĸ�2λ��ʾ��ʱʣ�������

void Load();
void KeyDriver();
void all();
void Reset();
void StartBuzz();

void main()
{
    EA    = 1; // �����ж�
    ENLED = 0; // ʹ��ѡ�������
    ADDR3 = 1;
    TMOD  = 0x01;
    TH0   = 0xF8; // T0��ʱ2ms
    TL0   = 0xCD;
    ET0   = 1;
    TR0   = 1;

    P2 = 0xF7; // P2.3��0��ѡ���1�а�����Ϊ��������

    while (1) {
        Load();
        KeyDriver(); // ���ð�����������
    }
}

// ������ܵ�ֵ���ص�������
void Load()
{
    // 24���ּ��ص���2λ
    LedBuff[0] = LedChar[Low % 10];
    LedBuff[1] = LedChar[Low / 10];
    // �벿�ּ��ص���2λ
    LedBuff[2] = LedChar[Mid % 10];
    LedBuff[3] = LedChar[Mid / 10];
    // �ֲ��ּ��ص���2λ
    LedBuff[4] = LedChar[High % 10];
    LedBuff[5] = LedChar[High / 10];

    LedBuff[2] &= 0x7F; // ����С����
}

// ���λ����
void Reset()
{
    Low = 24;
    Load(); // ȷ����ͣʱ���Խ�24��λ
}

// ������������ÿ��1s����һ�ν����������ۼ�
void Count()
{
    unsigned int i = 0;
    if (RunFLag) // ����������״̬ʱ��������ֵ
    {
        Low--;       // 24����-1
        if (Low < 0) // 24����Ϊ0
        {
            if (Mid >= 24 || High > 0)
                Low = 24;
            else if (Mid < 24)
                Low = 0;
        }
        if (Mid <= 0) // �м���λΪ0�����¼�ʱ������λ��1
        {
            Mid = 60;
            High--;
        }
        Mid--;
        if (High == 0 && Mid == 0) // ��ʱ������ȫ������
        {
            all();
        }
    }
}

// ����������������ⰴ������
void KeyDriver()
{
    unsigned char i;
    static unsigned char backup[4] = {1, 1, 1, 1};

    for (i = 0; i < 4; i++) // ѭ�����4������
    {
        if (backup[i] != KeySta[i]) // ��ⰴ������
        {
            if (backup[i] != 0) // ��������ʱִ�ж���
            {
                if (i == 1 && (Mid >= 24 || High > 0)) // Esc����λ���,ֻ���ڼ�ʱ����24��ʱ��Ч
                    Reset();
                else if (i == 0) // �س�����ͣ���
                    RunFLag = ~RunFLag;
            }
            backup[i] = KeySta[i]; // ˢ��ǰһ�εı���ֵ
        }
    }
}

// ����ɨ�躯��
void KeyScan()
{
    unsigned char i;
    static unsigned char keybuf[4] = {// ����ɨ�軺����
                                      0xFF, 0xFF, 0xFF, 0xFF};

    // ����ֵ���뻺����
    keybuf[0] = (keybuf[0] << 1) | KEY1;
    keybuf[1] = (keybuf[1] << 1) | KEY2;
    keybuf[2] = (keybuf[2] << 1) | KEY3;
    keybuf[3] = (keybuf[3] << 1) | KEY4;

    // ��������°���״̬
    for (i = 0; i < 2; i++) {
        if (keybuf[i] == 0x00) { // ����8��ɨ��ֵΪ0����16ms�ڶ��ǰ���״̬ʱ������Ϊ�������ȶ��İ���
            KeySta[i] = 0;
        } else if (keybuf[i] == 0xFF) { // ����8��ɨ��ֵΪ1����16ms�ڶ��ǵ���״̬ʱ������Ϊ�������ȶ��ĵ���
            KeySta[i] = 1;
        }
    }
}

// ����ܶ�̬ɨ��ˢ�º��������ڶ�ʱ�ж��е���
void LedScan()
{
    static unsigned char i = 0; // ��̬ɨ������

    P0 = 0xFF; // �ر����ж�ѡλ����ʾ����
    switch (i) {
        case 0:
            ADDR2 = 0;
            ADDR1 = 0;
            ADDR0 = 0;
            i++;
            P0 = LedBuff[0];
            break;
        case 1:
            ADDR2 = 0;
            ADDR1 = 0;
            ADDR0 = 1;
            i++;
            P0 = LedBuff[1];
            break;
        case 2:
            ADDR2 = 0;
            ADDR1 = 1;
            ADDR0 = 0;
            i++;
            P0 = LedBuff[2];
            break;
        case 3:
            ADDR2 = 0;
            ADDR1 = 1;
            ADDR0 = 1;
            i++;
            P0 = LedBuff[3];
            break;
        case 4:
            ADDR2 = 1;
            ADDR1 = 0;
            ADDR0 = 0;
            i++;
            P0 = LedBuff[4];
            break;
        case 5:
            ADDR2 = 1;
            ADDR1 = 0;
            ADDR0 = 1;
            i     = 0;
            P0    = LedBuff[5];
            break;
        default:
            break;
    }
}
void all()
{
    unsigned char x = 0;
    unsigned char i = 0;
    while (1) {
        P0 = 0xFF;
        switch (x) {
            // ���������
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
                P0 = LedChar[0];
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

            // ����С��
            case 6:
                ADDR3 = 1;
                ADDR2 = 1;
                ADDR1 = 1;
                ADDR0 = 0;
                x++;
                P0 = 0x00;
                break;

            // ��������
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

void StartBuzz()
{
    BUZZ = ~BUZZ;
}

// T0�жϷ��������������ܡ�����ɨ����������
void InterruptTimer0() interrupt 1
{
    static int count           = 0; // ������ѭ������
    static unsigned int tmr2ms = 0;

    TH0 = 0xF8; // ���¼�������ֵ
    TL0 = 0xCD;
    LedScan(); // �����ɨ����ʾ
    KeyScan(); // ����ɨ��
    if (Low <= 0) {
        if (count < 100)
            StartBuzz();
        if (Mid < 24 && High == 0)
            count++;
    }
    // ��ʱ2ms����һ��������
    tmr2ms++;
    if (tmr2ms >= 500) {
        tmr2ms = 0;
        Count(); // ��������������
    }
}