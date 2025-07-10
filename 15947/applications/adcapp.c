/* applications/adcapp.c */

#include <rtthread.h>
#include <rtdevice.h>
#include "pv_diagnosis.h"

/* 外部函数声明 - 故障检测模块 */
extern void pv_fault_detector_init(void);
extern int pv_fault_detection_run(void);
extern const char* pv_fault_get_status_string(void);
extern const char* pv_fault_get_multi_status_string(void);
extern int pv_fault_get_count(void);
extern void pv_fault_analyze_pattern(void);
extern rt_bool_t pv_fault_is_baseline_ready(void);

/* ADC参数定义 */
#define VOLTAGE_REF         3300    // 参考电压 3.3V (mV)
#define ADC_MAX_VALUE       65535   // 16位ADC最大值
#define READ_INTERVAL_MS    1000    // 读取间隔 (ms)
#define SAMPLE_COUNT        19      // 采样次数 (取19次平均值)

/* ADC1的正确通道号 */
#define ADC1_CHANNEL_PA0    0       // PA0 -> ADC1_IN0 (实测验证)
#define ADC1_CHANNEL_PA1    1       // PA1 -> ADC1_IN1 (实测验证)
#define ADC1_CHANNEL_PB0    9       // PB0 -> ADC1_INP9 (P2 Pin38)
#define ADC1_CHANNEL_PB1    5       // PB1 -> ADC1_INP5 (P2 Pin37)
#define ADC1_CHANNEL_PA6    3       // PA6 -> ADC1_INP3 (P2 Pin23)
#define ADC1_CHANNEL_PA7    7       // PA7 -> ADC1_INP7 (P2 Pin21)

/* 全局变量 */
rt_bool_t voltage_detection_enabled = RT_FALSE;  // 电压检测循环开关，初始关闭（全局可访问）

/* 函数声明 */
void adc_display_with_diagnosis(rt_uint32_t* adc_values, rt_uint32_t* voltages, pv_diagnosis_result_t* diag_result);

/**
 * @brief 多次采样取平均值 (这个辅助函数很好，我们保留)
 */
