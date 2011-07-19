#include "jam.hpp"
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/ioa.hpp>

#include <iostream>
#include <queue>
#include <set>

namespace jam {

  class mftp_client_automaton :
    public ioa::automaton,
    private ioa::observer {

  private:
    ioa::handle_manager<mftp_client_automaton> m_self;
    std::set<mftp::fileid> meta_files;
    std::map<mftp::fileid, ioa::automaton_manager<mftp::mftp_automaton>* > data_files;
    ioa::automaton_manager<mftp::mftp_sender_automaton>* sender;
    ioa::automaton_manager<mftp::mftp_receiver_automaton>* receiver;
    std::string m_filename;

    static const ioa::time PROGRESS_INTERVAL;

  public:
    mftp_client_automaton (std::string fname) :
      m_self (ioa::get_aid ()),
      m_filename (fname)
    {
      sender = new ioa::automaton_manager<mftp::mftp_sender_automaton> (this, ioa::make_generator<mftp::mftp_sender_automaton> (jam::SEND_ADDR));
      receiver = new ioa::automaton_manager<mftp::mftp_receiver_automaton> (this, ioa::make_generator<mftp::mftp_receiver_automaton> (jam::LOCAL_ADDR, jam::MULTICAST_ADDR));

      add_observable (sender);
      add_observable (receiver);
    }
  private:
    void schedule () const { }

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
	  mftp::file f (m_filename.c_str (), m_filename.size (), QUERY_TYPE);

	  // Create the query server.
	  ioa::automaton_manager<mftp::mftp_automaton>* query = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (f, sender->get_handle (), receiver->get_handle (), meta_predicate (), meta_filename_predicate (m_filename), true, false));
	  
	  ioa::make_binding_manager (this,
				     query, &mftp::mftp_automaton::match_complete,
				     &m_self, &mftp_client_automaton::match_complete);
	}
      }
    }
  
    void match_complete_effect (const ioa::const_shared_ptr<mftp::file>& f) {
      mftp::fileid fid;
      memcpy (&fid, f->get_data_ptr (), sizeof (mftp::fileid));
      fid.convert_to_host();
      
      ioa::automaton_manager<mftp::mftp_automaton>* file_home = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (mftp::file (fid), sender->get_handle(), receiver->get_handle(), false));

      data_files.insert(std::make_pair(fid, file_home));
      
      ioa::make_binding_manager (this,
				 file_home, &mftp::mftp_automaton::download_complete,
				 &m_self, &mftp_client_automaton::file_complete);

      /*ioa::make_binding_manager (this,
				 file_home, &mftp::mftp_automaton::report_progress,
				 &m_self, &mftp_client_automaton::update_progress);*/
    }
  
  public:
    V_UP_INPUT (mftp_client_automaton, match_complete, ioa::const_shared_ptr<mftp::file>);
  
  private:
    void file_complete_effect (const mftp::file& f, ioa::aid_t){
      std::string path (m_filename + "-" + f.get_mfileid ().get_fileid ().to_string ());
      // TODO:  Add error checking.
      FILE* fp = fopen (path.c_str (), "w");
      fwrite (f.get_data_ptr (), 1, f.get_mfileid ().get_original_length (), fp);
      fclose (fp);
      std::cout << "Created " << path << std::endl;
    }
  
  public:
    V_AP_INPUT (mftp_client_automaton, file_complete, mftp::file);
  

  private:
    void progress_update_effect (const uint32_t& have, ioa::aid_t id) {
      //Move the iterator to the right fileid.
      std::map<mftp::fileid, ioa::automaton_manager<mftp::mftp_automaton>*>::iterator it;
      for(it = data_files.begin ();
	  it->second->get_handle() != id;
	  ++it) { ;  }

      if (it == data_files.end ()){
	std::cerr << "OH NOES" << std::endl;
      }

      mftp::mfileid mid (it->first);
      std::cout << "Received " << have << " of " << mid.get_fragment_count () << " fragments" << std::endl;
    }
    
  public:
    V_AP_INPUT (mftp_client_automaton, progress_update, uint32_t);
    
  private:
    bool report_progress_precondition () const {
      return false; 
    }
    
    void report_progress_effect () {
      //print progress
    }
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
