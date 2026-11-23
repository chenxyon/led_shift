/**
 * @file     led_shift.h
 * @brief    LedShift —— 74HC595 移位寄存器 LED 驱动组件
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║        永久全局规则（所有组件强制约束，后续不得违反）       ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║ 1. 组件必须用 class 封装（禁止纯 C 函数 / namespace 作 API）║
 * ║ 2. 配置通过构造函数 Config 结构体注入（禁止硬编码）         ║
 * ║ 3. 组件零板级依赖（禁止 include board_config.h / pin）      ║
 * ║ 4. 状态封装在实例里（禁止全局单例 / static 全局变量）       ║
 * ║ 5. 禁止 Arduino.h 冲突宏（RESET→SOFT_RESET / DISPLAY→IO_DISPLAY / ERROR→BAD_CMD）║
 * ║ 6. 兼容旧 API 用 #define CMD_XXX 别名                      ║
 * ║ 7. 组件目录结构：include/ + src/（PlatformIO 自动识别）     ║
 * ║ 8. 板子侧只做组装：main.cpp 实例化 + 传 Config              ║
 * ║ 9. GPIO 扩展优先：同类 GPIO ≥ 2 个时，优先用类似串行扩展器       ║
 * ║    （如 74HC595/165、PCA9685、CD4021/4094、MCP23017/ADS1115 等）║
 * ║    芯片型号+数量+节约引脚数在注释中列出，由用户审核后实施        ║
 * ║ 10. 版本强制：所有自定义组件必须接入 version_manager，       ║
 * ║     包含 xxx_version.h + version.txt + COMPONENT_VERSION_   ║
 * ║     REGISTER() 宏调用，禁止跳过。板子固件也必须注册自身版本   ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * 📌 本组件即为规则 9 触发的典型案例（10 个 LED → 2×74HC595，3 根线，节约 7 个 GPIO）
 *
 * 硬件：74HC595 × N 片级联
 *   主控引脚：DS(数据) + SHCP(时钟) + STCP(锁存)
 *   级联：U1.QH → U2.DS，主控只接 U1
 *
 * 输出位编号（2 片级联共 16 位）：
 *   0~7  →  U1.QA~QH  （第一片，先移出）
 *   8~15 →  U2.QA~QH  （第二片，后移出）
 *
 * 使用示例：
 *   LedShift led({ .dataPin=40, .clockPin=39, .latchPin=42, .chainLen=2 });
 *   led.init();
 *   led.set(0, true);   // 点亮第 0 路
 *   led.flush();        // 刷到硬件（内部有脏标记，变化才刷）
 */
#ifndef LED_SHIFT_H
#define LED_SHIFT_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

class LedShift {
public:
    struct Config {
        gpio_num_t dataPin;     /* 595 DS (串行数据入) */
        gpio_num_t clockPin;    /* 595 SHCP (移位时钟) */
        gpio_num_t latchPin;    /* 595 STCP (锁存) */
        uint8_t    chainLen = 2;/* 级联片数 */
        const char* tag = "LedShift";
    };

    explicit LedShift(const Config& cfg);
    ~LedShift() = default;

    esp_err_t init();

    /* 通用接口：设第 n 路亮/灭（n=0~chainLen*8-1）*/
    void set(uint8_t n, bool on);
    bool get(uint8_t n) const;

    /* RGB 便捷接口（默认占用 3 个连续通道，如 9/10/11）*/
    void setRgb(bool r, bool g, bool b);
    void setRgbChannel(uint8_t r, uint8_t g, uint8_t b);

    /* 立即刷到硬件（内部有脏标记，无变化则跳过）*/
    void flush();

    /* 一次刷 8 位 */
    void writeAll();

private:
    Config   cfg_;
    uint16_t state_ = 0x0000;  /* 当前输出状态（小端：bit0=U1.QA）*/
    bool     dirty_ = true;    /* 是否有变化需要刷硬件 */

    void shiftOut16(uint16_t data);
};

#endif /* LED_SHIFT_H */
