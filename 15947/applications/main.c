/* 完整、正确的 main.c 文件 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h" // 保留，因为INIT_BOARD_EXPORT等宏可能在这里面
#include "stm32h7xx.h"   // 保留，因为SCB和QSPI_BASE的定义在这里面
/* ADC应用函数声明 */
extern int adc_start(void);
extern void adc_pv_integration_init(void);

/* UART1应用函数声明 */
#include "uart1_app.h"


/* * ======================================================================
 * 第一部分：必须保留的板级初始化代码 (The Lifeline)
 * 这是让ART-Pi从外部Flash正常启动的基础，绝对不能删除！
 * ======================================================================
 */
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


/* * ======================================================================
 * 第二部分：ADC数据采集应用
 * (ADC Data Acquisition Application)
 * ======================================================================
 */


/*
 * main函数：ADC数据采集应用主函数
 */
int main(void)
{
    rt_kprintf("=== RT-Thread ADC应用 ===\n");
    rt_kprintf("📡 UART1 has been released for air724ug module use.\n");
    rt_kprintf("🔧 ADC functionality uses UART4 (console) and works independently.\n");

    /* 等待系统稳定 */
    rt_thread_mdelay(500); // 减少等待时间

    /* 初始化UART1 (115200波特率) - 已禁用，释放给air724ug使用 */
    // uart1_init_default();  // 注释掉，释放UART1给air724ug模块

    /* 初始化ADC与PV诊断模块的集成 */
    adc_pv_integration_init();

    /* 启动ADC应用 (但电压检测循环初始为关闭状态) */
    adc_start();

    rt_kprintf("✅ ADC system initialized. Use 'Enable_Voltage_Detection' to start monitoring.\n");
    rt_kprintf("📋 Quick commands: adc_quick_test, test_pa6_pa7_channels, help_adc\n");
    rt_kprintf("🎯 Type 'clear_welcome' for a clean start!\n");

    /* main函数完成初始化后返回，ADC采集线程将继续运行 */
    return RT_EOK;
}

