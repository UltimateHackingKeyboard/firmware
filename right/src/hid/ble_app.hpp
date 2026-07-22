#pragma once

#include "command_app.hpp"
#include "controls_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"

class ble_session : public keyboard_base_session {
  public:
    using keys_boot_report = hid::app::keyboard::keys_input_report<0>;
    using keys_6kro_report = hid::app::keyboard::keys_input_report<report_ids::IN_KEYBOARD_6KRO>;
    using keys_nkro_report = keyboard_app::keys_nkro_report_base<report_ids::IN_KEYBOARD_NKRO>;
    using mouse_report = mouse_app::mouse_report_base<report_ids::IN_MOUSE>;
    using controls_report = controls_app::controls_report_base<report_ids::IN_CONTROLS>;

    ble_session(const hid::session::params &p) : keyboard_base_session(p)
    {
        receive_report(&resolution_buffer_);
        receive_report(&out_buffer_);
    }

    leds_boot_report get_leds_report() const override { return leds_buffer_; }
    const auto &resolution_report() const { return resolution_buffer_; }

    // definition in transport_ble.cpp
    static ble_session *lookup_by_conn(::bt_conn *conn);
    ::bt_conn *get_conn();

  protected:
    mouse_session::scroll_resolution_report resolution_buffer_{};
    command_session::report_in in_buffer_{};
    command_session::report_out out_buffer_{};
    leds_boot_report leds_buffer_{};

    void report_sent(const std::span<const uint8_t> &data) override;

    std::span<const uint8_t> get_report(
        hid::report::selector select, const std::span<uint8_t> &buffer) override;

    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override;
};

class ble_app : public hid::application {
  public:
    static constexpr auto report_desc()
    {
        return hid::rdf::descriptor(keyboard_app::report_desc(), mouse_app::report_desc(),
            command_app::report_desc(), controls_app::report_desc());
    }
    static ble_app &handle()
    {
        static ble_app app{hid::report_protocol::from_descriptor<report_desc()>()};
        return app;
    }

  private:
    ble_app(const hid::report_protocol &rp) : hid::application(rp) {}

    hid::session &start(const hid::session::params &params) override;
    void stop(hid::session &sess) override;
};
