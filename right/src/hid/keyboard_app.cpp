#include "keyboard_app.hpp"
extern "C" {
#include "hid/transport.h"
#include "ledmap.h"
#include "usb_state.h"
#include "utils.h"
#if __has_include(<zephyr/sys/printk.h>)
    #include <zephyr/sys/printk.h>
#endif
#ifdef __ZEPHYR__
    #include "connections.h"
#endif
}

#if DEVICE_IS_UHK60
// TODO: tune this value to match reality
static constexpr uint8_t LED_UPDATE_DELAY_MS = 20;

struct position_mm {
    uint16_t x;
    uint16_t y;
};

static constexpr uint16_t lamp_position_z = 15;

static constexpr auto left_lamp_positions = std::to_array<position_mm>({// first row
    {16, 33}, {35, 33}, {54, 33}, {73, 33}, {92, 33}, {112, 33}, {131, 33},
    // second row
    {20, 52}, {43, 52}, {63, 52}, {82, 52}, {101, 52}, {120, 52},
    // third row
    {23, 71}, {48, 71}, {67, 71}, {86, 71}, {105, 71}, {124, 71},
    // fourth row
    {18, 91}, {40, 91}, {59, 91}, {78, 91}, {97, 91}, {116, 91}, {135, 91},
    // fifth row
    {18, 110}, {42, 110}, {66, 110}, {89, 110}, {117, 110},
    // key cluster
    {154, 91}, {144, 110}, {168, 117}});
static_assert(left_lamp_positions.size() == keyboard_session::LEFT_LAMP_COUNT);

static constexpr auto right_lamp_positions = std::to_array<position_mm>({// first row
    {21, 33}, {40, 33}, {59, 33}, {78, 33}, {97, 33}, {116, 33}, {140, 33},
    // second row
    {10, 52}, {29, 52}, {48, 52}, {77, 52}, {86, 52}, {105, 52}, {124, 52}, {143, 52},
    // third row
    {14, 71}, {33, 71}, {52, 71}, {71, 71}, {90, 71}, {110, 71}, {137, 71},
    // fourth row
    {24, 91}, {43, 91}, {63, 91}, {82, 91}, {101, 91}, {132, 91},
    // fifth row
    {22, 110}, {51, 110}, {75, 110}, {98, 110}, {132, 110}});
static_assert(right_lamp_positions.size() == keyboard_session::RIGHT_LAMP_COUNT);

// intensity_level_count is 1, so hosts only distinguish "off" (0) from "on" (non-zero)
static rgb_t lamp_rgbi_to_rgb(const hid::app::lamparray::rgbi_tuple &rgbi)
{
    if (rgbi.intensity == 0) {
        return rgb_t{};
    }
    return rgb_t{
        .red = static_cast<uint8_t>(rgbi.red),
        .green = static_cast<uint8_t>(rgbi.green),
        .blue = static_cast<uint8_t>(rgbi.blue),
    };
}
#endif

keyboard_app::keyboard_app(const hid::report_protocol &rp)
    : hid::application(rp),
      usb_function_{*this, nullptr, {}, usb::hid::boot_protocol_mode::KEYBOARD}
{}

void keyboard_app::set_rollover(rollover_t mode)
{
    // swap the HID report descriptor, which needs USB re-enumeration
    report_info_ =
        mode == rollover_t::ROLLOVER_N_KEY ? nkro_report_protocol() : default_report_protocol();
}

hid::session &keyboard_app::start(const hid::session::params &params)
{
    assert(!session_.has_value());
    UsbState_SetUsbTransportUp(true);
    auto &session = session_.emplace(params);
    // has to run after the session is in place - the protocol is read off it
#ifdef __ZEPHYR__
    if (Connections_IsCurrentHost(ConnectionId_UsbHidRight)) {
        Hid_UpdateKeyboardProtocol();
    }
#else
    Hid_UpdateKeyboardProtocol();
#endif
    return session;
}

void keyboard_app::stop(hid::session &sess)
{
    assert(&sess == &session_.value());
    UsbState_SetUsbTransportUp(false);
    return session_.reset();
}

