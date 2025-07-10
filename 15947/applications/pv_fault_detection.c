/* applications/pv_fault_detection.c */
/* 光伏板故障检测模块 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "pv_diagnosis.h"

/* 故障检测配置参数 */
#define BASELINE_SAMPLES_COUNT    10      // 基准值采样次数
#define FAULT_DROP_THRESHOLD      0.50f   // 故障检测阈值：相对基准下降50%以上
#define MIN_BASELINE_VOLTAGE      0.02f   // 最小有效基准电压 (20mV) - 降低要求
#define SEVERE_NEGATIVE_THRESHOLD -0.1f   // 严重负电压阈值 (-100mV) - 只检测极端情况
#define MIN_VOLTAGE_DIFF_THRESHOLD 1.0f   // 最小电压差阈值 (1V) - 保留用于交叉检测

/* 故障代码枚举 */
typedef enum {
    PV_FAULT_NONE = 0,
    PV_FAULT_PV1,
    PV_FAULT_PV2,
    PV_FAULT_PV3,
    PV_FAULT_PV4,
    PV_FAULT_PV5,
    // PV_FAULT_PV6 已移除 - 现在只有5个光伏板
    PV_FAULT_UNKNOWN
} pv_fault_code_t;

/* 故障检测器状态结构 */
typedef struct {
    rt_bool_t is_baseline_set;           // 基准值是否已建立
    int baseline_sample_count;           // 当前基准采样计数

    /* 上一次的累计电压读数 (用于组内自检) */
    float prev_cumulative_voltages[5];   // va1,va2,va3,vb1,vb2 (移除vb3)

    /* 上一次的独立电压读数 (用于时序比较) */
    float prev_individual_voltages[5];   // PV1~PV5 (移除PV6)

    /* 基准值：各独立光伏板的电压 */
    float baseline_individual_voltages[5]; // PV1~PV5 (移除PV6)

    /* 基准值：配对板之间的电压差 */
    float baseline_diffs[2];             // [0]:PV1-PV4, [1]:PV2-PV5 (移除PV3-PV5)

    /* 用于计算基准平均值的累加器 */
    float baseline_accumulator[5];       // 累加器数组 (移除PV6)

    /* 当前故障状态 */
    pv_fault_code_t current_fault;

    /* 多故障检测 */
    rt_bool_t fault_status[5];           // 每个PV的故障状态 (移除PV6)
    int fault_count;                     // 当前故障数量

} pv_fault_detector_t;

/* 全局故障检测器实例 */
static pv_fault_detector_t g_fault_detector;

/* 外部函数声明 */
extern rt_err_t adc_get_pv_data(pv_adc_data_t* data);

/**
 * @brief 从累计电压计算出每个独立光伏板的电压 (5个PV)
 */
static void calculate_individual_voltages(const float cumulative_v[5], float individual_v[5])
{
    individual_v[0] = cumulative_v[0];                          // PV1 = va1
    individual_v[1] = cumulative_v[1] - cumulative_v[0];        // PV2 = va2 - va1
    individual_v[2] = cumulative_v[2] - cumulative_v[1];        // PV3 = va3 - va2
    individual_v[3] = cumulative_v[3];                          // PV4 = vb1
    individual_v[4] = cumulative_v[4] - cumulative_v[3];        // PV5 = vb2 - vb1
    // PV6 = vb3 - vb2 已移除
}

/**
 * @brief 故障代码转换为字符串
 */
static const char* fault_code_to_string(pv_fault_code_t code)
{
    switch (code) {
        case PV_FAULT_NONE: return "Normal";
        case PV_FAULT_PV1:  return "PV1 Fault";
        case PV_FAULT_PV2:  return "PV2 Fault";
        case PV_FAULT_PV3:  return "PV3 Fault";
        case PV_FAULT_PV4:  return "PV4 Fault";
        case PV_FAULT_PV5:  return "PV5 Fault";
        // PV_FAULT_PV6 已移除
        default:            return "Unknown Fault";
    }
}

/**
 * @brief 初始化故障检测器
 */
void pv_fault_detector_init(void)
{
    rt_memset(&g_fault_detector, 0, sizeof(pv_fault_detector_t));
    g_fault_detector.is_baseline_set = RT_FALSE;
    g_fault_detector.baseline_sample_count = 0;
    g_fault_detector.current_fault = PV_FAULT_NONE;
    
    rt_kprintf("PV Fault Detector Initialized\n");
}

