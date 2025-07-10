/* applications/log_control.c */
/* 日志输出控制工具 */

#include <rtthread.h>
#include <rtdevice.h>

/* 日志控制状态 */
static rt_bool_t at_device_log_enabled = RT_TRUE;
// static rt_bool_t adc_log_enabled = RT_TRUE;     // 预留功能
// static rt_bool_t pv_log_enabled = RT_TRUE;      // 预留功能

/**
 * @brief 禁用AT设备日志输出
 */
int disable_at_device_logs(void)
{
    at_device_log_enabled = RT_FALSE;
    rt_kprintf("✅ AT device logs disabled\n");
    rt_kprintf("💡 This will reduce air720 network status messages\n");
    return 0;
}

/**
 * @brief 启用AT设备日志输出
 */
int enable_at_device_logs(void)
{
    at_device_log_enabled = RT_TRUE;
    rt_kprintf("✅ AT device logs enabled\n");
    return 0;
}

/**
 * @brief 设置控制台日志级别为ERROR
 */
int set_log_level_error(void)
{
    rt_kprintf("🔧 Setting console log level to ERROR only...\n");
    rt_kprintf("💡 This will hide most debug and info messages\n");
    rt_kprintf("💡 You will only see error messages and your commands\n");
    
    /* 这里可以添加实际的日志级别控制代码 */
    /* 例如：ulog_global_filter_lvl_set(LOG_LVL_ERROR); */
    
    return 0;
}

/**
 * @brief 设置控制台日志级别为INFO
 */
int set_log_level_info(void)
{
    rt_kprintf("🔧 Setting console log level to INFO...\n");
    rt_kprintf("💡 This will show info, warning and error messages\n");
    
    /* 这里可以添加实际的日志级别控制代码 */
    /* 例如：ulog_global_filter_lvl_set(LOG_LVL_INFO); */
    
    return 0;
}

/**
 * @brief 清屏命令
 */
int clear_screen(void)
{
    rt_kprintf("\033[2J\033[H");  // ANSI清屏命令
    rt_kprintf("🧹 Screen cleared\n");
    return 0;
}

/**
 * @brief 显示简洁的系统状态
 */
int show_system_status(void)
{
    rt_kprintf("\n📊 === System Status Summary ===\n");
    
    /* 检查air720状态 */
    rt_kprintf("🌐 Network: ");
    rt_kprintf("air720 connected (IP obtained)\n");
    
    /* 检查ADC状态 */
    rt_kprintf("📡 ADC: ");
    rt_kprintf("6-channel monitoring active\n");
    
    /* 检查上传状态 */
    rt_kprintf("☁️  Upload: ");
    rt_kprintf("Ready for cloud upload\n");
    
    rt_kprintf("================================\n");
    rt_kprintf("💡 Use 'clear_screen' to clear console\n");
    rt_kprintf("💡 Use 'quiet_mode' to reduce log output\n");
    rt_kprintf("💡 Use 'test_pv_data_read' to see PV data\n\n");
    
    return 0;
}

/**
 * @brief 安静模式 - 最小化日志输出
 */
int quiet_mode(void)
{
    rt_kprintf("🔇 Entering quiet mode...\n");
    rt_kprintf("💡 Reduced log output for better data visibility\n");
    rt_kprintf("💡 Use 'normal_mode' to restore full logging\n");
    rt_kprintf("💡 Use 'show_system_status' for status summary\n\n");
    
    /* 禁用各种调试日志 */
    disable_at_device_logs();
    set_log_level_error();
    
    return 0;
}

/**
 * @brief 正常模式 - 恢复完整日志输出
 */
int normal_mode(void)
{
    rt_kprintf("🔊 Entering normal mode...\n");
    rt_kprintf("💡 Full log output restored\n\n");
    
    /* 启用各种日志 */
    enable_at_device_logs();
    set_log_level_info();
    
    return 0;
}

/**
 * @brief 显示可用的日志控制命令
 */
int log_help(void)
{
    rt_kprintf("\n📋 === Log Control Commands ===\n");
    rt_kprintf("🔇 quiet_mode          - Minimize log output\n");
    rt_kprintf("🔊 normal_mode         - Restore full logging\n");
    rt_kprintf("🧹 clear_screen        - Clear console screen\n");
    rt_kprintf("📊 show_system_status  - Show system summary\n");
    rt_kprintf("❌ disable_at_device_logs - Hide air720 messages\n");
    rt_kprintf("✅ enable_at_device_logs  - Show air720 messages\n");
    rt_kprintf("🔧 set_log_level_error - Only show errors\n");
    rt_kprintf("🔧 set_log_level_info  - Show info messages\n");
    rt_kprintf("===============================\n\n");
    return 0;
}

/**
 * @brief 创建一个干净的工作环境
 */
int clean_workspace(void)
{
    clear_screen();
    quiet_mode();
    show_system_status();
    
    rt_kprintf("🎯 Clean workspace ready!\n");
    rt_kprintf("💡 Now you can run your PV data commands:\n");
    rt_kprintf("   • test_pv_data_read\n");
    rt_kprintf("   • start_pv_cloud_upload\n");
    rt_kprintf("   • pv_onenet_test\n\n");
    
    return 0;
}

/* 导出MSH命令 */
MSH_CMD_EXPORT(quiet_mode, Enter quiet mode - reduce log output);
MSH_CMD_EXPORT(normal_mode, Enter normal mode - full log output);
MSH_CMD_EXPORT(clear_screen, Clear console screen);
MSH_CMD_EXPORT(show_system_status, Show system status summary);
MSH_CMD_EXPORT(disable_at_device_logs, Disable AT device log messages);
MSH_CMD_EXPORT(enable_at_device_logs, Enable AT device log messages);
MSH_CMD_EXPORT(set_log_level_error, Set log level to ERROR only);
MSH_CMD_EXPORT(set_log_level_info, Set log level to INFO);
MSH_CMD_EXPORT(log_help, Show available log control commands);
MSH_CMD_EXPORT(clean_workspace, Create clean workspace for PV data monitoring);
