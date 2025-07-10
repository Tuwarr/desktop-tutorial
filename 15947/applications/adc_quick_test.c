/* applications/adc_quick_test.c */
/* 快速ADC测试工具 - 独立于AT设备问题 */

#include <rtthread.h>
#include <rtdevice.h>

#define VOLTAGE_REF         3300    // 参考电压 3.3V (mV)
#define ADC_MAX_VALUE       65535   // 16位ADC最大值
#define SAMPLE_COUNT        5       // 快速测试用较少采样

/* ADC1通道定义 */
#define ADC1_CHANNEL_PA0    0       // PA0 -> ADC1_IN0
#define ADC1_CHANNEL_PA1    1       // PA1 -> ADC1_IN1
#define ADC1_CHANNEL_PB0    9       // PB0 -> ADC1_INP9
#define ADC1_CHANNEL_PB1    5       // PB1 -> ADC1_INP5
#define ADC1_CHANNEL_PA6    3       // PA6 -> ADC1_INP3
#define ADC1_CHANNEL_PA7    7       // PA7 -> ADC1_INP7

/**
 * @brief 快速ADC读取（少量采样）
 */
static rt_uint32_t adc_quick_read(rt_adc_device_t adc_dev, rt_uint8_t channel)
{
    rt_uint32_t sum = 0;
    if (adc_dev == RT_NULL) {
        return 0;
    }

    /* 使能ADC通道 */
    rt_err_t result = rt_adc_enable(adc_dev, channel);
    if (result != RT_EOK) {
        rt_kprintf("Error: enable adc channel(%d) failed!\n", channel);
        return 0;
    }

    /* 快速采样5次 */
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        sum += rt_adc_read(adc_dev, channel);
        rt_thread_mdelay(1);
    }

    /* 关闭ADC通道 */
    rt_adc_disable(adc_dev, channel);

    return sum / SAMPLE_COUNT;
}

/**
 * @brief 快速测试所有6个ADC通道
 */
int adc_quick_test(void)
{
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");

    if (adc1_dev == RT_NULL) {
        rt_kprintf("❌ Error: ADC1 device not found\n");
        rt_kprintf("💡 Check RT-Thread Settings -> Hardware -> On-chip Peripheral Drivers -> ADC1\n");
        return -1;
    }

    rt_kprintf("\n🔬 === Quick ADC Test (All 6 Channels) ===\n");
    rt_kprintf("📊 Testing with %d samples per channel...\n", SAMPLE_COUNT);
    rt_kprintf("-----------------------------------------------\n");

    /* 测试所有6个通道 */
    struct {
        const char* name;
        rt_uint8_t channel;
        const char* pin;
    } channels[] = {
        {"PA0", ADC1_CHANNEL_PA0, "PA0"},
        {"PA1", ADC1_CHANNEL_PA1, "PA1"},
        {"PB0", ADC1_CHANNEL_PB0, "PB0"},
        {"PB1", ADC1_CHANNEL_PB1, "PB1"},
        {"PA6", ADC1_CHANNEL_PA6, "PA6 (NEW)"},
        {"PA7", ADC1_CHANNEL_PA7, "PA7 (NEW)"}
    };

    for (int i = 0; i < 6; i++) {
        rt_uint32_t adc_value = adc_quick_read(adc1_dev, channels[i].channel);
        rt_uint32_t voltage = (adc_value * VOLTAGE_REF) / ADC_MAX_VALUE;
        
        rt_kprintf("%s: %5d (%4dmV) [%s]\n", 
                   channels[i].name, adc_value, voltage, channels[i].pin);
    }

    rt_kprintf("-----------------------------------------------\n");
    rt_kprintf("✅ Quick test completed!\n");
    rt_kprintf("💡 For continuous monitoring: Enable_Voltage_Detection\n");
    rt_kprintf("🔧 For PA6/PA7 specific test: test_pa6_pa7_channels\n");

    return 0;
}

/**
 * @brief 检查ADC系统状态
 */
int adc_system_check(void)
{
    rt_kprintf("\n🔍 === ADC System Check ===\n");
    
    /* 检查ADC1设备 */
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");
    rt_kprintf("ADC1 Device: %s\n", adc1_dev ? "✅ Found" : "❌ Not Found");
    
    if (adc1_dev) {
        rt_kprintf("Device Type: %s\n", "RT-Thread ADC Device");
        rt_kprintf("Channels: 6 (PA0, PA1, PB0, PB1, PA6, PA7)\n");
        rt_kprintf("Resolution: 16-bit (0-65535)\n");
        rt_kprintf("Reference: 3.3V (3300mV)\n");
    }
    
    /* 检查UART1状态 */
    extern rt_bool_t uart1_is_initialized(void);
    rt_kprintf("UART1 Status: %s\n", uart1_is_initialized() ? "✅ Initialized" : "❌ Not Initialized");
    
    /* 检查电压检测状态 */
    extern rt_bool_t voltage_detection_enabled;
    rt_kprintf("Voltage Detection: %s\n", voltage_detection_enabled ? "✅ Enabled" : "❌ Disabled");
    
    rt_kprintf("==========================\n");
    rt_kprintf("💡 Quick commands:\n");
    rt_kprintf("   adc_quick_test           - Test all 6 channels\n");
    rt_kprintf("   Enable_Voltage_Detection - Start continuous monitoring\n");
    rt_kprintf("   test_pc2_pc3_channels    - Test new PC2/PC3 channels\n");
    rt_kprintf("   uart1_status             - Check UART1 status\n");
    
    return 0;
}

/**
 * @brief 单通道测试
 */
int adc_test_channel(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: adc_test_channel <channel_number>\n");
        rt_kprintf("Available channels: 0(PA0), 1(PA1), 5(PB1), 9(PB0), 3(PA6), 7(PA7)\n");
        return -1;
    }
    
    int channel = atoi(argv[1]);
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");
    
    if (adc1_dev == RT_NULL) {
        rt_kprintf("❌ ADC1 device not found\n");
        return -1;
    }
    
    rt_kprintf("🔬 Testing ADC1 Channel %d...\n", channel);
    
    for (int i = 0; i < 5; i++) {
        rt_uint32_t adc_value = adc_quick_read(adc1_dev, channel);
        rt_uint32_t voltage = (adc_value * VOLTAGE_REF) / ADC_MAX_VALUE;
        
        rt_kprintf("Test %d: %5d (%4dmV)\n", i+1, adc_value, voltage);
        rt_thread_mdelay(200);
    }
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(adc_quick_test, Quick test all 6 ADC channels);
MSH_CMD_EXPORT(adc_system_check, Check ADC system status and show commands);
MSH_CMD_EXPORT(adc_test_channel, Test specific ADC channel by number);