/**
 * @brief 建立基准值（增加基准期间的故障检测）
 */
static void establish_baseline(const float cumulative_v[5])
{
    if (g_fault_detector.is_baseline_set) return;

    float individual_v[5];
    calculate_individual_voltages(cumulative_v, individual_v);

    /* 基准建立期间的简单故障检测 */
    if (g_fault_detector.baseline_sample_count >= 3) {  // 至少3个样本后开始检测
        /* 计算当前的临时平均值 */
        float temp_avg[5];
        for (int i = 0; i < 5; i++) {
            temp_avg[i] = g_fault_detector.baseline_accumulator[i] / g_fault_detector.baseline_sample_count;
        }

        /* 检测明显的电压骤降（相对于临时平均值） */
        for (int i = 0; i < 5; i++) {
            if (temp_avg[i] > 0.05f) {  // 临时平均值 > 50mV才有意义
                float drop_ratio = (temp_avg[i] - individual_v[i]) / temp_avg[i];
                if (drop_ratio > 0.5f) {  // 电压下降超过50%
                    rt_kprintf(">>> BASELINE FAULT DETECTED: PV%d voltage dropped %.1f%% <<<\n",
                               i + 1, drop_ratio * 100);
                    rt_kprintf("    Expected: %dmV, Current: %dmV\n",
                               (int)(temp_avg[i] * 1000), (int)(individual_v[i] * 1000));
                    g_fault_detector.current_fault = (pv_fault_code_t)(i + 1);  // PV_FAULT_PV1 = 1, etc.
                }
            }
        }
    }

    /* 累加到基准累加器（过滤明显异常值） */
    rt_bool_t sample_valid = RT_TRUE;
    for (int i = 0; i < 5; i++) {
        /* 检查是否为明显异常值 */
        if (g_fault_detector.baseline_sample_count >= 3) {
            float temp_avg = g_fault_detector.baseline_accumulator[i] / g_fault_detector.baseline_sample_count;
            if (temp_avg > 0.05f) {  // 临时平均值 > 50mV
                float deviation = fabsf(individual_v[i] - temp_avg) / temp_avg;
                if (deviation > 0.5f) {  // 偏差超过50%
                    rt_kprintf("WARNING: Filtering abnormal sample PV%d: %dmV (expected ~%dmV)\n",
                               i + 1, (int)(individual_v[i] * 1000), (int)(temp_avg * 1000));
                    sample_valid = RT_FALSE;
                    break;
                }
            }
        }

        /* 只累加正常样本 */
        if (sample_valid) {
            g_fault_detector.baseline_accumulator[i] += individual_v[i];
        }
    }

    /* 只有正常样本才增加计数 */
    if (sample_valid) {
        g_fault_detector.baseline_sample_count++;
    } else {
        rt_kprintf("Sample rejected, continuing baseline establishment...\n");
    }

    /* 检查是否已采集足够样本 */
    if (g_fault_detector.baseline_sample_count >= BASELINE_SAMPLES_COUNT) {
        rt_kprintf("=== PV Baseline Established ===\n");

        /* 计算平均基准值 */
        for (int i = 0; i < 5; i++) {
            g_fault_detector.baseline_individual_voltages[i] =
                g_fault_detector.baseline_accumulator[i] / BASELINE_SAMPLES_COUNT;
            rt_kprintf("Baseline PV%d: %dmV", i + 1,
                       (int)(g_fault_detector.baseline_individual_voltages[i] * 1000));

            /* 检查基准值合理性 */
            if (fabsf(g_fault_detector.baseline_individual_voltages[i]) < 0.01f) {
                rt_kprintf(" (WARNING: Near zero baseline!)");
            } else if (g_fault_detector.baseline_individual_voltages[i] < -0.05f) {
                rt_kprintf(" (WARNING: Negative baseline!)");
            } else if (g_fault_detector.baseline_individual_voltages[i] < 0.1f) {
                rt_kprintf(" (WARNING: Very low baseline, may cause false positives!)");
            }
            rt_kprintf("\n");
        }

        /* 计算配对板的基准差值 */
        g_fault_detector.baseline_diffs[0] =
            g_fault_detector.baseline_individual_voltages[0] - g_fault_detector.baseline_individual_voltages[3]; // PV1-PV4
        g_fault_detector.baseline_diffs[1] =
            g_fault_detector.baseline_individual_voltages[1] - g_fault_detector.baseline_individual_voltages[4]; // PV2-PV5
        // PV3-PV5 基准差值已移除（PV6不存在）
        
        rt_kprintf("Baseline Diff (PV1-PV4): %dmV\n", (int)(g_fault_detector.baseline_diffs[0] * 1000));
        rt_kprintf("Baseline Diff (PV2-PV5): %dmV\n", (int)(g_fault_detector.baseline_diffs[1] * 1000));
        // PV3-PV5 基准差值已移除
        rt_kprintf("===============================\n");

        g_fault_detector.is_baseline_set = RT_TRUE;

        /* 保存当前值作为"上一次"的值 */
        for (int i = 0; i < 5; i++) {
            g_fault_detector.prev_cumulative_voltages[i] = cumulative_v[i];
        }
    } else {
        rt_kprintf("Baseline sampling: %d/%d\n", 
                   g_fault_detector.baseline_sample_count, BASELINE_SAMPLES_COUNT);
    }
}

