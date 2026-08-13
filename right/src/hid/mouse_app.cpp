#include "mouse_app.hpp"

using mouse_report = mouse_app::mouse_report_base<report_ids::IN_MOUSE>;

hid::session &mouse_app::start(const hid::session::params &params)
{
    assert(!session_.has_value());
    auto &sess = session_.emplace(params);
    mouse_resolution_changed_callback(sess, sess.resolution_report());
    return sess;
}

void mouse_app::stop(hid::session &sess)
{
    assert(&sess == &session_.value());
    return session_.reset();
}

void mouse_session::report_sent(const std::span<const uint8_t> &data)
{
    mouse_report_sent_callback(*this);
}

void mouse_session::set_report(hid::report::type type, const std::span<const uint8_t> &data)
{
    if (hid::report::selector(type, data.front()) != resolution_buffer_.selector()) {
        return;
    }
    // data is already received to this buffer
    // resolution_buffer_ = *reinterpret_cast<const decltype(resolution_buffer_) *>(data.data());
    mouse_resolution_changed_callback(*this, resolution_report());
    receive_report(&resolution_buffer_);
}

std::span<const uint8_t> mouse_session::get_report(
    hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select == resolution_buffer_.selector()) {
        return std::span<const uint8_t>(resolution_buffer_.data(), sizeof(resolution_buffer_));
    }

    if (select == hid::report::selector(hid::report::type::INPUT, report_ids::IN_MOUSE)) {
        assert(buffer.size() >= sizeof(mouse_report));
        std::ignore = new (buffer.data()) mouse_report{};
        return buffer.subspan(0, sizeof(mouse_report));
    }

    return {};
}
