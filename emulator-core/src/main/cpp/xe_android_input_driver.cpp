/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xe_android_input_driver.h"
#include "xenia/ui/virtual_key.h"
#include "xenia/base/logging.h"

namespace xe {
    namespace hid {
        namespace android {
            namespace {
            // Same order as key_maps in xendroid_emu.cpp: the frontend's key index.
            constexpr std::array<ui::VirtualKey, 24> kKeyOrder = {
                    ui::VirtualKey::kXInputPadDpadLeft,
                    ui::VirtualKey::kXInputPadDpadUp,
                    ui::VirtualKey::kXInputPadDpadRight,
                    ui::VirtualKey::kXInputPadDpadDown,
                    ui::VirtualKey::kXInputPadA,
                    ui::VirtualKey::kXInputPadB,
                    ui::VirtualKey::kXInputPadX,
                    ui::VirtualKey::kXInputPadY,
                    ui::VirtualKey::kXInputPadBack,
                    ui::VirtualKey::kXInputPadStart,

                    ui::VirtualKey::kXInputPadLShoulder,
                    ui::VirtualKey::kXInputPadRShoulder,
                    ui::VirtualKey::kXInputPadLThumbPress,
                    ui::VirtualKey::kXInputPadRThumbPress,
                    ui::VirtualKey::kXInputPadLTrigger,
                    ui::VirtualKey::kXInputPadRTrigger,

                    ui::VirtualKey::kXInputPadLThumbLeft,
                    ui::VirtualKey::kXInputPadLThumbUp,
                    ui::VirtualKey::kXInputPadLThumbRight,
                    ui::VirtualKey::kXInputPadLThumbDown,
                    ui::VirtualKey::kXInputPadRThumbLeft,
                    ui::VirtualKey::kXInputPadRThumbUp,
                    ui::VirtualKey::kXInputPadRThumbRight,
                    ui::VirtualKey::kXInputPadRThumbDown,
            };

            void ResetKeys(AndroidInputDriver::Slot& slot) {
                for (size_t i = 0; i < kKeyOrder.size(); ++i) {
                    slot.keys[i] = {kKeyOrder[i], false, 0, false};
                }
                slot.prev_keys = slot.keys;
                slot.mask = 0;
            }
            }  // namespace

            AndroidInputDriver::AndroidInputDriver(xe::ui::Window* window, size_t window_z_order)
                    : InputDriver(window, window_z_order) {
                for (auto& slot : slots_) {
                    ResetKeys(slot);
                }
                // P1 is always there: the on-screen pad and the first controller.
                slots_[0].connected = true;
                slots_[0].name = "Android Controller";
            }

            AndroidInputDriver::~AndroidInputDriver()=default;

            X_STATUS AndroidInputDriver::Setup() { return X_STATUS_SUCCESS; }

            bool AndroidInputDriver::SlotConnected(uint32_t slot) {
                if (slot >= kSlotCount) {
                    return false;
                }
                std::lock_guard<std::mutex> key_lock(key_status_mutex_);
                return slots_[slot].connected;
            }

            X_RESULT AndroidInputDriver::GetCapabilities(uint32_t user_index, uint32_t flags,X_INPUT_CAPABILITIES* out_caps) {
                if (!SlotConnected(user_index)) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }

                // TODO(benvanik): confirm with a real XInput controller.
                out_caps->type = 0x01;      // XINPUT_DEVTYPE_GAMEPAD
                out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
                out_caps->flags = 0;
                out_caps->gamepad.buttons = 0xFFFF;
                out_caps->gamepad.left_trigger = 0xFF;
                out_caps->gamepad.right_trigger = 0xFF;
                out_caps->gamepad.thumb_lx = (int16_t)0xFFFFu;
                out_caps->gamepad.thumb_ly = (int16_t)0xFFFFu;
                out_caps->gamepad.thumb_rx = (int16_t)0xFFFFu;
                out_caps->gamepad.thumb_ry = (int16_t)0xFFFFu;
                out_caps->vibration.left_motor_speed = 0;
                out_caps->vibration.right_motor_speed = 0;
                return X_ERROR_SUCCESS;
            }

            X_RESULT AndroidInputDriver::GetState(uint32_t user_index,X_INPUT_STATE* out_state) {
                if (user_index >= kSlotCount) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }

                uint16_t buttons = 0;
                uint8_t left_trigger = 0;
                uint8_t right_trigger = 0;
                int16_t thumb_lx = 0;
                int16_t thumb_ly = 0;
                int16_t thumb_rx = 0;
                int16_t thumb_ry = 0;
                uint32_t packet_number;

