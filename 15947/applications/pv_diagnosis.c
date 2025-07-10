/*
 * pv_diagnosis.c
 *
 * A relative change-based photovoltaic panel diagnosis algorithm for RT-Thread.
 * This algorithm first calibrates a baseline with uneven initial conditions,
 * then detects faults by checking for significant deviations from that baseline.
 *
 * SC-SP Structure: 6 panels total
 * String A: Panel 1, 2, 3 in series
 * String B: Panel 4, 5, 6 in series
 * String A and B are in parallel.
 *
 * ADC Inputs mapping:
 * v_a1 (PA0): Voltage of Panel 1 + Panel 2
 * v_a2 (PA1): Voltage of Panel 2 + Panel 3
 * v_b1 (PB0): Voltage of Panel 4 + Panel 5
 * v_b2 (PB1): Voltage of Panel 5 + Panel 6
 */

#include <rtthread.h>
#include <stdio.h>
#include "pv_diagnosis.h"

// 偏差阈值 (单位: mV): 当前电压对的差值与初始校准差值之间的最大允许变化量
#define DEVIATION_THRESHOLD_MV   500

// 用于存储校准值的全局静态变量
static int initial_delta_ab1 = 0; // 校准时: v_a1 - v_b1
static int initial_delta_ab2 = 0; // 校准时: v_a2 - v_b2
static rt_bool_t is_calibrated = RT_FALSE;

// 外部ADC数据获取函数指针 (低耦合设计)
static pv_adc_data_getter_t adc_data_getter = RT_NULL;

/**
 * @brief 注册ADC数据获取函数 (低耦合接口)
 * @param getter 指向ADC数据获取函数的指针
 */
void pv_diag_register_adc_getter(pv_adc_data_getter_t getter)
{
    adc_data_getter = getter;
    rt_kprintf("PV Diagnosis: ADC data getter registered\n");
}

/**
 * @brief 校准诊断系统，记录初始健康状态
 * @param v_a1_mv A组串联中，板1和板2的总电压 (单位: mV)
 * @param v_a2_mv A组串联中，板2和板3的总电压 (单位: mV)
 * @param v_b1_mv B组串联中，板4和板5的总电压 (单位: mV)
 * @param v_b2_mv B组串联中，板5和板6的总电压 (单位: mV)
 */
void pv_diag_calibrate(int v_a1_mv, int v_a2_mv, int v_b1_mv, int v_b2_mv)
{
    initial_delta_ab1 = v_a1_mv - v_b1_mv;
    initial_delta_ab2 = v_a2_mv - v_b2_mv;
    is_calibrated = RT_TRUE;

    rt_kprintf("--- PV Diagnosis System Calibrated ---\n");
    rt_kprintf("Initial V(A1,A2,B1,B2): %d, %d, %d, %d mV\n", v_a1_mv, v_a2_mv, v_b1_mv, v_b2_mv);
    rt_kprintf("Initial Delta V(A1-B1): %d mV\n", initial_delta_ab1);
    rt_kprintf("Initial Delta V(A2-B2): %d mV\n", initial_delta_ab2);
    rt_kprintf("Deviation Threshold: +/-%d mV\n", DEVIATION_THRESHOLD_MV);
    rt_kprintf("-------------------------------------\n");
}

/**
 * @brief 基本异常检测 (未校准时使用)
 * @param v_a1_mv, v_a2_mv, v_b1_mv, v_b2_mv 当前电压值
 * @param result 诊断结果
 */
void pv_basic_anomaly_detection(int v_a1_mv, int v_a2_mv, int v_b1_mv, int v_b2_mv, pv_diagnosis_result_t* result)
{
    // 计算平均电压
    int avg_voltage = (v_a1_mv + v_a2_mv + v_b1_mv + v_b2_mv) / 4;

    // 定义异常阈值：如果某个通道电压比平均值低50%以上，认为异常
    int anomaly_threshold = avg_voltage / 2;

    snprintf(result->details[result->detail_count++], 128,
            "Basic detection: avg=%dmV, threshold=%dmV", avg_voltage, anomaly_threshold);

    // 检测各通道异常
    if (v_a1_mv < anomaly_threshold && avg_voltage > 200) {
        // v_a1异常低，可能是P1或P2故障
        if (v_a2_mv < anomaly_threshold) {
            result->faulty_panels[result->fault_count++] = 2; // P2故障影响两个通道
        } else {
            result->faulty_panels[result->fault_count++] = 1; // P1故障
        }
    }

    if (v_a2_mv < anomaly_threshold && avg_voltage > 200) {
        // v_a2异常低，可能是P2或P3故障
        if (v_a1_mv >= anomaly_threshold) { // 确保不是P2重复检测
            result->faulty_panels[result->fault_count++] = 3; // P3故障
        }
    }

    if (v_b1_mv < anomaly_threshold && avg_voltage > 200) {
        // v_b1异常低，可能是P4或P5故障
        if (v_b2_mv < anomaly_threshold) {
            result->faulty_panels[result->fault_count++] = 5; // P5故障影响两个通道
        } else {
            result->faulty_panels[result->fault_count++] = 4; // P4故障
        }
    }

    if (v_b2_mv < anomaly_threshold && avg_voltage > 200) {
        // v_b2异常低，可能是P5或P6故障
        if (v_b1_mv >= anomaly_threshold) { // 确保不是P5重复检测
            result->faulty_panels[result->fault_count++] = 6; // P6故障
        }
    }

    // 更新状态
    if (result->fault_count > 0) {
        rt_strncpy(result->status, "Faulty", sizeof(result->status));
        snprintf(result->details[result->detail_count++], 128,
                "Detected %d anomalous channel(s)", result->fault_count);
    } else {
        rt_strncpy(result->status, "Healthy", sizeof(result->status));
    }
}

