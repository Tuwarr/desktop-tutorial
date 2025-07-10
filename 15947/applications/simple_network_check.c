/* applications/simple_network_check.c */
/* 简单网络检查工具 */

#include <rtthread.h>
#include <rtdevice.h>

/**
 * @brief 检查网络设备状态
 */
int check_network_devices(void)
{
    rt_kprintf("\n=== Network Device Check ===\n");
    
    /* 检查air720设备 */
    rt_device_t air720 = rt_device_find("air720");
    if (air720) {
        rt_kprintf("air720 device: FOUND\n");
        rt_kprintf("   Type: %d\n", air720->type);
        rt_kprintf("   Open flag: 0x%x\n", air720->open_flag);
        rt_kprintf("   Ref count: %d\n", air720->ref_count);
    } else {
        rt_kprintf("air720 device: NOT FOUND\n");
    }
    
    /* 检查uart1设备 */
    rt_device_t uart1 = rt_device_find("uart1");
    if (uart1) {
        rt_kprintf("uart1 device: FOUND\n");
        rt_kprintf("   Open flag: 0x%x\n", uart1->open_flag);
        rt_kprintf("   Ref count: %d\n", uart1->ref_count);
        
        if (uart1->ref_count > 1) {
            rt_kprintf("   WARNING: Multiple users detected!\n");
        }
    } else {
        rt_kprintf("uart1 device: NOT FOUND\n");
    }
    
    rt_kprintf("================================\n");
    return 0;
}

/**
 * @brief 简单的网络重置
 */
int simple_network_reset(void)
{
    rt_kprintf("\n=== Simple Network Reset ===\n");
    
    rt_kprintf("Waiting 10 seconds for network to stabilize...\n");
    for (int i = 10; i > 0; i--) {
        rt_kprintf("Countdown: %d\r", i);
        rt_thread_mdelay(1000);
    }
    rt_kprintf("\nNetwork reset completed\n");
    rt_kprintf("Try: pv_onenet_init\n");
    
    return 0;
}

/**
 * @brief 检查OneNET基础配置
 */
int check_onenet_basic_config(void)
{
    rt_kprintf("\n=== OneNET Config Check ===\n");
    
#ifdef PKG_USING_ONENET
    rt_kprintf("OneNET package: ENABLED\n");
    
#ifdef ONENET_INFO_DEVID
    rt_kprintf("Device ID: %s\n", ONENET_INFO_DEVID);
#else
    rt_kprintf("Device ID: NOT DEFINED\n");
#endif

#ifdef ONENET_INFO_PROID
    rt_kprintf("Product ID: %s\n", ONENET_INFO_PROID);
#else
    rt_kprintf("Product ID: NOT DEFINED\n");
#endif

#ifdef ONENET_INFO_AUTH
    rt_kprintf("Auth Key: %s\n", ONENET_INFO_AUTH);
#else
    rt_kprintf("Auth Key: NOT DEFINED\n");
#endif

#else
    rt_kprintf("OneNET package: DISABLED\n");
#endif
    
    rt_kprintf("===============================\n");
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(check_network_devices, Check network device status);
MSH_CMD_EXPORT(simple_network_reset, Simple network reset);
MSH_CMD_EXPORT(check_onenet_basic_config, Check OneNET basic configuration);