                {
                    std::lock_guard<std::mutex> key_lock(key_status_mutex_);
                    Slot& slot = slots_[user_index];
                    if (!slot.connected) {
                        return X_ERROR_DEVICE_NOT_CONNECTED;
                    }
                    packet_number = ++slot.packet_number;
                    for (const KeyStatus& ks : slot.keys) {
                        if (!ks.pressed) continue;
                        switch (ks.id) {
                            case ui::VirtualKey::kXInputPadA:
                                buttons |= 0x1000;  // XINPUT_GAMEPAD_A
                                break;
                            case ui::VirtualKey::kXInputPadY:
                                buttons |= 0x8000;  // XINPUT_GAMEPAD_Y
                                break;
                            case ui::VirtualKey::kXInputPadB:
                                buttons |= 0x2000;  // XINPUT_GAMEPAD_B
                                break;
                            case ui::VirtualKey::kXInputPadX:
                                buttons |= 0x4000;  // XINPUT_GAMEPAD_X
                                break;
                            case ui::VirtualKey::kXInputPadDpadLeft:
                                buttons |= 0x0004;  // XINPUT_GAMEPAD_DPAD_LEFT
                                break;
                            case ui::VirtualKey::kXInputPadDpadRight:
                                buttons |= 0x0008;  // XINPUT_GAMEPAD_DPAD_RIGHT
                                break;
                            case ui::VirtualKey::kXInputPadDpadDown:
                                buttons |= 0x0002;  // XINPUT_GAMEPAD_DPAD_DOWN
                                break;
                            case ui::VirtualKey::kXInputPadDpadUp:
                                buttons |= 0x0001;  // XINPUT_GAMEPAD_DPAD_UP
                                break;
                            case ui::VirtualKey::kXInputPadRThumbPress:
                                buttons |= 0x0080;  // XINPUT_GAMEPAD_RIGHT_THUMB
                                break;
                            case ui::VirtualKey::kXInputPadLThumbPress:
                                buttons |= 0x0040;  // XINPUT_GAMEPAD_LEFT_THUMB
                                break;
                            case ui::VirtualKey::kXInputPadBack:
                                buttons |= 0x0020;  // XINPUT_GAMEPAD_BACK
                                break;
                            case ui::VirtualKey::kXInputPadStart:
                                buttons |= 0x0010;  // XINPUT_GAMEPAD_START
                                break;
                            case ui::VirtualKey::kXInputPadLShoulder:
                                buttons |= 0x0100;  // XINPUT_GAMEPAD_LEFT_SHOULDER
                                break;
                            case ui::VirtualKey::kXInputPadRShoulder:
                                buttons |= 0x0200;  // XINPUT_GAMEPAD_RIGHT_SHOULDER
                                break;
                            // An analog trigger sends its travel, 0-255; a digital
                            // press (value < 0: the touch controls, an L2/R2 key)
                            // is full.
                            case ui::VirtualKey::kXInputPadLTrigger:
                                left_trigger = ks.value < 0 ? 0xFF
                                               : uint8_t(ks.value > 0xFF ? 0xFF : ks.value);
                                break;
                            case ui::VirtualKey::kXInputPadRTrigger:
                                right_trigger = ks.value < 0 ? 0xFF
                                                : uint8_t(ks.value > 0xFF ? 0xFF : ks.value);
                                break;
                            case ui::VirtualKey::kXInputPadLThumbLeft:
                            case ui::VirtualKey::kXInputPadLThumbRight:
                                thumb_lx = ks.value;
                                break;
                            case ui::VirtualKey::kXInputPadLThumbDown:
                            case ui::VirtualKey::kXInputPadLThumbUp:
                                thumb_ly = ks.value;
                                break;
                            case ui::VirtualKey::kXInputPadRThumbUp:
                            case ui::VirtualKey::kXInputPadRThumbDown:
                                thumb_ry = ks.value;
                                break;
                            case ui::VirtualKey::kXInputPadRThumbRight:
                            case ui::VirtualKey::kXInputPadRThumbLeft:
                                thumb_rx = ks.value;
                                break;
                            default:
                                break;
                        }
                    }
                }

                out_state->packet_number = packet_number;
                out_state->gamepad.buttons = buttons;
                out_state->gamepad.left_trigger = left_trigger;
                out_state->gamepad.right_trigger = right_trigger;
                out_state->gamepad.thumb_lx = thumb_lx;
                out_state->gamepad.thumb_ly = thumb_ly;
                out_state->gamepad.thumb_rx = thumb_rx;
                out_state->gamepad.thumb_ry = thumb_ry;

                return X_ERROR_SUCCESS;
            }

