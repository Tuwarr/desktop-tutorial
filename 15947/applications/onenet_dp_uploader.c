/* applications/onenet_dp_uploader.c */
/* OneNET数据点上传模块 - 使用标准DP格式 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pv_diagnosis.h"

#ifdef PKG_USING_ONENET
#include <onenet.h>
#endif

/* 外部函数声明 */
extern int pv_onenet_init(void);
extern rt_err_t adc_get_pv_data(pv_adc_data_t* data);

/* 故障检测模块函数声明 */
extern void pv_fault_detector_init(void);
extern int pv_fault_detection_run(void);
extern int pv_fault_get_count(void);
extern const char* pv_fault_get_multi_status_string(void);
extern rt_bool_t pv_fault_is_baseline_ready(void);
extern rt_bool_t pv_fault_get_individual_status(int pv_index);  // 获取单个PV故障状态



/* OneNET DP 配置 */
#define ONENET_DP_TOPIC         "$sys/81kgVdJcL2/voltage/dp/post/json"
#define ONENET_CLIENT_ID        "voltage"
#define ONENET_PRODUCT_ID       "81kgVdJcL2"

/* 电压数据结构 (5个光伏板) */
typedef struct {
    float va1, va2, va3;  // 节点电压 (V)
    float vb1, vb2;       // vb3已移除 - 现在只有5个光伏板
    int message_id;       // 消息ID

    /* 故障检测相关 */
    int fault_code_id;        // 主故障码 (0=正常, 1=PV1故障, 2=PV2故障, ...)
    char fault_code_str[32];  // 故障码字符串 ("PANEL_OK", "FAULT_PV1", ...)
    int fault_count;          // 故障数量
    char fault_list[64];      // 故障列表 ("PV2,PV6" 或 "NONE")

    /* 4个独立的故障字符串数据流 */
    char fault_code_str1[32]; // 第1个故障状态字符串
    char fault_code_str2[32]; // 第2个故障状态字符串
    char fault_code_str3[32]; // 第3个故障状态字符串
    char fault_code_str4[32]; // 第4个故障状态字符串
} voltage_dp_data_t;

/* 上传线程控制 */
static rt_thread_t dp_upload_thread = RT_NULL;
static rt_bool_t dp_upload_running = RT_FALSE;
static int global_message_id = 12345;  // 全局消息ID

/**
 * @brief 采集6个电压数据 - 使用现有ADC系统
 */
