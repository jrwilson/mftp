#include "mftp_automaton.hpp"
#include "jam.hpp"
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <stdio.h>
#include <string>

namespace jam {
  
  class mftp_server_automaton :
    public ioa::automaton,
    private ioa::observer
  {
  private:
    ioa::automaton_manager<mftp::mftp_sender_automaton>* sender;
    ioa::automaton_manager<mftp::mftp_receiver_automaton>* receiver;
    
    const std::string m_filename;
    const std::string m_sharename;

  public:
    mftp_server_automaton (const std::string& fname,
			   const std::string& sname):
      m_filename (fname),
      m_sharename (sname)
    {
      mftp::file file (m_filename, FILE_TYPE);
      mftp::fileid copy = file.get_mfileid ().get_fileid ();
      std::cout << "Sharing " << m_filename << " as " << (m_sharename + "-" + copy.to_string ()) << std::endl;

      sender = new ioa::automaton_manager<mftp::mftp_sender_automaton> (this, ioa::make_generator<mftp::mftp_sender_automaton> (jam::SEND_ADDR));
      
      receiver = new ioa::automaton_manager<mftp::mftp_receiver_automaton> (this, ioa::make_generator<mftp::mftp_receiver_automaton> (jam::LOCAL_ADDR, jam::MULTICAST_ADDR));

      add_observable (sender);
      add_observable (receiver); 
    }

    void observe (ioa::observable* o) {
      //need to do -1 == sender->get_handle () instead of the normal syntax
      //the other way it gets confused whether to convert -1 to a handle or the get_handle () handle to an int
      if (o == sender && -1 == sender->get_handle ()) {
	sender = 0;
      }
      else if (o == receiver && -1 == receiver->get_handle()) {
	receiver = 0;
      }

      if (sender != 0 && receiver != 0) {
	if (sender->get_handle () != -1 && receiver->get_handle () != -1) {
	  mftp::file file (m_filename, FILE_TYPE);
	  mftp::fileid copy = file.get_mfileid ().get_fileid ();
	  copy.convert_to_network ();

	  ioa::buffer buff;
	  buff.append (&copy, sizeof (mftp::fileid));
	  buff.append (m_sharename.c_str (), m_sharename.size ());

	  mftp::file meta (buff.data (), buff.size (), META_TYPE);
	
	  // Create the file server.
	  new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (file, sender->get_handle(), receiver->get_handle(), false));
	
	  // Create the meta server.
	  new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (meta, sender->get_handle (), receiver->get_handle (), query_predicate (), query_filename_predicate (m_sharename), false, false));

	}
      }
    }

  };

}

int main (int argc, char* argv[]) {
  if (!(argc == 2 || argc == 3)) {
    std::cerr << "Usage: " << argv[0] << " FILE [NAME]" << std::endl;
    exit(EXIT_FAILURE);
  }

  const char* real_path = argv[1];
  char* shared_as = argv[1];
  if (argc == 3) {
    shared_as = argv[2];
  }

  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<jam::mftp_server_automaton> (real_path, shared_as));

  return 0;
}
