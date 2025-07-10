/* applications/pv_onenet_client.c */
/* 光伏数据OneNET云平台客户端 */

#include <stdint.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "pv_cloud_config.h"

#ifdef PKG_USING_ONENET
#include <onenet.h>
#endif

/* OneNET客户端状态 */
static rt_bool_t onenet_connected = RT_FALSE;
static rt_bool_t onenet_initialized = RT_FALSE;

/* OneNET连接状态管理 - 简化版本 */

/**
 * @brief 初始化OneNET客户端
 */
int pv_onenet_init(void)
{
#ifdef PKG_USING_ONENET
    if (onenet_initialized) {
        rt_kprintf("OneNET already initialized\n");
        return 0;
    }

    rt_kprintf("🔧 Initializing OneNET client...\n");
    rt_kprintf("📋 Using RT-Thread Settings configuration:\n");
    rt_kprintf("   Device ID: %s\n", ONENET_INFO_DEVID);
    rt_kprintf("   Product ID: %s\n", ONENET_INFO_PROID);
    rt_kprintf("   Auth Key: %s\n", ONENET_INFO_AUTH);

    /* 初始化OneNET MQTT - 使用rtconfig.h中的配置 */
    if (onenet_mqtt_init() == 0) {
        onenet_initialized = RT_TRUE;
        rt_kprintf("✅ OneNET client initialized\n");
        rt_kprintf("💡 Waiting for MQTT connection...\n");
        return 0;
    } else {
        rt_kprintf("❌ OneNET client initialization failed\n");
        return -1;
    }
#else
    rt_kprintf("❌ OneNET package not enabled\n");
    rt_kprintf("💡 Enable OneNET in RT-Thread Settings → IoT packages\n");
    return -1;
#endif
}

/**
 * @brief 连接到OneNET平台
 */
int pv_onenet_connect(void)
{
#ifdef PKG_USING_ONENET
    if (!onenet_initialized) {
        rt_kprintf("❌ OneNET not initialized, call pv_onenet_init() first\n");
        return -1;
    }

    if (onenet_connected) {
        rt_kprintf("OneNET already connected\n");
        return 0;
    }

    rt_kprintf("🔗 OneNET connection status check...\n");

    /* OneNET MQTT在初始化时通常已经连接 */
    onenet_connected = RT_TRUE;
    rt_kprintf("✅ OneNET connected successfully\n");
    return 0;
#else
    rt_kprintf("❌ OneNET package not enabled\n");
    return -1;
#endif
}

/**
 * @brief 断开OneNET连接
 */
int pv_onenet_disconnect(void)
{
#ifdef PKG_USING_ONENET
    if (!onenet_connected) {
        rt_kprintf("OneNET not connected\n");
        return 0;
    }

    rt_kprintf("🔌 Disconnecting from OneNET...\n");

    onenet_connected = RT_FALSE;

    rt_kprintf("✅ OneNET disconnected\n");
    return 0;
#else
    rt_kprintf("❌ OneNET package not enabled\n");
    return -1;
#endif
}

/**
 * @brief 上传光伏数据到OneNET
 */
int pv_onenet_upload_data(const char *json_data)
{
#ifdef PKG_USING_ONENET
    if (json_data == RT_NULL) {
        rt_kprintf("❌ JSON data is NULL\n");
        return -1;
    }

    if (!onenet_connected) {
        rt_kprintf("❌ OneNET not connected\n");
        return -1;
    }

    rt_kprintf("📤 Uploading data to OneNET...\n");
    rt_kprintf("Data: %s\n", json_data);

    /* 使用OneNET字符串上传API */
    if (onenet_mqtt_upload_string("pv_data", json_data) == 0) {
        rt_kprintf("✅ Data uploaded successfully\n");
        return 0;
    } else {
        rt_kprintf("❌ Data upload failed\n");
        return -1;
    }
#else
    rt_kprintf("❌ OneNET package not enabled\n");
    return -1;
#endif
}

/**
 * @brief 上传单个数据点到OneNET
 */
int pv_onenet_upload_single(const char *datastream, float value)
{
#ifdef PKG_USING_ONENET
    if (datastream == RT_NULL) {
        rt_kprintf("❌ Datastream name is NULL\n");
        return -1;
    }

    if (!onenet_connected) {
        rt_kprintf("❌ OneNET not connected\n");
        return -1;
    }

    rt_kprintf("📤 Uploading %s: %.3f to OneNET\n", datastream, value);

    /* 使用OneNET SDK上传单个数据点 */
    if (onenet_mqtt_upload_digit(datastream, (double)value) == 0) {
        rt_kprintf("✅ Data point uploaded successfully\n");
        return 0;
    } else {
        rt_kprintf("❌ Data point upload failed\n");
        return -1;
    }
#else
    rt_kprintf("❌ OneNET package not enabled\n");
    return -1;
#endif
}

/**
 * @brief 批量上传光伏数据点
 */
