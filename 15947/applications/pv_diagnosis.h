/*
 * pv_diagnosis.h
 *
 * Header file for photovoltaic panel diagnosis system
 */

#ifndef PV_DIAGNOSIS_H
#define PV_DIAGNOSIS_H

#include <rtthread.h>

// ADC数据结构体 (低耦合接口)
typedef struct {
    int v_a1_mv;    // PA0: Voltage of Panel 1 + Panel 2 (mV)
    int v_a2_mv;    // PA1: Voltage of Panel 2 + Panel 3 (mV)
    int v_b1_mv;    // PB0: Voltage of Panel 4 + Panel 5 (mV)
    int v_b2_mv;    // PB1: Voltage of Panel 5 + Panel 6 (mV)
    int v_c1_mv;    // PA6: Additional voltage channel 1 (mV)
    int v_c2_mv;    // PA7: Additional voltage channel 2 (mV)
} pv_adc_data_t;

// 诊断结果结构体
typedef struct {
    char status[20];            // 系统状态: "Healthy", "Faulty", "Uncalibrated", "No Power"
    int faulty_panels[6];       // 故障板编号数组
    int fault_count;            // 故障板数量
    char details[5][128];       // 详细信息数组
    int detail_count;           // 详细信息条数
} pv_diagnosis_result_t;

// ADC数据获取函数指针类型 (低耦合设计)
typedef rt_err_t (*pv_adc_data_getter_t)(pv_adc_data_t* data);

// 函数声明

/**
 * @brief 注册ADC数据获取函数 (低耦合接口)
 * @param getter 指向ADC数据获取函数的指针
 */
void pv_diag_register_adc_getter(pv_adc_data_getter_t getter);

/**
 * @brief 校准诊断系统，记录初始健康状态
 * @param v_a1_mv A组串联中，板1和板2的总电压 (单位: mV)
 * @param v_a2_mv A组串联中，板2和板3的总电压 (单位: mV)
 * @param v_b1_mv B组串联中，板4和板5的总电压 (单位: mV)
 * @param v_b2_mv B组串联中，板5和板6的总电压 (单位: mV)
 */
void pv_diag_calibrate(int v_a1_mv, int v_a2_mv, int v_b1_mv, int v_b2_mv);

/**
 * @brief 使用当前ADC数据进行校准
 */
void pv_diag_calibrate_current(void);

/**
 * @brief 基于"相对变化"诊断光伏阵列的故障板
 * @param v_a1_now_mv 当前 A组 V(1+2) 电压 (mV)
 * @param v_a2_now_mv 当前 A组 V(2+3) 电压 (mV)
 * @param v_b1_now_mv 当前 B组 V(4+5) 电压 (mV)
 * @param v_b2_now_mv 当前 B组 V(5+6) 电压 (mV)
 * @param result 指向诊断结果结构体的指针
 */
void pv_diagnose_panels(int v_a1_now_mv, int v_a2_now_mv, int v_b1_now_mv, int v_b2_now_mv, 
                       pv_diagnosis_result_t* result);

/**
 * @brief 使用当前ADC数据进行诊断
 * @param result 指向诊断结果结构体的指针
 * @return RT_EOK 成功, -RT_ERROR 失败
 */
rt_err_t pv_diagnose_current(pv_diagnosis_result_t* result);

/**
 * @brief 基本异常检测 (未校准时使用)
 * @param v_a1_mv, v_a2_mv, v_b1_mv, v_b2_mv 当前电压值
 * @param result 诊断结果
 */
void pv_basic_anomaly_detection(int v_a1_mv, int v_a2_mv, int v_b1_mv, int v_b2_mv, pv_diagnosis_result_t* result);

/**
 * @brief 打印诊断结果
 * @param result 诊断结果结构体指针
 */
void pv_print_diagnosis_result(const pv_diagnosis_result_t* result);

#endif // PV_DIAGNOSIS_H
