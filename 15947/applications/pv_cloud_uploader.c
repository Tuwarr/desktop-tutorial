/* applications/pv_cloud_uploader.c */
/* 光伏数据云平台上传模块 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include <string.h>
#include "pv_cloud_config.h"

/* 外部函数声明 */
extern int pv_onenet_upload_pv_batch(float va1, float va2, float va3,
                                      float vb1, float vb2, float vb3,
                                      float pv1, float pv2, float pv3,
                                      float pv4, float pv5, float pv6,
                                      int fault_g1, int fault_g2);

/* ADC配置 */
#define ADC_DEV_NAME           "adc1"
#define SAMPLE_COUNT           19      // 采样次数，与现有系统保持一致

/* ADC通道定义 - 重新映射为光伏测量点 */
#define ADC_CH_VA1    0       // PA0 -> 测量点va1 (PV1)
#define ADC_CH_VA2    1       // PA1 -> 测量点va2 (PV1+PV2)
#define ADC_CH_VA3    3       // PA6 -> 测量点va3 (PV1+PV2+PV3)
#define ADC_CH_VB1    7       // PA7 -> 测量点vb1 (PV4)
#define ADC_CH_VB2    9       // PB0 -> 测量点vb2 (PV4+PV5)
#define ADC_CH_VB3    5       // PB1 -> 测量点vb3 (PV4+PV5+PV6)

/* 参数定义 */
#define VOLTAGE_REF            3300    // ADC参考电压 (mV)
#define ADC_MAX_VALUE          65535   // 16位ADC的最大值
#define FAULT_VOLTAGE_THRESHOLD 4000  // 单块光伏板电压低于此值则认为故障 (mV)
#define UPLOAD_INTERVAL_MS     20000  // 上传间隔20秒

/* 分压系数配置 - 根据实际硬件电路调整 */
// 如果使用分压电路，请修改此值
#define VOLTAGE_DIVIDER_RATIO  1.0f   // 无分压电路时为1.0

/* 光伏数据结构 */
typedef struct {
    rt_uint32_t raw_va1, raw_va2, raw_va3;  // 原始ADC值
    rt_uint32_t raw_vb1, raw_vb2, raw_vb3;
    
    rt_uint32_t volt_va1, volt_va2, volt_va3;  // 节点电压 (mV)
    rt_uint32_t volt_vb1, volt_vb2, volt_vb3;
    
    rt_uint32_t volt_pv1, volt_pv2, volt_pv3;  // 单块光伏板电压 (mV)
    rt_uint32_t volt_pv4, volt_pv5, volt_pv6;
    
    int fault_g1, fault_g2;  // 故障码
} pv_data_t;

/* 内部函数声明 */
static int collect_pv_data(pv_data_t *pv_data);

/* 全局变量 */
static rt_thread_t pv_upload_thread = RT_NULL;
static rt_bool_t upload_enabled = RT_FALSE;

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
        rt_thread_mdelay(1);
    }

    /* 关闭ADC通道 */
    rt_adc_disable(adc_dev, channel);

    return sum / count;
}

/**
 * @brief 读取所有光伏测量点数据
 */
