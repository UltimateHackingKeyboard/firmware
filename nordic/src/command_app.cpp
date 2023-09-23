#include "command_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"

namespace hid::page
{
    enum class ugl : usage_id_type;
    template<>
    struct info<ugl>
    {
        constexpr static usage_id_type base_id = 0xFF000000;
        constexpr static usage_id_type max_usage = 0x0003 | base_id;
        constexpr static const char* name = "UGL";
    };
    enum class ugl : usage_id_type
    {
        COMMAND_APP = 0x0001 | info<ugl>::base_id,
        COMMAND_DATA_IN = 0x0002 | info<ugl>::base_id,
        COMMAND_DATA_OUT = 0x0003 | info<ugl>::base_id,
    };
}
const hid::report_protocol& command_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;

    static constexpr auto rd = descriptor(
        usage_page<ugl>(),
        usage(ugl::COMMAND_APP),
        collection::application(
            conditional_report_id<REPORT_ID>(),
            report_size(8),
            report_count(MESSAGE_SIZE),
            logical_limits<1, 1>(0, 0xff),
            usage(ugl::COMMAND_DATA_IN),
            input::buffered_variable(),
            usage(ugl::COMMAND_DATA_OUT),
            output::buffered_variable()
        )
    );
    static constexpr hid::report_protocol rp {rd};
    return rp;
}

command_app& command_app::handle()
{
    static command_app app{};
    return app;
}

void command_app::start(hid::protocol prot)
{
    // start receiving reports
    receive_report(&out_buffer_);
}

void command_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable
    const auto& data_to_pass = out_buffer_.payload;
    // TODO: call UsbProtocolHandler()

    // always keep receiving new reports
    // if the report data is processed immediately, the same buffer can be used
    receive_report(&out_buffer_);
}

void command_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // fetching the IN report this way doesn't make sense
    send_report(std::span<const uint8_t>());
}
