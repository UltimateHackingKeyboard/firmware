#pragma once
#include <hid/application.hpp>
#include <span>
#include <stdint.h>

bool UsbLeft_KeyboardPending();
bool UsbLeft_QuiesceKeyboardProtocol();
int UsbLeft_TrackedSend(uint8_t kind, hid::session *session, std::span<const uint8_t> data);
int UsbLeft_QueueUsb(uint8_t kind, std::span<const uint8_t> data, uint32_t generation);