/**
 * @brief 逻辑一：组内自检（多故障检测版）
 */
static pv_fault_code_t detect_fault_self_check(const float current_cumulative[5])
{
    /* 计算当前独立电压 */
    float current_individual[5];
    calculate_individual_voltages(current_cumulative, current_individual);

    /* 时序故障检测：与上一时刻比较 */
    rt_bool_t has_prev_data = RT_FALSE;
    for (int i = 0; i < 5; i++) {
        if (fabsf(g_fault_detector.prev_individual_voltages[i]) > 0.01f) {
            has_prev_data = RT_TRUE;
            break;
        }
    }

    if (has_prev_data) {
        for (int i = 0; i < 5; i++) {
            float prev_voltage = g_fault_detector.prev_individual_voltages[i];
            float curr_voltage = current_individual[i];

            /* 严格的安全检查：确保数值在合理范围内 */
            if (fabsf(prev_voltage) > 5.0f || fabsf(curr_voltage) > 5.0f ||
                prev_voltage != prev_voltage || curr_voltage != curr_voltage) {  // 检查NaN
                /* 数值异常，跳过时序检测 */
                rt_kprintf("TEMPORAL CHECK: Skipping PV%d due to invalid data (prev:%.3f, curr:%.3f)\n",
                           i + 1, prev_voltage, curr_voltage);
                continue;
            }

            /* 检查显著的电压变化（不管正负，关键是变化幅度） */
            if (fabsf(prev_voltage) > 0.02f) {  // 前值 > 20mV才有意义
                float change_ratio = (prev_voltage - curr_voltage) / fabsf(prev_voltage);

                /* 检测显著下降（相对变化超过60%） */
                if (change_ratio > 0.60f && change_ratio < 2.0f) {  // 添加上限，避免异常值
                    rt_kprintf("TEMPORAL CHECK: PV%d significant drop %.1f%% (%dmV->%dmV)\n",
                               i + 1, change_ratio * 100,
                               (int)(prev_voltage * 1000), (int)(curr_voltage * 1000));
                    g_fault_detector.fault_status[i] = RT_TRUE;
                    /* 不在这里增加计数，统一在最后计数 */
                }
            }
        }
    }

    /* 检查A组 (PV1, PV2, PV3) - 处理负电压的故障检测 */
    float pv1_drop_ratio = 0, pv2_drop_ratio = 0, pv3_drop_ratio = 0;

    /* PV1检测 - 基于基准值的相对下降检测 */
    if (g_fault_detector.baseline_individual_voltages[0] > MIN_BASELINE_VOLTAGE) {
        pv1_drop_ratio = (g_fault_detector.baseline_individual_voltages[0] - current_individual[0]) /
                         g_fault_detector.baseline_individual_voltages[0];
        /* 只检测显著下降，忽略上升 */
        if (pv1_drop_ratio < 0) pv1_drop_ratio = 0;  // 电压上升不认为是故障
    }

    /* 只检测极端负电压情况 */
    if (current_individual[0] < SEVERE_NEGATIVE_THRESHOLD) {  // < -100mV
        rt_kprintf("PV1 severe negative voltage: %dmV\n", (int)(current_individual[0]*1000));
        pv1_drop_ratio = 1.0f;  // 强制触发故障检测
    }

    /* PV2检测 - 基于基准值的相对下降检测 */
    if (g_fault_detector.baseline_individual_voltages[1] > MIN_BASELINE_VOLTAGE) {
        pv2_drop_ratio = (g_fault_detector.baseline_individual_voltages[1] - current_individual[1]) /
                         g_fault_detector.baseline_individual_voltages[1];
        /* 只检测显著下降，忽略上升 */
        if (pv2_drop_ratio < 0) pv2_drop_ratio = 0;  // 电压上升不认为是故障
    }

    /* 只检测极端负电压情况 */
    if (current_individual[1] < SEVERE_NEGATIVE_THRESHOLD) {  // < -100mV
        rt_kprintf("PV2 severe negative voltage: %dmV\n", (int)(current_individual[1]*1000));
        pv2_drop_ratio = 1.0f;  // 强制触发故障检测
    }

    /* PV3检测 - 基于基准值的相对下降检测 */
    if (g_fault_detector.baseline_individual_voltages[2] > MIN_BASELINE_VOLTAGE) {
        pv3_drop_ratio = (g_fault_detector.baseline_individual_voltages[2] - current_individual[2]) /
                         g_fault_detector.baseline_individual_voltages[2];
        /* 只检测显著下降，忽略上升 */
        if (pv3_drop_ratio < 0) pv3_drop_ratio = 0;  // 电压上升不认为是故障
    }

    /* 只检测极端负电压情况 */
    if (current_individual[2] < SEVERE_NEGATIVE_THRESHOLD) {  // < -100mV
        rt_kprintf("PV3 severe negative voltage: %dmV\n", (int)(current_individual[2]*1000));
        pv3_drop_ratio = 1.0f;  // 强制触发故障检测
    }

    /* 检测显著的电压下降（阈值50%，基于可靠基准值） */
    if (pv1_drop_ratio > FAULT_DROP_THRESHOLD) {
        rt_kprintf("SELF-CHECK: PV1 dropped %d%% (%dmV->%dmV)\n",
                   (int)(pv1_drop_ratio*100), (int)(g_fault_detector.baseline_individual_voltages[0]*1000),
                   (int)(current_individual[0]*1000));
        g_fault_detector.fault_status[0] = RT_TRUE;
        /* 不在这里增加计数，统一在最后计数 */
    }
    if (pv2_drop_ratio > FAULT_DROP_THRESHOLD) {
        rt_kprintf("SELF-CHECK: PV2 dropped %d%% (%dmV->%dmV)\n",
                   (int)(pv2_drop_ratio*100), (int)(g_fault_detector.baseline_individual_voltages[1]*1000),
                   (int)(current_individual[1]*1000));
        g_fault_detector.fault_status[1] = RT_TRUE;
        /* 不在这里增加计数，统一在最后计数 */
    }
    if (pv3_drop_ratio > FAULT_DROP_THRESHOLD) {
        rt_kprintf("SELF-CHECK: PV3 dropped %d%% (%dmV->%dmV)\n",
                   (int)(pv3_drop_ratio*100), (int)(g_fault_detector.baseline_individual_voltages[2]*1000),
                   (int)(current_individual[2]*1000));
        g_fault_detector.fault_status[2] = RT_TRUE;
        /* 不在这里增加计数，统一在最后计数 */
    }
    
    /* 检查B组 (PV4, PV5) - 处理负电压的故障检测 */
    float pv4_drop_ratio = 0, pv5_drop_ratio = 0;

    /* PV4检测 - 基于基准值的相对下降检测 */
    if (g_fault_detector.baseline_individual_voltages[3] > MIN_BASELINE_VOLTAGE) {
        pv4_drop_ratio = (g_fault_detector.baseline_individual_voltages[3] - current_individual[3]) /
                         g_fault_detector.baseline_individual_voltages[3];
        /* 只检测显著下降，忽略上升 */
        if (pv4_drop_ratio < 0) pv4_drop_ratio = 0;  // 电压上升不认为是故障
    }

    /* 只检测极端负电压情况 */
    if (current_individual[3] < SEVERE_NEGATIVE_THRESHOLD) {  // < -100mV
        rt_kprintf("PV4 severe negative voltage: %dmV\n", (int)(current_individual[3]*1000));
        pv4_drop_ratio = 1.0f;  // 强制触发故障检测
    }

    /* PV5检测 - 基于基准值的相对下降检测 */
    if (g_fault_detector.baseline_individual_voltages[4] > MIN_BASELINE_VOLTAGE) {
        pv5_drop_ratio = (g_fault_detector.baseline_individual_voltages[4] - current_individual[4]) /
                         g_fault_detector.baseline_individual_voltages[4];
        /* 只检测显著下降，忽略上升 */
        if (pv5_drop_ratio < 0) pv5_drop_ratio = 0;  // 电压上升不认为是故障
    }

    /* 只检测极端负电压情况 */
    if (current_individual[4] < SEVERE_NEGATIVE_THRESHOLD) {  // < -100mV
        rt_kprintf("PV5 severe negative voltage: %dmV\n", (int)(current_individual[4]*1000));
        pv5_drop_ratio = 1.0f;  // 强制触发故障检测
    }

    /* PV6检测已移除 - 现在只有5个光伏板 */

    /* 检测显著的电压下降 */
    if (pv4_drop_ratio > FAULT_DROP_THRESHOLD) {
        rt_kprintf("SELF-CHECK: PV4 dropped %d%% (%dmV->%dmV)\n",
                   (int)(pv4_drop_ratio*100), (int)(g_fault_detector.baseline_individual_voltages[3]*1000),
                   (int)(current_individual[3]*1000));
        g_fault_detector.fault_status[3] = RT_TRUE;
        /* 不在这里增加计数，统一在最后计数 */
    }
    if (pv5_drop_ratio > FAULT_DROP_THRESHOLD) {
        rt_kprintf("SELF-CHECK: PV5 dropped %d%% (%dmV->%dmV)\n",
                   (int)(pv5_drop_ratio*100), (int)(g_fault_detector.baseline_individual_voltages[4]*1000),
                   (int)(current_individual[4]*1000));
        g_fault_detector.fault_status[4] = RT_TRUE;
        /* 不在这里增加计数，统一在最后计数 */
    }
    /* PV6故障检测已移除 */

    /* 移除过于严格的绝对值检测 - 已在各PV检测中处理极端情况 */

    /* 检测异常高电压（可能的测量错误） */
    for (int i = 0; i < 5; i++) {
        if (current_individual[i] > 0.5f) {  // 任何PV电压 > 500mV都可能异常
            rt_kprintf("WARNING: PV%d unusually high voltage %dmV (possible measurement error)\n",
                       i + 1, (int)(current_individual[i]*1000));
        }
    }

    /* 持续性故障检测将在主函数中处理 */

    /* 统一计算故障数量 - 避免重复计数 */
    g_fault_detector.fault_count = 0;
    for (int i = 0; i < 5; i++) {
        if (g_fault_detector.fault_status[i]) {
            g_fault_detector.fault_count++;
        }
    }

    rt_kprintf("FAULT SUMMARY: Total %d faults detected\n", g_fault_detector.fault_count);

    /* 返回第一个检测到的故障，或无故障 */
    for (int i = 0; i < 5; i++) {
        if (g_fault_detector.fault_status[i]) {
            return (pv_fault_code_t)(i + 1);  // PV_FAULT_PV1 = 1, etc.
        }
    }

    return PV_FAULT_NONE;
}

