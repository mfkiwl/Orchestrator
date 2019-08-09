/* This file contains the `main` free function for the Orchestrator-MPI
 * launcher. */

#include "LauncherMain.h"

#include <string>

/* Calls 'launch' in a try/catch block. */
int main(int argc, char** argv)
{
    /* Run the launcher in a try/catch block. */
    try
    {
        return Launcher::Launch(argc, argv);
    }

    catch(std::bad_alloc& e)
    {
        printf("%sWe caught a bad_alloc: %s. Closing.\n",
               Launcher::errorHeader, e.what());
        fflush(stdout);
        return 1;
    }

    catch(...)
    {
        printf("%sUnhandled exception caught in main. Closing.\n",
               Launcher::errorHeader);
        fflush(stdout);
        return 1;
    }
}

namespace Launcher
{

/* Constructs the MPI command to run and writes it to 'command', given a set
 * of 'hosts'. */
void BuildCommand(std::string overrideHost, std::set<std::string>* hosts,
                  std::string* command)
{
    /* Boilerplate, DRY with help string. */
    *command = "mpiexec.hydra"
        " -genv LD_LIBRARY_PATH " INTERNAL_LIB_PATH
        " : -n 1 ./root"
        " : -n 1 ./logserver"
        " : -n 1 ./rtcl";

    /* If we've set the override, just spawn a mothership on that host. */
    if (!overrideHost.empty())
    {
        *command += " : -n 1 --host " + overrideHost + " ./mothership";
    }

    /* Otherwise, if there are no hosts, just optimistically spawn a
     * mothership on this box. */
    else if (hosts->empty())
    {
        *command += " : -n 1 ./mothership";
    }

    /* Otherwise, spawn one mothership for each host. */
    else
    {
        WALKSET(string, (*hosts), host)
        {
            *command += " : -n 1 --host " + *host + " ./mothership";
        }
    }
}

/* Reads the hardware description file at hdfPath, and populates the vector at
 * hosts with the names of hosts obtained from that file. Returns 0 if all is
 * well, and 1 if we need to leave. */
int GetHosts(std::string hdfPath, std::set<std::string>* hosts)
{
    if (!file_readable(hdfPath.c_str()))
    {
        printf("%sCannot find hardware description file at %s. Closing.\n",
               errorHeader, hdfPath.c_str());
        return 1;
    }

// Take it away, ADB.
JNJ Jh(hdfPath);                        // OK, let's do it
vH sects;
Jh.FndSect("engine_box",sects);         // Got all the sections called ....
if (sects.empty()) return 0;            // None there?
WALKVECTOR(hJNJ,sects,i) {              // Walk all the sections...
  vH recds;
  Jh.GetRecd(*i,recds);                 // Get the records
  WALKVECTOR(hJNJ,recds,j) {            // Walk the records
    vH varis;
    Jh.GetVari(*j,varis);               // Get the variables (box names)
    WALKVECTOR(hJNJ,varis,k) {
      if ((*k)->str.empty()) continue;        // If it's not blank.....
      if ((*k)->qop==Lex::Sy_plus) continue;  // If it's not a '+'.....
      vH subs;
      Jh.GetSub(*k,subs);               // Get the box variable subname(s)
      WALKVECTOR(hJNJ,subs,l) {         // Walk them
        if ((*l)->str=="hostname") {    // Look for.....
          vH subs2;
          Jh.GetSub(*l,subs2);          // And get them
          if (subs2.empty()) continue;  // Non-empty set?
          hosts->insert(subs2[0]->str); // At last! Save the damn thing
        }
      }
    }
  }
}
return 0;
}

/* Launches the Orchestrator, unsurprisingly. */
int Launch(int argc, char** argv)
{
    /* Print input arguments, if we're in debug mode. */
    DebugPrint("%sWelcome to the POETS Launcher. Raw arguments:\n",
               debugHeader);
    #if ORCHESTRATOR_DEBUG
    for (int argIndex=0; argIndex<argc; argIndex++)
    {
        DebugPrint("%s%sArgument %d: %s\n", debugHeader, debugIndent, argIndex,
                   argv[argIndex]);
    }
    DebugPrint("%s\n", debugHeader);
    #endif

    /* Argument map. */
    std::map<std::string, std::string> argKeys;
    argKeys["help"] = "h";
    argKeys["file"] = "f";
    argKeys["override"] = "o";

    /* Defines help string, printed when user calls with `-h`. */
    std::string helpDoc = dformat(
"Usage: %s [-h] [-f] FILE [-o] HOST\n"
"\n"
"This is the Orchestrator launcher. It starts the following Orchestrator "
"processes:\n"
"\n"
" - root\n"
" - logserver\n"
" - rtcl\n"
" - mothership (some number of them)\n"
"\n"
"If you are bamboozled, try compiling with debugging enabled (i.e. `make "
"debug`). This launcher accepts the following optional arguments:\n"
"\n"
" -%s: Prints this help text.\n"
" -%s FILE: Path to a hardware file to read hostnames from, in order to start "
"motherships.\n"
" -%s HOST: Override all Mothership hosts, specified from a hardware "
"description file, with HOST.\n",
argv[0], argKeys["help"].c_str(), argKeys["file"].c_str(),
argKeys["override"].c_str());

    /* Parse the input arguments. */
    std::string concatenatedArgs;
    for(int i=1; i<argc; concatenatedArgs += argv[i++]);

    /* Decode, and check for errors. */
    Cli cli(concatenatedArgs);
    if (cli.problem.prob)
    {
        printf("%sCommand line error at about input character %d\n",
               errorHeader, cli.problem.col);
        return 1;
    }

    /* Staging for input arguments. */
    std::string hdfPath;
    std::string overrideHost;

    /* Read each argument in turn. */
    std::string currentArg;
    WALKVECTOR(Cli::Cl_t, cli.Cl_v, argIt)
    {
        currentArg = (*argIt).Cl;
        if (currentArg == argKeys["help"])  /* Help: Print and leave. */
        {
            printf(helpDoc.c_str());
            return 0;
        }

        if (currentArg == argKeys["file"])  /* Hardware file: Store it. */
        {
            if (hdfPath.empty())
            {
                hdfPath = (*argIt).GetP(0);
            }
            else
            {
                printf("[WARN] Launcher: Ignoring duplicate input file "
                       "argument.\n");
            }
        }

        if (currentArg == argKeys["override"])  /* Override: Store it. */
        {
            if (overrideHost.empty())
            {
                overrideHost = (*argIt).GetP(0);
                if (overrideHost.empty())
                {
                    printf("[WARN] Launcher: Override host argument empty. "
                           "Ignoring.\n");
                }
            }
            else
            {
                printf("[WARN] Launcher: Ignoring duplicate override"
                       "argument.\n");
            }
        }
    }

    /* Print what happened, if anyone is listening. */
    #if ORCHESTRATOR_DEBUG
    DebugPrint("%sParsed arguments:\n", debugHeader);
    if (hdfPath.empty())
    {
        DebugPrint("%s%sHardware description file path: Not specified.\n",
                   debugHeader, debugIndent);
    }
    else
    {
        DebugPrint("%s%sHardware description file path: %s\n",
                   debugHeader, debugIndent, hdfPath.c_str());
    }
    if (overrideHost.empty())
    {
        DebugPrint("%s%sOverride host: Not specified.\n",
                   debugHeader, debugIndent);
    }
    else
    {
        DebugPrint("%s%sOverride host: %s\n", debugHeader, debugIndent,
                   overrideHost.c_str());
    }
    DebugPrint("%s\n", debugHeader);
    #endif

    /* Read the input file, if supplied, and get a set of hosts we must launch
     * Mothership processes on. Don't bother if we're overriding though.
     *
     * If there is no input file, hosts will remain empty. */
    std::set<std::string> hosts;
    if (!hdfPath.empty() and overrideHost.empty())
    {
        int rc = GetHosts(hdfPath, &hosts);
        if (rc != 0) return rc;

        /* Print the hosts we found, if anyone is listening. */
        #if ORCHESTRATOR_DEBUG
        if(!hosts.empty())
        {
            DebugPrint("%sAfter reading the input file, I found the following "
                       "hosts:\n", debugHeader);
            WALKSET(std::string, hosts, hostIterator)
            {
                DebugPrint("%s%s- %s\n", debugHeader, debugIndent,
                           (*hostIterator).c_str());
            }
            DebugPrint("%s\n", debugHeader);
        }

        else
        {
            DebugPrint("%sAfter parsing the input file, I found no hosts.\n",
                       debugHeader);
        }
        #endif
    }

    /* Print that we're overriding, if we are. */
    #if ORCHESTRATOR_DEBUG
    else if (!overrideHost.empty())
    {
        DebugPrint("%sIgnoring input file, and instead using the override "
                   "passed in as an argument.\n", debugHeader);
    }
    #endif

    /* Build the MPI command. */
    std::string command;
    BuildCommand(overrideHost, &hosts, &command);
    DebugPrint("%sRunning this command: %s\n.", debugHeader, command.c_str());
    system(command.c_str());
    return 0;
}
}  /* End namespace */