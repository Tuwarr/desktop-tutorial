/*
 * uart1_app.h
 *
 * UART1应用模块头文件
 */

#ifndef UART1_APP_H
#define UART1_APP_H

#include <rtthread.h>
#include <rtdevice.h>
#include <stdarg.h>

// 函数声明

/**
 * @brief 初始化并配置 uart1
 * @param baud_rate 波特率
 * @return rt_err_t RT_EOK 表示成功，其他为失败
 */
rt_err_t uart1_init(rt_uint32_t baud_rate);

/**
 * @brief 使用默认波特率9600初始化UART1
 * @return rt_err_t RT_EOK 表示成功，其他为失败
 */
rt_err_t uart1_init_default(void);

/**
 * @brief 向UART1发送数据
 * @param data 要发送的数据指针
 * @param size 数据长度
 * @return rt_size_t 实际发送的字节数
 */
rt_size_t uart1_send(const void *data, rt_size_t size);

/**
 * @brief 向UART1发送字符串
 * @param str 要发送的字符串
 * @return rt_size_t 实际发送的字节数
 */
rt_size_t uart1_send_string(const char *str);

/**
 * @brief 向UART1发送格式化字符串 (类似printf)
 * @param fmt 格式化字符串
 * @param ... 可变参数
 * @return rt_size_t 实际发送的字节数
 */
rt_size_t uart1_printf(const char *fmt, ...);

/**
 * @brief 从UART1接收数据
 * @param buffer 接收缓冲区
 * @param size 缓冲区大小
 * @return rt_size_t 实际接收的字节数
 */
rt_size_t uart1_receive(void *buffer, rt_size_t size);

/**
 * @brief 检查UART1是否已初始化
 * @return rt_bool_t RT_TRUE已初始化，RT_FALSE未初始化
 */
rt_bool_t uart1_is_initialized(void);

/**
 * @brief 获取UART1设备句柄
 * @return rt_device_t 设备句柄，NULL表示未初始化
 */
rt_device_t uart1_get_device(void);

/**
 * @brief 关闭UART1设备
 * @return rt_err_t RT_EOK表示成功
 */
rt_err_t uart1_close(void);

#endif // UART1_APP_H
