/**
 * @file     led_shift.cpp
 * @brief    LedShift —— 74HC595 实现（纯 GPIO bit-bang）
 *
 * 修改：2026-11-23 新建
 */
#include "led_shift.h"
#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "LedShift";

LedShift::LedShift(const Config& cfg) : cfg_(cfg) {}

esp_err_t LedShift::init()
{
    pinMode(cfg_.dataPin,  OUTPUT);
    pinMode(cfg_.clockPin, OUTPUT);
    pinMode(cfg_.latchPin, OUTPUT);

    digitalWrite(cfg_.dataPin,  LOW);
    digitalWrite(cfg_.clockPin, LOW);
    digitalWrite(cfg_.latchPin, LOW);

    state_ = 0;
    dirty_ = true;
    writeAll();   /* 确保上电全灭 */

    ESP_LOGI(TAG, "LedShift 就绪 (chain=%d, data=%d clk=%d latch=%d, total=%d bits)",
             cfg_.chainLen, cfg_.dataPin, cfg_.clockPin, cfg_.latchPin,
             cfg_.chainLen * 8);
    return ESP_OK;
}

void LedShift::set(uint8_t n, bool on)
{
    if (n >= cfg_.chainLen * 8) return;

    uint16_t mask = (uint16_t)1 << n;
    if (on) {
        if (!(state_ & mask)) { state_ |= mask; dirty_ = true; }
    } else {
        if (state_ & mask)   { state_ &= ~mask; dirty_ = true; }
    }
}

bool LedShift::get(uint8_t n) const
{
    if (n >= cfg_.chainLen * 8) return false;
    return (state_ >> n) & 1;
}

/* RGB 默认占 9/10/11 三路 —— 板子侧按实际接线直接调 set() 指定通道 */
void LedShift::setRgb(bool r, bool g, bool b)
{
    set(9, r);
    set(10, g);
    set(11, b);
}

void LedShift::flush()
{
    if (!dirty_) return;
    writeAll();
    dirty_ = false;
}

void LedShift::writeAll()
{
    shiftOut16(state_);
}

/* ====================================================================
 *  核心：GPIO bit-bang 串行发送
 *  74HC595 时序：
 *    DS  送数据位（高位先送/低位先送？74HC595 是高位先移 → MSB first）
 *    SHCP 上升沿 → 移入寄存器
 *    STCP 上升沿 → 锁存到输出
 * ==================================================================== */
void LedShift::shiftOut16(uint16_t data)
{
    /* 先拉低锁存 → 移位期间输出保持不变 */
    digitalWrite(cfg_.latchPin, LOW);

    /* 片数决定移多少位 */
    uint8_t totalBits = cfg_.chainLen * 8;

    for (int i = totalBits - 1; i >= 0; i--) {
        uint8_t bit = (data >> i) & 1;
        digitalWrite(cfg_.dataPin, bit ? HIGH : LOW);
        digitalWrite(cfg_.clockPin, HIGH);
        digitalWrite(cfg_.clockPin, LOW);
    }

    /* 最后拉高锁存 → 所有 595 同时更新输出 */
    digitalWrite(cfg_.latchPin, HIGH);
    digitalWrite(cfg_.latchPin, LOW);
}

#include "led_shift_version.h"
#include "version_manager.h"
COMPONENT_VERSION_REGISTER(led_shift, LED_SHIFT_VERSION_STRING)