            X_RESULT AndroidInputDriver::SetState(uint32_t user_index,X_INPUT_VIBRATION* vibration) {
                if (!SlotConnected(user_index)) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }
                rumble_[user_index].store((uint32_t(uint16_t(vibration->left_motor_speed)) << 16) |
                                              uint16_t(vibration->right_motor_speed),
                                          std::memory_order_relaxed);
                return X_ERROR_SUCCESS;
            }

            uint32_t AndroidInputDriver::Rumble(size_t slot) const {
                return slot < kSlotCount ? rumble_[slot].load(std::memory_order_relaxed) : 0;
            }

            X_RESULT AndroidInputDriver::GetKeystroke(uint32_t user_index, uint32_t flags,X_INPUT_KEYSTROKE* out_keystroke) {
                if (user_index >= kSlotCount) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }

                X_RESULT result = X_ERROR_EMPTY;

                ui::VirtualKey xinput_virtual_key = ui::VirtualKey::kNone;
                uint16_t unicode = 0;
                uint16_t keystroke_flags = 0;
                uint8_t hid_code = 0;

                {
                    std::lock_guard<std::mutex> key_lock(key_status_mutex_);
                    Slot& slot = slots_[user_index];
                    if (!slot.connected) {
                        return X_ERROR_DEVICE_NOT_CONNECTED;
                    }
                    if (slot.mask == 0) {
                        // No keys!
                        return X_ERROR_EMPTY;
                    }
                    // One keystroke per call, lowest index first, clearing only
                    // its bit; the guest drains the rest by polling until EMPTY.
                    for (size_t i = 0; i < slot.keys.size(); i++) {
                        if (slot.mask & (1u << i)) {
                            xinput_virtual_key = slot.keys[i].id;
                            slot.mask &= ~(1u << i);
                            keystroke_flags |= slot.keys[i].stroke ? 0x0001   // XINPUT_KEYSTROKE_KEYDOWN
                                                                    : 0x0002;  // XINPUT_KEYSTROKE_KEYUP
                            if (slot.prev_keys[i].stroke == slot.keys[i].stroke) {
                                keystroke_flags |= 0x0004;  // XINPUT_KEYSTROKE_REPEAT
                            }
                            result = X_ERROR_SUCCESS;
                            break;
                        }
                    }
                }

                out_keystroke->virtual_key = uint16_t(xinput_virtual_key);
                out_keystroke->unicode = unicode;
                out_keystroke->flags = keystroke_flags;
                out_keystroke->user_index = uint8_t(user_index);
                out_keystroke->hid_code = hid_code;

                // X_ERROR_EMPTY if no new keys
                // X_ERROR_DEVICE_NOT_CONNECTED if no device
                // X_ERROR_SUCCESS if key
                return result;
            }

            void AndroidInputDriver::OnKey(size_t slot_index, int key_index, bool pressed, short value){
                if (slot_index >= kSlotCount || key_index < 0 || key_index >= int(kKeyOrder.size())) {
                    return;
                }
                std::lock_guard<std::mutex> key_lock(key_status_mutex_);
                Slot& slot = slots_[slot_index];
                if (!slot.connected) {
                    return;
                }
                KeyStatus& key = slot.keys[key_index];
                // #97: keystrokes see an analog trigger pressed past XInput's threshold
                // (> 0x1F, as Edge's SDL driver), not at its first bit of travel.
                const bool trigger = key.id == ui::VirtualKey::kXInputPadLTrigger ||
                                     key.id == ui::VirtualKey::kXInputPadRTrigger;
                const bool stroke = trigger && value >= 0 ? value > 0x1F : pressed;

                // Transitions only: analog axes re-send every motion sample, which
                // would keep GetKeystroke from ever reaching EMPTY, and would leave
                // the previous state equal to the new one (a KEYDOWN read as a repeat).
                if (stroke != key.stroke) {
                    slot.prev_keys[key_index] = key;
                    slot.mask |= (1u << key_index);
                }
                key.pressed = pressed;
                key.value = value;
                key.stroke = stroke;
            }

            void AndroidInputDriver::SetSlotConnected(size_t slot_index, bool connected, const std::string& name) {
                if (slot_index >= kSlotCount) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> key_lock(key_status_mutex_);
                    Slot& slot = slots_[slot_index];
                    if (!name.empty()) {
                        slot.name = name.substr(0, 64);
                    }
                    // P1 never disconnects: the on-screen pad always feeds it.
                    if (slot_index == 0 || slot.connected == connected) {
                        return;
                    }
                    slot.connected = connected;
                    ResetKeys(slot);
                    rumble_[slot_index].store(0, std::memory_order_relaxed);
                }
                XELOGI("Android controller slot {} {}", slot_index + 1, connected ? "connected" : "disconnected");
                NotifyDevicesChanged();
            }

            InputType AndroidInputDriver::GetInputType() const { return InputType::Controller; }

            std::vector<InputDeviceInfo> AndroidInputDriver::EnumerateDevices() {
                // XenDroid: edge's InputSystem only routes guest input to drivers
                // that are bound to a slot (slot_bindings_), and bindings are built
                // from EnumerateDevices(). Slot 0 is always present (on-screen pad
                // + first controller); slots 1-3 while a controller holds them.
                std::vector<InputDeviceInfo> out;
                std::lock_guard<std::mutex> key_lock(key_status_mutex_);
                for (size_t i = 0; i < kSlotCount; ++i) {
                    if (!slots_[i].connected) {
                        continue;
                    }
                    InputDeviceInfo info{};
                    info.driver_slot = uint8_t(i);
                    info.stable_id = i == 0 ? "android-gamepad" : "android-gamepad-" + std::to_string(i + 1);
                    info.display_name = slots_[i].name.empty() ? "Controller " + std::to_string(i + 1) : slots_[i].name;
                    info.preferred_slot = int8_t(i);  // P1..P4
                    // subtype defaults to XINPUT_DEVSUBTYPE_GAMEPAD, auto_bind to true.
                    out.push_back(std::move(info));
                }
                return out;
            }

        }
    }
}