/**
 * @brief 使用当前ADC数据进行校准
 */
void pv_diag_calibrate_current(void)
{
    if (adc_data_getter == RT_NULL) {
        rt_kprintf("Error: ADC data getter not registered\n");
        return;
    }
    
    pv_adc_data_t adc_data;
    if (adc_data_getter(&adc_data) == RT_EOK) {
        pv_diag_calibrate(adc_data.v_a1_mv, adc_data.v_a2_mv, 
                         adc_data.v_b1_mv, adc_data.v_b2_mv);
    } else {
        rt_kprintf("Error: Failed to get ADC data for calibration\n");
    }
}

/**
 * @brief 基于"相对变化"诊断光伏阵列的故障板
 * @param v_a1_now_mv 当前 A组 V(1+2) 电压 (mV)
 * @param v_a2_now_mv 当前 A组 V(2+3) 电压 (mV)
 * @param v_b1_now_mv 当前 B组 V(4+5) 电压 (mV)
 * @param v_b2_now_mv 当前 B组 V(5+6) 电压 (mV)
 * @param result 指向诊断结果结构体的指针
 */
void pv_diagnose_panels(int v_a1_now_mv, int v_a2_now_mv, int v_b1_now_mv, int v_b2_now_mv, 
                       pv_diagnosis_result_t* result)
{
    // 初始化结果结构体
    result->fault_count = 0;
    result->detail_count = 0;
    for(int i = 0; i < 6; i++) result->faulty_panels[i] = 0;
    for(int i = 0; i < 5; i++) result->details[i][0] = '\0';
    rt_strncpy(result->status, "Healthy", sizeof(result->status));

    // 检查是否已经校准
    if (!is_calibrated) {
        rt_strncpy(result->status, "Uncalibrated", sizeof(result->status));
        snprintf(result->details[result->detail_count++], 128,
                "System not calibrated. Using basic anomaly detection.");

        // 未校准时使用基本异常检测：检测明显的电压异常
        pv_basic_anomaly_detection(v_a1_now_mv, v_a2_now_mv, v_b1_now_mv, v_b2_now_mv, result);
        return;
    }
    
    // 添加校准基线信息到报告
    snprintf(result->details[result->detail_count++], 128, 
            "Baseline delta: d(A1-B1)=%d, d(A2-B2)=%d", initial_delta_ab1, initial_delta_ab2);

    // 处理特殊情况：所有电压都接近零（夜晚或系统断开）
    if (v_a1_now_mv < 100 && v_a2_now_mv < 100 && v_b1_now_mv < 100 && v_b2_now_mv < 100) {
        rt_strncpy(result->status, "No Power", sizeof(result->status));
        snprintf(result->details[result->detail_count++], 128, 
                "All voltages are near zero. System is off.");
        return;
    }

    // 计算当前差值和偏差
    int current_delta_ab1 = v_a1_now_mv - v_b1_now_mv;
    int deviation1 = current_delta_ab1 - initial_delta_ab1;

    int current_delta_ab2 = v_a2_now_mv - v_b2_now_mv;
    int deviation2 = current_delta_ab2 - initial_delta_ab2;

    snprintf(result->details[result->detail_count++], 128, 
            "Current delta: d(A1-B1)=%d, d(A2-B2)=%d", current_delta_ab1, current_delta_ab2);
    snprintf(result->details[result->detail_count++], 128, 
            "Deviation: dev1=%d mV, dev2=%d mV (Threshold: +/-%d)", 
            deviation1, deviation2, DEVIATION_THRESHOLD_MV);

    // 诊断逻辑
    if (deviation1 < -DEVIATION_THRESHOLD_MV) { 
        if (deviation2 < -DEVIATION_THRESHOLD_MV) {
             result->faulty_panels[result->fault_count++] = 2; // P2故障
        } else {
             result->faulty_panels[result->fault_count++] = 1; // P1故障
        }
    } 
    else if (deviation1 > DEVIATION_THRESHOLD_MV) {
        if (deviation2 > DEVIATION_THRESHOLD_MV) {
            result->faulty_panels[result->fault_count++] = 5; // P5故障
        } else {
            result->faulty_panels[result->fault_count++] = 4; // P4故障
        }
    }

    if (deviation2 < -DEVIATION_THRESHOLD_MV) {
        if (deviation1 >= -DEVIATION_THRESHOLD_MV) {
            result->faulty_panels[result->fault_count++] = 3; // P3故障
        }
    } 
    else if (deviation2 > DEVIATION_THRESHOLD_MV) {
        if (deviation1 <= DEVIATION_THRESHOLD_MV) {
            result->faulty_panels[result->fault_count++] = 6; // P6故障
        }
    }
    
    // 最终总结
    if (result->fault_count > 0) {
        rt_strncpy(result->status, "Faulty", sizeof(result->status));
    }
}

