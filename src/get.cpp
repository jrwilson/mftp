#include "jam.hpp"
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <queue>
#include <set>

namespace jam {

  const size_t FRAG_COUNT = 1;

  class mftp_client_automaton :
    public ioa::automaton,
    private ioa::observer {

  private:
    ioa::handle_manager<mftp_client_automaton> m_self;
    std::set<mftp::fileid> meta_files;
    ioa::automaton_manager<mftp::mftp_channel_automaton>* channel;
    std::map<mftp::fileid, ioa::automaton_manager<mftp::mftp_automaton>* > data_files;
    std::string m_filename;

    ioa::time m_transfer;

  public:
    mftp_client_automaton (std::string fname) :
      m_self (ioa::get_aid ()),
      m_filename (fname)
    {
      channel = new ioa::automaton_manager<mftp::mftp_channel_automaton> (this, ioa::make_generator<mftp::mftp_channel_automaton> (jam::SEND_ADDR, jam::LOCAL_ADDR, true));

      add_observable (channel);
    }
  private:
    void schedule () const { }

    void observe (ioa::observable* o) {
      //need to do -1 == sender->get_handle () instead of the normal syntax
      //the other way it gets confused whether to convert -1 to a handle or the get_handle () handle to an int
      if (o == channel && -1 == channel->get_handle ()) {
	channel = 0;
      }

      if (channel != 0) {
	if (channel->get_handle () != -1) {
	  mftp::file f (m_filename.c_str (), m_filename.size (), QUERY_TYPE);

	  // Create the query server.
	  ioa::automaton_manager<mftp::mftp_automaton>* query = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (f, channel->get_handle (), meta_predicate (), meta_inst_filename_predicate (m_filename), true, false, 0));
	  
	  ioa::make_binding_manager (this,
				     query, &mftp::mftp_automaton::match_complete,
				     &m_self, &mftp_client_automaton::match_complete);

	  //Create the instance server.
	  uuid_t id;
	  uuid_generate (id);

	  ioa::buffer buff;
	  buff.append (id, sizeof (uuid_t));
	  buff.append (m_filename.c_str (), m_filename.size ());

	  mftp::file instance (buff.data (), buff.size (), INSTANCE_TYPE);
	  new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (instance, channel->get_handle(), false, 0));
	}
      }
    }
  
    void match_complete_effect (const ioa::const_shared_ptr<mftp::file>& meta_file) {
      // Create the meta server.
      new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (*meta_file, channel->get_handle (), query_predicate (), query_filename_predicate (m_filename), false, false, 0));

      mftp::fileid fid;
      memcpy (&fid, meta_file->get_data_ptr (), sizeof (mftp::fileid));
      fid.convert_to_host();
      
      ioa::automaton_manager<mftp::mftp_automaton>* file_home = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (mftp::file (fid), channel->get_handle(), false, FRAG_COUNT));

      data_files.insert(std::make_pair(fid, file_home));
      
      ioa::make_binding_manager (this,
				 file_home, &mftp::mftp_automaton::download_complete,
				 &m_self, &mftp_client_automaton::file_complete);

      ioa::make_binding_manager (this,
      				 file_home, &mftp::mftp_automaton::fragment_count,
      				 &m_self, &mftp_client_automaton::update_progress);

      m_transfer = ioa::time::now ();
    }
  
  public:
    V_UP_INPUT (mftp_client_automaton, match_complete, ioa::const_shared_ptr<mftp::file>);
  
  private:
    void file_complete_effect (const mftp::file& f, ioa::aid_t){
      std::string path (m_filename + "-" + f.get_mfileid ().get_fileid ().to_string ());
      // TODO:  Add error checking.
      FILE* fp = fopen (path.c_str (), "w");
      if (fp == NULL) {
	perror ("fopen");
	exit (EXIT_FAILURE);
      }

      size_t er = fwrite (f.get_data_ptr (), 1, f.get_mfileid ().get_original_length (), fp);
      if (er < f.get_mfileid ().get_original_length ()) {
	std::cerr << "fwrite: couldn't write to " << m_filename << std::endl;
	exit (EXIT_FAILURE);
      }

      int err = fclose (fp);
      if (err != 0) {
	perror ("fclose");
	exit (EXIT_FAILURE);
      }
      
      ioa::time t = ioa::time::now () - m_transfer;
      double num_bytes = double (f.get_mfileid ().get_original_length ());
      double time = double (t.sec ()) + double(t.usec ())/1000000.0;
      double rate = num_bytes / time;
      if(num_bytes > 1024) {
	num_bytes /= 1024.0;
	rate /= 1024.0;
	if(num_bytes > 1024) {
	  num_bytes /= 1024.0;
	  rate /= 1024.0;
	  std::cout << num_bytes << " MB in " << time << " seconds (" << rate << " MB per second)." << std::endl;
	}
	else {
	  std::cout << num_bytes << " KB in " << time << " seconds (" << rate << " KB per second)." << std::endl;
	}
      }
      else {
	std::cout << num_bytes << " Bytes in " << time << " seconds (" << rate << " Bytes per second)." << std::endl;
      }
      
      std::cout << "Created " << path << std::endl;
    }
  
  public:
    V_AP_INPUT (mftp_client_automaton, file_complete, mftp::file);

  private:
    void update_progress_effect (const uint32_t& have, ioa::aid_t id) {
      //Move the iterator to the right fileid.
      std::map<mftp::fileid, ioa::automaton_manager<mftp::mftp_automaton>*>::iterator it;
      for(it = data_files.begin ();
	  it->second->get_handle() != id && it != data_files.end ();
	  ++it) { ;  }

      if (it == data_files.end ()){
	std::cerr << "OH NOES" << std::endl;
      }

      mftp::mfileid mid (it->first);
      if (have < mid.get_fragment_count ()) {
	std::cout << "Received " << have << " of " << mid.get_fragment_count () << " fragments" << std::endl;
      }
    }
    
  public:
    V_AP_INPUT (mftp_client_automaton, update_progress, uint32_t);

  };  
}

int main (int argc, char* argv[]) {
  if (argc != 2){
    std::cerr << "Usage: " << argv[0] << " FILE" << std::endl;
    exit(EXIT_FAILURE);
  }
  
  std::string fname (argv[1]);
  
  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<jam::mftp_client_automaton> (fname));
  return 0;
}
