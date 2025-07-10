/* applications/adc_test_pc2_pc3.c */
/* 测试新添加的PA6和PA7 ADC通道 */

#include <rtthread.h>
#include <rtdevice.h>

#define VOLTAGE_REF         3300    // 参考电压 3.3V (mV)
#define ADC_MAX_VALUE       65535   // 16位ADC最大值
#define SAMPLE_COUNT        19      // 采样次数

/* ADC1通道定义 */
#define ADC1_CHANNEL_PA6    3       // PA6 -> ADC1_INP3
#define ADC1_CHANNEL_PA7    7       // PA7 -> ADC1_INP7

/**
 * @brief 多次采样取平均值
 */
static rt_uint32_t adc_read_average(rt_adc_device_t adc_dev, rt_uint8_t channel, rt_uint8_t count)
{
    rt_uint32_t sum = 0;
    if (adc_dev == RT_NULL || count == 0) {
        return 0;
    }

    /* 使能ADC通道 */
    rt_err_t result = rt_adc_enable(adc_dev, channel);
    if (result != RT_EOK) {
        rt_kprintf("Error: enable adc channel(%d) failed!\n", channel);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        sum += rt_adc_read(adc_dev, channel);
        rt_thread_mdelay(1); // 每次采样间隔1ms
    }

    /* 关闭ADC通道 */
    rt_adc_disable(adc_dev, channel);

    return sum / count;
}

/**
 * @brief 测试PA6和PA7通道
 */
int test_pa6_pa7_channels(void)
{
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");

    if (adc1_dev == RT_NULL) {
        rt_kprintf("Error: ADC1 device not found\n");
        return -1;
    }

    rt_kprintf("\n=== PA6 & PA7 ADC Channel Test ===\n");
    rt_kprintf("Testing new channels PA6 (CH3) and PA7 (CH7)\n");
    rt_kprintf("-----------------------------------------------\n");

    for (int i = 0; i < 10; i++) {
        /* 读取PA6和PA7通道 */
        rt_uint32_t pa6_raw = adc_read_average(adc1_dev, ADC1_CHANNEL_PA6, SAMPLE_COUNT);
        rt_uint32_t pa7_raw = adc_read_average(adc1_dev, ADC1_CHANNEL_PA7, SAMPLE_COUNT);

        /* 转换为电压值 */
        rt_uint32_t pa6_voltage = (pa6_raw * VOLTAGE_REF) / ADC_MAX_VALUE;
        rt_uint32_t pa7_voltage = (pa7_raw * VOLTAGE_REF) / ADC_MAX_VALUE;

        rt_kprintf("Test %2d: PA6=%5d (%4dmV) | PA7=%5d (%4dmV)\n",
                   i+1, pa6_raw, pa6_voltage, pa7_raw, pa7_voltage);

        rt_thread_mdelay(500); // 每500ms测试一次
    }

    rt_kprintf("=======================================\n");
    rt_kprintf("Test completed. Check if values change when you connect signals to PA6/PA7.\n");

    return 0;
}

/**
 * @brief 连续监控PA6和PA7通道
 */
int monitor_pa6_pa7(void)
{
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");

    if (adc1_dev == RT_NULL) {
        rt_kprintf("Error: ADC1 device not found\n");
        return -1;
    }

    rt_kprintf("\n=== PA6 & PA7 Continuous Monitor ===\n");
    rt_kprintf("Press Ctrl+C to stop monitoring\n");
    rt_kprintf("------------------------------------\n");

    while (1) {
        /* 读取PA6和PA7通道 */
        rt_uint32_t pa6_raw = adc_read_average(adc1_dev, ADC1_CHANNEL_PA6, SAMPLE_COUNT);
        rt_uint32_t pa7_raw = adc_read_average(adc1_dev, ADC1_CHANNEL_PA7, SAMPLE_COUNT);

        /* 转换为电压值 */
        rt_uint32_t pa6_voltage = (pa6_raw * VOLTAGE_REF) / ADC_MAX_VALUE;
        rt_uint32_t pa7_voltage = (pa7_raw * VOLTAGE_REF) / ADC_MAX_VALUE;

        rt_kprintf("PA6: %5d (%4dmV) | PA7: %5d (%4dmV)\n",
                   pa6_raw, pa6_voltage, pa7_raw, pa7_voltage);

        rt_thread_mdelay(1000); // 每1秒更新一次
    }

    return 0;
}

/* 导出到MSH命令 */
MSH_CMD_EXPORT(test_pa6_pa7_channels, Test PA6 and PA7 ADC channels);
MSH_CMD_EXPORT(monitor_pa6_pa7, Monitor PA6 and PA7 channels continuously);
