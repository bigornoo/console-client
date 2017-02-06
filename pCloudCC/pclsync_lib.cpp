/*
 * Copyright (c) 2013-2015 pCloud Ltd.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of pCloud Ltd nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL pCloud Ltd BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pclsync_lib.h"
#include "psynclib.h"
#include "pcompat.h"

#include <iostream>

#include <iostream>
#include <string>
//#include <termios.h>
//#include <unistd.h>
#include <stdio.h>


namespace cc  = console_client;
namespace clib  = cc::clibrary;

#include <boost/shared_ptr.hpp>

clib::pclsync_lib& clib::pclsync_lib::get_lib(){
  static clib::pclsync_lib g_lib;
  return g_lib;}

/*static std::string exec(const char* cmd) {
    boost::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}
*/

void clib::pclsync_lib::get_pass_from_console()
{
  do_get_pass_from_console(password_);
}

void clib::pclsync_lib::get_cryptopass_from_console()
{
  do_get_pass_from_console(crypto_pass_);
}

void clib::pclsync_lib::do_get_pass_from_console(std::string& password)
{
  if (daemon_) {
     std::cout << "Not able to read password when started as daemon." << std::endl;
     exit(1);
  }
#ifdef P_OS_POSIX
  termios oldt;
  tcgetattr(STDIN_FILENO, &oldt);
  termios newt = oldt;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  std::cout << "Please, enter password" << std::endl;
  getline(std::cin, password);
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#else  
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD modeoff;
  DWORD modeon;
  DWORD mode;
  GetConsoleMode(hStdin, &mode);
    modeoff = mode & ~ENABLE_ECHO_INPUT;
    modeon = mode | ENABLE_ECHO_INPUT;
  SetConsoleMode(hStdin, modeoff);
  std::cout << "Please, enter password" << std::endl;
  getline(std::cin, password);
  SetConsoleMode(hStdin, modeon);
  //std::cout << "Password is " << password << std::endl;
#endif
}

void event_handler(psync_eventtype_t event, psync_eventdata_t eventdata){
 if (event<PEVENT_FIRST_USER_EVENT){
    if (event&PEVENT_TYPE_FOLDER)
      std::cout <<"folder event=" << event<<", syncid="<< eventdata.folder->syncid<<", folderid="<<eventdata.folder->folderid<<", name="
         <<eventdata.folder->name<<", local="<<eventdata.folder->localpath<<", remote="<< eventdata.folder->remotepath<<std::endl;
     else
      std::cout <<"file event=" << event<<", syncid="<< eventdata.folder->syncid<<", file="<<eventdata.file->fileid<<", name="
         << eventdata.file->name<<", local="<<eventdata.file->localpath<<", remote="<< eventdata.file->remotepath<<std::endl;
  }
  else if (event>=PEVENT_FIRST_SHARE_EVENT)
    std::cout <<"share event=" << event<<", folderid="<< eventdata.share->folderid<<", sharename="<<eventdata.share->sharename<<
                    ", email="<< eventdata.share->toemail<<", message="<<eventdata.share->message<<", userid="<< eventdata.share->userid<<
                    ", shareid="<<eventdata.share->shareid<<", sharerequestid="<<eventdata.share->sharerequestid<<
                    ", created="<<eventdata.share->created<<", canread="<<eventdata.share->canread<<", cancreate="<<eventdata.share->cancreate<<
                    ", canmodify="<<eventdata.share->canmodify<<", candelete="<<eventdata.share->candelete<<std::endl;
  else
    std::cout <<"event" << event << std::endl;
}

static void  lib_setup_cripto(){ 
  if (psync_crypto_issetup())
    std::cout << "crypto is setup, login result=" << psync_crypto_start(clib::pclsync_lib::get_lib().get_crypto_pass().c_str()) << std::endl;
  else{
    std::cout << "crypto is not setup" << std::endl;
    if (psync_crypto_setup(clib::pclsync_lib::get_lib().get_crypto_pass().c_str(), "no hint"))
      std::cout << "crypto setup failed" << std::endl;
    else{
      std::cout << "crypto setup successful, start=" << psync_crypto_start(clib::pclsync_lib::get_lib().get_crypto_pass().c_str()) << std::endl;
      std::cout << "creating folder=" << psync_crypto_mkdir(0, "Crypto", NULL, NULL) << std::endl;
    }
  }
  clib::pclsync_lib::get_lib().crypto_on_ = true;
}

static char const * status2string (uint32_t status){
  switch (status){
    case PSTATUS_READY: return "READY";
    case PSTATUS_DOWNLOADING: return "DOWNLOADING";
    case PSTATUS_UPLOADING: return "UPLOADING";
    case PSTATUS_DOWNLOADINGANDUPLOADING: return "DOWNLOADINGANDUPLOADING";
    case PSTATUS_LOGIN_REQUIRED: return "LOGIN_REQUIRED";
    case PSTATUS_BAD_LOGIN_DATA: return "BAD_LOGIN_DATA";
    case PSTATUS_BAD_LOGIN_TOKEN : return "BAD_LOGIN_TOKEN";
    case PSTATUS_ACCOUNT_FULL: return "ACCOUNT_FULL";
    case PSTATUS_DISK_FULL: return "DISK_FULL";
    case PSTATUS_PAUSED: return "PAUSED";
    case PSTATUS_STOPPED: return "STOPPED";
    case PSTATUS_OFFLINE: return "OFFLINE";
    case PSTATUS_CONNECTING: return "CONNECTING";
    case PSTATUS_SCANNING: return "SCANNING";
    case PSTATUS_USER_MISMATCH: return "USER_MISMATCH";
    case PSTATUS_ACCOUT_EXPIRED: return "ACCOUT_EXPIRED";
    default :return "Unrecognized status";
  }
}

