/* applications/pv_cloud_config.h */
/* 光伏云平台配置头文件 */

#ifndef PV_CLOUD_CONFIG_H
#define PV_CLOUD_CONFIG_H

/* ========== 硬件配置 ========== */

/* ADC通道映射 - 根据您的实际硬件连接 */
#define PV_ADC_CH_VA1    0       // PA0 -> 测量点va1 (PV1)
#define PV_ADC_CH_VA2    1       // PA1 -> 测量点va2 (PV1+PV2)
#define PV_ADC_CH_VA3    3       // PA6 -> 测量点va3 (PV1+PV2+PV3)
#define PV_ADC_CH_VB1    7       // PA7 -> 测量点vb1 (PV4)
#define PV_ADC_CH_VB2    9       // PB0 -> 测量点vb2 (PV4+PV5)
#define PV_ADC_CH_VB3    5       // PB1 -> 测量点vb3 (PV4+PV5+PV6)

/* 电压参数配置 */
#define PV_VOLTAGE_REF            3300    // ADC参考电压 (mV)
#define PV_ADC_MAX_VALUE          65535   // 16位ADC的最大值
#define PV_SAMPLE_COUNT           19      // 采样次数

/* 分压电路配置 */
// 如果您的光伏板电压超过3.3V，需要使用分压电路
// 例如：10:1分压电路，则设置为10.0f
#define PV_VOLTAGE_DIVIDER_RATIO  1.0f    // 无分压电路时为1.0

/* 故障检测阈值 */
#define PV_FAULT_VOLTAGE_THRESHOLD 4000   // 单块光伏板电压低于此值认为故障 (mV)

/* ========== 云平台配置 ========== */

/* 数据上传配置 */
#define PV_UPLOAD_INTERVAL_MS     60000   // 上传间隔 (毫秒) - 改为60秒提高稳定性
#define PV_JSON_BUFFER_SIZE       1024    // JSON缓冲区大小

/* OneNET平台配置 */
#ifdef PKG_USING_ONENET
#define PV_ONENET_DEVICE_ID       "2454811797"
#define PV_ONENET_API_KEY         "bXBsNFQzSmNtbWo1S1ltalE2Wk5xa0Z5MG5UMktLVjk="
#define PV_ONENET_PRODUCT_ID      "81kgVdJcL2"
#endif

/* MQTT配置 */
#ifdef PKG_USING_PAHOMQTT
#define PV_MQTT_BROKER_HOST       "mqtt.heclouds.com"
#define PV_MQTT_BROKER_PORT       1883
#define PV_MQTT_CLIENT_ID         "pv_monitor_001"
#define PV_MQTT_USERNAME          "voltage"
#define PV_MQTT_PASSWORD          "version=2018-10-31&res=products%2F81kgVdJcL2&et=1815069830&method=sha1&sign=GeDBT2dpem870kc4yBCda3izvR8%3D"
#define PV_MQTT_TOPIC             "$sys/81kgVdJcL2 /2454811797/dp/post/json"
#endif

/* HTTP配置 */
#define PV_HTTP_SERVER_HOST       "api.heclouds.com"
#define PV_HTTP_SERVER_PORT       80
#define PV_HTTP_API_PATH          "/devices/2454811797/datapoints"

/* ========== 数据流标识符配置 ========== */

/* OneNET数据流标识符 - 必须与平台配置一致 */
#define PV_DATASTREAM_VA1         "va1"      // 节点电压va1
#define PV_DATASTREAM_VA2         "va2"      // 节点电压va2
#define PV_DATASTREAM_VA3         "va3"      // 节点电压va3
#define PV_DATASTREAM_VB1         "vb1"      // 节点电压vb1
#define PV_DATASTREAM_VB2         "vb2"      // 节点电压vb2
#define PV_DATASTREAM_VB3         "vb3"      // 节点电压vb3

