#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试字符编码的Python脚本
用于验证中文字符在不同编码下的显示效果
"""

def test_encoding():
    """测试不同编码方式"""
    
    # 原始的乱码字符串（从您提供的输出中复制）
    garbled_text = "瑙備紬2: 鐢靛奖鐪?濂界湅锛屽績婊℃剰瓒冲湴绂诲紑銆?瑙備紬1: 鎴愬姛涔板埌绁紝杩涘叆鐢靛奖闄紒寮€濮嬬湅鐢靛奖..."
    
    print("=== 字符编码测试 ===")
    print(f"乱码文本: {garbled_text}")
    print()
    
    # 尝试不同的编码解码方式
    encodings = ['utf-8', 'gbk', 'gb2312', 'big5', 'latin1']
    
    for encoding in encodings:
        try:
            # 先编码为字节，再用不同编码解码
            bytes_data = garbled_text.encode('utf-8')
            decoded = bytes_data.decode(encoding, errors='ignore')
            print(f"{encoding:>8}: {decoded}")
        except Exception as e:
            print(f"{encoding:>8}: 解码失败 - {e}")
    
    print("\n=== 正确的中文文本 ===")
    correct_texts = [
        "观众1: 想看电影，正在电影院门口排队...",
        "观众1: 成功买到票，进入电影院！开始看电影...",
        "观众1: 电影看完了，离开电影院。",
        "观众2: 也想看电影，正在电影院门口排队...",
        "观众2: 哈哈，我也买到票了！进入电影院！",
        "观众2: 电影真好看，心满意足地离开。"
    ]
    
    for text in correct_texts:
        print(f"UTF-8: {text}")
        # 显示对应的英文版本
        english_map = {
            "观众1: 想看电影，正在电影院门口排队...": "Audience1: Want to watch movie, queuing at cinema entrance...",
            "观众1: 成功买到票，进入电影院！开始看电影...": "Audience1: Successfully bought ticket, entering cinema! Starting to watch movie...",
            "观众1: 电影看完了，离开电影院。": "Audience1: Movie finished, leaving cinema.",
            "观众2: 也想看电影，正在电影院门口排队...": "Audience2: Also want to watch movie, queuing at cinema entrance...",
            "观众2: 哈哈，我也买到票了！进入电影院！": "Audience2: Haha, I also got a ticket! Entering cinema!",
            "观众2: 电影真好看，心满意足地离开。": "Audience2: Movie was great, leaving satisfied."
        }
        if text in english_map:
            print(f"英文: {english_map[text]}")
        print()

def check_file_encoding(filename):
    """检查文件编码"""
    import chardet
    
    try:
        with open(filename, 'rb') as f:
            raw_data = f.read()
            result = chardet.detect(raw_data)
            print(f"文件 {filename} 的编码检测结果:")
            print(f"  编码: {result['encoding']}")
            print(f"  置信度: {result['confidence']:.2%}")
    except FileNotFoundError:
        print(f"文件 {filename} 不存在")
    except Exception as e:
        print(f"检测文件编码时出错: {e}")

if __name__ == "__main__":
    test_encoding()
    print("\n" + "="*50)
    check_file_encoding("applications/main.c")
    check_file_encoding("applications/main_chinese.c")
