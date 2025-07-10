/* applications/uart_diagnosis.c */
/* UART诊断工具 - 解决air724ug连接问题 */

#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>

/**
 * @brief 检查所有可用的串口设备
 */
int uart_list(void)
{
    rt_kprintf("\n🔍 === Available UART Devices ===\n");
    
    /* 检查常见的串口设备 */
    const char* uart_names[] = {
        "uart1", "uart2", "uart3", "uart4", "uart5", "uart6",
        "lpuart1", "usart1", "usart2", "usart3"
    };
    
    int found_count = 0;
    
    for (int i = 0; i < sizeof(uart_names)/sizeof(uart_names[0]); i++) {
        rt_device_t device = rt_device_find(uart_names[i]);
        if (device) {
            rt_kprintf("✅ %s: Available", uart_names[i]);
            
            /* 检查设备是否已打开 */
            if (device->open_flag & RT_DEVICE_OFLAG_OPEN) {
                rt_kprintf(" (Currently OPEN - In Use)");
            } else {
                rt_kprintf(" (Available for use)");
            }
            rt_kprintf("\n");
            found_count++;
        }
    }
    
    if (found_count == 0) {
        rt_kprintf("❌ No UART devices found\n");
    }
    
    rt_kprintf("=====================================\n");
    rt_kprintf("💡 Recommendation:\n");
    rt_kprintf("   - Use uart2 or uart3 for air724ug if available\n");
    rt_kprintf("   - Keep uart1 for ADC data output\n");
    
    return 0;
}

/**
 * @brief 检查AT设备配置
 */
