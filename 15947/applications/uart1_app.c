/*
 * uart1_app.c
 *
 * UART1应用模块 - 专用于数据输出
 * 波特率: 9600
 * 引脚: PA9(TX), PA10(RX)
 * 用途: ADC数据输出、PV诊断结果输出
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "uart1_app.h"

#define UART_DEVICE_NAME "uart1"    // 要操作的串口设备名称
#define UART1_BAUD_RATE  115200     // 固定波特率115200

static rt_device_t serial_dev;      // 全局的串口设备句柄
static rt_bool_t uart1_initialized = RT_FALSE;

/**
 * @brief 初始化并配置 uart1
 * @param baud_rate 波特率 (默认使用9600)
 * @return rt_err_t RT_EOK 表示成功，其他为失败
 */
rt_err_t uart1_init(rt_uint32_t baud_rate)
{
    rt_err_t res = RT_EOK;

    // 如果已经初始化，直接返回
    if (uart1_initialized) {
        rt_kprintf("UART1 already initialized\n");
        return RT_EOK;
    }

    /* 1. 查找串口设备 */
    serial_dev = rt_device_find(UART_DEVICE_NAME);
    if (!serial_dev)
    {
        rt_kprintf("Error: find %s failed! Please check RT-Thread Settings.\n", UART_DEVICE_NAME);
        return RT_ERROR;
    }

    /* 2. 定义串口配置结构体，并填入参数 */
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  // 使用默认配置
    config.baud_rate = baud_rate;       // 设置波特率
    config.data_bits = DATA_BITS_8;     // 数据位 8
    config.stop_bits = STOP_BITS_1;     // 停止位 1
    config.parity    = PARITY_NONE;     // 无校验位
    config.bufsz     = 512;             // 接收缓冲区大小

    /* 3. 应用配置参数 */
    res = rt_device_control(serial_dev, RT_DEVICE_CTRL_CONFIG, &config);
    if (res != RT_EOK)
    {
        rt_kprintf("Error: control %s failed! (error code: %d)\n", UART_DEVICE_NAME, res);
        return res;
    }

    /* 4. 打开串口设备 */
    // 使用中断接收方式打开 (未开启DMA)
    res = rt_device_open(serial_dev, RT_DEVICE_FLAG_INT_RX);
    if (res != RT_EOK)
    {
        rt_kprintf("Error: open %s failed! (error code: %d)\n", UART_DEVICE_NAME, res);
        return res;
    }

    uart1_initialized = RT_TRUE;
    rt_kprintf("✅ UART1 initialized successfully:\n");
    rt_kprintf("   Device: %s\n", UART_DEVICE_NAME);
    rt_kprintf("   Baud Rate: %d\n", baud_rate);
    rt_kprintf("   Pins: PA9(TX), PA10(RX)\n");
    rt_kprintf("   Mode: Interrupt RX (No DMA)\n");
    
    return RT_EOK;
}

/**
 * @brief 使用默认波特率9600初始化UART1
 * @return rt_err_t RT_EOK 表示成功，其他为失败
 */
rt_err_t uart1_init_default(void)
{
    return uart1_init(UART1_BAUD_RATE);
}

/**
 * @brief 向UART1发送数据
 * @param data 要发送的数据指针
 * @param size 数据长度
 * @return rt_size_t 实际发送的字节数
 */
rt_size_t uart1_send(const void *data, rt_size_t size)
{
    if (!uart1_initialized || !serial_dev) {
        rt_kprintf("Error: UART1 not initialized\n");
        return 0;
    }
    
    return rt_device_write(serial_dev, 0, data, size);
}

/**
 * @brief 向UART1发送字符串
 * @param str 要发送的字符串
 * @return rt_size_t 实际发送的字节数
 */
rt_size_t uart1_send_string(const char *str)
{
    if (!str) return 0;
    return uart1_send(str, rt_strlen(str));
}

/**
 * @brief 向UART1发送格式化字符串 (类似printf)
 * @param fmt 格式化字符串
 * @param ... 可变参数
 * @return rt_size_t 实际发送的字节数
 */
