#include "app_base.hpp"

void app_base::stop()
{
    sending_sem_.release();
}

void app_base::send(const std::span<const uint8_t> &buffer)
{
    if (!active()) {
        return;
    }
    if (!sending_sem_.try_acquire_for(SEMAPHORE_RESET_TIMEOUT)) {
        return;
    }
    std::copy(buffer.begin(), buffer.end(), in_buffer_.data() + sizeof(in_id_));
    auto result = send_report(in_buffer_);
    if (result != hid::result::OK) {
        sending_sem_.release();
    }
}

void app_base::in_report_sent(const std::span<const uint8_t> &data)
{
    if (data.front() != in_id_) {
        return;
    }
    sending_sem_.release();
}

void app_base::get_report(hid::report::selector select, const std::span<uint8_t> &buffer)
{
    if (select != hid::report::selector(hid::report::type::INPUT, in_id_)) {
        return;
    }
    assert(buffer.size() >= in_buffer_.size());

    memcpy(buffer.data(), in_buffer_.data(), in_buffer_.size());
    send_report(buffer.subspan(0, in_buffer_.size()));
}