/**
 * @brief 使用当前ADC数据进行诊断
 * @param result 指向诊断结果结构体的指针
 * @return RT_EOK 成功, -RT_ERROR 失败
 */
rt_err_t pv_diagnose_current(pv_diagnosis_result_t* result)
{
    if (adc_data_getter == RT_NULL) {
        rt_kprintf("Error: ADC data getter not registered\n");
        return -RT_ERROR;
    }
    
    pv_adc_data_t adc_data;
    rt_err_t ret = adc_data_getter(&adc_data);
    if (ret == RT_EOK) {
        pv_diagnose_panels(adc_data.v_a1_mv, adc_data.v_a2_mv, 
                          adc_data.v_b1_mv, adc_data.v_b2_mv, result);
    }
    
    return ret;
}

/**
 * @brief 打印诊断结果
 * @param result 诊断结果结构体指针
 */
void pv_print_diagnosis_result(const pv_diagnosis_result_t* result)
{
    rt_kprintf("\n--- PV Diagnosis Report ---\n");
    rt_kprintf("Status: %s\n", result->status);
    
    if (result->fault_count > 0) {
        rt_kprintf("Detected %d faulty panel(s): ", result->fault_count);
        for (int i = 0; i < result->fault_count; i++) {
            rt_kprintf("P%d ", result->faulty_panels[i]);
        }
        rt_kprintf("\n");
    }
    
    rt_kprintf("\nDetails:\n");
    for (int i = 0; i < result->detail_count; i++) {
        rt_kprintf("- %s\n", result->details[i]);
    }
    rt_kprintf("---------------------------\n");
}

// MSH命令导出
static void cmd_pv_calibrate(int argc, char **argv)
{
    pv_diag_calibrate_current();
}
MSH_CMD_EXPORT_ALIAS(cmd_pv_calibrate, pv_calibrate, Calibrate PV diagnosis with current ADC values);

static void cmd_pv_diagnose(int argc, char **argv)
{
    pv_diagnosis_result_t result;
    
    if (pv_diagnose_current(&result) == RT_EOK) {
        pv_print_diagnosis_result(&result);
    } else {
        rt_kprintf("Failed to perform diagnosis\n");
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_pv_diagnose, pv_diagnose, Diagnose PV panels with current ADC values);

static void cmd_pv_status(int argc, char **argv)
{
    if (adc_data_getter == RT_NULL) {
        rt_kprintf("Error: ADC data getter not registered\n");
        return;
    }

    pv_adc_data_t adc_data;
    rt_err_t ret = adc_data_getter(&adc_data);

    if (ret != RT_EOK) {
        rt_kprintf("Error: Failed to get ADC data\n");
        return;
    }

    rt_kprintf("\n--- Current PV System Status ---\n");
    rt_kprintf("ADC Raw Values (mV):\n");
    rt_kprintf("  v_a1 (PA0): %d mV  [Panel 1+2]\n", adc_data.v_a1_mv);
    rt_kprintf("  v_a2 (PA1): %d mV  [Panel 2+3]\n", adc_data.v_a2_mv);
    rt_kprintf("  v_b1 (PB0): %d mV  [Panel 4+5]\n", adc_data.v_b1_mv);
    rt_kprintf("  v_b2 (PB1): %d mV  [Panel 5+6]\n", adc_data.v_b2_mv);

    if (is_calibrated) {
        rt_kprintf("\nCalibration Status: ✅ Calibrated\n");
        rt_kprintf("Baseline deltas: d(A1-B1)=%d, d(A2-B2)=%d mV\n",
                   initial_delta_ab1, initial_delta_ab2);

        // 显示当前偏差
        int current_delta_ab1 = adc_data.v_a1_mv - adc_data.v_b1_mv;
        int current_delta_ab2 = adc_data.v_a2_mv - adc_data.v_b2_mv;
        int deviation1 = current_delta_ab1 - initial_delta_ab1;
        int deviation2 = current_delta_ab2 - initial_delta_ab2;

        rt_kprintf("Current deltas: d(A1-B1)=%d, d(A2-B2)=%d mV\n",
                   current_delta_ab1, current_delta_ab2);
        rt_kprintf("Deviations: dev1=%d, dev2=%d mV (Threshold: ±%d)\n",
                   deviation1, deviation2, DEVIATION_THRESHOLD_MV);
    } else {
        rt_kprintf("\nCalibration Status: ❌ Not Calibrated\n");
        rt_kprintf("Please run 'pv_calibrate' first\n");
    }
    rt_kprintf("--------------------------------\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_pv_status, pv_status, Show current PV system status and ADC values);
