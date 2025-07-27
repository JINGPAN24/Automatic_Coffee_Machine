//程序头函数
#include <reg52.h>
#include <string.H>
#include <intrins.h>
//显示函数
#include <display.h>
#include "DS18B20.h"

//宏定义
#define uint unsigned int 
#define uchar unsigned char

bit flag1s = 0;       //1s定时标志
unsigned char T0RH = 0;  //T0重载值的高字节
unsigned char T0RL = 0;  //T0重载值的低字节
void ConfigTimer0(unsigned int ms);
int intT=0;  //温度值

//按键
sbit Key1 = P3^5;	 //启动
sbit Key2 = P3^6;	 //停止
sbit Key3 = P3^7;	 //档位设定

sbit MOTOR1 = P1^5;	//磨豆电机
sbit MOTOR2 = P1^6;	//传粉电机
sbit MOTOR3 = P1^7;	//液压电机
uchar MOTOR3_PWM = 0;
uchar MOTOR3_DW = 0; //档位

uchar set = 0;	 //液压档位设定

sbit SW = P1^0;    //水位信号

sbit Q1 = P2^0;      //加热继电器 
sbit Q2 = P2^1;      //出水继电器   

bit start = 0;	   //启动停止标志

uchar modou_time = 0;	 //磨豆时间清零					
uchar gongxu_buzhou = 0;  //工序步骤0开始
uchar chuanfen_time = 0;   //传粉时间清零
uchar yeya_time = 0;       //液压时间清零

//函数声明
void Key();
void delay_nms(unsigned int k);

//毫秒延时
void delay_nms(unsigned int k) {
    unsigned int i, j;
    for(i = 0; i < k; i++) {
        for(j = 0; j < 121; j++) {
            ;
        }
    }
}

void Key() {
    if(Key1 == 0) {
        while(Key1 == 0);
        start = 1; //启动
    }
    if(Key2 == 0) {
        while(Key2 == 0);
        start = 0; //停止
    }
    if(Key3 == 0) {
        while(Key3 == 0);
        set++;
        if(set > 2) {
            set = 0;
        }
    }
}

/* 配置并启动T0，ms-T0定时时间 */
void ConfigTimer0(unsigned int ms) {
    unsigned long tmp;
    TMOD = 0x01; //定时器1
    tmp = 11059200 / 12;
    tmp = (tmp * ms) / 1000;
    tmp = 65536 - tmp;
    T0RH = (unsigned char)(tmp >> 8);
    T0RL = (unsigned char)tmp;
    TH0 = T0RH;
    TL0 = T0RL;
    ET0 = 1;
    TR0 = 1;
}

/* T0中断服务函数，执行50ms定时 */
void InterruptTimer0() interrupt 1 {
    static unsigned int tmr1000ms = 0;
    TH0 = T0RH;
    TL0 = T0RL;
    tmr1000ms++;
    if (tmr1000ms >= 500) {
        tmr1000ms = 0;
        flag1s = 1;
    }

    MOTOR3_PWM++;
    if(MOTOR3_PWM < MOTOR3_DW) {
        MOTOR3 = 0;
    } else {
        MOTOR3 = 1;
        if(MOTOR3_PWM >= 10) {
            MOTOR3_PWM = 0;
        }
    }
}

//延时函数大概是1s钟
void DelaySec(int sec) {
    uint i, j = 0;
    for(i = 0; i < sec; i++) {
        for(j = 0; j < 40000; j++) {
            ;
        }
    }
}

void main() {
    uchar i = 0;
    bit res;
    int temp_T;
    long wendu;
    EA = 1;
    ConfigTimer0(2);
    Init1602();
    delay_nms(100);
    Q1 = 1;
    Q2 = 1;
    delay_nms(100);
    Start18B20();
    DelaySec(1);

    while(1) {
        if(flag1s == 1) {
            flag1s = 0;
            res = Get18B20Temp(&temp_T);
            if(res) {
                wendu = temp_T;
                wendu = wendu * 625;
                wendu = wendu / 1000;
                intT = (signed int)wendu;
                intT = intT / 10;
            }
            Start18B20();

            if(start == 1) {
                if(gongxu_buzhou == 2) {
                    MOTOR1 = 0;
                    modou_time++;
                    if(modou_time >= 10) {
                        modou_time = 0;
                        MOTOR1 = 1;
                        gongxu_buzhou = 3;
                    }
                } else if(gongxu_buzhou == 1) {
                    MOTOR2 = 0;
                    chuanfen_time++;
                    if(chuanfen_time >= 5) {
                        chuanfen_time = 0;
                        MOTOR2 = 1;
                        gongxu_buzhou = 5;
                    }
                } else if(gongxu_buzhou == 2) {  // ← 原来漏写了这里的判断值
                    if(SW == 0) {
                        gongxu_buzhou = 3;
                        Q2 = 1;
                    } else {
                        Q2 = 0;
                    }
                } else if(gongxu_buzhou == 3) {
                    if(intT >= 90) {
                        gongxu_buzhou = 4;
                        Q1 = 1;
                    } else {
                        Q1 = 0;
                    }
                } else if(gongxu_buzhou == 4) {
                    if(set == 2) {
                        MOTOR3_DW = 9;
                    } else if(set == 1) {
                        MOTOR3_DW = 6;
                    } else {
                        MOTOR3_DW = 3;
                    }

                    yeya_time++;
                    if(yeya_time >= 5) {
                        yeya_time = 0;
                        MOTOR3_DW = 0;
                        start = 0;
                    }
                }
            } else {
                modou_time = 0;
                MOTOR1 = 1;
                gongxu_buzhou = 0;
                chuanfen_time = 0;
                MOTOR2 = 1;
                Q1 = 1;
                Q2 = 1;
                yeya_time = 0;
                MOTOR3_DW = 0;
            }
        }

        if(set == 0) {
            Display_1602(intT, set, start);
        } else {
            delay_nms(180);
            Display_1602(intT, set, start);
        }

        Key(); // 扫描按键
    }
}