rt_size_t uart1_printf(const char *fmt, ...)
{
    va_list args;
    char buffer[256];
    int len;
    
    if (!uart1_initialized || !serial_dev) {
        return 0;
    }
    
    va_start(args, fmt);
    len = rt_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    if (len > 0) {
        return uart1_send(buffer, len);
    }
    
    return 0;
}

/**
 * @brief 从UART1接收数据
 * @param buffer 接收缓冲区
 * @param size 缓冲区大小
 * @return rt_size_t 实际接收的字节数
 */
rt_size_t uart1_receive(void *buffer, rt_size_t size)
{
    if (!uart1_initialized || !serial_dev) {
        return 0;
    }
    
    return rt_device_read(serial_dev, 0, buffer, size);
}

/**
 * @brief 检查UART1是否已初始化
 * @return rt_bool_t RT_TRUE已初始化，RT_FALSE未初始化
 */
rt_bool_t uart1_is_initialized(void)
{
    return uart1_initialized;
}

/**
 * @brief 获取UART1设备句柄
 * @return rt_device_t 设备句柄，NULL表示未初始化
 */
rt_device_t uart1_get_device(void)
{
    return uart1_initialized ? serial_dev : RT_NULL;
}

/**
 * @brief 关闭UART1设备
 * @return rt_err_t RT_EOK表示成功
 */
rt_err_t uart1_close(void)
{
    if (!uart1_initialized || !serial_dev) {
        return RT_ERROR;
    }
    
    rt_device_close(serial_dev);
    uart1_initialized = RT_FALSE;
    serial_dev = RT_NULL;
    
    rt_kprintf("UART1 closed\n");
    return RT_EOK;
}

// MSH命令导出
static void cmd_uart1_init(int argc, char **argv)
{
    rt_uint32_t baud_rate = UART1_BAUD_RATE;
    
    if (argc > 1) {
        baud_rate = atoi(argv[1]);
        if (baud_rate == 0) {
            rt_kprintf("Invalid baud rate, using default %d\n", UART1_BAUD_RATE);
            baud_rate = UART1_BAUD_RATE;
        }
    }
    
    rt_err_t result = uart1_init(baud_rate);
    if (result == RT_EOK) {
        rt_kprintf("UART1 initialization completed\n");
    } else {
        rt_kprintf("UART1 initialization failed\n");
    }
}

static void cmd_uart1_test(int argc, char **argv)
{
    if (!uart1_is_initialized()) {
        rt_kprintf("UART1 not initialized. Run 'uart1_init' first.\n");
        return;
    }
    
    rt_kprintf("Sending test message to UART1...\n");
    uart1_printf("UART1 Test Message - Baud Rate: %d\r\n", UART1_BAUD_RATE);
    uart1_printf("Time: %d ms\r\n", rt_tick_get());
    uart1_printf("System: RT-Thread ADC-PV Diagnosis\r\n");
    uart1_printf("Status: UART1 Working\r\n\r\n");
    
    rt_kprintf("Test message sent to UART1 (9600 baud)\n");
}

static void cmd_uart1_status(int argc, char **argv)
{
    rt_kprintf("\n=== UART1 Status ===\n");
    rt_kprintf("Initialized: %s\n", uart1_is_initialized() ? "✅ Yes" : "❌ No");
    rt_kprintf("Device Name: %s\n", UART_DEVICE_NAME);
    rt_kprintf("Baud Rate: %d\n", UART1_BAUD_RATE);
    rt_kprintf("Pins: PA9(TX), PA10(RX)\n");
    rt_kprintf("Mode: Interrupt RX (No DMA)\n");
    rt_kprintf("===================\n");
}

// 导出MSH命令
MSH_CMD_EXPORT_ALIAS(cmd_uart1_init, uart1_init, Initialize UART1 with specified baud rate);
MSH_CMD_EXPORT_ALIAS(cmd_uart1_test, uart1_test, Send test message to UART1);
MSH_CMD_EXPORT_ALIAS(cmd_uart1_status, uart1_status, Show UART1 status information);