/**
 * @brief 逻辑二：组间互检
 */
static pv_fault_code_t detect_fault_cross_check(const float current_individual[5])
{
    /* 计算当前各配对的电压差 */
    float current_diff_1_4 = current_individual[0] - current_individual[3];
    float current_diff_2_5 = current_individual[1] - current_individual[4];
    // PV3-PV5差值已移除（PV6不存在）
    
    /* 检查 PV1 vs PV4 */
    if (fabsf(g_fault_detector.baseline_diffs[0]) > MIN_VOLTAGE_DIFF_THRESHOLD) {
        float change_1_4 = (current_diff_1_4 - g_fault_detector.baseline_diffs[0]) / g_fault_detector.baseline_diffs[0];
        if (fabsf(change_1_4) > 0.15f) {  // 降低阈值到15%提高敏感度
            rt_kprintf("CROSS-CHECK: PV1-PV4 diff changed %d%% (baseline:%dmV, current:%dmV)\n",
                       (int)(change_1_4*100), (int)(g_fault_detector.baseline_diffs[0]*1000),
                       (int)(current_diff_1_4*1000));
            if (change_1_4 < 0) {
                return (g_fault_detector.baseline_diffs[0] > 0) ? PV_FAULT_PV1 : PV_FAULT_PV4;
            } else {
                return (g_fault_detector.baseline_diffs[0] > 0) ? PV_FAULT_PV4 : PV_FAULT_PV1;
            }
        }
    }
    
    /* 检查 PV2 vs PV5 */
    if (fabsf(g_fault_detector.baseline_diffs[1]) > MIN_VOLTAGE_DIFF_THRESHOLD) {
        float change_2_5 = (current_diff_2_5 - g_fault_detector.baseline_diffs[1]) / g_fault_detector.baseline_diffs[1];
        if (fabsf(change_2_5) > 0.15f) {  // 降低阈值到15%
            rt_kprintf("CROSS-CHECK: PV2-PV5 diff changed %d%% (baseline:%dmV, current:%dmV)\n",
                       (int)(change_2_5*100), (int)(g_fault_detector.baseline_diffs[1]*1000),
                       (int)(current_diff_2_5*1000));
            if (change_2_5 < 0) {
                return (g_fault_detector.baseline_diffs[1] > 0) ? PV_FAULT_PV2 : PV_FAULT_PV5;
            } else {
                return (g_fault_detector.baseline_diffs[1] > 0) ? PV_FAULT_PV5 : PV_FAULT_PV2;
            }
        }
    }

    /* PV3-PV5交叉检测已移除（PV6不存在） */
    
    return PV_FAULT_NONE;
}

