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
    //std::map<mftp::fileid, ioa::automaton_manager<mftp::mftp_automaton>* > descs;
    std::string m_filename;

    //ioa::time m_transfer;  //necessary?

    static const ioa::time PROGRESS_INTERVAL;

  public:

    spec_automaton (const std::string& sp) :
      m_self (ioa::get_aid ()),
      m_filename (sp)
    {
      channel = new ioa::automaton_manager<mftp::mftp_channel_automaton> (this, ioa::make_generator<mftp::mftp_channel_automaton> (dd::SEND_ADDR, dd::LOCAL_ADDR, true));

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
	  mftp::file spec (m_filename, SPECIFICATION);

	  std::string spec_string (static_cast<const char*> (spec.get_data_ptr ()), spec.get_mfileid ().get_original_length ());
	  std::cout << "specification: " << spec_string << std::endl;
	  //Create the spec server.
	  ioa::automaton_manager<mftp::mftp_automaton>* spec_server = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (spec, channel->get_handle(), description_predicate (), full_description_predicate (spec_string), true, false, 0));

	  ioa::make_binding_manager (this,
				     spec_server, &mftp::mftp_automaton::match_complete,
				     &m_self, &spec_automaton::match_complete);
	}
      }
    }

    void match_complete_effect (const ioa::const_shared_ptr<mftp::file>& f) {
      // std::string path (m_filename+ "-" + f->get_mfileid ().get_fileid ().to_string ());
      // FILE* fp = fopen (path.c_str (), "w");
      // if (fp == NULL) {
      // 	perror ("fopen");
      // 	exit (EXIT_FAILURE);
      // }

      // size_t er = fwrite (static_cast <const char*> (f->get_data_ptr ()), 1, f->get_mfileid ().get_original_length (), fp);
      // if (er < f->get_mfileid ().get_original_length ()) {
      // 	std::cerr << "fwrite: couldn't write to " << m_filename << std::endl;
      // 	exit (EXIT_FAILURE);
      // }

      // int err = fclose (fp);
      // if (err != 0) {
      // 	perror ("fclose");
      // 	exit (EXIT_FAILURE);
      // }

      // std::cout << "Created " << path << std::endl;

      std::cout << "I matched something!" << std::endl;

    }

  public:
    V_UP_INPUT (spec_automaton, match_complete, ioa::const_shared_ptr<mftp::file>);

  };
}

int main (int argc, char* argv[]) {
  if (argc != 2){
    std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string fname (argv[1]);
 
  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<dd::spec_automaton> (fname));
  return 0;

}
