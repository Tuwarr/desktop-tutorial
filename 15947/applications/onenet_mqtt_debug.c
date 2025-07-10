/* applications/onenet_mqtt_debug.c */
/* OneNET MQTT连接调试工具 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

#ifdef PKG_USING_ONENET
#include <onenet.h>

/* 外部变量声明 */
extern struct rt_onenet_info onenet_info;
#endif

/**
 * @brief 显示实际的MQTT连接参数
 */
int debug_mqtt_params(void)
{
    rt_kprintf("\n🔍 === MQTT Connection Parameters Debug ===\n");
    
#ifdef PKG_USING_ONENET
    rt_kprintf("📋 RT-Thread Configuration (rtconfig.h):\n");
    rt_kprintf("   ONENET_INFO_DEVID: '%s'\n", ONENET_INFO_DEVID);
    rt_kprintf("   ONENET_INFO_PROID: '%s'\n", ONENET_INFO_PROID);
    rt_kprintf("   ONENET_INFO_AUTH:  '%s'\n", ONENET_INFO_AUTH);
    
    rt_kprintf("\n🔍 Runtime onenet_info Structure:\n");
    rt_kprintf("   device_id: '%s'\n", onenet_info.device_id);
    rt_kprintf("   pro_id:    '%s'\n", onenet_info.pro_id);
    rt_kprintf("   auth_info: '%s'\n", onenet_info.auth_info);
    rt_kprintf("   server_uri:'%s'\n", onenet_info.server_uri);
    
    rt_kprintf("\n🌐 MQTT Connection Details:\n");
    rt_kprintf("   Server:    183.230.40.96:1883\n");
    rt_kprintf("   Client ID: '%s'\n", onenet_info.device_id);
    rt_kprintf("   Username:  '%s'\n", onenet_info.pro_id);
    rt_kprintf("   Password:  '%s'\n", onenet_info.auth_info);
    
    /* 检查字符串长度 */
    rt_kprintf("\n📏 String Length Check:\n");
    rt_kprintf("   device_id length: %d\n", rt_strlen(onenet_info.device_id));
    rt_kprintf("   pro_id length:    %d\n", rt_strlen(onenet_info.pro_id));
    rt_kprintf("   auth_info length: %d\n", rt_strlen(onenet_info.auth_info));
    
    /* 检查是否有空格或特殊字符 */
    rt_kprintf("\n🔍 Character Analysis:\n");
    
    const char *device_id = onenet_info.device_id;
    const char *pro_id = onenet_info.pro_id;
    
    if (device_id[0] == ' ') {
        rt_kprintf("   ⚠️  Device ID starts with space!\n");
    }
    
    if (pro_id[0] == ' ') {
        rt_kprintf("   ⚠️  Product ID starts with space!\n");
    }
    
    /* 检查Device ID是否包含空格 */
    for (int i = 0; i < rt_strlen(device_id); i++) {
        if (device_id[i] == ' ') {
            rt_kprintf("   ⚠️  Device ID contains space at position %d\n", i);
        }
    }
    
    rt_kprintf("\n💡 Expected vs Actual:\n");
    rt_kprintf("   Expected Username: 81kgVdLcL2\n");
    rt_kprintf("   Actual Username:   '%s'\n", onenet_info.pro_id);
    rt_kprintf("   Match: %s\n", (rt_strcmp(onenet_info.pro_id, "81kgVdLcL2") == 0) ? "✅ YES" : "❌ NO");
    
#else
    rt_kprintf("❌ OneNET package not enabled\n");
#endif
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 检查rtconfig.h中的配置问题
 */
int check_rtconfig_issues(void)
{
    rt_kprintf("\n🔧 === rtconfig.h Issues Check ===\n");
    
#ifdef PKG_USING_ONENET
    rt_kprintf("📋 Checking for common configuration issues:\n\n");
    
    /* 检查Device ID是否有前导空格 */
    const char *devid = ONENET_INFO_DEVID;
    if (devid[0] == ' ') {
        rt_kprintf("❌ ISSUE FOUND: ONENET_INFO_DEVID has leading space!\n");
        rt_kprintf("   Current: '%s'\n", devid);
        rt_kprintf("   Should be: '2454811797'\n");
        rt_kprintf("   Fix: Remove space in rtconfig.h line 218\n\n");
    } else {
        rt_kprintf("✅ ONENET_INFO_DEVID: No leading space\n");
    }
    
    /* 检查Product ID */
    const char *proid = ONENET_INFO_PROID;
    if (rt_strcmp(proid, "81kgVdLcL2") == 0) {
        rt_kprintf("✅ ONENET_INFO_PROID: Correct value\n");
    } else {
        rt_kprintf("❌ ISSUE FOUND: ONENET_INFO_PROID mismatch!\n");
        rt_kprintf("   Current: '%s'\n", proid);
        rt_kprintf("   Should be: '81kgVdLcL2'\n\n");
    }
    
    /* 检查Auth Info格式 */
    const char *auth = ONENET_INFO_AUTH;
    if (rt_strstr(auth, "version=2018-10-31") != RT_NULL &&
        rt_strstr(auth, "res=products%2F81kgVdLcL2") != RT_NULL &&
        rt_strstr(auth, "method=sha256") != RT_NULL) {
        rt_kprintf("✅ ONENET_INFO_AUTH: Format appears correct\n");
    } else {
        rt_kprintf("❌ ISSUE FOUND: ONENET_INFO_AUTH format issue!\n");
        rt_kprintf("   Check token format and Product ID in resource\n\n");
    }
    
    rt_kprintf("🔧 If issues found:\n");
    rt_kprintf("   1. Edit rtconfig.h directly\n");
    rt_kprintf("   2. Remove any leading spaces\n");
    rt_kprintf("   3. Ensure Product ID matches token\n");
    rt_kprintf("   4. Recompile: make clean && make\n");
    
#else
    rt_kprintf("❌ OneNET package not enabled\n");
#endif
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 建议的修复方案
 */
int suggest_fixes(void)
{
    rt_kprintf("\n💡 === Suggested Fixes ===\n");
    
    rt_kprintf("🔧 Based on common OneNET authentication issues:\n\n");
    
    rt_kprintf("1️⃣  Check rtconfig.h for spaces:\n");
    rt_kprintf("   Line 218: #define ONENET_INFO_DEVID \"2454811797\"\n");
    rt_kprintf("   (No space before 2454811797)\n\n");
    
    rt_kprintf("2️⃣  Verify Product ID consistency:\n");
    rt_kprintf("   rtconfig.h: ONENET_INFO_PROID \"81kgVdLcL2\"\n");
    rt_kprintf("   Token: res=products%2F81kgVdLcL2\n");
    rt_kprintf("   Must match exactly!\n\n");
    
    rt_kprintf("3️⃣  Check OneNET platform:\n");
    rt_kprintf("   - Device status: Should be enabled\n");
    rt_kprintf("   - Product status: Should be active\n");
    rt_kprintf("   - Device Secret: Should match token generation\n\n");
    
    rt_kprintf("4️⃣  Alternative test:\n");
    rt_kprintf("   - Try creating new device on OneNET\n");
    rt_kprintf("   - Generate fresh token\n");
    rt_kprintf("   - Update all configuration\n\n");
    
    rt_kprintf("5️⃣  Network test:\n");
    rt_kprintf("   - Verify air720 can reach 183.230.40.96:1883\n");
    rt_kprintf("   - Check firewall/proxy settings\n");
    
    rt_kprintf("=====================================\n");
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(debug_mqtt_params, Debug MQTT connection parameters);
MSH_CMD_EXPORT(check_rtconfig_issues, Check rtconfig.h configuration issues);
MSH_CMD_EXPORT(suggest_fixes, Suggest fixes for OneNET authentication);