static rt_uint32_t adc_read_average(rt_adc_device_t adc_dev, rt_uint8_t channel, rt_uint8_t count)
{
    rt_uint32_t sum = 0;
    if (adc_dev == RT_NULL || count == 0) {
        return 0;
    }

    /* 使能ADC通道 */
    rt_err_t result = rt_adc_enable(adc_dev, channel);
    if (result != RT_EOK)
    {
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
 * @brief ADC读取线程入口函数
 */
static void adc_thread_entry(void *parameter)
{
    rt_uint32_t adc_values[6];
    rt_uint32_t voltages[6];

    /* 查找ADC设备句柄 */
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");

    /* 检查设备是否找到 */
    if (adc1_dev == RT_NULL) {
        rt_kprintf("Warning: rt_device_find('adc1') failed.\n");
        return;
    }

    rt_kprintf("ADC monitoring thread started. Reading every %dms.\n", READ_INTERVAL_MS);
    rt_kprintf("📌 Voltage detection is DISABLED by default. Use 'Enable_Voltage_Detection' to start.\n");
    rt_kprintf("--------------------------------------------------------------------\n");
    rt_kprintf(" Pin |  Raw ADC Value | Voltage \n");
    rt_kprintf("--------------------------------------------------------------------\n");


    /* 主循环 */
    while (1)
    {
        /* 检查电压检测是否启用 */
        if (voltage_detection_enabled)
        {
            /* 读取ADC1的六个通道 */
            adc_values[0] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA0, SAMPLE_COUNT);
            adc_values[1] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA1, SAMPLE_COUNT);
            adc_values[2] = adc_read_average(adc1_dev, ADC1_CHANNEL_PB0, SAMPLE_COUNT);
            adc_values[3] = adc_read_average(adc1_dev, ADC1_CHANNEL_PB1, SAMPLE_COUNT);
            adc_values[4] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA6, SAMPLE_COUNT);
            adc_values[5] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA7, SAMPLE_COUNT);

            /* 转换为电压值 */
            for (int i = 0; i < 6; i++) {
                voltages[i] = (adc_values[i] * VOLTAGE_REF) / ADC_MAX_VALUE;
            }

            /* 原有故障检测 */
            pv_diagnosis_result_t diag_result;
            pv_diagnose_panels(voltages[0], voltages[1], voltages[2], voltages[3], &diag_result);

            /* 新增：高级故障检测 */
            int fault_code = pv_fault_detection_run();
            const char* fault_status = pv_fault_get_status_string();

            /* 显示格式化的结果，带异常标注 */
            adc_display_with_diagnosis(adc_values, voltages, &diag_result);

            /* 计算并显示独立光伏板电压 */
            int pv1 = voltages[0];                          // PV1 = va1
            int pv2 = voltages[1] - voltages[0];            // PV2 = va2 - va1
            int pv3 = voltages[4] - voltages[1];            // PV3 = va3 - va2 (va3是voltages[4])
            int pv4 = voltages[5];                          // PV4 = vb1 (vb1是voltages[5])
            int pv5 = voltages[2] - voltages[5];            // PV5 = vb2 - vb1 (vb2是voltages[2])
            int pv6 = voltages[3] - voltages[2];            // PV6 = vb3 - vb2 (vb3是voltages[3])

            rt_kprintf("Individual PV: PV1=%dmV PV2=%dmV PV3=%dmV | PV4=%dmV PV5=%dmV PV6=%dmV\n",
                       pv1, pv2, pv3, pv4, pv5, pv6);

            /* 显示高级故障检测结果 */
            const char* multi_status = pv_fault_get_multi_status_string();
            int fault_count = pv_fault_get_count();

            if (pv_fault_is_baseline_ready()) {
                if (fault_code != 0) {  // 0 = PV_FAULT_NONE
                    if (fault_count > 1) {
                        rt_kprintf(">>> MULTIPLE FAULTS DETECTED: %s <<<\n", multi_status);
                        pv_fault_analyze_pattern();  // 分析故障模式
                    } else {
                        rt_kprintf(">>> ADVANCED FAULT DETECTED: %s <<<\n", fault_status);
                    }
                } else {
                    rt_kprintf("Advanced Fault Status: %s\n", fault_status);
                }
            } else {
                /* 基准建立期间也检查故障 */
                if (fault_code != 0) {  // 0 = PV_FAULT_NONE
                    if (fault_count > 1) {
                        rt_kprintf(">>> MULTIPLE BASELINE FAULTS: %s <<<\n", multi_status);
                    } else {
                        rt_kprintf(">>> BASELINE FAULT DETECTED: %s <<<\n", fault_status);
                    }
                } else {
                    rt_kprintf("Advanced Fault Detection: Establishing baseline...\n");
                }
            }

            rt_thread_mdelay(READ_INTERVAL_MS);
        }
        else
        {
            /* 电压检测关闭时，线程休眠更长时间以节省CPU */
            rt_thread_mdelay(1000);
        }
    }
}

/**
 * @brief 启动ADC监控线程
 */
int adc_start(void)
{
    rt_thread_t adc_thread;

    adc_thread = rt_thread_create("adc_reader",      // 线程名
                                  adc_thread_entry,  // 线程入口
                                  RT_NULL,           // 参数
                                  2048,              // 栈大小
                                  25,                // 优先级
                                  10);               // 时间片

    if (adc_thread != RT_NULL) {
        rt_thread_startup(adc_thread);
        rt_kprintf("ADC reader thread created and started.\n");
        return 0;
    }

    rt_kprintf("Error: Create ADC reader thread failed!\n");
    return -1;
}

/**
 * @brief ADC数据获取函数 (为PV诊断模块提供数据接口)
 * @param data 指向ADC数据结构体的指针
 * @return RT_EOK 成功, -RT_ERROR 失败
 */
