#include <hal_assert.h>
#include <hal_board.h>
#include <hal_int.h>
#include <hal_joystick.h>
#include <hal_lcd.h>
#include <hal_led.h>
#include "basic_rf.h"
#include "hal_button.h"
#include "hal_mcu.h"
#include "hal_rf.h"
#include "util_lcd.h"

#define RF_CHANNEL 25  // 2.4 GHz RF channel

#define PAN_ID 0x2007
#define SWITCH_ADDR 0x2520
#define LIGHT_ADDR 0xBEEF
#define APP_PAYLOAD_LENGTH 1
#define LIGHT_TOGGLE_CMD 0

#define APP_PAYLOAD_LENGTH 1

#define SWITCH 1
#define LIGHT 2

static uint8 pTxData[APP_PAYLOAD_LENGTH];
static uint8 pRxData[APP_PAYLOAD_LENGTH];
static basicRfCfg_t basicRfConfig;

static void appSwitch() {
    pTxData[0] = LIGHT_TOGGLE_CMD;
    // Initialize BasicRF
    basicRfConfig.myAddr = SWITCH_ADDR;
    if (basicRfInit(&basicRfConfig) == FAILED) {
        HAL_ASSERT(FALSE);
    }
    // 由于模块只需要发射，所以把接收屏蔽掉以降低功耗。
    basicRfReceiveOff();
    while (TRUE) {
        // TI Joystick使用IO口我们用作按键1，所以要修改一下 if(
        // halJoystickPushed() )
        if (halButtonPushed() == HAL_BUTTON_1) {
            //函数功能：给目的短地址发送指定长度的数据，发送成功刚返回SUCCESS，失败则返回FAILED
            // LIGHT_ADDR目的短地址;pTxData 指向发送缓冲区的指针;
            // APP_PAYLOAD_LENGTH发送数据长度
            basicRfSendPacket(LIGHT_ADDR, pTxData, APP_PAYLOAD_LENGTH);

            // Put MCU to sleep. It will wake up on joystick interrupt
            halIntOff();
            halMcuSetLowPowerMode(HAL_MCU_LPM_3);  // Will turn on global
            // interrupt enable
            halIntOn();
        }
    }
}

static void appLight() {
    // Initialize BasicRF
    basicRfConfig.myAddr = LIGHT_ADDR;
    if (basicRfInit(&basicRfConfig) == FAILED) {
        HAL_ASSERT(FALSE);
    }

    basicRfReceiveOn();

    // Main loop
    while (TRUE) {
        while (!basicRfPacketIsReady())
            ;  //检查模块是否已经可以接收下一个数据，如果准备好刚返回 TRUE

        //把收到的数据复制到buffer中
        if (basicRfReceive(pRxData, APP_PAYLOAD_LENGTH, NULL) > 0) {
            if (pRxData[0] ==
                LIGHT_TOGGLE_CMD) {  //判断接收到的数据是否就是LIGHT_TOGGLE_CMD
                halLedToggle(1);  //改变Led1的状态
            }
        }
    }
}

void main() {
    uint8 appMode = LIGHT;  // appMode取值:NONE、SWITCH、LIGHT
                             // appMode等于SWITCH为发射模块 按键S1对应P0_1
                             // appMode等于LIGHT为接收模块  LED1对应P1_0
    // Config basicRF
    basicRfConfig.panId = PAN_ID;        //设置节点PAN ID
    basicRfConfig.channel = RF_CHANNEL;  //设置节点RF通道
    basicRfConfig.ackRequest = TRUE;     //目标确认
#ifdef SECURITY_CCM  //是否加密，预定义里取消了加密
    basicRfConfig.securityKey = key;
#endif

    // Initalise board peripherals 始化外围设备
    halBoardInit();
    // halJoystickInit();

    // Initalise hal_rf 硬件抽象层的rf进行初始化初
    if (halRfInit() == FAILED) {
        HAL_ASSERT(FALSE);
    }

    halLedSet(1);  //关LED1 由于我们开发板是低电平点亮的，与TI不同
                   //更符合中国人的习惯

    if (appMode == SWITCH) {
        // No return from here
        appSwitch();  //发射模块 按键S1对应P0_1
    } else if (appMode == LIGHT) {
        // No return from here
        appLight();  //接收模块 LED1对应P1_0
    }
}