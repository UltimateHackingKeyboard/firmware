#ifndef __CONTROLS_APP_HEADER__
#define __CONTROLS_APP_HEADER__

#include "double_buffer.hpp"
#include "hid/application.hpp"
#include "hid/page/consumer.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/page/telephony.hpp"

using system_code = hid::page::generic_desktop;
using consumer_code = hid::page::consumer;
using telephony_code = hid::page::telephony;

class controls_app : public hid::application
{
    static const hid::report_protocol& report_protocol();

    static constexpr size_t CONSUMER_CODE_COUNT = 2;
    static constexpr size_t SYSTEM_CODE_COUNT = 2;
    static constexpr size_t TELEPHONY_CODE_COUNT = 2;

    controls_app()
        : application(report_protocol())
    {}

  public:
    struct controls_report : public hid::report::base<hid::report::type::INPUT, 0>
    {
        template <typename T, size_t SIZE>
        struct report_array : public std::array<T, SIZE>
        {
            bool set(hid::usage_index_type usage, bool value = true)
            {
                auto it = std::find(this->begin(), this->end(), value ? 0 : usage);
                if (it != this->end())
                {
                    *it = value ? usage : 0;
                    return true;
                }
                return false;
            }
            bool test(hid::usage_index_type usage) const
            {
                return std::find(this->begin(), this->end(), usage) != this->end();
            }
            constexpr report_array() = default;
        };
        report_array<hid::le_uint16_t, CONSUMER_CODE_COUNT> consumer_codes{};
        report_array<hid::le_uint16_t, SYSTEM_CODE_COUNT> system_codes{};
        report_array<hid::le_uint16_t, TELEPHONY_CODE_COUNT> telephony_codes{};

        constexpr controls_report() = default;

        bool operator==(const controls_report& other) const = default;
        bool operator!=(const controls_report& other) const = default;

        bool set_code(system_code c, bool value = true)
        {
            return system_codes.set(static_cast<hid::usage_index_type>(c), value);
        }
        bool test(system_code c) const
        {
            return system_codes.test(static_cast<hid::usage_index_type>(c));
        }
        bool set_code(consumer_code c, bool value = true)
        {
            return consumer_codes.set(static_cast<hid::usage_index_type>(c), value);
        }
        bool test(consumer_code c) const
        {
            return consumer_codes.test(static_cast<hid::usage_index_type>(c));
        }
        bool set_code(telephony_code c, bool value = true)
        {
            return telephony_codes.set(static_cast<hid::usage_index_type>(c), value);
        }
        bool test(telephony_code c) const
        {
            return telephony_codes.test(static_cast<hid::usage_index_type>(c));
        }
    };

    static controls_app& handle();

    void set_report_state(const controls_report& data);

  private:
    void start(hid::protocol prot) override;
    void stop() override;
    void set_report(hid::report::type type, const std::span<const uint8_t>& data) override
    {
        // no FEATURE or OUTPUT reports
    }
    void in_report_sent(const std::span<const uint8_t>& data) override;
    void get_report(hid::report::selector select, const std::span<uint8_t>& buffer) override;
    void send_buffer(const controls_report& report);

    double_buffer<controls_report> report_buffer_{};
};

using controls_buffer = controls_app::controls_report;

#endif // __CONTROLS_APP_HEADER__