static void status_change(pstatus_t* status) {
  static int cryptocheck=0;
  static int mount_set=0;
  std::cout << "Down: " <<  status->downloadstr << "| Up: " << status->uploadstr <<", status is " << status2string(status->status) << std::endl;
  *clib::pclsync_lib::get_lib().status_ = *status;
  if (status->status==PSTATUS_LOGIN_REQUIRED){
    psync_set_user_pass(clib::pclsync_lib::get_lib().get_username().c_str(), clib::pclsync_lib::get_lib().get_password().c_str(), (int) clib::pclsync_lib::get_lib().save_pass_);
    std::cout << "logging in" << std::endl;
  }
  else if (status->status==PSTATUS_BAD_LOGIN_DATA){
    if (!clib::pclsync_lib::get_lib().newuser_) {
      clib::pclsync_lib::get_lib().get_pass_from_console();
      psync_set_user_pass(clib::pclsync_lib::get_lib().get_username().c_str(), clib::pclsync_lib::get_lib().get_password().c_str(), (int) clib::pclsync_lib::get_lib().save_pass_);
    }
    else {
    std::cout << "registering" << std::endl;
    if (psync_register(clib::pclsync_lib::get_lib().get_username().c_str(), clib::pclsync_lib::get_lib().get_password().c_str(),1, NULL)){
      std::cout << "both login and registration failed" << std::endl;
      exit(1);
    }
    else{
      std::cout << "registered, logging in" << std::endl;
      psync_set_user_pass(clib::pclsync_lib::get_lib().get_username().c_str(), clib::pclsync_lib::get_lib().get_password().c_str(), (int) clib::pclsync_lib::get_lib().save_pass_);
    }
      
    }
  }
  if (status->status==PSTATUS_READY || status->status==PSTATUS_UPLOADING || status->status==PSTATUS_DOWNLOADING || status->status==PSTATUS_DOWNLOADINGANDUPLOADING){
    if (!cryptocheck){
      cryptocheck=1;
      if (clib::pclsync_lib::get_lib().setup_crypto_) {
        lib_setup_cripto();
      }
    }
    psync_fs_start();
  }
  if (clib::pclsync_lib::get_lib().status_callback_)
    clib::pclsync_lib::get_lib().status_callback_((int)status->status, status2string(status->status));
}

int clib::pclsync_lib::statrt_crypto (const char* pass) {
  std::cout << "calling startcrypto pass: "<<pass << std::endl;
  get_lib().crypto_pass_ = pass;
  lib_setup_cripto();
  return 0;
}
int clib::pclsync_lib::stop_crypto (const char* path) {
  psync_crypto_stop();
  get_lib().crypto_on_ = false;
  return 0;
}
int clib::pclsync_lib::finalize (const char* path) {
  psync_destroy();
  exit(0);
}
static const std::string client_name = " Console Client v.2.0.1";
int clib::pclsync_lib::init()//std::string& username, std::string& password, std::string* crypto_pass, int setup_crypto, int usesrypto_userpass)
{
	std::string software_string = "Windows Dll";// exec("lsb_release -ds");
  psync_set_software_string(software_string.append(client_name).c_str());
  if (setup_crypto_ && crypto_pass_.empty() )
    return 3;
  was_init_ = true;
  
  if (psync_init()){
    std::cout <<"init failed\n"; 
    return 1;
  }
  
   if (!get_mount().empty())
    psync_set_string_setting("fsroot",get_mount().c_str());
  
// _tunnel  = psync_ssl_tunnel_start("127.0.0.1", 9443, "62.210.116.50", 443);
  
  psync_start_sync(status_change, event_handler);
  char * username_old = psync_get_username();

  if (username_old){
    if (username_.compare(username_old) != 0){
      std::cout << "logged in with user " << username_old <<", not "<< username_ <<", unlinking"<<std::endl;
      psync_unlink();
      psync_free(username_old);
      return 2;
    }
    psync_free(username_old);
  }
  
  psync_add_overlay_callback(20,&clib::pclsync_lib::statrt_crypto);
  psync_add_overlay_callback(21,&clib::pclsync_lib::stop_crypto);
  psync_add_overlay_callback(22,&clib::pclsync_lib::finalize);
  
  return 0;
}

clib::pclsync_lib::pclsync_lib() : status_(new pstatus_struct_() ), was_init_(false), setup_crypto_(false)
{}

clib::pclsync_lib::~pclsync_lib()
{

}