rt_err_t adc_get_pv_data(pv_adc_data_t* data)
{
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");

    if (adc1_dev == RT_NULL || data == RT_NULL) {
        return -RT_ERROR;
    }

    /* 读取六个ADC通道并转换为电压值 */
    rt_uint32_t adc_values[6];
    adc_values[0] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA0, SAMPLE_COUNT);
    adc_values[1] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA1, SAMPLE_COUNT);
    adc_values[2] = adc_read_average(adc1_dev, ADC1_CHANNEL_PB0, SAMPLE_COUNT);
    adc_values[3] = adc_read_average(adc1_dev, ADC1_CHANNEL_PB1, SAMPLE_COUNT);
    adc_values[4] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA6, SAMPLE_COUNT);
    adc_values[5] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA7, SAMPLE_COUNT);

    /* 转换为电压值 (mV) */
    data->v_a1_mv = (adc_values[0] * VOLTAGE_REF) / ADC_MAX_VALUE;
    data->v_a2_mv = (adc_values[1] * VOLTAGE_REF) / ADC_MAX_VALUE;
    data->v_b1_mv = (adc_values[2] * VOLTAGE_REF) / ADC_MAX_VALUE;
    data->v_b2_mv = (adc_values[3] * VOLTAGE_REF) / ADC_MAX_VALUE;
    data->v_c1_mv = (adc_values[4] * VOLTAGE_REF) / ADC_MAX_VALUE;
    data->v_c2_mv = (adc_values[5] * VOLTAGE_REF) / ADC_MAX_VALUE;

    return RT_EOK;
}

/**
 * @brief 显示ADC数据，带故障诊断标注
 * @param adc_values ADC原始值数组
 * @param voltages 电压值数组 (mV)
 * @param diag_result 诊断结果
 */
void adc_display_with_diagnosis(rt_uint32_t* adc_values, rt_uint32_t* voltages, pv_diagnosis_result_t* diag_result)
{
    /* 检查每个面板是否有故障 */
    rt_bool_t panel_faults[7] = {RT_FALSE}; // 索引1-6对应Panel 1-6

    if (diag_result->fault_count > 0) {
        for (int i = 0; i < diag_result->fault_count; i++) {
            int panel_num = diag_result->faulty_panels[i];
            if (panel_num >= 1 && panel_num <= 6) {
                panel_faults[panel_num] = RT_TRUE;
            }
        }
    }

    /* 根据面板故障情况确定通道异常标注 */
    rt_bool_t channel_abnormal[6] = {RT_FALSE};

    // PA0 (va1): PV1 - 如果PV1故障则标注异常
    if (panel_faults[1]) {
        channel_abnormal[0] = RT_TRUE;
    }

    // PA1 (va2): PV1+PV2 - 如果PV1或PV2故障则标注异常
    if (panel_faults[1] || panel_faults[2]) {
        channel_abnormal[1] = RT_TRUE;
    }

    // PA6 (va3): PV1+PV2+PV3 - 如果PV1、PV2或PV3故障则标注异常
    if (panel_faults[1] || panel_faults[2] || panel_faults[3]) {
        channel_abnormal[4] = RT_TRUE;  // PA6是adc_values[4]
    }

    // PA7 (vb1): PV4 - 如果PV4故障则标注异常
    if (panel_faults[4]) {
        channel_abnormal[5] = RT_TRUE;  // PA7是adc_values[5]
    }

    // PB0 (vb2): PV4+PV5 - 如果PV4或PV5故障则标注异常
    if (panel_faults[4] || panel_faults[5]) {
        channel_abnormal[2] = RT_TRUE;  // PB0是adc_values[2]
    }

    // PB1 (vb3): PV4+PV5+PV6 - 如果PV4、PV5或PV6故障则标注异常
    if (panel_faults[4] || panel_faults[5] || panel_faults[6]) {
        channel_abnormal[3] = RT_TRUE;  // PB1是adc_values[3]
    }

    /* 显示格式化的结果 - 按照正确的光伏板测量点逻辑 */
    rt_kprintf(" PA0 | %14d | %4dmV  [va1: PV1]%s\n",
               adc_values[0], voltages[0],
               channel_abnormal[0] ? " (abnormality)" : "");

    rt_kprintf(" PA1 | %14d | %4dmV  [va2: PV1+PV2]%s\n",
               adc_values[1], voltages[1],
               channel_abnormal[1] ? " (abnormality)" : "");

    rt_kprintf(" PA6 | %14d | %4dmV  [va3: PV1+PV2+PV3]%s\n",
               adc_values[4], voltages[4],
               channel_abnormal[4] ? " (abnormality)" : "");

    rt_kprintf(" PA7 | %14d | %4dmV  [vb1: PV4]%s\n",
               adc_values[5], voltages[5],
               channel_abnormal[5] ? " (abnormality)" : "");

    rt_kprintf(" PB0 | %14d | %4dmV  [vb2: PV4+PV5]%s\n",
               adc_values[2], voltages[2],
               channel_abnormal[2] ? " (abnormality)" : "");

    rt_kprintf(" PB1 | %14d | %4dmV  [vb3: PV4+PV5+PV6]%s\n",
               adc_values[3], voltages[3],
               channel_abnormal[3] ? " (abnormality)" : "");

    rt_kprintf("--------------------------------------------------------------------\n");

    /* 如果检测到故障，显示故障摘要 */
    if (diag_result->fault_count > 0 && rt_strcmp(diag_result->status, "Faulty") == 0) {
        rt_kprintf("⚠️  Detected %d faulty panel(s): ", diag_result->fault_count);
        for (int i = 0; i < diag_result->fault_count; i++) {
            rt_kprintf("P%d ", diag_result->faulty_panels[i]);
        }
        rt_kprintf("\n");
        rt_kprintf("--------------------------------------------------------------------\n");
    }
}

