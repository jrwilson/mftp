#include "mftp_automaton.hpp"
#include "jam.hpp"
#include "conversion_channel_automaton.hpp"
#include <ioa/udp_sender_automaton.hpp>
#include <ioa/udp_receiver_automaton.hpp>
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <stdio.h>
#include <string>

class mftp_server_automaton :
  public ioa::automaton,
  private ioa::observer {
private:
  ioa::automaton_manager<ioa::udp_sender_automaton>* sender;
  ioa::automaton_manager<conversion_channel_automaton>* converter;

  jam::interesting_meta_predicate* imp;
  jam::matching_meta_predicate* mmp;
  interesting_file_predicate* IFNULLPTR;
  matching_file_predicate* MFNULLPTR;

  const char* m_filename;
  const char* m_sharename;

public:
  mftp_server_automaton (const char* fname, const char* sname):
    IFNULLPTR (0),
    MFNULLPTR (0),
    m_filename (fname),
    m_sharename (sname)
  {
    mftp::file file (m_filename, FILE_TYPE);
    mftp::fileid copy = file.get_mfileid ().get_fileid ();
    std::cout << "Sharing " << fname << " as " << (std::string (sname) + "-" + copy.to_string ()) << std::endl;

    const std::string address = "0.0.0.0";
    const std::string mc_address = "224.0.0.137";
    const unsigned short port = 54321;
  
    ioa::inet_address local_address (address, port);
    ioa::inet_address multicast_address (mc_address, port);

    sender = new ioa::automaton_manager<ioa::udp_sender_automaton> (this, ioa::make_generator<ioa::udp_sender_automaton> ());

    ioa::automaton_manager<ioa::udp_receiver_automaton>* receiver = new ioa::automaton_manager<ioa::udp_receiver_automaton> (this, ioa::make_generator<ioa::udp_receiver_automaton> (multicast_address, local_address));

    converter = new ioa::automaton_manager<conversion_channel_automaton> (this, ioa::make_generator<conversion_channel_automaton> ());

    add_observable (sender);
    add_observable (converter);
    ioa::make_binding_manager (this,
			       receiver, &ioa::udp_receiver_automaton::receive,
			       converter, &conversion_channel_automaton::receive_buffer);
 
  }

  void observe (ioa::observable* o) {
    //need to do -1 == sender->get_handle () instead of the normal syntax
    //the other way it gets confused whether to convert -1 to a handle or the get_handle () handle to an int
    if (o == sender && -1 == sender->get_handle ()) {
      sender = 0;
    }
    else if (o == converter && -1 == converter->get_handle()) {
      converter = 0;
    }

    if (sender != 0 && converter != 0) {
      if (sender->get_handle () != -1 && converter->get_handle () != -1) {
	mftp::file file (m_filename, FILE_TYPE);
	
	mftp::fileid copy = file.get_mfileid ().get_fileid ();
	
	copy.convert_to_network ();
	
	uint32_t size = strlen (m_sharename);
	
	ioa::buffer buff (sizeof (mftp::fileid) + size * sizeof (char));
	
	memcpy (buff.data (), &copy, sizeof (mftp::fileid));
	memcpy (static_cast<char*>(buff.data ()) + sizeof(mftp::fileid), m_sharename, size);
	
	mftp::file meta (buff.data (), buff.size (), META_TYPE);
	
	ioa::automaton_manager<mftp::mftp_automaton>* file_server = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (file, false, true, sender->get_handle(), converter->get_handle(),IFNULLPTR, MFNULLPTR));
	
	ioa::automaton_manager<mftp::mftp_automaton>* meta_server = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (meta, true, false, sender->get_handle (), converter->get_handle (), imp, mmp));

      }
    }
  }

};

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
  ioa::run (sched, ioa::make_generator<mftp_server_automaton> (real_path, shared_as));

  return 0;
}