static rt_err_t read_pv_data(pv_data_t *data)
{
    rt_adc_device_t adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL) {
        rt_kprintf("Error: can't find ADC device %s\n", ADC_DEV_NAME);
        return -RT_ERROR;
    }

    /* 读取所有节点的原始ADC值 */
    data->raw_va1 = adc_read_average(adc_dev, ADC_CH_VA1, SAMPLE_COUNT);
    data->raw_va2 = adc_read_average(adc_dev, ADC_CH_VA2, SAMPLE_COUNT);
    data->raw_va3 = adc_read_average(adc_dev, ADC_CH_VA3, SAMPLE_COUNT);
    data->raw_vb1 = adc_read_average(adc_dev, ADC_CH_VB1, SAMPLE_COUNT);
    data->raw_vb2 = adc_read_average(adc_dev, ADC_CH_VB2, SAMPLE_COUNT);
    data->raw_vb3 = adc_read_average(adc_dev, ADC_CH_VB3, SAMPLE_COUNT);

    /* 将ADC值转换为实际节点电压 (mV) */
    data->volt_va1 = (rt_uint32_t)((data->raw_va1 * VOLTAGE_REF * VOLTAGE_DIVIDER_RATIO) / ADC_MAX_VALUE);
    data->volt_va2 = (rt_uint32_t)((data->raw_va2 * VOLTAGE_REF * VOLTAGE_DIVIDER_RATIO) / ADC_MAX_VALUE);
    data->volt_va3 = (rt_uint32_t)((data->raw_va3 * VOLTAGE_REF * VOLTAGE_DIVIDER_RATIO) / ADC_MAX_VALUE);
    data->volt_vb1 = (rt_uint32_t)((data->raw_vb1 * VOLTAGE_REF * VOLTAGE_DIVIDER_RATIO) / ADC_MAX_VALUE);
    data->volt_vb2 = (rt_uint32_t)((data->raw_vb2 * VOLTAGE_REF * VOLTAGE_DIVIDER_RATIO) / ADC_MAX_VALUE);
    data->volt_vb3 = (rt_uint32_t)((data->raw_vb3 * VOLTAGE_REF * VOLTAGE_DIVIDER_RATIO) / ADC_MAX_VALUE);

    /* 计算每一块独立光伏板的电压 */
    data->volt_pv1 = data->volt_va1;
    data->volt_pv2 = (data->volt_va2 > data->volt_va1) ? (data->volt_va2 - data->volt_va1) : 0;
    data->volt_pv3 = (data->volt_va3 > data->volt_va2) ? (data->volt_va3 - data->volt_va2) : 0;
    data->volt_pv4 = data->volt_vb1;
    data->volt_pv5 = (data->volt_vb2 > data->volt_vb1) ? (data->volt_vb2 - data->volt_vb1) : 0;
    data->volt_pv6 = (data->volt_vb3 > data->volt_vb2) ? (data->volt_vb3 - data->volt_vb2) : 0;

    return RT_EOK;
}

/**
 * @brief 光伏故障诊断
 */
static void diagnose_pv_faults(pv_data_t *data)
{
    /* 组1(PV1,PV2,PV3)的故障诊断 */
    data->fault_g1 = 0;  // 0:正常
    if (data->volt_pv1 < FAULT_VOLTAGE_THRESHOLD) {
        data->fault_g1 = 1;  // PV1故障
    } else if (data->volt_pv2 < FAULT_VOLTAGE_THRESHOLD) {
        data->fault_g1 = 2;  // PV2故障
    } else if (data->volt_pv3 < FAULT_VOLTAGE_THRESHOLD) {
        data->fault_g1 = 3;  // PV3故障
    }

    /* 组2(PV4,PV5,PV6)的故障诊断 */
    data->fault_g2 = 0;  // 0:正常
    if (data->volt_pv4 < FAULT_VOLTAGE_THRESHOLD) {
        data->fault_g2 = 4;  // PV4故障
    } else if (data->volt_pv5 < FAULT_VOLTAGE_THRESHOLD) {
        data->fault_g2 = 5;  // PV5故障
    } else if (data->volt_pv6 < FAULT_VOLTAGE_THRESHOLD) {
        data->fault_g2 = 6;  // PV6故障
    }
}

/**
 * @brief 构建JSON数据包
 */
static void build_json_payload(pv_data_t *data, char *json_buffer, size_t buffer_size)
{
    snprintf(json_buffer, buffer_size,
        "{"
            "\"params\":{"
                "\"va1\":%.3f,"
                "\"va2\":%.3f,"
                "\"va3\":%.3f,"
                "\"vb1\":%.3f,"
                "\"vb2\":%.3f,"
                "\"vb3\":%.3f,"
                "\"pv1\":%.3f,"
                "\"pv2\":%.3f,"
                "\"pv3\":%.3f,"
                "\"pv4\":%.3f,"
                "\"pv5\":%.3f,"
                "\"pv6\":%.3f,"
                "\"fault_g1\":%d,"
                "\"fault_g2\":%d,"
                "\"timestamp\":%d"
            "}"
        "}",
        data->volt_va1 / 1000.0f, data->volt_va2 / 1000.0f, data->volt_va3 / 1000.0f,
        data->volt_vb1 / 1000.0f, data->volt_vb2 / 1000.0f, data->volt_vb3 / 1000.0f,
        data->volt_pv1 / 1000.0f, data->volt_pv2 / 1000.0f, data->volt_pv3 / 1000.0f,
        data->volt_pv4 / 1000.0f, data->volt_pv5 / 1000.0f, data->volt_pv6 / 1000.0f,
        data->fault_g1, data->fault_g2,
        (int)rt_tick_get()
    );
}

