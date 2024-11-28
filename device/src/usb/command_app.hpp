#ifndef __COMMAND_APP_HEADER__
#define __COMMAND_APP_HEADER__

#include "double_buffer.hpp"
#include "hid/application.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "report_ids.h"
#include "../device.h"

namespace hid::page {
enum class ugl : uint8_t;
template <>
struct info<ugl> {
    constexpr static page_id_t page_id = 0xFF00;
    constexpr static usage_id_t max_usage_id = 0x0003;
    constexpr static const char *name = "UGL";
};
enum class ugl : uint8_t {
    COMMAND_APP = 0x0001,
    COMMAND_DATA_IN = 0x0002,
    COMMAND_DATA_OUT = 0x0003,
};

} // namespace hid::page

class command_app : public hid::application {
    command_app() : application(report_protocol()) {}

  public:
    static constexpr size_t MESSAGE_SIZE = 63;

    static constexpr auto report_desc()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return descriptor(
            usage_page<ugl>(),
            usage(ugl::COMMAND_APP),
            collection::application(
                conditional_report_id<report_ids::IN_COMMAND>(),
                report_size(8),
                report_count(MESSAGE_SIZE),
                logical_limits<1, 1>(0, 0xff),
                usage(ugl::COMMAND_DATA_IN),
                input::buffered_variable(),
                conditional_report_id<report_ids::OUT_COMMAND>(),
                usage(ugl::COMMAND_DATA_OUT),
                output::buffered_variable()
            )
        );
        // clang-format off
    }

    template <hid::report::type TYPE, hid::report::id::type ID>
    struct report_base : public hid::report::base<TYPE, ID>
    {
        std::array<uint8_t, MESSAGE_SIZE> payload{};
    };
    using report_in = report_base<hid::report::type::INPUT, report_ids::IN_COMMAND>;
    using report_out = report_base<hid::report::type::OUTPUT, report_ids::OUT_COMMAND>;

    static command_app& usb_handle();
#if DEVICE_IS_UHK80_RIGHT
    static command_app& handle();
#endif

    void start(hid::protocol prot) override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    void in_report_sent(const std::span<const uint8_t>& data) override;

  private:
    static hid::report_protocol report_protocol()
    {
        static constexpr const auto rd{report_desc()};
        constexpr hid::report_protocol rp{rd};
        return rp;
    }

    double_buffer<report_in> in_buffer_{};
    double_buffer<report_out> out_buffer_{};
};

#endif // __COMMAND_APP_HEADER__
