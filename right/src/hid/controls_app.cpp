#include "controls_app.hpp"

hid::session &controls_app::start(const hid::session_params &params)
{
    assert(!session_.has_value());
    auto &sess = session_.emplace();
    return sess;
}

void controls_app::stop(hid::session &sess)
{
    assert(&sess == &session_.value());
    return session_.reset();
}

void controls_session::report_sent(const std::span<const uint8_t> &data)
{
    controls_report_sent_callback(*this);
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