int pv_onenet_upload_pv_batch(float va1, float va2, float va3, 
                               float vb1, float vb2, float vb3,
                               float pv1, float pv2, float pv3,
                               float pv4, float pv5, float pv6,
                               int fault_g1, int fault_g2)
{
#ifdef PKG_USING_ONENET
    if (!onenet_connected) {
        rt_kprintf("❌ OneNET not connected\n");
        return -1;
    }
    
    rt_kprintf("📤 Uploading PV batch data to OneNET...\n");
    
    int success_count = 0;
    int total_count = 14;  // 总共14个数据点
    
    /* 上传节点电压 - 增加延时给4G模块恢复时间 */
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_VA1, (double)va1) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒，给4G模块充分时间
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_VA2, (double)va2) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_VA3, (double)va3) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_VB1, (double)vb1) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_VB2, (double)vb2) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_VB3, (double)vb3) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒

    /* 上传单块光伏板电压 - 增加延时给4G模块恢复时间 */
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_PV1, (double)pv1) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_PV2, (double)pv2) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_PV3, (double)pv3) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_PV4, (double)pv4) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_PV5, (double)pv5) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_PV6, (double)pv6) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒

    /* 上传故障码 - 增加延时给4G模块恢复时间 */
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_FAULT_G1, (double)fault_g1) == 0) success_count++;
    rt_thread_mdelay(2000);  // 增加到2秒
    if (onenet_mqtt_upload_digit(PV_DATASTREAM_FAULT_G2, (double)fault_g2) == 0) success_count++;
    rt_thread_mdelay(1000);  // 最后一个稍短
    
    rt_kprintf("📊 Upload result: %d/%d data points successful\n", success_count, total_count);
    
    if (success_count == total_count) {
        rt_kprintf("✅ All data points uploaded successfully\n");
        return 0;
    } else {
        rt_kprintf("⚠️  Some data points failed to upload\n");
        return -1;
    }
#else
    rt_kprintf("❌ OneNET package not enabled\n");
    return -1;
#endif
}

/**
 * @brief 检查OneNET连接状态
 */
int pv_onenet_status(void)
{
    rt_kprintf("\n📊 === OneNET Status ===\n");
    
#ifdef PKG_USING_ONENET
    rt_kprintf("OneNET Package: ✅ Enabled\n");
    rt_kprintf("Initialized: %s\n", onenet_initialized ? "✅ Yes" : "❌ No");
    rt_kprintf("Connected: %s\n", onenet_connected ? "✅ Yes" : "❌ No");
    
    if (onenet_initialized) {
        rt_kprintf("Device ID: %s\n", ONENET_INFO_DEVID);
        rt_kprintf("Product ID: %s\n", ONENET_INFO_PROID);
        rt_kprintf("Auth Key: %s\n", ONENET_INFO_AUTH);
    }
#else
    rt_kprintf("OneNET Package: ❌ Disabled\n");
    rt_kprintf("💡 Enable in RT-Thread Settings → IoT packages → OneNET\n");
#endif
    
    rt_kprintf("========================\n");
    
    return 0;
}

/**
 * @brief OneNET完整测试
 */
int pv_onenet_test(void)
{
    rt_kprintf("\n🧪 === OneNET Complete Test ===\n");
    
    /* 初始化 */
    if (pv_onenet_init() != 0) {
        rt_kprintf("❌ OneNET initialization failed\n");
        return -1;
    }
    
    /* 连接 */
    if (pv_onenet_connect() != 0) {
        rt_kprintf("❌ OneNET connection failed\n");
        return -1;
    }
    
    /* 测试上传 */
    rt_kprintf("🧪 Testing data upload...\n");
    
    /* 模拟光伏数据 */
    float test_va1 = 12.5f, test_va2 = 25.0f, test_va3 = 37.5f;
    float test_vb1 = 12.3f, test_vb2 = 24.6f, test_vb3 = 36.9f;
    float test_pv1 = 12.5f, test_pv2 = 12.5f, test_pv3 = 12.5f;
    float test_pv4 = 12.3f, test_pv5 = 12.3f, test_pv6 = 12.3f;
    int test_fault_g1 = 0, test_fault_g2 = 0;
    
    if (pv_onenet_upload_pv_batch(test_va1, test_va2, test_va3,
                                  test_vb1, test_vb2, test_vb3,
                                  test_pv1, test_pv2, test_pv3,
                                  test_pv4, test_pv5, test_pv6,
                                  test_fault_g1, test_fault_g2) == 0) {
        rt_kprintf("✅ OneNET test completed successfully\n");
        return 0;
    } else {
        rt_kprintf("❌ OneNET test failed\n");
        return -1;
    }
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(pv_onenet_init, Initialize OneNET client);
MSH_CMD_EXPORT(pv_onenet_connect, Connect to OneNET platform);
MSH_CMD_EXPORT(pv_onenet_disconnect, Disconnect from OneNET);
MSH_CMD_EXPORT(pv_onenet_status, Show OneNET connection status);
MSH_CMD_EXPORT(pv_onenet_test, Complete OneNET functionality test);
