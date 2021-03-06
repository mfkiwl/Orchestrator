#include "PMsg_p.hpp"

//==============================================================================

/* This is the MPI-aware class derived from Msg_p (which is MPI-agnostic).
It's not my finest. Ultimately we should be able to send to sets of ranks, and
lots of other fiddly stuff, but at the moment I'm just shoving functins in as
and when I come across a need.
*/

//==============================================================================

PMsg_p::PMsg_p(MPI_Comm c):Msg_p(){comm=c;}
PMsg_p::PMsg_p(byte * pb,int l,MPI_Comm c):Msg_p(pb,l){comm=c;}
PMsg_p::PMsg_p(PMsg_p & r):Msg_p(r){comm=r.comm;}
PMsg_p::PMsg_p(const PMsg_p & r):Msg_p(const_cast<PMsg_p&>(r)){comm=r.comm;}
PMsg_p::~PMsg_p(void){}

//------------------------------------------------------------------------------

void PMsg_p::Send()
{
Send(Tgt());
}

//------------------------------------------------------------------------------

void PMsg_p::Bcast()
// I don't *know* that MPI_Bcast doesn't work.
// BUT I *do* know that this does.
{
if (comm==MPI_COMM_NULL) return;   // no communicator associated with this message?
int flag, sflag;
MPI_Finalized(&flag);
if (flag) return;   // MPI closed down already?
int Usize,Urank;
MPI_Comm_test_inter(comm,&flag); // sending over an intercomm?
if (flag)
{
   MPI_Comm_remote_size(comm,&Usize);  // MPI remote Universe size
   MPI_Comm_rank(MPI_COMM_WORLD,&Urank);  // My place in my local Universe
}
else
{
   MPI_Comm_size(comm,&Usize);  // MPI Universe size
   MPI_Comm_rank(comm,&Urank);  // My place within it
}
Src(Urank);
byte * bs = Stream();                  // Turn the message into a unified stream
unsigned len = Length();               // Length of stream
for (int i=0;i<Usize;i++) {            // Send a copy everywhere
  if (!flag && (i==Urank)) continue;   // ...except to myself
  Tgt(i);                              // Load target rank
  Ztime(0,MPI_Wtime());                // Timestamp departure
  MPI_Request request;
  MPI_Ibsend(bs,len,MPI_CHAR,i,tag,comm,&request);
  MPI_Status status;
  do MPI_Test(&request,&sflag,&status);
  while (sflag==0);
}
}

//------------------------------------------------------------------------------

void PMsg_p::Send(int dest)
{
if (comm==MPI_COMM_NULL) return;   // no communicator associated with this message?
int flag,Usize;
if (MPI_Finalized(&flag)!=0) return;          // MPI closed down already?
MPI_Comm_test_inter(comm,&flag);              // Sending over an intercomm?
if (flag) MPI_Comm_remote_size(comm,&Usize);  // MPI remote Universe size
else MPI_Comm_size(comm,&Usize);              // MPI Universe size
if (dest < 0 || dest >= Usize) return;        // Invalid rank?
Tgt(dest);                                    // In case it's not there
Ztime(0,MPI_Wtime());                         // Timestamp departure
MPI_Request request;
MPI_Status status;
//printf("& Byte stream = %#010x, length = %d\n",Stream(),Length()); fflush(stdout);
//printf("PMsg_p::Send(key=%#010x,dest=%d), BEFORE\n",Key(),dest); fflush(stdout);
MPI_Ibsend(Stream(),Length(),MPI_CHAR,dest,tag,comm,&request);
//printf("PMsg_p::Send, waiting\n"); fflush(stdout);
int i=0;
do { i++; MPI_Test(&request,&flag,&status); } while (flag==0);
// MPI_Wait(&request,&status);
//printf("PMsg_p::Send(key=%#010x,dest=%d), returning after %d tests\n",
//        Key(),dest,i);
//fflush(stdout);
}

//==============================================================================

