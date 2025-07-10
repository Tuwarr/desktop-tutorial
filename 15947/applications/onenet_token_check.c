/* applications/onenet_token_check.c */
/* OneNET Token时间戳检查工具 */

#include <rtthread.h>
#include <rtdevice.h>
#include <time.h>

/**
 * @brief 检查OneNET token时间戳
 */
int check_onenet_token_time(void)
{
    rt_kprintf("\n🕐 === OneNET Token Time Check ===\n");
    
    /* 从当前token中提取时间戳 */
    const char *current_token = "version=2018-10-31&res=products%2F81kgVdJcL2&et=1815130683&method=sha1&sign=6f2VsyTv%2FPNqUStGVDpSNz2BuMo%3D";
    
    rt_kprintf("📋 Current Token Analysis:\n");
    rt_kprintf("Token: %s\n\n", current_token);
    
    /* 提取时间戳 */
    rt_uint32_t token_timestamp = 1815130683;  // et=1815130683
    
    rt_kprintf("⏰ Token Timestamp: %u\n", token_timestamp);
    
    /* 获取当前时间戳 */
    time_t current_time = time(RT_NULL);
    rt_kprintf("⏰ Current Time: %u\n", (rt_uint32_t)current_time);
    
    /* 检查是否过期 */
    if (current_time < token_timestamp) {
        rt_uint32_t remaining = token_timestamp - current_time;
        rt_uint32_t days = remaining / (24 * 3600);
        rt_uint32_t hours = (remaining % (24 * 3600)) / 3600;
        
        rt_kprintf("✅ Token is VALID\n");
        rt_kprintf("⏳ Remaining time: %u days, %u hours\n", days, hours);
    } else {
        rt_uint32_t expired = current_time - token_timestamp;
        rt_uint32_t days = expired / (24 * 3600);
        
        rt_kprintf("❌ Token is EXPIRED\n");
        rt_kprintf("⏳ Expired %u days ago\n", days);
    }
    
    rt_kprintf("\n💡 Token Format Analysis:\n");
    rt_kprintf("   version: 2018-10-31\n");
    rt_kprintf("   resource: products/81kgVdJcL2\n");
    rt_kprintf("   expiry: 1815130683 (Unix timestamp)\n");
    rt_kprintf("   method: sha1\n");
    rt_kprintf("   signature: 6f2VsyTv/PNqUStGVDpSNz2BuMo=\n");
    
    rt_kprintf("\n🔧 If token is expired:\n");
    rt_kprintf("   1. Use token.exe tool to generate new token\n");
    rt_kprintf("   2. Update ONENET_INFO_AUTH in rtconfig.h\n");
    rt_kprintf("   3. Recompile and test\n");
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 显示token生成指南
 */
int onenet_token_guide(void)
{
    rt_kprintf("\n🔑 === OneNET Token Generation Guide ===\n");
    
    rt_kprintf("📋 Steps to generate new token:\n\n");
    
    rt_kprintf("1️⃣  Locate token tool:\n");
    rt_kprintf("   Path: packages/onenet-latest/tools/token.exe\n\n");
    
    rt_kprintf("2️⃣  Run token tool:\n");
    rt_kprintf("   > cd packages/onenet-latest/tools/\n");
    rt_kprintf("   > token.exe\n\n");
    
    rt_kprintf("3️⃣  Input parameters:\n");
    rt_kprintf("   Product ID: 81kgVdJcL2\n");
    rt_kprintf("   Device Name: 2454811797\n");
    rt_kprintf("   Device Secret: bXBsNFQzSmNtbWo1S1ltalE2Wk5xa0Z5MG5UMktLVjk=\n");
    rt_kprintf("   Expiry Time: (choose future date, e.g., 2030-12-31)\n\n");
    
    rt_kprintf("4️⃣  Update configuration:\n");
    rt_kprintf("   Edit rtconfig.h:\n");
    rt_kprintf("   #define ONENET_INFO_AUTH \"[NEW_TOKEN_HERE]\"\n\n");
    
    rt_kprintf("5️⃣  Recompile and test:\n");
    rt_kprintf("   > make clean && make\n");
    rt_kprintf("   > pv_onenet_init\n\n");
    
    rt_kprintf("💡 Alternative: Manual token generation\n");
    rt_kprintf("   If token.exe doesn't work, you can:\n");
    rt_kprintf("   1. Use online OneNET token generator\n");
    rt_kprintf("   2. Check OneNET documentation for token format\n");
    rt_kprintf("   3. Ensure expiry time is in future\n");
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 检查OneNET配置完整性
 */
int check_onenet_config(void)
{
    rt_kprintf("\n🔍 === OneNET Configuration Check ===\n");
    
#ifdef PKG_USING_ONENET
    rt_kprintf("📋 Current Configuration:\n");
    rt_kprintf("   Device ID: %s\n", ONENET_INFO_DEVID);
    rt_kprintf("   Product ID: %s\n", ONENET_INFO_PROID);
    rt_kprintf("   Auth Length: %d chars\n", rt_strlen(ONENET_INFO_AUTH));
    
    /* 检查配置格式 */
    const char *auth = ONENET_INFO_AUTH;
    
    rt_kprintf("\n🔍 Auth Format Check:\n");
    
    if (rt_strstr(auth, "version=") != RT_NULL) {
        rt_kprintf("   ✅ Contains version parameter\n");
    } else {
        rt_kprintf("   ❌ Missing version parameter\n");
    }
    
    if (rt_strstr(auth, "res=products") != RT_NULL) {
        rt_kprintf("   ✅ Contains resource parameter\n");
    } else {
        rt_kprintf("   ❌ Missing resource parameter\n");
    }
    
    if (rt_strstr(auth, "et=") != RT_NULL) {
        rt_kprintf("   ✅ Contains expiry time\n");
    } else {
        rt_kprintf("   ❌ Missing expiry time\n");
    }
    
    if (rt_strstr(auth, "method=") != RT_NULL) {
        rt_kprintf("   ✅ Contains method parameter\n");
    } else {
        rt_kprintf("   ❌ Missing method parameter\n");
    }
    
    if (rt_strstr(auth, "sign=") != RT_NULL) {
        rt_kprintf("   ✅ Contains signature\n");
    } else {
        rt_kprintf("   ❌ Missing signature\n");
    }
    
    /* 检查是否是旧格式 */
    if (rt_strlen(auth) < 100 && rt_strstr(auth, "version=") == RT_NULL) {
        rt_kprintf("\n⚠️  WARNING: Auth appears to be old Device Secret format!\n");
        rt_kprintf("   Current: %s\n", auth);
        rt_kprintf("   Expected: version=...&res=...&et=...&method=...&sign=...\n");
        rt_kprintf("   Action: Generate new token using token.exe\n");
    }
    
#else
    rt_kprintf("❌ OneNET package not enabled\n");
#endif
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 完整的OneNET诊断
 */
int onenet_complete_diagnosis(void)
{
    rt_kprintf("\n🏥 === OneNET Complete Diagnosis ===\n");
    
    check_onenet_config();
    rt_thread_mdelay(1000);
    
    check_onenet_token_time();
    rt_thread_mdelay(1000);
    
    onenet_token_guide();
    
    rt_kprintf("\n🎯 === Diagnosis Summary ===\n");
    rt_kprintf("💡 Most likely issues:\n");
    rt_kprintf("   1. Token expired (check timestamp)\n");
    rt_kprintf("   2. Wrong Device Secret used in token generation\n");
    rt_kprintf("   3. Product/Device configuration mismatch\n");
    rt_kprintf("   4. OneNET platform device status\n");
    
    rt_kprintf("\n🔧 Recommended actions:\n");
    rt_kprintf("   1. Generate fresh token with future expiry\n");
    rt_kprintf("   2. Verify Device Secret on OneNET platform\n");
    rt_kprintf("   3. Check device status on OneNET console\n");
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(check_onenet_token_time, Check OneNET token timestamp);
MSH_CMD_EXPORT(onenet_token_guide, OneNET token generation guide);
MSH_CMD_EXPORT(check_onenet_config, Check OneNET configuration format);
MSH_CMD_EXPORT(onenet_complete_diagnosis, Complete OneNET diagnosis);
