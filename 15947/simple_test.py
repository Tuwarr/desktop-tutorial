#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简单的编码测试脚本
"""

def analyze_garbled_text():
    """分析乱码文本"""
    
    # 您提供的乱码文本
    garbled = "瑙備紬2: 鐢靛奖鐪?濂界湅锛屽績婊℃剰瓒冲湴绂诲紑銆?瑙備紬1: 鎴愬姛涔板埌绁紝杩涘叆鐢靛奖闄紒寮€濮嬬湅鐢靛奖..."
    
    print("=== 乱码分析 ===")
    print("原始乱码:", garbled)
    print()
    
    # 这种乱码通常是UTF-8字节被错误地以其他编码解释造成的
    # 让我们尝试逆向工程
    
    try:
        # 将乱码文本编码为UTF-8字节
        garbled_bytes = garbled.encode('utf-8')
        
        # 尝试用不同编码解码
        print("尝试不同编码解码:")
        encodings = ['gbk', 'gb2312', 'big5', 'latin1']
        
        for enc in encodings:
            try:
                decoded = garbled_bytes.decode(enc, errors='ignore')
                print(f"{enc:>8}: {decoded[:50]}...")
            except:
                print(f"{enc:>8}: 解码失败")
                
    except Exception as e:
        print(f"分析失败: {e}")
    
    print("\n=== 解决方案 ===")
    print("1. 最简单的解决方案：使用英文替代")
    print("   - 避免所有编码问题")
    print("   - 提高代码可移植性")
    print("   - 已在 main.c 中实现")
    print()
    
    print("2. 如果必须使用中文：")
    print("   - 确保源文件以UTF-8编码保存")
    print("   - 串口工具设置为UTF-8编码")
    print("   - 参考 main_chinese.c 文件")
    print()
    
    print("3. 串口工具设置建议：")
    print("   - 波特率: 115200")
    print("   - 数据位: 8")
    print("   - 停止位: 1")
    print("   - 校验位: 无")
    print("   - 字符编码: UTF-8")

def show_correct_output():
    """显示正确的输出应该是什么样的"""
    
    print("\n=== 修复后的预期输出 ===")
    
    english_outputs = [
        "Audience1: Want to watch movie, queuing at cinema entrance...",
        "Audience1: Successfully bought ticket, entering cinema! Starting to watch movie...",
        "Audience2: Also want to watch movie, queuing at cinema entrance...",
        "Audience1: Movie finished, leaving cinema.",
        "Audience2: Haha, I also got a ticket! Entering cinema!",
        "Audience2: Movie was great, leaving satisfied."
    ]
    
    print("英文版本输出（推荐）:")
    for i, output in enumerate(english_outputs, 1):
        print(f"{i}. {output}")
    
    print("\n中文版本输出（需要正确配置编码）:")
    chinese_outputs = [
        "观众1: 想看电影，正在电影院门口排队...",
        "观众1: 成功买到票，进入电影院！开始看电影...",
        "观众2: 也想看电影，正在电影院门口排队...",
        "观众1: 电影看完了，离开电影院。",
        "观众2: 哈哈，我也买到票了！进入电影院！",
        "观众2: 电影真好看，心满意足地离开。"
    ]
    
    for i, output in enumerate(chinese_outputs, 1):
        print(f"{i}. {output}")

if __name__ == "__main__":
    analyze_garbled_text()
    show_correct_output()