/**
 * @brief 初始化ADC与PV诊断模块的集成
 */
void adc_pv_integration_init(void)
{
    /* 注册ADC数据获取函数到PV诊断模块 */
    pv_diag_register_adc_getter(adc_get_pv_data);
    rt_kprintf("ADC-PV integration initialized\n");
}

/**
 * @brief 单次ADC读取并显示PV诊断结果
 */
int adc_pv_snapshot(void)
{
    rt_adc_device_t adc1_dev = (rt_adc_device_t)rt_device_find("adc1");

    if (adc1_dev == RT_NULL) {
        rt_kprintf("Error: ADC1 device not found\n");
        return -1;
    }

    /* 读取ADC数据 */
    rt_uint32_t adc_values[6];
    rt_uint32_t voltages[6];

    adc_values[0] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA0, SAMPLE_COUNT);
    adc_values[1] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA1, SAMPLE_COUNT);
    adc_values[2] = adc_read_average(adc1_dev, ADC1_CHANNEL_PB0, SAMPLE_COUNT);
    adc_values[3] = adc_read_average(adc1_dev, ADC1_CHANNEL_PB1, SAMPLE_COUNT);
    adc_values[4] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA6, SAMPLE_COUNT);
    adc_values[5] = adc_read_average(adc1_dev, ADC1_CHANNEL_PA7, SAMPLE_COUNT);

    /* 转换为电压值 */
    for (int i = 0; i < 6; i++) {
        voltages[i] = (adc_values[i] * VOLTAGE_REF) / ADC_MAX_VALUE;
    }

    /* 显示ADC数据 - 按照正确的光伏板测量点逻辑 */
    rt_kprintf("\n=== ADC Snapshot ===\n");
    rt_kprintf("PA0: %5d (%4dmV) [va1: PV1]\n", adc_values[0], voltages[0]);
    rt_kprintf("PA1: %5d (%4dmV) [va2: PV1+PV2]\n", adc_values[1], voltages[1]);
    rt_kprintf("PA6: %5d (%4dmV) [va3: PV1+PV2+PV3]\n", adc_values[4], voltages[4]);
    rt_kprintf("PA7: %5d (%4dmV) [vb1: PV4]\n", adc_values[5], voltages[5]);
    rt_kprintf("PB0: %5d (%4dmV) [vb2: PV4+PV5]\n", adc_values[2], voltages[2]);
    rt_kprintf("PB1: %5d (%4dmV) [vb3: PV4+PV5+PV6]\n", adc_values[3], voltages[3]);

    /* 执行PV诊断 */
    pv_diagnosis_result_t result;
    pv_diagnose_panels(voltages[0], voltages[1], voltages[2], voltages[3], &result);

    /* 显示诊断结果 */
    rt_kprintf("\n=== PV Diagnosis ===\n");
    rt_kprintf("Status: %s\n", result.status);

    if (result.fault_count > 0) {
        rt_kprintf("⚠️  Faulty Panels: ");
        for (int i = 0; i < result.fault_count; i++) {
            rt_kprintf("P%d ", result.faulty_panels[i]);
        }
        rt_kprintf("\n");
    } else if (rt_strcmp(result.status, "Healthy") == 0) {
        rt_kprintf("✅ All panels healthy\n");
    }

    /* 显示详细信息 */
    if (result.detail_count > 0) {
        rt_kprintf("\nDetails:\n");
        for (int i = 0; i < result.detail_count; i++) {
            rt_kprintf("  %s\n", result.details[i]);
        }
    }
    rt_kprintf("===================\n");

    return 0;
}