/**
 * @brief 显示光伏数据
 */
static void display_pv_data(pv_data_t *data)
{
    rt_kprintf("\n=== PV Data Collection ===\n");
    rt_kprintf("Node Voltages:\n");
    rt_kprintf("  va1: %4dmV  va2: %4dmV  va3: %4dmV\n", data->volt_va1, data->volt_va2, data->volt_va3);
    rt_kprintf("  vb1: %4dmV  vb2: %4dmV  vb3: %4dmV\n", data->volt_vb1, data->volt_vb2, data->volt_vb3);
    
    rt_kprintf("Individual PV Voltages:\n");
    rt_kprintf("  PV1: %4dmV  PV2: %4dmV  PV3: %4dmV\n", data->volt_pv1, data->volt_pv2, data->volt_pv3);
    rt_kprintf("  PV4: %4dmV  PV5: %4dmV  PV6: %4dmV\n", data->volt_pv4, data->volt_pv5, data->volt_pv6);
    
    rt_kprintf("Fault Status:\n");
    rt_kprintf("  Group1: %s  Group2: %s\n", 
               data->fault_g1 ? "FAULT" : "OK",
               data->fault_g2 ? "FAULT" : "OK");
    
    if (data->fault_g1) rt_kprintf("  -> PV%d fault detected\n", data->fault_g1);
    if (data->fault_g2) rt_kprintf("  -> PV%d fault detected\n", data->fault_g2);
    
    rt_kprintf("==========================\n");
}

/**
 * @brief 云平台数据上传线程
 */
static void pv_upload_thread_entry(void *parameter)
{
    pv_data_t pv_data;
    char json_buffer[1024];
    
    rt_kprintf("PV Cloud Upload Thread Started\n");
    rt_kprintf("Upload interval: %d seconds\n", UPLOAD_INTERVAL_MS / 1000);
    
    while (upload_enabled)
    {
        /* 读取光伏数据 */
        if (read_pv_data(&pv_data) != RT_EOK) {
            rt_kprintf("Error: Failed to read PV data\n");
            rt_thread_mdelay(5000);
            continue;
        }
        
        /* 故障诊断 */
        diagnose_pv_faults(&pv_data);
        
        /* 显示数据 */
        display_pv_data(&pv_data);
        
        /* 构建JSON */
        build_json_payload(&pv_data, json_buffer, sizeof(json_buffer));
        
        rt_kprintf("JSON Payload: %s\n", json_buffer);
        
        /* 上传到云平台 */
#if PV_UPLOAD_METHOD_ONENET
        /* 使用OneNET上传 */
        if (pv_onenet_upload_pv_batch(
                pv_data.volt_va1 / 1000.0f, pv_data.volt_va2 / 1000.0f, pv_data.volt_va3 / 1000.0f,
                pv_data.volt_vb1 / 1000.0f, pv_data.volt_vb2 / 1000.0f, pv_data.volt_vb3 / 1000.0f,
                pv_data.volt_pv1 / 1000.0f, pv_data.volt_pv2 / 1000.0f, pv_data.volt_pv3 / 1000.0f,
                pv_data.volt_pv4 / 1000.0f, pv_data.volt_pv5 / 1000.0f, pv_data.volt_pv6 / 1000.0f,
                pv_data.fault_g1, pv_data.fault_g2) == 0) {
            rt_kprintf("✅ Data uploaded to OneNET successfully\n\n");
        } else {
            rt_kprintf("❌ Failed to upload data to OneNET\n\n");
        }
#else
        /* 其他上传方式 */
        rt_kprintf("📤 JSON ready for upload: %s\n", json_buffer);
        rt_kprintf("💡 Configure upload method in pv_cloud_config.h\n\n");
#endif
        
        /* 等待下次上传 */
        rt_thread_mdelay(UPLOAD_INTERVAL_MS);
    }
    
    rt_kprintf("PV Cloud Upload Thread Stopped\n");
}

/**
 * @brief 启动云平台数据上传
 */
