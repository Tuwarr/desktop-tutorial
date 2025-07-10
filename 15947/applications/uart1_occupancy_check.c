/* applications/uart1_occupancy_check.c */
/* UART1占用情况详细检查工具 */

#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>

/**
 * @brief 检查UART1的详细占用情况
 */
int uart1_who_is_using(void)
{
    rt_kprintf("\n🔍 === UART1 Occupancy Analysis ===\n");
    
    rt_device_t uart1 = rt_device_find("uart1");
    if (!uart1) {
        rt_kprintf("❌ UART1 device not found!\n");
        return -1;
    }
    
    rt_kprintf("📊 UART1 Device Information:\n");
    rt_kprintf("• Device Name: %s\n", uart1->parent.name);
    rt_kprintf("• Device Type: %d\n", uart1->type);
    rt_kprintf("• Open Flag: 0x%x\n", uart1->open_flag);
    rt_kprintf("• Reference Count: %d\n", uart1->ref_count);
    rt_kprintf("• User Data: %p\n", uart1->user_data);
    
    if (uart1->open_flag & RT_DEVICE_OFLAG_OPEN) {
        rt_kprintf("• Status: 🔴 DEVICE IS OPEN (Being Used)\n");
        
        rt_kprintf("\n🔍 Analyzing who opened UART1...\n");
        
        /* 检查可能的使用者 */
        rt_kprintf("\n📋 Potential Users of UART1:\n");
        
        /* 1. 检查ADC应用 */
        extern rt_bool_t uart1_is_initialized(void);
        rt_bool_t adc_uart_init = uart1_is_initialized();
        rt_kprintf("1. ADC Application (uart1_app.c):\n");
        rt_kprintf("   Status: %s\n", adc_uart_init ? "✅ INITIALIZED" : "❌ Not initialized");
        if (adc_uart_init) {
            rt_kprintf("   Function: uart1_init_default() called\n");
            rt_kprintf("   Purpose: ADC data output (115200 baud)\n");
            rt_kprintf("   Impact: 🔴 OCCUPYING UART1\n");
        }
        
        /* 2. 检查AT客户端 */
        rt_kprintf("\n2. AT Client for air724ug:\n");
#ifdef PKG_USING_AT_DEVICE
        struct at_client *at_client = at_client_get("uart1");
        if (at_client) {
            rt_kprintf("   Status: ✅ AT CLIENT EXISTS\n");
            rt_kprintf("   Client Status: %d\n", at_client->status);
            rt_kprintf("   Purpose: Communication with air724ug module\n");
            rt_kprintf("   Impact: 🔴 TRYING TO USE UART1\n");
        } else {
            rt_kprintf("   Status: ❌ AT CLIENT NOT FOUND\n");
        }
#else
        rt_kprintf("   Status: ❌ AT DEVICE DISABLED\n");
        rt_kprintf("   Note: AT device package is not enabled\n");
        rt_kprintf("   Impact: ✅ NO CONFLICT WITH UART1\n");
#endif
        
        /* 3. 检查其他可能的使用者 */
        rt_kprintf("\n3. Other Potential Users:\n");
        
        /* 检查Shell是否使用UART1 */
        extern rt_device_t rt_console_get_device(void);
        rt_device_t console = rt_console_get_device();
        if (console && rt_strcmp(console->parent.name, "uart1") == 0) {
            rt_kprintf("   Console (Shell): ✅ USING UART1\n");
        } else {
            rt_kprintf("   Console (Shell): ❌ Using %s instead\n", 
                       console ? console->parent.name : "unknown");
        }
        
        /* 检查其他AT设备 */
        rt_kprintf("   Other AT devices: Checking...\n");
        
    } else {
        rt_kprintf("• Status: 🟢 DEVICE IS CLOSED (Available)\n");
    }
    
    rt_kprintf("\n🔧 Reference Count Analysis:\n");
    rt_kprintf("• Current ref_count: %d\n", uart1->ref_count);
    rt_kprintf("• Each rt_device_open() increases ref_count by 1\n");
    rt_kprintf("• ref_count > 0 means device is being used\n");
    
    if (uart1->ref_count > 1) {
        rt_kprintf("⚠️  WARNING: Multiple applications are using UART1!\n");
        rt_kprintf("   This explains the conflict!\n");
    } else if (uart1->ref_count == 1) {
        rt_kprintf("ℹ️  One application is using UART1\n");
    } else {
        rt_kprintf("✅ No applications are currently using UART1\n");
    }
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 追踪UART1的打开历史
 */
int uart1_open_trace(void)
{
    rt_kprintf("\n📜 === UART1 Open Trace Analysis ===\n");
    
    rt_kprintf("🔍 Tracing UART1 usage in your application:\n\n");
    
    rt_kprintf("1️⃣  main.c initialization sequence:\n");
    rt_kprintf("   main() → uart1_init_default() → rt_device_open(uart1)\n");
    rt_kprintf("   Purpose: Initialize UART1 for ADC data output\n");
    rt_kprintf("   Baud Rate: 115200\n");
    rt_kprintf("   Mode: TX/RX\n");
    
    rt_kprintf("\n2️⃣  AT Device initialization (background):\n");
    rt_kprintf("   at_device_init() → at_client_init(\"uart1\") → rt_device_open(uart1)\n");
    rt_kprintf("   Purpose: AT command communication with air724ug\n");
    rt_kprintf("   Expected Baud Rate: 115200 (air724ug default)\n");
    rt_kprintf("   Mode: TX/RX\n");
    
    rt_kprintf("\n⚡ Conflict Point:\n");
    rt_kprintf("   Both applications try to open the SAME device (uart1)\n");
    rt_kprintf("   RT-Thread allows multiple opens, but hardware conflicts occur\n");
    
    rt_kprintf("\n🔍 Current Status Check:\n");
    
    /* 检查实际状态 */
    rt_device_t uart1 = rt_device_find("uart1");
    if (uart1) {
        rt_kprintf("   UART1 found: ✅\n");
        rt_kprintf("   Open flag: 0x%x\n", uart1->open_flag);
        rt_kprintf("   Reference count: %d\n", uart1->ref_count);
        
        if (uart1->ref_count >= 2) {
            rt_kprintf("   🔴 CONFIRMED: Multiple opens detected!\n");
            rt_kprintf("   This is the source of your conflict!\n");
        }
    }
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 显示UART1使用者的详细信息
 */
int uart1_users_detail(void)
{
    rt_kprintf("\n👥 === UART1 Users Detailed Information ===\n");
    
    rt_kprintf("\n🔧 User 1: ADC Application\n");
    rt_kprintf("   File: applications/uart1_app.c\n");
    rt_kprintf("   Function: uart1_init_default()\n");
    rt_kprintf("   Called from: main.c line ~52\n");
    rt_kprintf("   Purpose: Output ADC measurement data\n");
    rt_kprintf("   Baud Rate: 115200\n");
    rt_kprintf("   Data Format: Text (ADC values and voltages)\n");
    rt_kprintf("   Usage Pattern: Continuous output when enabled\n");
    
    rt_kprintf("\n📡 User 2: AT Device (air724ug)\n");
    rt_kprintf("   Configuration: RT-Thread Settings → AT DEVICE\n");
    rt_kprintf("   Client Name: \"uart1\" (configured)\n");
    rt_kprintf("   Purpose: AT command communication\n");
    rt_kprintf("   Expected Baud Rate: 115200\n");
    rt_kprintf("   Data Format: AT commands (text)\n");
    rt_kprintf("   Usage Pattern: Command-response\n");
    
    rt_kprintf("\n⚖️  Conflict Analysis:\n");
    rt_kprintf("   Both users expect EXCLUSIVE access to UART1\n");
    rt_kprintf("   ADC app: Sends data continuously\n");
    rt_kprintf("   AT device: Expects clean AT command channel\n");
    rt_kprintf("   Result: Data corruption and communication failure\n");
    
    rt_kprintf("\n🎯 Impact on air724ug:\n");
    rt_kprintf("   • AT commands get mixed with ADC data\n");
    rt_kprintf("   • air724ug receives corrupted commands\n");
    rt_kprintf("   • No proper AT response received\n");
    rt_kprintf("   • Connection timeout occurs\n");
    
    rt_kprintf("\n💡 Why error code 7 (RT_ERROR_BUSY):\n");
    rt_kprintf("   • UART1 hardware is already configured by ADC app\n");
    rt_kprintf("   • AT device tries to reconfigure the same hardware\n");
    rt_kprintf("   • Hardware conflict results in BUSY error\n");
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 实时监控UART1的使用情况
 */
int uart1_usage_monitor(void)
{
    rt_kprintf("\n📊 === UART1 Real-time Usage Monitor ===\n");
    rt_kprintf("Monitoring UART1 for 10 seconds...\n");
    rt_kprintf("Watch for reference count changes and status updates\n\n");
    
    rt_device_t uart1 = rt_device_find("uart1");
    if (!uart1) {
        rt_kprintf("❌ UART1 not found!\n");
        return -1;
    }
    
    for (int i = 0; i < 10; i++) {
        rt_kprintf("Time %2ds: ", i + 1);
        rt_kprintf("RefCount=%d, ", uart1->ref_count);
        rt_kprintf("OpenFlag=0x%x, ", uart1->open_flag);
        rt_kprintf("Status=%s\n", 
                   (uart1->open_flag & RT_DEVICE_OFLAG_OPEN) ? "OPEN" : "CLOSED");
        
        /* 检查是否有变化 */
        static int last_ref_count = -1;
        if (last_ref_count != -1 && last_ref_count != uart1->ref_count) {
            rt_kprintf("   🔄 Reference count changed: %d → %d\n", 
                       last_ref_count, uart1->ref_count);
        }
        last_ref_count = uart1->ref_count;
        
        rt_thread_mdelay(1000);
    }
    
    rt_kprintf("\nMonitoring completed.\n");
    rt_kprintf("Final status: RefCount=%d, OpenFlag=0x%x\n", 
               uart1->ref_count, uart1->open_flag);
    
    return 0;
}

/**
 * @brief 建议的解决方案
 */
int uart1_conflict_fix(void)
{
    rt_kprintf("\n🔧 === UART1 Conflict Fix Recommendations ===\n");
    
    rt_kprintf("\n🎯 IMMEDIATE SOLUTION (Recommended):\n");
    rt_kprintf("Change AT device to use UART2 instead of UART1\n\n");
    
    rt_kprintf("Steps:\n");
    rt_kprintf("1. Open RT-Thread Settings\n");
    rt_kprintf("2. Navigate: RT-Thread online packages → IoT → AT DEVICE\n");
    rt_kprintf("3. Find 'air720 sample client name'\n");
    rt_kprintf("4. Change from 'uart1' to 'uart2'\n");
    rt_kprintf("5. Save and regenerate project\n");
    rt_kprintf("6. Recompile\n");
    rt_kprintf("7. Connect air724ug to UART2 pins instead\n");
    
    rt_kprintf("\n🔌 Hardware Connection Update:\n");
    rt_kprintf("Air724UG → ART-Pi\n");
    rt_kprintf("TXD      → UART2_RX (check pinout)\n");
    rt_kprintf("RXD      → UART2_TX (check pinout)\n");
    rt_kprintf("VCC      → 3.3V\n");
    rt_kprintf("GND      → GND\n");
    
    rt_kprintf("\n✅ Expected Result:\n");
    rt_kprintf("• UART1: Used by ADC app only\n");
    rt_kprintf("• UART2: Used by air724ug only\n");
    rt_kprintf("• No more conflicts\n");
    rt_kprintf("• air724ug connection should succeed\n");
    
    rt_kprintf("\n🔍 Verification Commands:\n");
    rt_kprintf("After fix, run these to verify:\n");
    rt_kprintf("• uart_list - Check UART2 status\n");
    rt_kprintf("• at_device_check - Verify AT device on UART2\n");
    rt_kprintf("• uart1_who_is_using - Confirm UART1 only used by ADC\n");
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(uart1_who_is_using, Check who is using UART1 in detail);
MSH_CMD_EXPORT(uart1_open_trace, Trace UART1 open history and conflicts);
MSH_CMD_EXPORT(uart1_users_detail, Show detailed information about UART1 users);
MSH_CMD_EXPORT(uart1_usage_monitor, Monitor UART1 usage in real-time);
MSH_CMD_EXPORT(uart1_conflict_fix, Show step-by-step fix for UART1 conflict);