void key_report_buffer::reset_to(hid::protocol prot, rollover_t rollover)
{
    reset();
    // make sure that no keys are pressed when this happens
    // or send an empty report on the virtual keyboard that is deactivated by this switch?
    if (prot == hid::protocol::BOOT) {
        mode = MODE_BOOT;
        (*this)[0].boot = {};
        (*this)[1].boot = {};
    } else if (rollover == rollover_t::ROLLOVER_N_KEY) {
        mode = MODE_NKRO;
        (*this)[0].nkro = {};
        (*this)[1].nkro = {};
    } else {
        mode = MODE_6KRO;
        (*this)[0].sixkro = {};
        (*this)[1].sixkro = {};
    }
}

std::span<const uint8_t> key_report_buffer::insert(const hid_keyboard_report_t &report)
{
    auto buf_idx = active_side();
    if (mode == MODE_NKRO) {
        auto &keys_nkro = (*this)[buf_idx].nkro;

        ::memcpy(&keys_nkro.modifiers, &report.modifiers, sizeof(report.modifiers));
        ::memcpy(
            static_cast<void *>(&keys_nkro.scancodes), &report.bitfield, sizeof(report.bitfield));

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_nkro), sizeof(keys_nkro));
    }

    if (mode == MODE_BOOT) {
        auto &keys_6kro = (*this)[buf_idx].boot;

        ::memcpy(&keys_6kro.modifiers, &report.modifiers, sizeof(report.modifiers));
        keys_6kro.scancodes.reset();
        for (auto code = uint8_t(LOWEST_SCANCODE); code <= uint8_t(HIGHEST_SCANCODE); ++code) {
            keys_6kro.scancodes.set(
                scancode(code), test_bit(code - uint8_t(LOWEST_SCANCODE), report.bitfield));
        }

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_6kro), sizeof(keys_6kro));
    }

    {
        auto &keys_6kro = (*this)[buf_idx].sixkro;

        ::memcpy(&keys_6kro.modifiers, &report.modifiers, sizeof(report.modifiers));
        keys_6kro.scancodes.reset();
        for (auto code = uint8_t(LOWEST_SCANCODE); code <= uint8_t(HIGHEST_SCANCODE); ++code) {
            keys_6kro.scancodes.set(
                scancode(code), test_bit(code - uint8_t(LOWEST_SCANCODE), report.bitfield));
        }

        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(&keys_6kro), sizeof(keys_6kro));
    }
}

keyboard_session::leds_boot_report keyboard_session::get_leds_report() const
{
    auto *ptr = reinterpret_cast<const uint8_t *>(&leds_buffer_);
    if ((protocol() != hid::protocol::BOOT) and (sizeof(leds_report) > sizeof(leds_boot_report))) {
        ptr += sizeof(leds_report) - sizeof(leds_boot_report);
    }
    return reinterpret_cast<const leds_boot_report &>(*ptr);
}