/**
 * @brief 执行一次完整的故障检测
 */
pv_fault_code_t pv_fault_detection_run(void)
{
    pv_adc_data_t adc_data;
    
    /* 获取ADC数据 */
    if (adc_get_pv_data(&adc_data) != RT_EOK) {
        return PV_FAULT_UNKNOWN;
    }
    
    /* 构建累计电压数组 (转换为V) - 按照正确的测量点映射 */
    float cumulative_v[5] = {
        adc_data.v_a1_mv / 1000.0f,  // va1 (PA0) = PV1
        adc_data.v_a2_mv / 1000.0f,  // va2 (PA1) = PV1+PV2
        adc_data.v_c1_mv / 1000.0f,  // va3 (PA6) = PV1+PV2+PV3
        adc_data.v_c2_mv / 1000.0f,  // vb1 (PA7) = PV4
        adc_data.v_b1_mv / 1000.0f   // vb2 (PB0) = PV4+PV5 (vb3已移除)
    };
    
    /* 如果基准未建立，进行基准采集（但仍可能检测到故障） */
    if (!g_fault_detector.is_baseline_set) {
        establish_baseline(cumulative_v);
        return g_fault_detector.current_fault;  // 返回基准期间检测到的故障
    }
    
    /* 保存上一轮的故障状态，用于持续性检测 */
    rt_bool_t prev_fault_status[5];
    for (int i = 0; i < 5; i++) {
        prev_fault_status[i] = g_fault_detector.fault_status[i];
        g_fault_detector.fault_status[i] = RT_FALSE;  // 临时重置，重新检测
    }
    g_fault_detector.fault_count = 0;
    g_fault_detector.current_fault = PV_FAULT_NONE;

    /* 执行两种故障检测逻辑 */
    detect_fault_self_check(cumulative_v);

    float individual_v[5];
    calculate_individual_voltages(cumulative_v, individual_v);
    detect_fault_cross_check(individual_v);

    /* 恢复持续性故障 - 如果之前有故障且当前检测条件仍满足，保持故障状态 */
    for (int i = 0; i < 5; i++) {
        if (prev_fault_status[i] && !g_fault_detector.fault_status[i]) {
            /* 之前有故障，当前检测没有发现，检查是否真正恢复 */
            rt_bool_t still_faulty = RT_FALSE;

            /* 检查是否仍然满足故障条件 */
            if (g_fault_detector.baseline_individual_voltages[i] > MIN_BASELINE_VOLTAGE) {
                float drop_ratio = (g_fault_detector.baseline_individual_voltages[i] - individual_v[i]) /
                                   g_fault_detector.baseline_individual_voltages[i];
                if (drop_ratio > FAULT_DROP_THRESHOLD * 0.8f) {  // 80%的阈值，避免抖动
                    still_faulty = RT_TRUE;
                }
            }

            /* 检查任何负电压（不只是极端负电压） */
            if (individual_v[i] < -0.02f) {  // 任何 < -20mV都认为是故障
                still_faulty = RT_TRUE;
            }

            /* 检查异常低正电压 */
            if (individual_v[i] > 0 && individual_v[i] < 0.05f) {  // 0-50mV异常低
                still_faulty = RT_TRUE;
            }

            if (still_faulty) {
                rt_kprintf("PERSISTENT FAULT: PV%d fault continues (%dmV)\n",
                           i + 1, (int)(individual_v[i]*1000));
                g_fault_detector.fault_status[i] = RT_TRUE;
            } else {
                rt_kprintf("FAULT RECOVERY: PV%d fault cleared (%dmV) - voltage normalized\n",
                           i + 1, (int)(individual_v[i]*1000));
            }
        }
    }

    /* 重新计算故障数量（包含持续性故障） */
    g_fault_detector.fault_count = 0;
    for (int i = 0; i < 5; i++) {
        if (g_fault_detector.fault_status[i]) {
            g_fault_detector.fault_count++;
        }
    }

    rt_kprintf("FAULT SUMMARY: Total %d faults detected\n", g_fault_detector.fault_count);

    /* 更新故障状态 - 选择最严重的故障作为主故障码 */
    if (g_fault_detector.fault_count > 0) {
        /* 计算当前独立电压，找出最严重的故障 */
        float individual_v[6];
        calculate_individual_voltages(cumulative_v, individual_v);

        float worst_voltage = 1.0f;  // 初始化为正常值
        int worst_panel = -1;

        for (int i = 0; i < 5; i++) {
            if (g_fault_detector.fault_status[i]) {
                /* 对于负电压，绝对值越大越严重 */
                if (individual_v[i] < 0) {
                    float abs_voltage = fabsf(individual_v[i]);
                    if (abs_voltage > fabsf(worst_voltage) || worst_voltage > 0) {
                        worst_voltage = individual_v[i];
                        worst_panel = i;
                    }
                }
                /* 对于正电压，值越小越严重（相对于基准） */
                else if (g_fault_detector.baseline_individual_voltages[i] > 0.05f) {
                    float drop_ratio = (g_fault_detector.baseline_individual_voltages[i] - individual_v[i]) /
                                       g_fault_detector.baseline_individual_voltages[i];
                    if (drop_ratio > 0.5f && (worst_voltage > 0 || individual_v[i] < worst_voltage)) {
                        worst_voltage = individual_v[i];
                        worst_panel = i;
                    }
                }
            }
        }

        if (worst_panel >= 0) {
            g_fault_detector.current_fault = (pv_fault_code_t)(worst_panel + 1);
            rt_kprintf("PRIMARY FAULT: PV%d (worst: %dmV)\n", worst_panel + 1, (int)(worst_voltage * 1000));
        } else {
            /* 如果没有找到最严重的，返回第一个 */
            for (int i = 0; i < 5; i++) {
                if (g_fault_detector.fault_status[i]) {
                    g_fault_detector.current_fault = (pv_fault_code_t)(i + 1);
                    break;
                }
            }
        }
    }
    
    /* 更新"上一次"的电压记录 */
    for (int i = 0; i < 5; i++) {
        g_fault_detector.prev_cumulative_voltages[i] = cumulative_v[i];
    }

    /* 更新上一次的独立电压读数 */
    float current_individual[5];
    calculate_individual_voltages(cumulative_v, current_individual);
    for (int i = 0; i < 5; i++) {
        g_fault_detector.prev_individual_voltages[i] = current_individual[i];
    }

    return g_fault_detector.current_fault;
}