int at_device_check(void)
{
    rt_kprintf("\n🔍 === AT Device Configuration Check ===\n");
    
    /* 检查AT客户端 */
#ifdef PKG_USING_AT_DEVICE
    struct at_client *client = at_client_get("uart1");

    if (client) {
        rt_kprintf("✅ AT Client found on uart1\n");
        rt_kprintf("   Status: %s\n", client->status == AT_STATUS_INITIALIZED ? "Initialized" : "Not Ready");
    } else {
        rt_kprintf("❌ AT Client not found on uart1\n");
    }
#else
    rt_kprintf("ℹ️  AT Device package is disabled\n");
    rt_kprintf("   This means no AT client conflict with UART1\n");
#endif
    
    /* 检查UART1设备状态 */
    rt_device_t uart1 = rt_device_find("uart1");
    if (uart1) {
        rt_kprintf("✅ UART1 device found\n");
        rt_kprintf("   Open flags: 0x%x\n", uart1->open_flag);
        rt_kprintf("   Type: %d\n", uart1->type);
        
        if (uart1->open_flag & RT_DEVICE_OFLAG_OPEN) {
            rt_kprintf("⚠️  UART1 is currently OPEN (conflict possible)\n");
        }
    } else {
        rt_kprintf("❌ UART1 device not found\n");
    }
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 测试AT命令通信
 */
int at_test_communication(void)
{
    rt_kprintf("\n🔧 === AT Communication Test ===\n");
    
    /* 获取AT客户端 */
#ifdef PKG_USING_AT_DEVICE
    struct at_client *client = at_client_get("uart1");

    if (!client) {
        rt_kprintf("❌ AT client not available\n");
        return -1;
    }

    rt_kprintf("📡 Testing basic AT communication...\n");

    /* 尝试发送基本AT命令 */
    at_response_t resp = at_create_resp(64, 0, rt_tick_from_millisecond(1000));
    if (!resp) {
        rt_kprintf("❌ Failed to create AT response\n");
        return -1;
    }

    /* 测试基本AT命令 */
    rt_err_t result = at_obj_exec_cmd(client, resp, "AT");
    if (result == RT_EOK) {
        rt_kprintf("✅ AT command successful - Module responding\n");
    } else {
        rt_kprintf("❌ AT command failed (error: %d)\n", result);
        rt_kprintf("💡 Possible issues:\n");
        rt_kprintf("   - Wrong baud rate (try 9600, 115200, 460800)\n");
        rt_kprintf("   - Wrong UART pins\n");
        rt_kprintf("   - Module not powered\n");
        rt_kprintf("   - Module not in AT mode\n");
    }

    at_delete_resp(resp);
#else
    rt_kprintf("ℹ️  AT device package is currently disabled\n");
    rt_kprintf("📋 Cannot test AT communication without AT device package\n");
    rt_kprintf("🔧 To enable AT communication testing:\n");
    rt_kprintf("   1. Fix AT device package download issue\n");
    rt_kprintf("   2. Re-enable AT device in RT-Thread Settings\n");
    rt_kprintf("   3. Recompile project\n");
    rt_kprintf("   4. Run this test again\n");
    return 0;  // Not an error, just disabled
#endif
    rt_kprintf("=====================================\n");
    
    return result == RT_EOK ? 0 : -1;
}

/**
 * @brief 显示air724ug连接指南
 */
int air724ug_guide(void)
{
    rt_kprintf("\n📚 === Air724UG Connection Guide ===\n");
    rt_kprintf("\n🔌 Hardware Connections:\n");
    rt_kprintf("Air724UG Pin → ART-Pi Pin\n");
    rt_kprintf("VCC          → 3.3V or 5V\n");
    rt_kprintf("GND          → GND\n");
    rt_kprintf("TXD          → PA10 (UART1_RX) or other UART RX\n");
    rt_kprintf("RXD          → PA9  (UART1_TX) or other UART TX\n");
    rt_kprintf("RST          → Optional GPIO for reset\n");
    rt_kprintf("PWR_KEY      → Optional GPIO for power control\n");
    
    rt_kprintf("\n⚙️  Common Baud Rates:\n");
    rt_kprintf("• 9600   (default for some modules)\n");
    rt_kprintf("• 115200 (most common)\n");
    rt_kprintf("• 460800 (high speed)\n");
    
    rt_kprintf("\n🔧 Troubleshooting Steps:\n");
    rt_kprintf("1. Check power supply (3.3V-5V)\n");
    rt_kprintf("2. Verify UART connections (TX↔RX, RX↔TX)\n");
    rt_kprintf("3. Try different baud rates\n");
    rt_kprintf("4. Use different UART port (uart2, uart3)\n");
    rt_kprintf("5. Check if module is in AT command mode\n");
    
    rt_kprintf("\n💡 Quick Fix Commands:\n");
    rt_kprintf("uart_list              - Check available UARTs\n");
    rt_kprintf("at_device_check        - Check AT device status\n");
    rt_kprintf("at_test_communication  - Test AT communication\n");
    
    rt_kprintf("========================================\n");
    
    return 0;
}

/**
 * @brief 尝试不同波特率测试
 */
int test_baud_rates(void)
{
    rt_kprintf("\n🔧 === Baud Rate Test ===\n");
    rt_kprintf("Testing common baud rates for air724ug...\n");
    
    rt_uint32_t baud_rates[] = {9600, 115200, 460800, 38400, 57600};
    int rate_count = sizeof(baud_rates) / sizeof(baud_rates[0]);
    
    for (int i = 0; i < rate_count; i++) {
        rt_kprintf("\n📡 Testing baud rate: %d\n", baud_rates[i]);
        
        /* 这里可以添加实际的波特率测试逻辑 */
        rt_kprintf("   Use RT-Thread Settings to change UART1 baud rate to %d\n", baud_rates[i]);
        rt_kprintf("   Then recompile and test\n");
    }
    
    rt_kprintf("\n💡 Recommendation:\n");
    rt_kprintf("   Most air724ug modules use 115200 baud rate by default\n");
    rt_kprintf("   If that doesn't work, try 9600 or 460800\n");
    rt_kprintf("=============================\n");
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(uart_list, List all available UART devices);
MSH_CMD_EXPORT(at_device_check, Check AT device configuration and status);
MSH_CMD_EXPORT(at_test_communication, Test AT command communication);
MSH_CMD_EXPORT(air724ug_guide, Show air724ug connection guide);
MSH_CMD_EXPORT(test_baud_rates, Test different baud rates for air724ug);