void keyboard_session::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (type == hid::report::type::OUTPUT) {
        keyboard_leds_changed_callback(*this);

        // always keep receiving new reports
        // if the report data is processed immediately, the same buffer can be used
        receive_report(&leds_buffer_);
    } else {
#if DEVICE_IS_UHK60
        if (data.empty()) {
            return;
        }
        switch (hid::report::selector(type, data.front())) {
        case left_lamp_attrs_req_report::selector():
            if (data.size() >= sizeof(left_lamp_attrs_req_report)) {
                auto *req = reinterpret_cast<const left_lamp_attrs_req_report *>(data.data());
                if (req->lamp_id < keyboard_session::LEFT_LAMP_COUNT) {
                    req_led_left_ = req->lamp_id;
                }
            }
            break;
        case right_lamp_attrs_req_report::selector():
            if (data.size() >= sizeof(right_lamp_attrs_req_report)) {
                auto *req = reinterpret_cast<const right_lamp_attrs_req_report *>(data.data());
                if (req->lamp_id < keyboard_session::RIGHT_LAMP_COUNT) {
                    req_led_right_ = req->lamp_id;
                }
            }
            break;
        case left_lamp_control_report::selector():
            if (data.size() >= sizeof(left_lamp_control_report)) {
                auto *report = reinterpret_cast<const left_lamp_control_report *>(data.data());
                if (report->autonomous_mode) {
                    Ledmap_ResetTemporaryLedBacklightingMode();
                    Ledmap_TriggerFullUpdate();
                } else {
                    Ledmap_SetTemporaryLedBacklightingMode(BacklightingMode_DynamicLighting);
                }
            }
            break;
        case right_lamp_control_report::selector():
            if (data.size() >= sizeof(right_lamp_control_report)) {
                auto *report = reinterpret_cast<const right_lamp_control_report *>(data.data());
                if (report->autonomous_mode) {
                    Ledmap_ResetTemporaryLedBacklightingMode();
                    Ledmap_TriggerFullUpdate();
                } else {
                    Ledmap_SetTemporaryLedBacklightingMode(BacklightingMode_DynamicLighting);
                }
            }
            break;
        case left_lamp_multi_update_report::selector():
            if (data.size() >= sizeof(left_lamp_multi_update_report)) {
                auto *report = reinterpret_cast<const left_lamp_multi_update_report *>(data.data());
                if (report->lamp_count > keyboard_session::LEFT_LAMP_COUNT) {
                    break;
                }
                for (size_t i = 0; i < report->lamp_count; ++i) {
                    uint16_t lamp_id = report->lamp_ids[i];
                    if (lamp_id >= keyboard_session::LEFT_LAMP_COUNT) {
                        continue;
                    }
                    rgb_t color = lamp_rgbi_to_rgb(report->values[i]);

                    if (lamp_id < LEFT_HALF_LAMP_COUNT) {
                        Ledmap_SetKeyColor(&color, SlotId_LeftKeyboardHalf, lamp_id);
                    } else if (lamp_id < LEFT_LAMP_COUNT) {
                        Ledmap_SetKeyColor(
                            &color, SlotId_LeftModule, lamp_id - LEFT_HALF_LAMP_COUNT);
                    }
                }
                if (report->update_flags == hid::app::lamparray::update_flags::COMPLETE) {
                    Ledmap_TriggerFullUpdate();
                }
            }
            break;
        case right_lamp_multi_update_report::selector():
            if (data.size() >= sizeof(right_lamp_multi_update_report)) {
                auto *report =
                    reinterpret_cast<const right_lamp_multi_update_report *>(data.data());
                if (report->lamp_count > keyboard_session::RIGHT_LAMP_COUNT) {
                    break;
                }
                for (size_t i = 0; i < report->lamp_count; ++i) {
                    uint16_t lamp_id = report->lamp_ids[i];
                    if (lamp_id >= keyboard_session::RIGHT_LAMP_COUNT) {
                        continue;
                    }
                    rgb_t color = lamp_rgbi_to_rgb(report->values[i]);

                    Ledmap_SetKeyColor(&color, SlotId_RightKeyboardHalf, lamp_id);
                }
                if (report->update_flags == hid::app::lamparray::update_flags::COMPLETE) {
                    Ledmap_TriggerFullUpdate();
                }
            }
            break;
        case left_lamp_range_update_report::selector():
            if (data.size() >= sizeof(left_lamp_range_update_report)) {
                auto *report = reinterpret_cast<const left_lamp_range_update_report *>(data.data());
                if ((report->lamp_id_start > report->lamp_id_end) or
                    (report->lamp_id_end >= keyboard_session::LEFT_LAMP_COUNT)) {
                    break;
                }
                rgb_t color = lamp_rgbi_to_rgb(report->value);
                for (size_t i = report->lamp_id_start; i <= report->lamp_id_end; ++i) {
                    if (i < LEFT_HALF_LAMP_COUNT) {
                        Ledmap_SetKeyColor(&color, SlotId_LeftKeyboardHalf, i);
                    } else if (i < LEFT_LAMP_COUNT) {
                        Ledmap_SetKeyColor(&color, SlotId_LeftModule, i - LEFT_HALF_LAMP_COUNT);
                    }
                }
                if (report->update_flags == hid::app::lamparray::update_flags::COMPLETE) {
                    Ledmap_TriggerFullUpdate();
                }
            }
            break;
        case right_lamp_range_update_report::selector():
            if (data.size() >= sizeof(right_lamp_range_update_report)) {
                auto *report =
                    reinterpret_cast<const right_lamp_range_update_report *>(data.data());
                if ((report->lamp_id_start > report->lamp_id_end) or
                    (report->lamp_id_end >= keyboard_session::RIGHT_LAMP_COUNT)) {
                    break;
                }
                rgb_t color = lamp_rgbi_to_rgb(report->value);
                for (size_t i = report->lamp_id_start; i <= report->lamp_id_end; ++i) {
                    Ledmap_SetKeyColor(&color, SlotId_RightKeyboardHalf, i);
                }
                if (report->update_flags == hid::app::lamparray::update_flags::COMPLETE) {
                    Ledmap_TriggerFullUpdate();
                }
            }
            break;
        default:
            break;
        }
#endif
    }
}