/**
 * @brief 获取当前故障状态
 */
pv_fault_code_t pv_fault_get_current_status(void)
{
    return g_fault_detector.current_fault;
}

/**
 * @brief 获取当前故障状态（整数形式，用于OneNET上传）
 */
int pv_fault_get_current_status_int(void)
{
    return (int)g_fault_detector.current_fault;
}

/**
 * @brief 获取故障状态字符串
 */
const char* pv_fault_get_status_string(void)
{
    return fault_code_to_string(g_fault_detector.current_fault);
}

/**
 * @brief 获取多故障状态字符串
 */
const char* pv_fault_get_multi_status_string(void)
{
    static char multi_status[128];

    if (g_fault_detector.fault_count == 0) {
        return "Normal";
    } else if (g_fault_detector.fault_count == 1) {
        return fault_code_to_string(g_fault_detector.current_fault);
    } else {
        /* 多故障情况 */
        rt_snprintf(multi_status, sizeof(multi_status), "Multiple Faults (%d): ", g_fault_detector.fault_count);
        for (int i = 0; i < 5; i++) {
            if (g_fault_detector.fault_status[i]) {
                char temp[16];
                rt_snprintf(temp, sizeof(temp), "PV%d ", i + 1);
                strcat(multi_status, temp);  // 使用标准C库的strcat
            }
        }
        return multi_status;
    }
}

