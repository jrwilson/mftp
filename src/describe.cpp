#include "dd.hpp"
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <stdio.h>
#include <string>

namespace dd {

  class desc_automaton :
    public ioa::automaton,
    public ioa::observer

  {
  private:
    ioa::automaton_manager<mftp::mftp_channel_automaton> * channel;

    const std::string m_filename;

  public:
    desc_automaton (const std::string& fname) :
      m_filename (fname)
    {
      mftp::file file (m_filename, DESCRIPTION);
      std::cout << "Sharing " << m_filename << std::endl;

      channel = new ioa::automaton_manager<mftp::mftp_channel_automaton> (this, ioa::make_generator<mftp::mftp_channel_automaton> (dd::SEND_ADDR, dd::LOCAL_ADDR, true));

      add_observable (channel);
    }

    void observe (ioa::observable* o) {
      if (o == channel && -1 == channel->get_handle ()) {
	channel = 0;
      }

      if (channel != 0) {
	if (channel->get_handle () != -1) {
	  mftp::file file (m_filename, DESCRIPTION);

	  std::string desc_string (static_cast<const char*> (file.get_data_ptr ()), file.get_mfileid ().get_original_length ());
	  std::cout << "description: \n" << desc_string << std::endl;
	  //Create the description server.
	  new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (file, channel->get_handle (), specification_predicate (), full_specification_predicate (desc_string), false, false));
	}
      }
    }

  };
}

int main (int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " FILE " << std::endl;
    exit(EXIT_FAILURE);
  }

  const char* path = argv[1];
  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<dd::desc_automaton> (path));

  return 0;
}
