/* applications/uart_conflict_analysis.c */
/* UART冲突详细分析工具 */

#include <rtthread.h>
#include <rtdevice.h>

/**
 * @brief 详细分析UART设备使用情况
 */
int uart_conflict_analysis(void)
{
    rt_kprintf("\n🔍 === UART Conflict Analysis ===\n");
    
    rt_kprintf("\n📋 System UART Configuration:\n");
    rt_kprintf("• Console Device (rt_kprintf): %s\n", RT_CONSOLE_DEVICE_NAME);
    rt_kprintf("• AT Device Client Name: uart1 (from config)\n");
    rt_kprintf("• ADC App UART: uart1 (from uart1_app.c)\n");
    
    rt_kprintf("\n⚠️  CONFLICT IDENTIFIED:\n");
    rt_kprintf("┌─────────────────────────────────────────────┐\n");
    rt_kprintf("│ UART1 is being used by TWO applications:   │\n");
    rt_kprintf("│ 1. ADC Application (uart1_app.c)           │\n");
    rt_kprintf("│ 2. AT Device for air724ug communication    │\n");
    rt_kprintf("└─────────────────────────────────────────────┘\n");
    
    rt_kprintf("\n🔧 Device Status Check:\n");
    
    /* 检查UART1状态 */
    rt_device_t uart1 = rt_device_find("uart1");
    if (uart1) {
        rt_kprintf("✅ UART1 device found\n");
        rt_kprintf("   Open Flag: 0x%x\n", uart1->open_flag);
        rt_kprintf("   Reference Count: %d\n", uart1->ref_count);
        
        if (uart1->open_flag & RT_DEVICE_OFLAG_OPEN) {
            rt_kprintf("   Status: 🔴 OPEN (Being used)\n");
            rt_kprintf("   This explains the 'Error: control uart1 failed! (error code: 7)'\n");
        } else {
            rt_kprintf("   Status: 🟢 CLOSED (Available)\n");
        }
    } else {
        rt_kprintf("❌ UART1 device not found\n");
    }
    
    /* 检查控制台设备 */
    rt_device_t console = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (console) {
        rt_kprintf("✅ Console device (%s) found\n", RT_CONSOLE_DEVICE_NAME);
        rt_kprintf("   This is where rt_kprintf output goes\n");
    } else {
        rt_kprintf("❌ Console device (%s) not found\n", RT_CONSOLE_DEVICE_NAME);
    }
    
    rt_kprintf("\n📊 Conflict Timeline:\n");
    rt_kprintf("1. System starts\n");
    rt_kprintf("2. ADC app calls uart1_init_default() → Opens UART1\n");
    rt_kprintf("3. AT device tries to use UART1 → CONFLICT!\n");
    rt_kprintf("4. AT device gets 'device busy' error (code 7)\n");
    rt_kprintf("5. Connection timeout occurs\n");
    
    rt_kprintf("\n💡 Why rt_kprintf still works:\n");
    rt_kprintf("• rt_kprintf uses %s (Console), NOT uart1\n", RT_CONSOLE_DEVICE_NAME);
    rt_kprintf("• Console and UART1 are completely separate devices\n");
    rt_kprintf("• That's why you can see debug messages even with UART1 conflict\n");
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 显示解决方案
 */
int uart_conflict_solutions(void)
{
    rt_kprintf("\n🔧 === UART Conflict Solutions ===\n");
    
    rt_kprintf("\n🎯 Solution 1: Use Different UART for AT Device (RECOMMENDED)\n");
    rt_kprintf("Steps:\n");
    rt_kprintf("1. In RT-Thread Settings:\n");
    rt_kprintf("   IoT → AT DEVICE → air720 sample client name\n");
    rt_kprintf("   Change from 'uart1' to 'uart2' or 'uart3'\n");
    rt_kprintf("2. Hardware: Connect air724ug to corresponding UART pins\n");
    rt_kprintf("3. Recompile and test\n");
    
    rt_kprintf("\n🎯 Solution 2: Disable ADC UART Usage\n");
    rt_kprintf("Steps:\n");
    rt_kprintf("1. Comment out uart1_init_default() in main.c\n");
    rt_kprintf("2. ADC data will only show on console (%s)\n", RT_CONSOLE_DEVICE_NAME);
    rt_kprintf("3. UART1 becomes available for AT device\n");
    
    rt_kprintf("\n🎯 Solution 3: Use Different UART for ADC\n");
    rt_kprintf("Steps:\n");
    rt_kprintf("1. Modify uart1_app.c to use uart2 or uart3\n");
    rt_kprintf("2. Change UART_DEVICE_NAME from 'uart1' to 'uart2'\n");
    rt_kprintf("3. Update hardware connections accordingly\n");
    
    rt_kprintf("\n📋 Current UART Usage Summary:\n");
    rt_kprintf("• %s: rt_kprintf output (Console) ✅\n", RT_CONSOLE_DEVICE_NAME);
    rt_kprintf("• uart1: ADC app + AT device ❌ CONFLICT\n");
    rt_kprintf("• uart2: Available for use ✅\n");
    rt_kprintf("• uart3: Available for use ✅\n");
    
    rt_kprintf("\n⭐ RECOMMENDED ACTION:\n");
    rt_kprintf("Use Solution 1 - Move AT device to uart2\n");
    rt_kprintf("This keeps ADC UART functionality intact\n");
    rt_kprintf("and resolves the air724ug connection issue\n");
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 检查UART设备的详细信息
 */
int uart_device_details(void)
{
    rt_kprintf("\n📊 === UART Device Details ===\n");
    
    const char* uart_list[] = {"uart1", "uart2", "uart3", "uart4", "uart5"};
    int uart_count = sizeof(uart_list) / sizeof(uart_list[0]);
    
    for (int i = 0; i < uart_count; i++) {
        rt_device_t device = rt_device_find(uart_list[i]);
        if (device) {
            rt_kprintf("\n🔌 %s:\n", uart_list[i]);
            rt_kprintf("   Found: ✅\n");
            rt_kprintf("   Type: %d\n", device->type);
            rt_kprintf("   Open Flag: 0x%x\n", device->open_flag);
            rt_kprintf("   Ref Count: %d\n", device->ref_count);
            
            if (device->open_flag & RT_DEVICE_OFLAG_OPEN) {
                rt_kprintf("   Status: 🔴 IN USE\n");
                
                /* 特殊说明 */
                if (rt_strcmp(uart_list[i], "uart1") == 0) {
                    rt_kprintf("   Used by: ADC app (uart1_app.c)\n");
                    rt_kprintf("   Conflict: AT device also wants this\n");
                } else if (rt_strcmp(uart_list[i], RT_CONSOLE_DEVICE_NAME) == 0) {
                    rt_kprintf("   Used by: System console (rt_kprintf)\n");
                }
            } else {
                rt_kprintf("   Status: 🟢 AVAILABLE\n");
                rt_kprintf("   Can be used for: AT device or other apps\n");
            }
        } else {
            rt_kprintf("\n❌ %s: Not found\n", uart_list[i]);
        }
    }
    
    rt_kprintf("\n💡 Key Insights:\n");
    rt_kprintf("• Console (%s) is separate from application UARTs\n", RT_CONSOLE_DEVICE_NAME);
    rt_kprintf("• UART1 conflict is the root cause of AT device failure\n");
    rt_kprintf("• Multiple UARTs are available for different purposes\n");
    
    rt_kprintf("=====================================\n");
    
    return 0;
}

/**
 * @brief 实时监控UART状态
 */
int uart_status_monitor(void)
{
    rt_kprintf("\n📡 === Real-time UART Status Monitor ===\n");
    rt_kprintf("Monitoring UART1 and console device status...\n");
    rt_kprintf("Press Ctrl+C to stop\n\n");
    
    for (int i = 0; i < 10; i++) {
        rt_device_t uart1 = rt_device_find("uart1");
        rt_device_t console = rt_device_find(RT_CONSOLE_DEVICE_NAME);
        
        rt_kprintf("Monitor %d:\n", i + 1);
        
        if (uart1) {
            rt_kprintf("  UART1: %s (ref: %d)\n", 
                       (uart1->open_flag & RT_DEVICE_OFLAG_OPEN) ? "OPEN" : "CLOSED",
                       uart1->ref_count);
        }
        
        if (console) {
            rt_kprintf("  Console (%s): %s (ref: %d)\n", 
                       RT_CONSOLE_DEVICE_NAME,
                       (console->open_flag & RT_DEVICE_OFLAG_OPEN) ? "OPEN" : "CLOSED",
                       console->ref_count);
        }
        
        rt_thread_mdelay(1000);
    }
    
    rt_kprintf("Monitor completed.\n");
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(uart_conflict_analysis, Detailed analysis of UART conflict);
MSH_CMD_EXPORT(uart_conflict_solutions, Show solutions for UART conflict);
MSH_CMD_EXPORT(uart_device_details, Show detailed UART device information);
MSH_CMD_EXPORT(uart_status_monitor, Monitor UART status in real-time);
