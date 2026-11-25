#include "controls_app.hpp"

hid::session &controls_app::start(const hid::session::params &params)
{
    assert(!session_.has_value());
    auto &sess = session_.emplace(params);
    return sess;
}

void controls_app::stop(hid::session &sess)
{
    assert(&sess == &session_.value());
    return session_.reset();
}

void controls_session::send_report_callback(
    [[maybe_unused]] hid::report::selector select, c2usb::result result)
{
    controls_report_sent_callback(*this, result.to_int());
}

std::span<const uint8_t> controls_session::get_report(
    hid::report::selector select, const std::span<uint8_t> &buffer)
{
    using controls_report = controls_app::controls_report_base<report_ids::IN_CONTROLS>;

    if (select == hid::report::selector(hid::report::type::INPUT, report_ids::IN_CONTROLS)) {
        assert(buffer.size() >= sizeof(controls_report));
        std::ignore = new (buffer.data()) controls_report{};
        return buffer.subspan(0, sizeof(controls_report));
    }

    return {};
}
