#include "dd.hpp"
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <queue>
#include <set>

namespace dd {

  class spec_automaton :
    public ioa::automaton,
    private ioa::observer {

  private:
    ioa::handle_manager<spec_automaton> m_self;
    ioa::automaton_manager<mftp::mftp_channel_automaton>* channel;
    std::map<mftp::fileid, ioa::automaton_manager<mftp::mftp_automaton>* > descs;
    std::string m_filename;

    ioa::time m_transfer;  //necessary?

    static const ioa::time PROGRESS_INTERVAL;

  public:

    spec_automaton (const std::string& sp) :
      m_self (ioa::get_aid ()),
      m_filename (sp)
    {
      channel = new ioa::automaton_manager<mftp::mftp_channel_automaton> (this, ioa::make_generator<mftp::mftp_channel_automaton> (dd:SEND_ADDR, dd:LOCAL_ADDR, true));

      add_observable (channel);
    }

  private:
    void schedule () const {  }

    void observe (ioa::observable* o) {
      if (o == channel && -1 == channel->get_handle ()) {
	channel = 0;
      }

      if (channel != 0) {
	if (channel->get_handle () != -1) {
	  mftp
	}
      }



    }




  }












}