/**
 * @brief 启用电压检测循环
 */
int enable_voltage_detection(void)
{
    if (voltage_detection_enabled) {
        rt_kprintf("Voltage detection is already enabled.\n");
        return 0;
    }

    voltage_detection_enabled = RT_TRUE;

    /* 初始化高级故障检测器 */
    pv_fault_detector_init();

    rt_kprintf("✅ Voltage detection enabled.\n");
    rt_kprintf("ADC monitoring will start displaying data every %d ms.\n", READ_INTERVAL_MS);
    rt_kprintf("Advanced fault detection initialized.\n");

    return 0;
}

/**
 * @brief 禁用电压检测循环
 */
int disable_voltage_sense(void)
{
    if (!voltage_detection_enabled) {
        rt_kprintf("Voltage detection is already disabled.\n");
        return 0;
    }

    voltage_detection_enabled = RT_FALSE;
    rt_kprintf("❌ Voltage detection disabled.\n");
    rt_kprintf("ADC monitoring stopped. Use 'Enable Voltage Detection' to restart.\n");

    return 0;
}

/**
 * @brief 查看电压检测状态
 */
int voltage_detection_status(void)
{
    rt_kprintf("\n=== Voltage Detection Status ===\n");
    rt_kprintf("Status: %s\n", voltage_detection_enabled ? "✅ Enabled" : "❌ Disabled");
    rt_kprintf("Update Interval: %d ms\n", READ_INTERVAL_MS);
    rt_kprintf("Channels: PA0, PA1, PB0, PB1, PA6, PA7\n");
    rt_kprintf("PV Diagnosis: %s\n", voltage_detection_enabled ? "Active" : "Inactive");
    rt_kprintf("===============================\n");

    return 0;
}

/* 导出到MSH命令 */
MSH_CMD_EXPORT(adc_start, Start ADC value monitoring);
MSH_CMD_EXPORT(adc_pv_snapshot, Take ADC snapshot and run PV diagnosis);
MSH_CMD_EXPORT_ALIAS(enable_voltage_detection, Enable_Voltage_Detection, Enable voltage detection loop);
MSH_CMD_EXPORT_ALIAS(disable_voltage_sense, Disable_Voltage_Sense, Disable voltage detection loop);
MSH_CMD_EXPORT(voltage_detection_status, Show voltage detection status);
