#ifndef __APP_BASE_HEADER__
#define __APP_BASE_HEADER__

#include "hid/application.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "port/zephyr/semaphore.hpp"
#include <chrono>

class app_base : public hid::application {
  public:
    bool active() const;

  protected:
    static constexpr std::chrono::milliseconds SEMAPHORE_RESET_TIMEOUT{100};

    template <typename T>
    static hid::report_protocol rp()
    {
        static constexpr const auto rd{T::report_desc()};
        constexpr hid::report_protocol rp{rd};
        return rp;
    }
    template <typename T, typename TReport>
    constexpr app_base([[maybe_unused]] T *t, TReport &in_report_buffer)
        : application(rp<T>()),
          in_id_(in_report_buffer.ID),
          in_buffer_(in_report_buffer.data(), sizeof(TReport))
    {}

    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override {}
    void get_report(hid::report::selector select, const std::span<uint8_t> &buffer) override;
    void in_report_sent(const std::span<const uint8_t> &data) override;
    void send(const std::span<const uint8_t> &buffer);

    const hid::report::id in_id_;
    const std::span<uint8_t> in_buffer_;
    os::zephyr::binary_semaphore sending_sem_{1};
};

#endif // __APP_BASE_HEADER__
