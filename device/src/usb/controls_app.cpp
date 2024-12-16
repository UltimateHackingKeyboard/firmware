#include "controls_app.hpp"

controls_app &controls_app::handle()
{
    static controls_app app{};
    return app;
}

void controls_app::start(hid::protocol prot)
{
    // TODO start handling controls events
    report_buffer_ = {};
}

void controls_app::set_report_state(const controls_report_base<0> &data)
{
    send({data.data(), sizeof(data)});
}
