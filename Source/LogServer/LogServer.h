#ifndef __LogServerH__H
#define __LogServerH__H

#include "CommonBase.h"

//==============================================================================

class LogServer : public CommonBase
{

public:
                    LogServer(int,char **,string);
virtual ~           LogServer();

typedef unsigned    (LogServer::*pMeth)(PMsg_p *,unsigned);
typedef map<unsigned,pMeth> FnMap_t;

private:
string              Assemble(int,vector<string> &);
unsigned            Connect(string);
#include            "Decode.cpp"
void                Dump(FILE * = stdout);
void                InitFile();
void                LoadMessages(string);
void                OnIdle();
unsigned            OnLogP(PMsg_p *,unsigned);

vector<FnMap_t*>    FnMapx;


struct M {                             // Message file extracted
  M(){}                                // Keep the compiler happy
  M(char c,string & s):typ(c),Ms(s){}
  unsigned args();                     // Count the arguments in the template
  char typ;                            // Message category
  string Ms;                           // Message string
};
map<int,M>          Msgs;              // Message map
map<char,unsigned>  Mcount;            // Counter for each type
FILE *              logfp;             // Log file pointer
string              logfn;             // Log file name

};

//==============================================================================

#endif
/*
//------------------------------------------------------------------------------

#ifndef __log__H
#define __log__H

#include <stdio.h>
#include "flat.h"
#include "macros.h"
#include "dfprintf.h"
#include "filename.h"
#include <cstdarg>
#include <utility>

#include "jnj.h"
#include <map>
#include <string>
#include <vector>
using namespace std;

//==============================================================================
            
class Log
{
struct       data;
public:
             Log(string,string);
virtual ~    Log();
void         Post(int,vector<string> &);
static void  Post(void *,int,vector<string> &);
bool         Post(int,string=S_,string=S_,string=S_,string=S_,string=S_,
                      string=S_,string=S_,string=S_,string=S_,string=S_,
                      string=S_,string=S_,string=S_,string=S_,string=S_,
                      string=S_);
void         Dump(FILE * =stdout);
void         Dump(string);
void         DumpMap();
int          MessCnt(string);
void         MessCnt(FILE * =0);
bool         MOP(char,Lex::Sytype,char);
void         Reset();

static Log * SetupLog(char *[],string);
void         Silent(bool s = true) { silent = s; }

private:

void         Post(string,int = -1);
void         Dump(FILE *,data &);

bool   silent;                         // Talk-to-stdout flag

string messagefile;                    // Source message file

struct M {                             // Message file extracted
  M(){}                                // Keep the compiler happy
  M(char c,string & s):typ(c),Ms(s){}
  int args();                          // Count the arguments
  char typ;                            // Message category
  string Ms;                           // Message string
};
static map<int,M> Msgs;                // Message map

struct   data {                        // Data carrier
  data(){}                             // Keep the compiler happy
  data(char,int);                      // Usual way in
  data(string,int);                    // Broken way in
  void   init(int);
  char   typ;                          // Message type
  int    id;                           // Id
  double MPI_ti;                       // MPI time stored
  string WC_ti;                        // Wallclock time stored
  vector<string> args;                 // String arguments
};
vector<data> Vd;                       // The data itself

enum LoType {L_0=0,Lo_NoFl,Lo_NoMs,Lo_LimX,Lo_XXXX};
static const string BuiltIn[Lo_XXXX+1];
static const string S_;

};

//==============================================================================

#endif

*/
