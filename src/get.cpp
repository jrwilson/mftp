#include "mftp_automaton.hpp"
#include "jam.hpp"
#include "conversion_channel_automaton.hpp"
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
    ioa::automaton_manager<ioa::udp_sender_automaton>* sender;
    ioa::automaton_manager<conversion_channel_automaton>* converter;
    std::string m_filename;

  public:
    mftp_client_automaton (std::string fname) :
      m_self (ioa::get_aid ()),
      m_filename (fname)
    {
      // TODO:  Generalize this.
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

      /*ioa::make_binding_manager (this,
	converter, &conversion_channel_automaton::pass_message,
	&m_self, &mftp_client_automaton::receive);
      */
    
    }
  private:
    void schedule () const { }

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
	  mftp::file f (m_filename.c_str (), m_filename.size (), QUERY_TYPE);

	  // Create the query server.
	  ioa::automaton_manager<mftp::mftp_automaton>* query = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (f, sender->get_handle (), converter->get_handle (), meta_predicate (), meta_filename_predicate (m_filename)));
	  
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
      
      ioa::automaton_manager<mftp::mftp_automaton>* file_home = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (mftp::file (fid), sender->get_handle(), converter->get_handle()));
      
      ioa::make_binding_manager (this,
				 file_home, &mftp::mftp_automaton::download_complete,
				 &m_self, &mftp_client_automaton::file_complete);
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



  /* void receive_effect (const ioa::const_shared_ptr<mftp::message>& m){

    if (m->header.message_type == mftp::FRAGMENT) {
      if (m->frag.fid.type == META_TYPE && meta_files.count (m->frag.fid) == 0) {
	mftp::file f (m->frag.fid);
	f.write_chunk (m->frag.offset, m->frag.data);
	if(f.complete ()) {
	  process_meta_file (f);
	}
	else {
	  if (meta_files.count (m->frag.fid) == 0) {
	    meta_files.insert (m->frag.fid);
	    
	    ioa::automaton_manager<mftp::mftp_automaton>* meta_file_home = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (mftp::file (m->frag.fid)));
	    
	    ioa::make_binding_manager (this,
				       meta_file_home, &mftp::mftp_automaton::send,
				       sender, &ioa::udp_sender_automaton::send);
	    
	    ioa::make_binding_manager (this,
				       sender, &ioa::udp_sender_automaton::send_complete,
				       meta_file_home, &mftp::mftp_automaton::send_complete);
	    
	    ioa::make_binding_manager (this,
				       converter, &conversion_channel_automaton::pass_message,
				       meta_file_home, &mftp::mftp_automaton::receive);		
	    
	    ioa::make_binding_manager (this,
				       meta_file_home, &mftp::mftp_automaton::download_complete,
				       &m_self, &mftp_client_automaton::meta_complete);
	  }
	}
	meta_files.insert(m->frag.fid);
      }
    }  
    }
  
public:
    V_UP_INPUT (mftp_client_automaton, receive, ioa::const_shared_ptr<mftp::message>);
  */  

  /*void process_meta_file (const ioa::const_shared_ptr<mftp::file>& f) {
    if (f->get_mfileid ().get_original_length () > sizeof (mftp::fileid)) {
      std::string s (reinterpret_cast<const char*> (f->get_data_ptr ()) + sizeof (mftp::fileid), f->get_mfileid ().get_original_length () - sizeof (mftp::fileid));
      
      if (s == m_filename){
	mftp::fileid fid;
	memcpy (&fid, f->get_data_ptr (), sizeof (mftp::fileid));
	fid.convert_to_host();
	
	ioa::automaton_manager<mftp::mftp_automaton>* file_home = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (mftp::file (fid),false,false,sender->get_handle(), converter->get_handle()));
	
	ioa::make_binding_manager (this,
				   file_home, &mftp::mftp_automaton::download_complete,
				   &m_self, &mftp_client_automaton::file_complete);
      }	
    }  
    }*/
