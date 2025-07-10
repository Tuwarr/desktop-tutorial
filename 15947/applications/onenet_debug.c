/* applications/onenet_debug.c */
/* OneNET调试和诊断工具 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

#ifdef PKG_USING_ONENET
#include <onenet.h>
#endif

/**
 * @brief 显示OneNET配置信息
 */
int onenet_show_config(void)
{
    rt_kprintf("\n🔧 === OneNET Configuration Debug ===\n");
    
#ifdef PKG_USING_ONENET
    rt_kprintf("📋 RT-Thread Settings Configuration:\n");
    rt_kprintf("   ONENET_INFO_DEVID: %s\n", ONENET_INFO_DEVID);
    rt_kprintf("   ONENET_INFO_PROID: %s\n", ONENET_INFO_PROID);
    rt_kprintf("   ONENET_INFO_AUTH:  %s\n", ONENET_INFO_AUTH);
    
    rt_kprintf("\n🌐 MQTT Connection Details:\n");
    rt_kprintf("   Server: 183.230.40.96:1883\n");
    rt_kprintf("   Client ID: %s (Device ID)\n", ONENET_INFO_DEVID);
    rt_kprintf("   Username:  %s (Product ID)\n", ONENET_INFO_PROID);
    rt_kprintf("   Password:  %s (Device Secret)\n", ONENET_INFO_AUTH);
    
    rt_kprintf("\n💡 Troubleshooting Tips:\n");
    rt_kprintf("   1. Check OneNET platform device status\n");
    rt_kprintf("   2. Verify Device Secret is correct\n");
    rt_kprintf("   3. Ensure device is not disabled\n");
    rt_kprintf("   4. Check product MQTT protocol support\n");
    
#else
    rt_kprintf("❌ OneNET package not enabled\n");
#endif
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 测试OneNET认证信息
 */
int onenet_test_auth(void)
{
    rt_kprintf("\n🧪 === OneNET Authentication Test ===\n");
    
#ifdef PKG_USING_ONENET
    /* 检查配置长度 */
    rt_kprintf("📏 Configuration Length Check:\n");
    rt_kprintf("   Device ID length: %d\n", rt_strlen(ONENET_INFO_DEVID));
    rt_kprintf("   Product ID length: %d\n", rt_strlen(ONENET_INFO_PROID));
    rt_kprintf("   Auth Key length: %d\n", rt_strlen(ONENET_INFO_AUTH));
    
    /* 检查是否包含特殊字符 */
    rt_kprintf("\n🔍 Character Analysis:\n");
    
    const char *auth = ONENET_INFO_AUTH;
    rt_bool_t has_base64_chars = RT_FALSE;
    
    for (int i = 0; i < rt_strlen(auth); i++) {
        char c = auth[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
            (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=') {
            has_base64_chars = RT_TRUE;
        } else {
            rt_kprintf("   ⚠️  Unexpected character at position %d: '%c'\n", i, c);
        }
    }
    
    if (has_base64_chars) {
        rt_kprintf("   ✅ Auth key appears to be Base64 encoded\n");
    }
    
    /* 检查末尾是否有Base64填充 */
    if (auth[rt_strlen(auth)-1] == '=' || auth[rt_strlen(auth)-2] == '=') {
        rt_kprintf("   ✅ Base64 padding detected\n");
    }
    
    rt_kprintf("\n💡 Next Steps:\n");
    rt_kprintf("   1. Verify these values match OneNET platform exactly\n");
    rt_kprintf("   2. Check device status on OneNET console\n");
    rt_kprintf("   3. Try regenerating Device Secret if needed\n");
    
#else
    rt_kprintf("❌ OneNET package not enabled\n");
#endif
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief OneNET平台检查指南
 */
int onenet_platform_check(void)
{
    rt_kprintf("\n🌐 === OneNET Platform Check Guide ===\n");
    
    rt_kprintf("📋 Please verify on OneNET platform:\n\n");
    
    rt_kprintf("1️⃣  Device Status:\n");
    rt_kprintf("   • Login to OneNET console\n");
    rt_kprintf("   • Navigate to your product: 81kgVdJcL2\n");
    rt_kprintf("   • Find device: 2454811797\n");
    rt_kprintf("   • Check device status: Should be 'Online' or 'Enabled'\n\n");
    
    rt_kprintf("2️⃣  Device Secret:\n");
    rt_kprintf("   • Click on device details\n");
    rt_kprintf("   • Find 'Device Secret' or 'Authentication Key'\n");
    rt_kprintf("   • Compare with: bXBsNFQzSmNtbWo1S1ltalE2Wk5xa0Z5MG5UMktLVjk=\n");
    rt_kprintf("   • If different, update RT-Thread Settings\n\n");
    
    rt_kprintf("3️⃣  Product Configuration:\n");
    rt_kprintf("   • Check product protocol: Should support MQTT\n");
    rt_kprintf("   • Verify access permissions\n");
    rt_kprintf("   • Check if product is active\n\n");
    
    rt_kprintf("4️⃣  Network & Firewall:\n");
    rt_kprintf("   • Ensure 183.230.40.96:1883 is accessible\n");
    rt_kprintf("   • Check if MQTT traffic is blocked\n\n");
    
    rt_kprintf("5️⃣  Common Solutions:\n");
    rt_kprintf("   • Regenerate Device Secret on platform\n");
    rt_kprintf("   • Delete and recreate device\n");
    rt_kprintf("   • Check account permissions\n");
    rt_kprintf("   • Verify product quota limits\n\n");
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 创建新的Device Secret测试
 */
int onenet_test_new_secret(void)
{
    rt_kprintf("\n🔑 === Test New Device Secret ===\n");
    rt_kprintf("📋 Steps to test with new Device Secret:\n\n");
    
    rt_kprintf("1️⃣  Generate new secret on OneNET platform\n");
    rt_kprintf("2️⃣  Update RT-Thread Settings:\n");
    rt_kprintf("   • Open RT-Thread Settings\n");
    rt_kprintf("   • Go to Packages → IoT Cloud → OneNET\n");
    rt_kprintf("   • Update Device Secret field\n");
    rt_kprintf("   • Save configuration\n\n");
    
    rt_kprintf("3️⃣  Recompile and test:\n");
    rt_kprintf("   • Recompile project\n");
    rt_kprintf("   • Flash firmware\n");
    rt_kprintf("   • Run: pv_onenet_init\n\n");
    
    rt_kprintf("💡 Alternative: Manual configuration test\n");
    rt_kprintf("   You can also test by temporarily modifying rtconfig.h\n");
    rt_kprintf("   Change ONENET_INFO_AUTH to new secret\n");
    
    rt_kprintf("=====================================\n");
    return 0;
}

/**
 * @brief 完整的OneNET诊断流程
 */
int onenet_full_diagnosis(void)
{
    rt_kprintf("\n🏥 === OneNET Full Diagnosis ===\n");
    
    onenet_show_config();
    rt_thread_mdelay(1000);
    
    onenet_test_auth();
    rt_thread_mdelay(1000);
    
    onenet_platform_check();
    
    rt_kprintf("\n🎯 === Diagnosis Complete ===\n");
    rt_kprintf("💡 Most likely cause: Device Secret mismatch\n");
    rt_kprintf("🔧 Recommended action: Verify/regenerate Device Secret\n");
    rt_kprintf("📞 If issue persists: Check OneNET platform status\n");
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(onenet_show_config, Show OneNET configuration details);
MSH_CMD_EXPORT(onenet_test_auth, Test OneNET authentication info);
MSH_CMD_EXPORT(onenet_platform_check, OneNET platform verification guide);
MSH_CMD_EXPORT(onenet_test_new_secret, Guide for testing new Device Secret);
MSH_CMD_EXPORT(onenet_full_diagnosis, Complete OneNET diagnosis);