int start_pv_cloud_upload(void)
{
    if (pv_upload_thread != RT_NULL) {
        rt_kprintf("PV cloud upload is already running\n");
        return -1;
    }
    
    upload_enabled = RT_TRUE;
    
    pv_upload_thread = rt_thread_create("pv_upload",
                                        pv_upload_thread_entry,
                                        RT_NULL,
                                        4096,  // 足够的堆栈空间
                                        RT_THREAD_PRIORITY_MAX / 2,
                                        20);
    
    if (pv_upload_thread != RT_NULL) {
        rt_thread_startup(pv_upload_thread);
        rt_kprintf("✅ PV cloud upload started successfully\n");
        return 0;
    } else {
        upload_enabled = RT_FALSE;
        rt_kprintf("❌ Failed to create PV upload thread\n");
        return -1;
    }
}

/**
 * @brief 停止云平台数据上传
 */
int stop_pv_cloud_upload(void)
{
    if (pv_upload_thread == RT_NULL) {
        rt_kprintf("PV cloud upload is not running\n");
        return -1;
    }
    
    upload_enabled = RT_FALSE;
    
    /* 等待线程结束 */
    rt_thread_mdelay(1000);
    
    if (pv_upload_thread != RT_NULL) {
        rt_thread_delete(pv_upload_thread);
        pv_upload_thread = RT_NULL;
    }
    
    rt_kprintf("✅ PV cloud upload stopped\n");
    return 0;
}

/**
 * @brief 单次测试光伏数据读取
 */
int test_pv_data_read(void)
{
    pv_data_t pv_data;
    char json_buffer[1024];
    
    rt_kprintf("\n🔬 === Single PV Data Test ===\n");
    
    /* 读取数据 */
    if (read_pv_data(&pv_data) != RT_EOK) {
        rt_kprintf("❌ Failed to read PV data\n");
        return -1;
    }
    
    /* 诊断 */
    diagnose_pv_faults(&pv_data);
    
    /* 显示 */
    display_pv_data(&pv_data);
    
    /* 构建JSON */
    build_json_payload(&pv_data, json_buffer, sizeof(json_buffer));
    rt_kprintf("JSON Output:\n%s\n", json_buffer);
    
    rt_kprintf("✅ PV data test completed\n");
    return 0;
}

/**
 * @brief 采集完整的光伏数据（包括故障诊断）
 */
static int collect_pv_data(pv_data_t *pv_data)
{
    /* 读取ADC数据并计算电压 */
    if (read_pv_data(pv_data) != RT_EOK) {
        return -1;
    }

    /* 进行故障诊断 */
    diagnose_pv_faults(pv_data);

    return 0;
}

/**
 * @brief 为生产环境上传获取光伏数据
 */
int get_pv_data_for_upload(float *va1, float *va2, float *va3,
                          float *vb1, float *vb2, float *vb3,
                          float *pv1, float *pv2, float *pv3,
                          float *pv4, float *pv5, float *pv6,
                          int *fault_g1, int *fault_g2)
{
    pv_data_t pv_data;

    /* 采集光伏数据 */
    if (collect_pv_data(&pv_data) != 0) {
        rt_kprintf("❌ Failed to collect PV data\n");
        return -1;
    }

    /* 转换为浮点数 (V) */
    *va1 = pv_data.volt_va1 / 1000.0f;
    *va2 = pv_data.volt_va2 / 1000.0f;
    *va3 = pv_data.volt_va3 / 1000.0f;
    *vb1 = pv_data.volt_vb1 / 1000.0f;
    *vb2 = pv_data.volt_vb2 / 1000.0f;
    *vb3 = pv_data.volt_vb3 / 1000.0f;

    *pv1 = pv_data.volt_pv1 / 1000.0f;
    *pv2 = pv_data.volt_pv2 / 1000.0f;
    *pv3 = pv_data.volt_pv3 / 1000.0f;
    *pv4 = pv_data.volt_pv4 / 1000.0f;
    *pv5 = pv_data.volt_pv5 / 1000.0f;
    *pv6 = pv_data.volt_pv6 / 1000.0f;

    *fault_g1 = pv_data.fault_g1;
    *fault_g2 = pv_data.fault_g2;

    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(start_pv_cloud_upload, Start PV data upload to cloud platform);
MSH_CMD_EXPORT(stop_pv_cloud_upload, Stop PV data upload);
MSH_CMD_EXPORT(test_pv_data_read, Test PV data reading and JSON generation);
