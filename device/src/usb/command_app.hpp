#ifndef __COMMAND_APP_HEADER__
#define __COMMAND_APP_HEADER__

#include "double_buffer.hpp"
#include "hid/application.hpp"

class command_app : public hid::application
{
    static const hid::report_protocol& report_protocol();

    command_app()
        : application(report_protocol())
    {}

  public:
    static constexpr size_t MESSAGE_SIZE = 64;
    static constexpr uint8_t REPORT_ID = 0;

    template <hid::report::type TYPE>
    struct report_base : public hid::report::base<TYPE, 0>
    {
        std::array<uint8_t, MESSAGE_SIZE> payload{};
    };
    using report_in = report_base<hid::report::type::INPUT>;
    using report_out = report_base<hid::report::type::OUTPUT>;

    static command_app& handle();

    bool send(std::span<const uint8_t> buffer);

    void start(hid::protocol prot) override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    void in_report_sent(const std::span<const uint8_t>& data) override;

  private:
    double_buffer<report_in> in_buffer_{};
    double_buffer<report_out> out_buffer_{};
};

#endif // __COMMAND_APP_HEADER__
