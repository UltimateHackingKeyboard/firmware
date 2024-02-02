#include "command_app.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report_protocol.hpp"
#include "zephyr/sys/printk.h"

extern "C" bool CommandProtocolTx(const uint8_t* data, size_t size)
{
    return command_app::handle().send(std::span<const uint8_t>(data, size));
}

extern "C" void CommandProtocolRxHandler(const uint8_t* data, size_t size);

void __attribute__((weak)) CommandProtocolRxHandler(const uint8_t* data, size_t size)
{
    printk("CommandProtocolRxHandler: data[0]:%u size:%d\n", data[0], size);
    CommandProtocolTx(data, size);
}

namespace hid::page
{
enum class ugl : usage_id_type;
template <>
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

} // namespace hid::page
const hid::report_protocol& command_app::report_protocol()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    static constexpr auto rd = descriptor(
        usage_page<ugl>(), usage(ugl::COMMAND_APP),
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
    // clang-format off
    static constexpr hid::report_protocol rp{rd};
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
    receive_report(&out_buffer_[out_buffer_.active_side()]);
}

void command_app::set_report(hid::report::type type, const std::span<const uint8_t>& data)
{
    // only one report is receivable
    CommandProtocolRxHandler(data.data(), data.size());

    // always keep receiving new reports
    out_buffer_.swap_sides();
    receive_report(&out_buffer_[out_buffer_.active_side()]);
}

void command_app::get_report(hid::report::selector select, const std::span<uint8_t>& buffer)
{
    // fetching the IN report this way doesn't make sense
    send_report(std::span<const uint8_t>());
}

bool command_app::send(std::span<const uint8_t> buffer)
{
    auto buf_idx = in_buffer_.active_side();
    auto& report = in_buffer_[buf_idx];
    std::copy(buffer.begin(), buffer.end(), report.payload.begin());
    if (send_report(&in_buffer_[buf_idx]) == hid::result::OK)
    {
        in_buffer_.compare_swap(buf_idx);
        return true;
    }
    return false;
}

void command_app::in_report_sent(const std::span<const uint8_t>& data)
{
    auto buf_idx = in_buffer_.indexof(data.data());
    in_buffer_.compare_swap(buf_idx);
}
