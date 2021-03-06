//------------------------------------------------------------------------------

#include "Injector.h"
#include "CommonBase.h"
#include "PMsg_p.hpp"
#include "mpi.h"
#include "Pglobals.h"
#include "jnj.h"
#include "lex.h"
#include "Cli.h"
#include <stdio.h>

//==============================================================================

Injector::Injector(int argc,char * argv[],string d):
  CommonBase(argc,argv,d,string(__FILE__))
{

FnMapx.push_back(new FnMap_t);    // create a new event map in the derived class

                                       // Load the default message map
(*FnMapx[0])[PMsg_p::KEY(Q::INJCT,Q::ACK )] = &Injector::OnInjectAck;
(*FnMapx[0])[PMsg_p::KEY(Q::INJCT,Q::FLAG)] = &Injector::OnInjectFlag;

MPISpinner();                          // Spin on MPI messages; exit only on DIE

printf("********* Injector rank %d on the way out\n",Urank); fflush(stdout);
}

//------------------------------------------------------------------------------

Injector::~Injector()
{
//printf("********* Injector rank %d destructor\n",Urank); fflush(stdout);
WALKVECTOR(FnMap_t*, FnMapx, F)
    delete *F;
}

//------------------------------------------------------------------------------

unsigned Injector::Connect(string svc)
{
unsigned err = CommonBase::Connect(svc);
FnMapx.push_back(new FnMap_t);    // create a new event map in the derived class
// set up the function map for the Injector.
(*FnMapx[FnMapx.size()-1])[PMsg_p::KEY(Q::INJCT,Q::ACK )] = &Injector::OnInjectAck;
(*FnMapx[FnMapx.size()-1])[PMsg_p::KEY(Q::INJCT,Q::FLAG)] = &Injector::OnInjectFlag;
return err;
}

//------------------------------------------------------------------------------

void Injector::Dump(FILE * fp)
{
fprintf(fp,"Injector dump+++++++++++++++++++++++++++++++++++\n");
fprintf(fp,"\nI am a 'umble injector. I have no internal state.\n\n");
fprintf(fp,"Injector dump-----------------------------------\n");
CommonBase::Dump(fp);
}

//------------------------------------------------------------------------------

unsigned Injector::OnInjectFlag(PMsg_p * Z,unsigned cIdx)
// Incoming inject message from the monkey
// INJCT|FLAG|-|-| (1:string) originating command
{
string s;
Z->Get(1,s);                           // Unpack command string
Cli cmnd(s);                           // Rebuild command

cmnd.Dump();                           // For want of anything better?
Dump();

// Just for a giggle, inject a "What's the time?" command into the Root:

string query = "system /time";
PMsg_p Msg;
Msg.comm = Comms[cIdx];                // target the appropriate comm
Msg.Put(1,&query);                     // Wrap the command into a message
Msg.Key(Q::INJCT,Q::REQ);              // Message type
Msg.Send(pPmap[RootCIdx()]->U.Root);               // Send it to the injector proces

return 0;
}

//------------------------------------------------------------------------------

unsigned Injector::OnInjectAck(PMsg_p * Z,unsigned cIdx)
// Incoming inject message from whatever command has just executed
// INJCT|ACK|   -|   - (???)
{

Z->Dump();                             // For want of anything better?

return 0;
}

//==============================================================================