static int collect_voltage_dp_data(voltage_dp_data_t *data)
{
    pv_adc_data_t adc_data;
    
    /* 使用现有的ADC数据获取接口 */
    if (adc_get_pv_data(&adc_data) != RT_EOK) {
        rt_kprintf("❌ ADC数据获取失败\n");
        return -1;
    }
    
    /* 映射ADC数据到电压标识符 (mV -> V, 乘以5倍) - 按照正确的测量点映射 */
    data->va1 = (adc_data.v_a1_mv * 5.0f) / 1000.0f;  // PA0 -> va1 (PV1) (x5)
    data->va2 = (adc_data.v_a2_mv * 5.0f) / 1000.0f;  // PA1 -> va2 (PV1+PV2) (x5)
    data->va3 = (adc_data.v_c1_mv * 5.0f) / 1000.0f;  // PA6 -> va3 (PV1+PV2+PV3) (x5)
    data->vb1 = (adc_data.v_c2_mv * 5.0f) / 1000.0f;  // PA7 -> vb1 (PV4) (x5)
    data->vb2 = (adc_data.v_b1_mv * 5.0f) / 1000.0f;  // PB0 -> vb2 (PV4+PV5) (x5)
    // vb3 已移除 - 现在只有5个光伏板

    data->message_id = global_message_id++;

    /* OneNET内置故障检测 - 独立运行，不依赖Enable_Voltage_Detection */
    int fault_code = pv_fault_detection_run();  // 直接运行故障检测
    data->fault_code_id = fault_code;
    data->fault_count = pv_fault_get_count();

    /* 获取故障状态字符串 */
    const char* fault_status = pv_fault_get_multi_status_string();

    /* 检查基准是否已建立 */
    if (pv_fault_is_baseline_ready()) {

        /* 基准已建立，设置故障码字符串 */
        if (data->fault_code_id == 0) {
            strcpy(data->fault_code_str, "PANEL_OK");
            strcpy(data->fault_list, "NONE");
        } else {
            /* 根据故障码设置字符串 */
            switch (data->fault_code_id) {
                case 1: strcpy(data->fault_code_str, "FAULT_PV1"); break;
                case 2: strcpy(data->fault_code_str, "FAULT_PV2"); break;
                case 3: strcpy(data->fault_code_str, "FAULT_PV3"); break;
                case 4: strcpy(data->fault_code_str, "FAULT_PV4"); break;
                case 5: strcpy(data->fault_code_str, "FAULT_PV5"); break;
                case 6: strcpy(data->fault_code_str, "FAULT_PV6"); break;
                default: strcpy(data->fault_code_str, "FAULT_UNKNOWN"); break;
            }

            /* 解析故障列表 */
            if (data->fault_count > 1) {
                /* 多故障情况，从状态字符串中提取 */
                const char* colon_pos = strchr(fault_status, ':');
                if (colon_pos) {
                    strncpy(data->fault_list, colon_pos + 2, sizeof(data->fault_list) - 1);
                    data->fault_list[sizeof(data->fault_list) - 1] = '\0';
                } else {
                    strcpy(data->fault_list, "MULTIPLE");
                }
            } else {
                /* 单故障情况 */
                snprintf(data->fault_list, sizeof(data->fault_list), "PV%d", data->fault_code_id);
            }
        }

        /* 生成4个故障字符串数据流 */
        rt_bool_t fault_status[6];
        int fault_slots[4] = {-1, -1, -1, -1};  // 存储故障PV索引，-1表示无故障
        int slot_index = 0;

        /* 收集所有故障PV */
        for (int i = 0; i < 6; i++) {
            fault_status[i] = pv_fault_get_individual_status(i);
            if (fault_status[i] && slot_index < 4) {
                fault_slots[slot_index] = i;
                slot_index++;
            }
        }

        /* 生成4个故障字符串 */
        for (int i = 0; i < 4; i++) {
            char* target_str = NULL;
            switch (i) {
                case 0: target_str = data->fault_code_str1; break;
                case 1: target_str = data->fault_code_str2; break;
                case 2: target_str = data->fault_code_str3; break;
                case 3: target_str = data->fault_code_str4; break;
            }

            if (fault_slots[i] >= 0) {
                /* 有故障 */
                snprintf(target_str, 32, "FAULT_PV%d", fault_slots[i] + 1);
            } else {
                /* 无故障 */
                strcpy(target_str, "NOTFAULT_PVOK");
            }
        }

    } else {
        /* 基准未建立 */
        data->fault_code_id = 0;
        strcpy(data->fault_code_str, "BASELINE_BUILDING");
        data->fault_count = 0;
        strcpy(data->fault_list, "NONE");

        /* 基准未建立时，所有故障字符串为基准建立中 */
        strcpy(data->fault_code_str1, "BASELINE_BUILDING");
        strcpy(data->fault_code_str2, "BASELINE_BUILDING");
        strcpy(data->fault_code_str3, "BASELINE_BUILDING");
        strcpy(data->fault_code_str4, "BASELINE_BUILDING");
    }
    
    return 0;
}

/**
 * @brief 生成OneNET DP格式的JSON数据
 */