/**
 * @brief 获取故障数量
 */
int pv_fault_get_count(void)
{
    return g_fault_detector.fault_count;
}

/**
 * @brief 获取单个PV的故障状态
 * @param pv_index PV索引 (0=PV1, 1=PV2, ..., 5=PV6)
 * @return RT_TRUE=故障, RT_FALSE=正常
 */
rt_bool_t pv_fault_get_individual_status(int pv_index)
{
    if (pv_index < 0 || pv_index >= 5) {  // 改为5个PV
        return RT_FALSE;
    }
    return g_fault_detector.fault_status[pv_index];
}

/**
 * @brief 分析故障模式
 */
void pv_fault_analyze_pattern(void)
{
    if (g_fault_detector.fault_count <= 1) return;

    rt_kprintf("=== FAULT PATTERN ANALYSIS ===\n");
    rt_kprintf("Total faults detected: %d\n", g_fault_detector.fault_count);

    /* 检查是否为组内故障 */
    rt_bool_t group_a_fault = g_fault_detector.fault_status[0] || g_fault_detector.fault_status[1] || g_fault_detector.fault_status[2];
    rt_bool_t group_b_fault = g_fault_detector.fault_status[3] || g_fault_detector.fault_status[4];  // 移除PV6

    if (group_a_fault && group_b_fault) {
        rt_kprintf("Pattern: Cross-group failure (both A and B groups affected)\n");
        rt_kprintf("Possible causes: System-wide issue, power supply problem, environmental factor\n");
    } else if (group_a_fault) {
        rt_kprintf("Pattern: Group A failure (PV1-PV3 affected)\n");
        rt_kprintf("Possible causes: Group A wiring issue, inverter problem\n");
    } else if (group_b_fault) {
        rt_kprintf("Pattern: Group B failure (PV4-PV5 affected)\n");  // 移除PV6
        rt_kprintf("Possible causes: Group B wiring issue, inverter problem\n");
    }

    /* 检查配对故障 - PV6相关检查已移除 */
    if (g_fault_detector.fault_status[1] && g_fault_detector.fault_status[4]) {  // PV2 & PV5
        rt_kprintf("Special pattern: PV2 & PV5 simultaneous failure\n");
        rt_kprintf("Recommendation: Check similar installation conditions or batch issues\n");
    }

    rt_kprintf("==============================\n");
}

/**
 * @brief 检查基准是否已建立
 */
rt_bool_t pv_fault_is_baseline_ready(void)
{
    return g_fault_detector.is_baseline_set;
}

/**
 * @brief 重置故障检测器
 */
int reset_pv_fault_detector(void)
{
    pv_fault_detector_init();
    rt_kprintf("PV Fault Detector Reset\n");
    return 0;
}

/**
 * @brief 强制重建基准值（当基准值不合理时使用）
 */
int rebuild_pv_baseline(void)
{
    rt_kprintf("Rebuilding PV baseline values...\n");
    g_fault_detector.is_baseline_set = RT_FALSE;
    g_fault_detector.baseline_sample_count = 0;

    /* 清零累加器 */
    for (int i = 0; i < 5; i++) {
        g_fault_detector.baseline_accumulator[i] = 0.0f;
        g_fault_detector.baseline_individual_voltages[i] = 0.0f;
    }

    rt_kprintf("Baseline reset. Will re-establish in next %d samples.\n", BASELINE_SAMPLES_COUNT);
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(reset_pv_fault_detector, Reset PV fault detector);
MSH_CMD_EXPORT(rebuild_pv_baseline, Rebuild PV baseline values);
