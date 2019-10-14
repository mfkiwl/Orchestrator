#ifndef __TMothH__H
#define __TMothH__H

#include <deque>
#include "CommonBase.h"
#include "Debug.h"
#include "PMsg_p.hpp"
#include "Cli.h"
#include "HostLink.h"
#include "pthread.h"
#include "TaskInfo.h"
#include "poets_msg.h"
#include "P_addr.h"

//==============================================================================

class TMoth : public CommonBase, public HostLink
{

public:
                      TMoth(int,char **,string);
virtual ~             TMoth();
 
// somewhat bodgey function to return the hardware address from the composite; needed because of
// the actual hardware mappings; this is the Orchestrator-side equivalent of toAddr.
static inline unsigned GetHWAddr(P_addr& VAddr) {
	return 	(VAddr.A_box << P_BOX_HWOS)
			| (VAddr.A_board << P_BOARD_HWOS)
			| (VAddr.A_mailbox << P_MAILBOX_HWOS)
			| (VAddr.A_core << P_CORE_HWOS)
			| (VAddr.A_thread << P_THREAD_HWOS);
												};
// static void*          LoadBoard(void*); // threaded version of bootloader
static void*          Twig(void*); // thread to handle Tinsel messages

private:
unsigned              Boot(string);
unsigned              CmLoad(string);
unsigned              CmRun(string);
unsigned              CmStop(string);
#include              "Decode.cpp"
inline virtual string Dname(){ return typeid(*this).name(); }
void                  Dump(FILE * = stdout);
long                  LoadBoard(P_board*); // unthreaded version of bootloader
unsigned              NameDist(PMsg_p*);
unsigned              NameRecl(PMsg_p*);
unsigned              NameTdir(const string&, const string&);
unsigned              OnCmnd(PMsg_p *,unsigned);
unsigned              OnExit(PMsg_p *,unsigned);
void                  OnIdle();
unsigned              OnName(PMsg_p *,unsigned);
unsigned              OnSuper(PMsg_p *,unsigned);
unsigned              OnSyst(PMsg_p *,unsigned);
unsigned              OnTinsel(PMsg_p*, unsigned);
unsigned              OnTinselOut(P_Msg_t *);
void                  StopTwig();
unsigned              SystHW(const vector<string>&);
unsigned              SystKill();
unsigned              SystShow();
unsigned              SystTopo();

void*                 SuperHandle; // dynamically loadable supervisor
int                   (*SupervisorCall)(PMsg_p*, PMsg_p*); // entry point for the Supervisor

P_Msg_t               logMsgBuf[P_MAX_LOGMSG_FRAG];
unsigned              logMsgCnt;
unsigned              logMsgMax;
 
public:
unsigned              PAddress; // address of this mothership in POETS-space
bool                  ForwardMsgs;
 
typedef unsigned (TMoth::*pMeth)(PMsg_p *,unsigned);
typedef map<unsigned,pMeth> FnMap_t;
typedef map<uint16_t,char*> PinBuf_t; // type to hold buffers for messages received from devices

map<string, TaskInfo_t*> TaskMap;             // which tasks are mapped to the machine
vector<pair<pthread_t*,int*>> BootMap;        // which booter is starting which board
vector<FnMap_t*>    FnMapx;
pthread_t           Twig_thread;              // thread for processing tinsel connections
bool                twig_running;             // flag to show twig thread is active

static const int    NumBoards = NUM_BOARDS_PER_BOX;
 
};

//==============================================================================

#endif