void keyboard_session::report_sent(const std::span<const uint8_t> &data)
{
    keyboard_report_sent_callback(*this);
}

std::span<const uint8_t> keyboard_session::get_report(
    hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (protocol() == hid::protocol::BOOT) {
        if (select == keyboard_app::keys_boot_report::selector()) {
            assert(buffer.size() >= sizeof(keyboard_app::keys_boot_report));
            std::ignore = new (buffer.data()) keyboard_app::keys_boot_report{};
            return buffer.subspan(0, sizeof(keyboard_app::keys_boot_report));
        }
        if (select == leds_boot_report::selector()) {
            assert(buffer.size() >= sizeof(leds_boot_report));
            auto *ptr = new (buffer.data()) leds_boot_report{};
            ptr->leds = leds_buffer_.leds;
            return buffer.subspan(0, sizeof(leds_boot_report));
        }
        return {};
    }

    switch (select) {
    case keyboard_app::keys_6kro_report::selector():
        assert(buffer.size() >= sizeof(keyboard_app::keys_6kro_report));
        std::ignore = new (buffer.data()) keyboard_app::keys_6kro_report{};
        return buffer.subspan(0, sizeof(keyboard_app::keys_6kro_report));

    case keyboard_app::keys_nkro_report::selector():
        assert(buffer.size() >= sizeof(keyboard_app::keys_nkro_report));
        std::ignore = new (buffer.data()) keyboard_app::keys_nkro_report{};
        return buffer.subspan(0, sizeof(keyboard_app::keys_nkro_report));

    case leds_report::selector(): {
        assert(buffer.size() >= sizeof(leds_report));
        auto *ptr = new (buffer.data()) leds_report{};
        ptr->leds = leds_buffer_.leds;
        return buffer.subspan(0, sizeof(leds_report));
    }

#if DEVICE_IS_UHK60
    case left_lamp_attrs_report::selector(): {
        assert(buffer.size() >= sizeof(left_lamp_attrs_report));
        auto *ptr = new (buffer.data()) left_lamp_attrs_report{};
        // UHK60 left + module
        ptr->lamp_count = LEFT_LAMP_COUNT;
        ptr->bounding_box.width = 185 * 1000;  // um
        ptr->bounding_box.height = 136 * 1000; // um
        ptr->bounding_box.depth = 30 * 1000;   // um
        ptr->min_update_interval = LED_UPDATE_DELAY_MS * 1000;
        ptr->kind = hid::app::lamparray::kind::KEYBOARD;
        return buffer.subspan(0, sizeof(left_lamp_attrs_report));
    }

    case right_lamp_attrs_report::selector(): {
        assert(buffer.size() >= sizeof(right_lamp_attrs_report));
        auto *ptr = new (buffer.data()) right_lamp_attrs_report{};
        // UHK60 right
        ptr->lamp_count = RIGHT_LAMP_COUNT;
        ptr->bounding_box.width = 159 * 1000;  // um
        ptr->bounding_box.height = 130 * 1000; // um
        ptr->bounding_box.depth = 30 * 1000;   // um
        ptr->min_update_interval = LED_UPDATE_DELAY_MS * 1000;
        ptr->kind = hid::app::lamparray::kind::KEYBOARD;
        return buffer.subspan(0, sizeof(right_lamp_attrs_report));
    }

    case left_lamp_attrs_req_report::selector(): {
        assert(buffer.size() >= sizeof(left_lamp_attrs_req_report));
        auto *ptr = new (buffer.data()) left_lamp_attrs_req_report{};
        ptr->lamp_id = req_led_left_;
        return buffer.subspan(0, sizeof(left_lamp_attrs_req_report));
    }

    case right_lamp_attrs_req_report::selector(): {
        assert(buffer.size() >= sizeof(right_lamp_attrs_req_report));
        auto *ptr = new (buffer.data()) right_lamp_attrs_req_report{};
        ptr->lamp_id = req_led_right_;
        return buffer.subspan(0, sizeof(right_lamp_attrs_req_report));
    }

    case left_lamp_attrs_rsp_report::selector(): {
        assert(buffer.size() >= sizeof(left_lamp_attrs_rsp_report));
        auto *ptr = new (buffer.data()) left_lamp_attrs_rsp_report{};
        ptr->position.x = left_lamp_positions[req_led_left_].x * 1000;
        ptr->position.y = left_lamp_positions[req_led_left_].y * 1000;
        ptr->position.z = lamp_position_z * 1000;
        ptr->update_latency = LED_UPDATE_DELAY_MS / 2 * 1000;
        ptr->red_level_count = std::numeric_limits<uint8_t>::max();
        ptr->green_level_count = std::numeric_limits<uint8_t>::max();
        ptr->blue_level_count = std::numeric_limits<uint8_t>::max();
        ptr->intensity_level_count = 1;
        ptr->is_programmable = true;
        ptr->purposes = hid::app::lamparray::purposes::CONTROL |
                        hid::app::lamparray::purposes::ACCENT |
                        hid::app::lamparray::purposes::STATUS;
        // TODO set according to mapped key
        ptr->input_binding = 0;

        ptr->lamp_id = req_led_left_;
        req_led_left_ = (req_led_left_ + 1) % LEFT_LAMP_COUNT;
        return buffer.subspan(0, sizeof(left_lamp_attrs_rsp_report));
    }

    case right_lamp_attrs_rsp_report::selector(): {
        assert(buffer.size() >= sizeof(right_lamp_attrs_rsp_report));
        auto *ptr = new (buffer.data()) right_lamp_attrs_rsp_report{};
        ptr->position.x = right_lamp_positions[req_led_right_].x * 1000;
        ptr->position.y = right_lamp_positions[req_led_right_].y * 1000;
        ptr->position.z = lamp_position_z * 1000;
        ptr->update_latency = LED_UPDATE_DELAY_MS / 2 * 1000;
        ptr->red_level_count = std::numeric_limits<uint8_t>::max();
        ptr->green_level_count = std::numeric_limits<uint8_t>::max();
        ptr->blue_level_count = std::numeric_limits<uint8_t>::max();
        ptr->intensity_level_count = 1;
        ptr->is_programmable = true;
        ptr->purposes = hid::app::lamparray::purposes::CONTROL |
                        hid::app::lamparray::purposes::ACCENT |
                        hid::app::lamparray::purposes::STATUS;
        // TODO set according to mapped key
        ptr->input_binding = 0;

        ptr->lamp_id = req_led_right_;
        req_led_right_ = (req_led_right_ + 1) % RIGHT_LAMP_COUNT;
        return buffer.subspan(0, sizeof(right_lamp_attrs_rsp_report));
    }

    case left_lamp_control_report::selector(): {
        assert(buffer.size() >= sizeof(left_lamp_control_report));
        auto *ptr = new (buffer.data()) left_lamp_control_report{};
        ptr->autonomous_mode =
            Ledmap_GetEffectiveBacklightMode() != BacklightingMode_DynamicLighting;
        return buffer.subspan(0, sizeof(left_lamp_control_report));
    }

    case attributes_report::selector(): {
        assert(buffer.size() >= sizeof(attributes_report));
        auto *ptr = new (buffer.data()) attributes_report{};
        // TODO: either we set the values realistically,
        // or we set them to common values that conform to the expected OS layout
        ptr->form_factor = hid::app::keyboard::form_factor::FULL_SIZE;
        ptr->key_type = hid::app::keyboard::key_type::FULL_TRAVEL;
        ptr->layout = hid::app::keyboard::layout::_102;
        ptr->ietf_lang_tag_index = keyboard_app::usb_function().string_index(0);
        return buffer.subspan(0, sizeof(attributes_report));
    }
#endif

    default:
        return {};
    }
}