#define PV_DATASTREAM_PV1         "pv1"      // 单块光伏板电压PV1
#define PV_DATASTREAM_PV2         "pv2"      // 单块光伏板电压PV2
#define PV_DATASTREAM_PV3         "pv3"      // 单块光伏板电压PV3
#define PV_DATASTREAM_PV4         "pv4"      // 单块光伏板电压PV4
#define PV_DATASTREAM_PV5         "pv5"      // 单块光伏板电压PV5
#define PV_DATASTREAM_PV6         "pv6"      // 单块光伏板电压PV6

#define PV_DATASTREAM_FAULT_G1    "fault_g1" // 组1故障码
#define PV_DATASTREAM_FAULT_G2    "fault_g2" // 组2故障码
#define PV_DATASTREAM_TIMESTAMP   "timestamp" // 时间戳

/* ========== 调试配置 ========== */

/* 调试输出控制 */
#define PV_DEBUG_ENABLE           1          // 启用调试输出
#define PV_DEBUG_VERBOSE          0          // 详细调试信息

#if PV_DEBUG_ENABLE
#define PV_LOG_I(fmt, ...)       rt_kprintf("[PV_INFO] " fmt "\n", ##__VA_ARGS__)
#define PV_LOG_W(fmt, ...)       rt_kprintf("[PV_WARN] " fmt "\n", ##__VA_ARGS__)
#define PV_LOG_E(fmt, ...)       rt_kprintf("[PV_ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define PV_LOG_I(fmt, ...)
#define PV_LOG_W(fmt, ...)
#define PV_LOG_E(fmt, ...)
#endif

#if PV_DEBUG_VERBOSE
#define PV_LOG_D(fmt, ...)       rt_kprintf("[PV_DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define PV_LOG_D(fmt, ...)
#endif

/* ========== 功能开关 ========== */

/* 功能模块开关 */
#define PV_ENABLE_FAULT_DETECTION 1          // 启用故障检测
#define PV_ENABLE_DATA_LOGGING    1          // 启用数据记录
#define PV_ENABLE_CLOUD_UPLOAD    1          // 启用云平台上传
#define PV_ENABLE_LOCAL_STORAGE   0          // 启用本地存储

/* 上传方式选择 */
#define PV_UPLOAD_METHOD_ONENET   1          // 使用OneNET SDK (已修复，启用)
#define PV_UPLOAD_METHOD_MQTT     0          // 使用MQTT
#define PV_UPLOAD_METHOD_HTTP     0          // 使用HTTP (禁用)

/* ========== 系统配置 ========== */

/* 线程配置 */
#define PV_THREAD_STACK_SIZE      4096       // 线程堆栈大小
#define PV_THREAD_PRIORITY        (RT_THREAD_PRIORITY_MAX / 2)
#define PV_THREAD_TIMESLICE       20

/* 内存配置 */
#define PV_MAX_RETRY_COUNT        3          // 最大重试次数
#define PV_NETWORK_TIMEOUT_MS     10000      // 网络超时时间

/* ========== 用户自定义配置区域 ========== */

/*
 * 用户可以在这里添加自定义的配置参数
 * 例如：特殊的算法参数、自定义的数据流等
 */

/* 自定义光伏板参数 */
#define PV_PANEL_RATED_VOLTAGE    12000      // 光伏板额定电压 (mV)
#define PV_PANEL_COUNT_GROUP1     3          // 组1光伏板数量
#define PV_PANEL_COUNT_GROUP2     3          // 组2光伏板数量

/* 自定义故障检测参数 */
#define PV_FAULT_CHECK_INTERVAL   5000       // 故障检测间隔 (ms)
#define PV_FAULT_RECOVERY_THRESHOLD 5000     // 故障恢复阈值 (mV)

/* 自定义数据处理参数 */
#define PV_DATA_SMOOTH_FACTOR     0.8f       // 数据平滑系数
#define PV_VOLTAGE_CALIBRATION    1.0f       // 电压校准系数

#endif /* PV_CLOUD_CONFIG_H */