static int generate_dp_json(voltage_dp_data_t *data, char *json_buf, size_t buf_size)
{
    /* 生成标准OneNET DP格式JSON - 包含故障检测数据和4个故障字符串 */
    int len = snprintf(json_buf, buf_size,
        "{"
        "\"id\":%d,"
        "\"dp\":{"
        "\"va1\":[{\"v\":%.3f}],"
        "\"va2\":[{\"v\":%.3f}],"
        "\"va3\":[{\"v\":%.3f}],"
        "\"vb1\":[{\"v\":%.3f}],"
        "\"vb2\":[{\"v\":%.3f}],"
        "\"fault_code_id\":[{\"v\":%d}],"
        "\"fault_code_str\":[{\"v\":\"%s\"}],"
        "\"fault_count\":[{\"v\":%d}],"
        "\"fault_list\":[{\"v\":\"%s\"}],"
        "\"fault_code_str1\":[{\"v\":\"%s\"}],"
        "\"fault_code_str2\":[{\"v\":\"%s\"}],"
        "\"fault_code_str3\":[{\"v\":\"%s\"}],"
        "\"fault_code_str4\":[{\"v\":\"%s\"}]"
        "}"
        "}",
        data->message_id,
        data->va1, data->va2, data->va3,
        data->vb1, data->vb2,
        data->fault_code_id, data->fault_code_str,
        data->fault_count, data->fault_list,
        data->fault_code_str1, data->fault_code_str2,
        data->fault_code_str3, data->fault_code_str4
    );
    
    if (len >= buf_size) {
        rt_kprintf("❌ JSON缓冲区太小\n");
        return -1;
    }
    
    return len;
}

/**
 * @brief 使用OneNET MQTT发布DP数据
 */
static int publish_dp_data(voltage_dp_data_t *data)
{
#ifdef PKG_USING_ONENET
    char json_payload[512];
    
    /* 生成JSON数据 */
    int json_len = generate_dp_json(data, json_payload, sizeof(json_payload));
    if (json_len < 0) {
        return -1;
    }
    
    rt_kprintf("Publishing DP data to OneNET:\n");
    rt_kprintf("   Topic: %s\n", ONENET_DP_TOPIC);
    rt_kprintf("   Payload: %s\n", json_payload);

    /* 使用OneNET MQTT发布 */
    if (onenet_mqtt_publish(ONENET_DP_TOPIC, (uint8_t*)json_payload, json_len) == 0) {
        rt_kprintf("SUCCESS: DP data published\n");
        return 0;
    } else {
        rt_kprintf("ERROR: DP data publish failed\n");
        return -1;
    }
#else
    rt_kprintf("❌ OneNET未启用\n");
    return -1;
#endif
}

/**
 * @brief DP数据上传线程
 */
