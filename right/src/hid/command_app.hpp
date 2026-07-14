#pragma once

extern "C" {
#include "device.h"
#include "report_ids.h"
#include "usb_protocol_handler.h"
}
#include "double_buffer.hpp"
#include "hid/application.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"

namespace hid::page {
enum class ugl : uint8_t;
template <>
constexpr inline auto get_info<ugl>()
{
    return info(0xFF00, 0x0003, "UGL", [](hid::usage_id_t id) { return id ? "UHK {}" : nullptr; });
}
enum class ugl : uint8_t {
    COMMAND_APP = 0x0001,
    COMMAND_DATA_IN = 0x0002,
    COMMAND_DATA_OUT = 0x0003,
};
} // namespace hid::page

class command_session : public hid::session {
  public:
    static constexpr size_t MESSAGE_SIZE = USB_COMMAND_BUFFER_LENGTH;
    template <hid::report::type TYPE, hid::report::id::type ID>
    struct report_base : public hid::report::base<TYPE, ID> {
        std::array<uint8_t, MESSAGE_SIZE> payload{};
    };
    using report_in = report_base<hid::report::type::INPUT, report_ids::IN_COMMAND>;
    using report_out = report_base<hid::report::type::OUTPUT, report_ids::OUT_COMMAND>;

    command_session() { receive_report(&out_buffer_); }

  private:
    C2USB_USB_TRANSFER_ALIGN(report_in, in_buffer_) {};
    C2USB_USB_TRANSFER_ALIGN(report_out, out_buffer_) {};

    std::span<const uint8_t> get_report(
        hid::report::selector select, const std::span<uint8_t> &buffer) override;

    void set_report(hid::report::type type, const std::span<const uint8_t> &data) override;
};

class command_app : public hid::application {
  public:
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
                report_count(command_session::MESSAGE_SIZE),
                logical_limits<1, 1>(0, 0xff),
                usage(ugl::COMMAND_DATA_IN),
                input::buffered_variable(),
                conditional_report_id<report_ids::OUT_COMMAND>(),
                usage(ugl::COMMAND_DATA_OUT),
                output::buffered_variable()
            )
        );
        // clang-format on
    }

    static command_app &usb_handle()
    {
        static command_app app{};
        return app;
    }

  private:
    static hid::report_protocol report_protocol()
    {
        static constexpr const auto rd{report_desc()};
        constexpr hid::report_protocol rp{rd};
        return rp;
    }
    std::optional<command_session> session_{};

    command_app() : application(report_protocol()) {}
    hid::session &start(const hid::session_params &params) override;
    void stop(hid::session &sess) override;
};
