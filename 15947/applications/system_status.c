/* applications/system_status.c */
/* 系统状态显示工具 */

#include <rtthread.h>
#include <rtdevice.h>

/**
 * @brief 显示系统启动信息和状态
 */
int system_info(void)
{
    rt_kprintf("\n🚀 === RT-Thread ADC System Information ===\n");
    rt_kprintf("📅 Build Date: %s %s\n", __DATE__, __TIME__);
    rt_kprintf("🔧 RT-Thread Version: %d.%d.%d\n", 
               RT_VERSION, RT_SUBVERSION, RT_REVISION);
    rt_kprintf("💾 System Memory: Available\n");
    rt_kprintf("🎯 Main Features: 6-Channel ADC + PV Diagnosis\n");
    
    rt_kprintf("\n📡 === Device Status ===\n");
    
    /* ADC设备状态 */
    rt_device_t adc1 = rt_device_find("adc1");
    rt_kprintf("ADC1: %s\n", adc1 ? "✅ Available" : "❌ Not Found");
    
    /* UART设备状态 */
    rt_device_t uart1 = rt_device_find("uart1");
    rt_kprintf("UART1: %s\n", uart1 ? "✅ Available" : "❌ Not Found");
    
    /* 检查电压检测状态 */
    extern rt_bool_t voltage_detection_enabled;
    rt_kprintf("Voltage Detection: %s\n", 
               voltage_detection_enabled ? "🟢 Running" : "🔴 Stopped");
    
    rt_kprintf("\n📋 === Quick Start Guide ===\n");
    rt_kprintf("1️⃣  adc_quick_test           - Test all ADC channels\n");
    rt_kprintf("2️⃣  Enable_Voltage_Detection - Start continuous monitoring\n");
    rt_kprintf("3️⃣  test_pc2_pc3_channels    - Test new PC2/PC3 channels\n");
    rt_kprintf("4️⃣  adc_pv_snapshot          - ADC snapshot with PV diagnosis\n");
    rt_kprintf("5️⃣  system_info              - Show this information\n");
    
    rt_kprintf("\n⚠️  === Important Notes ===\n");
    rt_kprintf("📡 AT device errors are normal if 4G module not connected\n");
    rt_kprintf("🔧 ADC functionality works independently of AT device\n");
    rt_kprintf("📌 Voltage detection is disabled by default to save CPU\n");
    rt_kprintf("🔌 Connect test signals to PA0,PA1,PB0,PB1,PC2,PC3 for testing\n");
    
    rt_kprintf("==========================================\n");
    
    return 0;
}

/**
 * @brief 显示ADC通道映射信息
 */
int adc_pinout(void)
{
    rt_kprintf("\n📍 === ADC Channel Pinout ===\n");
    rt_kprintf("┌─────────┬──────────┬─────────────┬──────────────┐\n");
    rt_kprintf("│ Channel │ Pin Name │ Physical    │ Description  │\n");
    rt_kprintf("├─────────┼──────────┼─────────────┼──────────────┤\n");
    rt_kprintf("│   0     │   PA0    │ P2-15       │ Panel 1+2    │\n");
    rt_kprintf("│   1     │   PA1    │ P2-16       │ Panel 2+3    │\n");
    rt_kprintf("│   9     │   PB0    │ P2-38       │ Panel 4+5    │\n");
    rt_kprintf("│   5     │   PB1    │ P2-37       │ Panel 5+6    │\n");
    rt_kprintf("│   3     │   PA6    │ P2-23 (NEW) │ Additional 1 │\n");
    rt_kprintf("│   7     │   PA7    │ P2-21 (NEW) │ Additional 2 │\n");
    rt_kprintf("└─────────┴──────────┴─────────────┴──────────────┘\n");
    
    rt_kprintf("\n🔧 Technical Specifications:\n");
    rt_kprintf("• Resolution: 16-bit (0-65535)\n");
    rt_kprintf("• Reference Voltage: 3.3V\n");
    rt_kprintf("• Input Range: 0V - 3.3V\n");
    rt_kprintf("• Sampling: 19x average for precision\n");
    rt_kprintf("• Update Rate: 1 second\n");
    
    rt_kprintf("\n⚡ Voltage Calculation:\n");
    rt_kprintf("voltage_mV = (adc_value × 3300) ÷ 65535\n");
    rt_kprintf("Example: ADC=32768 → 1650mV (1.65V)\n");
    
    return 0;
}

/**
 * @brief 显示可用命令列表
 */
int help_adc(void)
{
    rt_kprintf("\n📚 === ADC System Commands ===\n");
    
    rt_kprintf("\n🔬 Testing Commands:\n");
    rt_kprintf("adc_quick_test           - Quick test all 6 channels\n");
    rt_kprintf("test_pa6_pa7_channels    - Test PA6/PA7 channels specifically\n");
    rt_kprintf("monitor_pa6_pa7          - Continuous PA6/PA7 monitoring\n");
    rt_kprintf("adc_test_channel <num>   - Test specific channel by number\n");
    
    rt_kprintf("\n📊 Monitoring Commands:\n");
    rt_kprintf("adc_start                - Start ADC monitoring thread\n");
    rt_kprintf("Enable_Voltage_Detection - Enable continuous voltage display\n");
    rt_kprintf("Disable_Voltage_Sense    - Disable voltage display\n");
    rt_kprintf("voltage_detection_status - Show monitoring status\n");
    
    rt_kprintf("\n🔍 Diagnosis Commands:\n");
    rt_kprintf("adc_pv_snapshot          - ADC snapshot with PV diagnosis\n");
    rt_kprintf("pv_calibrate             - Calibrate PV diagnosis system\n");
    rt_kprintf("pv_diagnose              - Run PV fault diagnosis\n");
    rt_kprintf("pv_test                  - Test PV diagnosis with simulated data\n");
    
    rt_kprintf("\n🔧 System Commands:\n");
    rt_kprintf("system_info              - Show system information\n");
    rt_kprintf("adc_system_check         - Check ADC system status\n");
    rt_kprintf("adc_pinout               - Show ADC channel pinout\n");
    rt_kprintf("uart1_status             - Show UART1 status\n");
    rt_kprintf("help_adc                 - Show this help\n");
    
    rt_kprintf("\n💡 Quick Start:\n");
    rt_kprintf("1. adc_quick_test        - Verify all channels work\n");
    rt_kprintf("2. Enable_Voltage_Detection - Start monitoring\n");
    rt_kprintf("3. Connect test signals and observe values\n");
    
    return 0;
}

/**
 * @brief 清屏并显示欢迎信息
 */
int clear_welcome(void)
{
    /* 清屏 */
    rt_kprintf("\033[2J\033[H");
    
    rt_kprintf("🎯 ================================\n");
    rt_kprintf("   RT-Thread ADC System Ready!\n");
    rt_kprintf("   6-Channel ADC + PV Diagnosis\n");
    rt_kprintf("🎯 ================================\n");
    rt_kprintf("\n💡 Type 'help_adc' for command list\n");
    rt_kprintf("🔬 Type 'adc_quick_test' to start\n");
    rt_kprintf("📍 Type 'adc_pinout' for pin mapping\n\n");
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(system_info, Show system information and quick start guide);
MSH_CMD_EXPORT(adc_pinout, Show ADC channel pinout and specifications);
MSH_CMD_EXPORT(help_adc, Show all available ADC commands);
MSH_CMD_EXPORT(clear_welcome, Clear screen and show welcome message);
