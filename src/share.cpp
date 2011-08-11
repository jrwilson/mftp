#include "jam.hpp"
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>

namespace jam {
  
  class mftp_server_automaton :
    public ioa::automaton,
    private ioa::observer
  {
  private:
    ioa::automaton_manager<mftp::mftp_channel_automaton>* channel;
    
    const std::string m_filename;
    const std::string m_sharename;

  public:
    mftp_server_automaton (const std::string& fname,
			   const std::string& sname):
      m_filename (fname),
      m_sharename (sname)
    {
      channel = new ioa::automaton_manager<mftp::mftp_channel_automaton> (this, ioa::make_generator<mftp::mftp_channel_automaton> (jam::SEND_ADDR, jam::LOCAL_ADDR, true));
      
      add_observable (channel);
    }

    void observe (ioa::observable* o) {
      //need to do -1 == channel->get_handle () instead of the normal syntax
      //the other way it gets confused whether to convert -1 to a handle or the get_handle () handle to an int
      if (o == channel && -1 == channel->get_handle ()) {
	channel = 0;
      }

      if (channel != 0) {
	if (channel->get_handle () != -1) {
	  int fd = open (m_filename.c_str (), O_RDONLY);
	  if (fd == -1) {
	    perror ("open");
	    exit (EXIT_FAILURE);
	  }

	  struct stat stats;
	  if (fstat (fd, &stats) == -1) {
	    perror ("fstat");
	    exit (EXIT_FAILURE);
	  }

	  // TODO:  Use streams.
	  char* buf = new char[stats.st_size];
	  if (read (fd, buf, stats.st_size) != stats.st_size) {
	    perror ("read");
	    exit (EXIT_FAILURE);
	  }

	  close (fd);

	  std::auto_ptr<mftp::file> file (new mftp::file (buf, stats.st_size, FILE_TYPE));
	  delete[] buf;

	  mftp::fileid copy = file->get_mfileid ().get_fileid ();
	  std::cout << "Sharing " << m_filename << " as " << (m_sharename + "-" + copy.to_string ()) << std::endl;
	  copy.convert_to_network ();

	  std::auto_ptr<mftp::file> meta (new mftp::file ());

	  meta->get_data ().append (reinterpret_cast<char *> (&copy), sizeof (mftp::fileid));
	  meta->get_data ().append (m_sharename);
	  meta->finalize (META_TYPE);

	  // Create the file server.
	  new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (file, channel->get_handle(), false, 0));
	
	  // Create the meta server.
	  new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (meta, channel->get_handle (), query_predicate (m_sharename), query_filename_predicate (m_sharename), false, false, 0));
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