static void dp_upload_thread_entry(void *parameter)
{
    rt_kprintf("OneNET DP Upload Thread Started\n");
    rt_kprintf("Upload Interval: 2 seconds\n");
    rt_kprintf("Format: OneNET Standard DP Format\n");
    rt_kprintf("Built-in Fault Detection: Enabled\n");

    /* 初始化内置故障检测器 */
    pv_fault_detector_init();
    rt_kprintf("Fault detector initialized. Establishing baseline...\n");
    
    while (dp_upload_running) {
        voltage_dp_data_t dp_data;
        
        /* 采集电压数据 */
        if (collect_voltage_dp_data(&dp_data) == 0) {
            rt_kprintf("\n=== Voltage Data Collection (x5) ===\n");
            rt_kprintf("   va1: %dmV  va2: %dmV  va3: %dmV\n",
                       (int)(dp_data.va1 * 1000), (int)(dp_data.va2 * 1000), (int)(dp_data.va3 * 1000));
            rt_kprintf("   vb1: %dmV  vb2: %dmV\n",
                       (int)(dp_data.vb1 * 1000), (int)(dp_data.vb2 * 1000));
            rt_kprintf("   Message ID: %d\n", dp_data.message_id);

            /* 显示故障检测信息 */
            rt_kprintf("=== Fault Detection Status ===\n");
            rt_kprintf("   Fault Code ID: %d\n", dp_data.fault_code_id);
            rt_kprintf("   Fault Code Str: %s\n", dp_data.fault_code_str);
            rt_kprintf("   Fault Count: %d\n", dp_data.fault_count);
            rt_kprintf("   Fault List: %s\n", dp_data.fault_list);
            rt_kprintf("=== Fault Code Strings ===\n");
            rt_kprintf("   Fault Code Str1: %s\n", dp_data.fault_code_str1);
            rt_kprintf("   Fault Code Str2: %s\n", dp_data.fault_code_str2);
            rt_kprintf("   Fault Code Str3: %s\n", dp_data.fault_code_str3);
            rt_kprintf("   Fault Code Str4: %s\n", dp_data.fault_code_str4);
            
            /* 发布到OneNET */
            if (publish_dp_data(&dp_data) == 0) {
                rt_kprintf("SUCCESS: Data uploaded\n");
            } else {
                rt_kprintf("ERROR: Data upload failed\n");
            }
        } else {
            rt_kprintf("ERROR: Voltage data collection failed\n");
        }

        /* 等待下一个上传周期 */
        rt_kprintf("Waiting 2 seconds...\n\n");
        for (int i = 0; i < 2 && dp_upload_running; i++) {
            rt_thread_mdelay(1000);
        }
    }
    
    rt_kprintf("OneNET DP Upload Thread Stopped\n");
}

/**
 * @brief 测试DP格式数据生成
 */
int test_dp_json_format(void)
{
    rt_kprintf("\n🧪 === DP格式JSON测试 ===\n");
    
    voltage_dp_data_t test_data = {
        .va1 = 6.170, .va2 = 11.725, .va3 = 17.280,  // 示例：原始值x5
        .vb1 = 22.835, .vb2 = 28.390,                // vb3已移除
        .message_id = 12345,
        .fault_code_id = 2,                           // 测试：PV2主故障
        .fault_code_str = "FAULT_PV2",
        .fault_count = 2,                             // 测试：多故障
        .fault_list = "PV2,PV3",
        .fault_code_str1 = "FAULT_PV2",               // 第1个故障：PV2
        .fault_code_str2 = "FAULT_PV3",               // 第2个故障：PV3
        .fault_code_str3 = "NOTFAULT_PVOK",           // 第3个：无故障
        .fault_code_str4 = "NOTFAULT_PVOK"            // 第4个：无故障
    };
    
    char json_buf[512];
    int len = generate_dp_json(&test_data, json_buf, sizeof(json_buf));
    
    if (len > 0) {
        rt_kprintf("✅ JSON生成成功 (%d字节):\n", len);
        rt_kprintf("%s\n", json_buf);
        return 0;
    } else {
        rt_kprintf("❌ JSON生成失败\n");
        return -1;
    }
}

/**
 * @brief 测试单次DP数据上传
 */
int test_dp_upload_once(void)
{
    rt_kprintf("\n🧪 === 单次DP数据上传测试 ===\n");
    
    voltage_dp_data_t dp_data;
    
    /* 采集真实电压数据 */
    if (collect_voltage_dp_data(&dp_data) == 0) {
        rt_kprintf("Collected voltage data (x5):\n");
        rt_kprintf("   va1: %dmV  va2: %dmV  va3: %dmV\n",
                   (int)(dp_data.va1 * 1000), (int)(dp_data.va2 * 1000), (int)(dp_data.va3 * 1000));
        rt_kprintf("   vb1: %dmV  vb2: %dmV\n",
                   (int)(dp_data.vb1 * 1000), (int)(dp_data.vb2 * 1000));

        /* 显示故障检测信息 */
        rt_kprintf("Fault Detection Status:\n");
        rt_kprintf("   Fault Code ID: %d\n", dp_data.fault_code_id);
        rt_kprintf("   Fault Code Str: %s\n", dp_data.fault_code_str);
        rt_kprintf("   Fault Count: %d\n", dp_data.fault_count);
        rt_kprintf("   Fault List: %s\n", dp_data.fault_list);
        rt_kprintf("Fault Code Strings:\n");
        rt_kprintf("   Fault Code Str1: %s\n", dp_data.fault_code_str1);
        rt_kprintf("   Fault Code Str2: %s\n", dp_data.fault_code_str2);
        rt_kprintf("   Fault Code Str3: %s\n", dp_data.fault_code_str3);
        rt_kprintf("   Fault Code Str4: %s\n", dp_data.fault_code_str4);

        /* 上传到OneNET */
        return publish_dp_data(&dp_data);
    } else {
        rt_kprintf("❌ 电压数据采集失败\n");
        return -1;
    }
}

