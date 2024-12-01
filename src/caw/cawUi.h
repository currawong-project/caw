//| Copyright: (C) 2020-2024 Kevin Larke <contact AT larke DOT org> 
//| License: GNU GPL version 3.0 or above. See the accompanying LICENSE file.
#ifndef cawUi_h
#define cawUi_h


namespace caw
{
  namespace ui
  {
    typedef cw::handle<struct ui_str> handle_t;

    cw::rc_t create( handle_t& hRef,
                     cw::io::handle_t ioH,
                     cw::io_flow_ctl::handle_t ioFlowH,
                     const cw::flow::ui_net_t* ui_net);

    cw::rc_t destroy( handle_t& hRef );
  }
}

#endif