/**
 * @brief 启动DP数据自动上传
 */
int start_dp_upload(void)
{
    rt_kprintf("\n=== Start OneNET DP Upload ===\n");
    rt_kprintf("📊 Built-in fault detection enabled\n");
    rt_kprintf("🔧 Independent operation (no Enable_Voltage_Detection required)\n");

    if (dp_upload_running) {
        rt_kprintf("WARNING: DP upload already running\n");
        return 0;
    }
    
    dp_upload_running = RT_TRUE;
    
    /* 创建DP上传线程 */
    dp_upload_thread = rt_thread_create("dp_upload",
                                       dp_upload_thread_entry,
                                       RT_NULL,
                                       4096,
                                       15,
                                       20);
    
    if (dp_upload_thread != RT_NULL) {
        rt_thread_startup(dp_upload_thread);
        rt_kprintf("SUCCESS: OneNET DP upload started\n");
        rt_kprintf("Using standard DP format: $sys/81kgVdJcL2/voltage/dp/post/json\n");
        return 0;
    } else {
        dp_upload_running = RT_FALSE;
        rt_kprintf("❌ 创建DP上传线程失败\n");
        return -1;
    }
}

/**
 * @brief 停止DP数据上传
 */
int stop_dp_upload(void)
{
    rt_kprintf("\n🛑 === 停止OneNET DP上传 ===\n");
    
    if (!dp_upload_running) {
        rt_kprintf("⚠️  DP上传未运行\n");
        return 0;
    }
    
    dp_upload_running = RT_FALSE;
    
    if (dp_upload_thread != RT_NULL) {
        rt_thread_mdelay(2000);  // 等待线程结束
        rt_kprintf("✅ OneNET DP上传已停止\n");
        dp_upload_thread = RT_NULL;
    }
    
    return 0;
}

/**
 * @brief 查看DP上传状态
 */
int dp_upload_status(void)
{
    rt_kprintf("\n📊 === OneNET DP上传状态 ===\n");
    
    if (dp_upload_running) {
        rt_kprintf("Status: RUNNING\n");
        rt_kprintf("Data Points: 5 voltages (va1,va2,va3,vb1,vb2) x5 amplified\n");
        rt_kprintf("Format: OneNET Standard DP Format\n");
        rt_kprintf("Topic: %s\n", ONENET_DP_TOPIC);
        rt_kprintf("Interval: 2 seconds\n");
        rt_kprintf("Current Message ID: %d\n", global_message_id);
    } else {
        rt_kprintf("Status: STOPPED\n");
        rt_kprintf("Use 'start_dp_upload' to start\n");
    }
    
    rt_kprintf("=====================================\n");
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(test_dp_json_format, Test DP JSON format generation);
MSH_CMD_EXPORT(test_dp_upload_once, Test single DP data upload);
MSH_CMD_EXPORT(start_dp_upload, Start OneNET DP data upload);
MSH_CMD_EXPORT(stop_dp_upload, Stop OneNET DP data upload);
MSH_CMD_EXPORT(dp_upload_status, Check DP upload status);
