//------------------------------------------------------------------------------

#include "TMoth.h"
#include <stdio.h>
#include <sstream>
#include <dlfcn.h>
#include "Pglobals.h"
#include "CMsg_p.h"
#include "flat.h"
#include "string.h"
#include "limits.h"

#include <iostream>
#include <iomanip>

const int   TMoth::NumBoards;

//==============================================================================

TMoth::TMoth(int argc,char * argv[],string d) :
CommonBase(argc,argv,d,string(__FILE__)), HostLink()
{


    FnMapx.push_back(new FnMap_t);    // create a new event map in the derived class

    // Load the incoming event map
    (*FnMapx[0])[PMsg_p::KEY(Q::EXIT                )] = &TMoth::OnExit; // overloads CommonBase
    (*FnMapx[0])[PMsg_p::KEY(Q::CMND,Q::LOAD        )] = &TMoth::OnCmnd;
    (*FnMapx[0])[PMsg_p::KEY(Q::CMND,Q::RUN         )] = &TMoth::OnCmnd;
    (*FnMapx[0])[PMsg_p::KEY(Q::CMND,Q::STOP        )] = &TMoth::OnCmnd;
    (*FnMapx[0])[PMsg_p::KEY(Q::NAME,Q::DIST        )] = &TMoth::OnName;
    (*FnMapx[0])[PMsg_p::KEY(Q::NAME,Q::RECL        )] = &TMoth::OnName;
    (*FnMapx[0])[PMsg_p::KEY(Q::NAME,Q::TDIR        )] = &TMoth::OnName;
    (*FnMapx[0])[PMsg_p::KEY(Q::SUPR                )] = &TMoth::OnSuper;
    (*FnMapx[0])[PMsg_p::KEY(Q::SYST,Q::HARD        )] = &TMoth::OnSyst;
    (*FnMapx[0])[PMsg_p::KEY(Q::SYST,Q::KILL        )] = &TMoth::OnSyst;
    (*FnMapx[0])[PMsg_p::KEY(Q::SYST,Q::SHOW        )] = &TMoth::OnSyst;
    (*FnMapx[0])[PMsg_p::KEY(Q::SYST,Q::TOPO        )] = &TMoth::OnSyst;
    (*FnMapx[0])[PMsg_p::KEY(Q::TINS                )] = &TMoth::OnTinsel;

    // mothership's address in POETS space is the host thread ID: X coordinate 0,
    // Y coordinate max (within box?? TODO: check this!). TODO: Update this for multi-box!
    PAddress = TinselMeshYLenWithinBox << (TinselMeshXBits+TinselLogCoresPerBoard+TinselLogThreadsPerCore);
    
    twig_running = false;
    ForwardPkts = false; // don't forward tinsel traffic yet
    
    InstrumentationInit();          // Inisitalise Instrumentation dir

    MPISpinner();                          // Spin on *all* messages; exit on DIE
    DebugPrint("Exiting Mothership. Closedown flags: AcceptConns: %s, "
    "ForwardPkts: %s\n", AcceptConns ? "true" : "false",
    ForwardPkts ? "true" : "false");
    if (twig_running) StopTwig(); // wait for the twig thread, if it's still somehow running.
    printf("********* Mothership rank %d on the way out\n",Urank); fflush(stdout);
}

//------------------------------------------------------------------------------

TMoth::~TMoth()
{
    //printf("********* Mothership rank %d destructor\n",Urank); fflush(stdout);
    WALKMAP(string,TaskInfo_t*,TaskMap,T)
    delete T->second;
    
    InstrumentationEnd();       // Teardown the Instrumentation map
    LogHandlerEnd();            // Teardown the Log Packet map
}

//------------------------------------------------------------------------------

unsigned TMoth::Boot(string task)
{
    DebugPrint("Entering boot stage\n");
    if (TaskMap.find(task) == TaskMap.end())
    {
      Post(107,task);
      return 1;
    }
    
    DebugPrint("Task %s being booted\n", task.c_str());
    switch(TaskMap[task]->status)
    {
    case TaskInfo_t::TASK_BOOT:
        Post(513, task, TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
        TaskMap[task]->status = TaskInfo_t::TASK_ERR;
        return 2;
        
      
    case TaskInfo_t::TASK_RDY:
      {
        DebugPrint("Task is ready to be booted\n");   // never called?
        uint32_t mX, mY, core, thread;
        // create a response bitmap to receive the various startup barrier packets.
        // do this before running 'go' so that we can receive the responses in one block.
        map<unsigned, vector<unsigned>*> t_start_bitmap;
        vector<P_core*> taskCores = TaskMap[task]->CoresForTask();
        P_addr coreAddress;
        WALKVECTOR(P_core*,taskCores,C)
        {
            unsigned numThreads = (*C)->P_threadm.size();
            if (numThreads)
            {
                (*C)->get_hardware_address()->populate_a_software_address(&coreAddress);
                t_start_bitmap[TMoth::GetHWAddr(coreAddress)] = new vector<unsigned>(numThreads/(8*sizeof(unsigned)),UINT_MAX);
                unsigned remainder = numThreads%(8*sizeof(unsigned));
                if (remainder) t_start_bitmap[TMoth::GetHWAddr(coreAddress)]->push_back(UINT_MAX >> ((8*sizeof(unsigned))-remainder));
            }
        }
        DebugPrint("Task start bitmaps created for %d cores\n", t_start_bitmap.size());

        /* Actually boot the cores. This is pretty damn nuanced - at the
         * moment, we are starting all of the cores, indiscriminantly of
         * whether or not they are actually used for this task. This is due to
         * bug #41 on GitHub (Some applications when restricted to
         * NumDevicesPerThread<32 won't pass barrier), where we would get stuck
         * in the barrier-while loop below.
         *
         * Problematically, starting all of the cores is undesirable, because
         * another task could be deployed to this mothership.
         *
         * Another way to do this (which doesn't work), is to 'go' all cores
         * individually, after starting them all. This is apparently safer than
         * 'starting and going' in the same loop (verbally from MFN).
         *
         * YMMV. */
        WALKVECTOR(P_core*,taskCores,C)
        {
            (*C)->get_hardware_address()->populate_a_software_address(&coreAddress);
            fromAddr(TMoth::GetHWAddr(coreAddress),&mX,&mY,&core,&thread);
            DebugPrint("Booting %d threads on core 0x%X (at x:%d y:%d c:%d)\n",(*C)->P_threadm.size(),TMoth::GetHWAddr(coreAddress),mX,mY,core);
            DebugPrint("* startOne(meshX=%u, meshY=%u, coreId=%u, numThreads=%lu)\n",
            mX, mY, core, (*C)->P_threadm.size());
            startOne(mX,mY,core,(*C)->P_threadm.size());
        }
        DebugPrint("* go()\n");
        go();  /* From HostLink. */
        DebugPrint("Instructed %u cores to go.\n", taskCores.size());

        //----------------------------------------------------------------------
        // Handle Barrier Messages
        //----------------------------------------------------------------------
        P_Pkt_t barrier_pkt;
        while (!t_start_bitmap.empty())
        {
            // Tinsel call, so this is a Message rather than a Packet
            recvMsg(&barrier_pkt, p_hdr_size());
            DebugPrint("Received a packet from a core during application barrier\n");
            
            if (   (barrier_pkt.header.swAddr & P_SW_MOTHERSHIP_MASK)
                && (barrier_pkt.header.swAddr & P_SW_CNC_MASK)
                && (((barrier_pkt.header.swAddr & P_SW_OPCODE_MASK)
                        >> P_SW_OPCODE_SHIFT) == P_CNC_BARRIER) )
            {
                uint32_t srcAddr = barrier_pkt.header.pinAddr;
                
                DebugPrint("Barrier packet from thread ID 0x%X\n", srcAddr);
                
                fromAddr(srcAddr,&mX,&mY,&core,&thread);       // Decode Address
                
                unsigned hw_core = toAddr(mX,mY,core,0);
                DebugPrint("Received a barrier acknowledge from core %#X\n", hw_core);
                
                if (t_start_bitmap.find(hw_core) != t_start_bitmap.end())
                {
                    DebugPrint("Thread %d on core %d responding\n", thread, core);
                    DebugPrint("Core bitmap for thread %d before ack: %#X\n", thread, (*t_start_bitmap[hw_core])[thread/(8*sizeof(unsigned))]);
                    (*t_start_bitmap[hw_core])[thread/(8*sizeof(unsigned))] &= (~(1 << (thread%(8*sizeof(unsigned)))));
                    DebugPrint("Core bitmap for thread %d after acknowledge: %#X\n", thread, (*t_start_bitmap[hw_core])[thread/(8*sizeof(unsigned))]);
                    vector<unsigned>::iterator S;
                    DebugPrint("Core bitmap currently has %d elements\n", t_start_bitmap.size());
                    for (S = t_start_bitmap[hw_core]->begin(); S != t_start_bitmap[hw_core]->end(); S++) if (*S) break;
                    DebugPrint("Core bitmap for core %d has %d subelements\n", hw_core, t_start_bitmap[hw_core]->size());
                    if (S == t_start_bitmap[hw_core]->end())
                    {
                        DebugPrint("Removing core bitmap for core %d\n", thread, t_start_bitmap[hw_core]->size());
                        t_start_bitmap[hw_core]->clear();
                        delete t_start_bitmap[hw_core];
                        t_start_bitmap.erase(hw_core);
                    }
                }
            }
        }
        //----------------------------------------------------------------------
        
        
        DebugPrint("%d cores passed Mothership barrier and awaiting start signal \n", taskCores.size());
        // MPI_Barrier(Comms[0]);         // barrier on the mothercore side (temporarily removed until we have multi-mothership systems)
        // create a thread (later will be a process) to deal with packets from
        // tinsel cores. Have to do it here, after all the initial setup is complete,
        // otherwise setup barrier communications may fail.
        void* args = this;
        // at this point, load the Supervisor for the task.
        SuperHandle = dlopen((TaskMap[task]->BinPath+"/libSupervisor.so").c_str(), RTLD_NOW);
        if (!SuperHandle) Post(532,(TaskMap[task]->BinPath+"/libSupervisor.so"),int2str(Urank),string(dlerror()));
        else
        {
            Post (540,int2str(Urank));
            int (*SupervisorInit)() = reinterpret_cast<int (*)()>(dlsym(SuperHandle, "SupervisorInit"));
            string badFunc("");
            if (!SupervisorInit || (*SupervisorInit)()) badFunc = "SupervisorInit";
            else if ((SupervisorCall = reinterpret_cast<int (*)(PMsg_p*, PMsg_p*)>(dlsym(SuperHandle, "SupervisorCall"))) == NULL) badFunc = "SupervisorCall";
            if (badFunc.size()) Post(533,badFunc,int2str(Urank),string(dlerror()));
        }
        ForwardPkts = true; // set forwarding on so thread doesn't immediately exit
        if (pthread_create(&Twig_thread,NULL,Twig,args))
        {
            twig_running = false;
            ForwardPkts = false;
            Post(531, int2str(Urank));
        }
        else twig_running =true;
        TaskMap[task]->status = TaskInfo_t::TASK_BARR; // now at barrier on the tinsel side.
        return 0;
      }
    case TaskInfo_t::TASK_RUN:
    case TaskInfo_t::TASK_STOP:
    case TaskInfo_t::TASK_END:
        break;
    default:
        TaskMap[task]->status = TaskInfo_t::TASK_ERR;
    }
    Post(511,task,"booted",TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
    return 3;
}

//------------------------------------------------------------------------------

unsigned TMoth::CmLoad(string task)
// Load a task to the system
{
    DebugPrint("%d tasks deployed to Mothership\n", TaskMap.size());
    if (TaskMap.find(task) == TaskMap.end())
    {
        Post(515, task, int2str(Urank));
        return 0;
    }
    DebugPrint("Task %s found\n", task.c_str());
    WALKMAP(string,TaskInfo_t*,TaskMap,T)
    {
        // only one task can be active at a time (for the moment. Later we may want more sophisticated task mapping
        // that allows multiple active tasks on non-overlapping board sets)
        if ((T->first != task) && (T->second->status & (TaskInfo_t::TASK_BOOT | TaskInfo_t::TASK_RDY | TaskInfo_t::TASK_BARR | TaskInfo_t::TASK_RUN | TaskInfo_t::TASK_STOP | TaskInfo_t::TASK_ERR)))
        {
            map<unsigned char,string>::const_iterator othStatus = TaskInfo_t::Task_Status.find(T->second->status);
            if (othStatus == TaskInfo_t::Task_Status.end())
            T->second->status = TaskInfo_t::TASK_ERR;
            Post(508,T->first);
            return 2;
            Post(509,task,T->first,othStatus->second);
            return 0;
        }
    }
    DebugPrint("Task status confirmed as TASK_LOAD\n");
    if (!((TaskMap[task]->status == TaskInfo_t::TASK_IDLE) || (TaskMap[task]->status == TaskInfo_t::TASK_END)))
    {
        Post(511,task,"loaded to hardware",TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
        return 0;
    }
    TaskMap[task]->status = TaskInfo_t::TASK_BOOT;
    DebugPrint("Task status is TASK_BOOT\n");
    long coresThisTask = 0;
    long coresLoaded = 0;
    DebugPrint("Task will use %ld boards.\n",
    TaskMap[task]->BoardsForTask().size());
    // for each board mapped to the task,
    WALKVECTOR(P_board*,TaskMap[task]->BoardsForTask(),B)
    {
        DebugPrint("Board \"%s\" will use %ld mailboxes.\n",
        (*B)->Name().c_str(), (*B)->G.SizeNodes());
        WALKPDIGRAPHNODES(AddressComponent, P_mailbox*,
        unsigned, P_link*,
        unsigned, P_port*, (*B)->G, MB)
        {
            DebugPrint("Mailbox \"%s\" will use %ld cores\n",
            (*B)->G.NodeData(MB)->Name().c_str(),
            (*B)->G.NodeData(MB)->P_corem.size());
            // less work to compute as we go along than to call CoresForTask.size()
            // MLV: Not so sure in a post-mailbox world any more...
            coresThisTask += (*B)->G.NodeData(MB)->P_corem.size();
        }
        coresLoaded += LoadBoard(*B); // load the board (unthreaded model)
    }
    DebugPrint("All boards finished. %ld cores were in the hardware model for "
    "this task. %ld cores have been loaded.\n",
    coresThisTask, coresLoaded);
    // abandoning the boot if load failed
    if (coresLoaded < coresThisTask)
    {
        TaskMap[task]->status = TaskInfo_t::TASK_ERR;
        Post(518,task,int2str(coresLoaded),int2str(coresThisTask));
        return 1;
    }
    TaskMap[task]->status = TaskInfo_t::TASK_RDY;
    DebugPrint("Task status is TASK_RDY\n");
    // boot the board (which has to be done from the main thread as it is not
    // thread-safe)
    return Boot(task);
}

//------------------------------------------------------------------------------

unsigned TMoth::CmRun(string task)
// Run a specified task. This passes the tinsel-side barrier to start
// application execution
{
    if (TaskMap.find(task) == TaskMap.end())
    {
        Post(107, task);
        return 0;
    }
    switch(TaskMap[task]->status)
    {
    case TaskInfo_t::TASK_BOOT:
    case TaskInfo_t::TASK_RDY:
        {
            Post(513, task, TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
            TaskMap[task]->status = TaskInfo_t::TASK_ERR;
            return 1;
        }
    case TaskInfo_t::TASK_IDLE:
    case TaskInfo_t::TASK_END:
        Post(511, task,"run",TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
        return 0;
    case TaskInfo_t::TASK_STOP:
        Post(512, task,"run","TASK_STOP");
        TaskMap[task]->status = TaskInfo_t::TASK_ERR;
        return 2;
    case TaskInfo_t::TASK_RUN:
        Post(511, task,"run","TASK_RUN");
        return 0;
    case TaskInfo_t::TASK_BARR:
        {
            DebugPrint("Task %s entering tinsel barrier\n",task.c_str());
            
            //Assemble a Barrier packet
            P_Pkt_t barrier_pkt;
            barrier_pkt.header.swAddr = ((0 << P_SW_MOTHERSHIP_SHIFT)
                                            & P_SW_MOTHERSHIP_MASK);
            barrier_pkt.header.swAddr |= ((1 << P_SW_CNC_SHIFT)
                                            & P_SW_CNC_MASK);
            barrier_pkt.header.swAddr |= ((P_CNC_INIT << P_SW_OPCODE_SHIFT)
                                            & P_SW_OPCODE_MASK);                // and is of packet type __init__
            barrier_pkt.header.swAddr |= ((P_ADDR_BROADCAST << P_SW_DEVICE_SHIFT)
                                            & P_SW_DEVICE_MASK);                               
            
            barrier_pkt.header.pinAddr = ((P_SUP_PIN_INIT << P_HD_TGTPIN_SHIFT)
                                            & P_HD_TGTPIN_MASK);                // it goes to the system __init__ pin
            barrier_pkt.header.pinAddr |= ((0 << P_HD_DESTEDGEINDEX_SHIFT)
                                            & P_HD_DESTEDGEINDEX_MASK);         // no edge index necessary.
            
            uint32_t flits = p_hdr_size() >> TinselLogBytesPerFlit;
            if(flits == 0) ++flits;
            
            DebugPrint("Building thread list for task %s\n",task.c_str());
            // build a list of the threads in this task (that should be released from barrier)
            vector<unsigned> threadsToRelease;
            P_addr threadAddress;
            WALKVECTOR(P_thread*,TaskMap[task]->ThreadsForTask(),R)
            {
                (*R)->get_hardware_address()->populate_a_software_address(&threadAddress);
                threadsToRelease.push_back(TMoth::GetHWAddr(threadAddress));
            }
            DebugPrint("Issuing barrier release to %d threads in task %s, using "
            "packet address 0x%X\n", threadsToRelease.size(),
            task.c_str(), DEST_BROADCAST);
            // and then issue the barrier release to the threads.
            WALKVECTOR(unsigned,threadsToRelease,R)
            {
                DebugPrint("Attempting to send barrier release packet to the thread "
                "with hardware address %u.\n", *R);
                send(*R, flits, &barrier_pkt, true);
            }
            DebugPrint("Tinsel threads now on their own for task %s\n",task.c_str());
            TaskMap[task]->status = TaskInfo_t::TASK_RUN;
            return 0;
        }
    default:
        TaskMap[task]->status = TaskInfo_t::TASK_ERR;
    }
    Post(511,task,"run",TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
    return 2;
}

//------------------------------------------------------------------------------

unsigned TMoth::CmStop(string task)
// Handle a stop command (which ends the simulation)
{
    if (TaskMap.find(task) == TaskMap.end())
    {
        Post(107, task);
        return 0;
    }
    switch(TaskMap[task]->status)
    {
    case TaskInfo_t::TASK_IDLE:
    case TaskInfo_t::TASK_END:
        Post(511, task,"stopped",TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
        return 0;   
    case TaskInfo_t::TASK_BOOT:
    case TaskInfo_t::TASK_RDY:
        {
            Post(813, task, TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
            return 1;
        }
    case TaskInfo_t::TASK_BARR:
        {
            // if we are at the barrier the simplest approach to
            // stop cleanly is simply to start and immediately stop.
            // thus as long as starting doesn't error, fall through
            // to the running condition immediately below.
            if (CmRun(task))
            {
                TaskMap[task]->status = TaskInfo_t::TASK_ERR;
                break;
            }
        }
    case TaskInfo_t::TASK_RUN:
        {
            TaskMap[task]->status = TaskInfo_t::TASK_STOP;
            
            // set up for shutdown by creating a global stop packet
            //Assemble a Barrier packet
            P_Pkt_t stop_pkt;
            stop_pkt.header.swAddr = ((0 << P_SW_MOTHERSHIP_SHIFT)
                                        & P_SW_MOTHERSHIP_MASK);
            stop_pkt.header.swAddr |= ((1 << P_SW_CNC_SHIFT)
                                        & P_SW_CNC_MASK);
            stop_pkt.header.swAddr |= ((P_CNC_STOP << P_SW_OPCODE_SHIFT)
                                        & P_SW_OPCODE_MASK);                // and is of packet type STOP
            stop_pkt.header.swAddr |= ((P_ADDR_BROADCAST << P_SW_DEVICE_SHIFT)
                                        & P_SW_DEVICE_MASK);                               
            
            stop_pkt.header.pinAddr = 0;                 // Pin address does not matter
            
            uint32_t flits = p_hdr_size() >> TinselLogBytesPerFlit;
            if(flits == 0) ++flits;
            
            DebugPrint("Stopping task %s\n",task.c_str());
            // go through each thread of the task,
            //vector<P_thread*> threads_for_task = TaskMap[task]->ThreadsForTask();
            P_addr threadAddress;
            WALKVECTOR(P_thread*, TaskMap[task]->ThreadsForTask(), R)
            {
                (*R)->get_hardware_address()->populate_a_software_address(&threadAddress);
                uint32_t destDevAddr = TMoth::GetHWAddr(threadAddress);
                DebugPrint("Stopping thread %d in task %s\n", destDevAddr, task.c_str());

                // then issue the stop packet
                send(destDevAddr,flits, &stop_pkt, true);
            }
            TaskMap[task]->status = TaskInfo_t::TASK_END;
            // check to see if there are any other active tasks
            WALKMAP(string, TaskInfo_t*, TaskMap, tsk)
            if ((tsk->second->status == TaskInfo_t::TASK_BARR) || (tsk->second->status == TaskInfo_t::TASK_RUN)) return 0;
            // if not, shut down the Twig thread.
            if (twig_running) StopTwig();
            return 0;
        }
    case TaskInfo_t::TASK_STOP:
        break;
    default:
        TaskMap[task]->status = TaskInfo_t::TASK_ERR;
    }
    Post(813,task,"stopped",TaskInfo_t::Task_Status.find(TaskMap[task]->status)->second);
    return 2;
}

//------------------------------------------------------------------------------

void TMoth::Dump(FILE * fp)
{
    fprintf(fp,"Mothership dump+++++++++++++++++++++++++++++++++++\n");
    fprintf(fp,"Event handler table:\n");
    unsigned cIdx = 0;
    WALKVECTOR(FnMap_t*,FnMapx,F)
    {
        fprintf(fp,"Function table for comm %d:\n", cIdx++);
        fprintf(fp,"Key        Method\n");
        WALKMAP(unsigned,pMeth,(**F),i)
        fprintf(fp,"%#010x %018lx\n",(*i).first,(uint64_t)&(*i).second); //0x%010x
    }
    fprintf(fp,"Loaded tasks:\n");
    WALKMAP(string,TaskInfo_t*,TaskMap,Task)
    {
        fprintf(fp,"Task %s in state %s:\n",Task->second->TaskName.c_str(),TaskInfo_t::Task_Status.find(Task->second->status)->second.c_str());
        fprintf(fp,"Reading from binary path %s\n",Task->second->BinPath.c_str());
        fprintf(fp,"Task map dump:\n");
        Task->second->VirtualBox->Dump(fp);
        fprintf(fp,".......................................\n");
    }
    fprintf(fp,"Mothership dump-----------------------------------\n");
    fflush(fp);
    CommonBase::Dump(fp);
}

//------------------------------------------------------------------------------

long TMoth::LoadBoard(P_board* board)
{
    long coresLoaded = 0;
    string task;

    // Find the task to load onto this board.
    WALKMAP(string, TaskInfo_t*, TaskMap, K)
    {
        if (K->second->status == TaskInfo_t::TASK_BOOT) task = K->first;
        break;
    }
    if (!task.empty()) // anything to do?
    {
        TaskInfo_t* task_map = TaskMap[task];
        P_addr coreAddress;
        uint32_t mX, mY, core, thread;  // Intermediates for HostLink-side
        // address components.
        WALKPDIGRAPHNODES(AddressComponent, P_mailbox*,
        unsigned, P_link*,
        unsigned, P_port*, board->G, MB)
        {
            WALKMAP(AddressComponent, P_core*,
            board->G.NodeData(MB)->P_corem, C)
            {
                // Grab this core's code and data file
                string code_f(task_map->BinPath + "/softswitch_code_" +
                int2str(task_map->getCore(C->second)) + ".v");
                string data_f(task_map->BinPath + "/softswitch_data_" +
                int2str(task_map->getCore(C->second)) + ".v");
                C->second->instructionBinary = new Bin(fopen(code_f.c_str(),
                "r"));
                C->second->dataBinary = new Bin(fopen(data_f.c_str(), "r"));

                // Populate the P_addr coreAddress from the core's hardware
                // address object. Use coreAddress to define mX, mY, core, and
                // thread, for compatbilitity with HostLink's loading methods.
                C->second->get_hardware_address()->
                populate_a_software_address(&coreAddress);
                DebugPrint("Loading core with virtual address Bx:%d, Bd:%d, "
                "Mb: %d, Cr:%d\n",
                coreAddress.A_box, coreAddress.A_board,
                coreAddress.A_mailbox, coreAddress.A_core);
                fromAddr(TMoth::GetHWAddr(coreAddress), &mX, &mY, &core,
                &thread);
                DebugPrint("Loading hardware thread 0x%X at x:%d y:%d c:%d\n",
                TMoth::GetHWAddr(coreAddress), mX, mY, core);

                // Load instruction memory, then data memory.
                DebugPrint("* loadInstrsOntoCore(codeFilename=%s, meshX=%u, "
                "meshY=%u, coreId=%u)\n",
                code_f.c_str(), mX, mY, core);
                loadInstrsOntoCore(code_f.c_str(), mX, mY, core);
                DebugPrint("* loadDataViaCore(dataFilename=%s, meshX=%u, "
                "meshY=%u, coreId=%u)\n",
                data_f.c_str(), mX, mY, core);
                loadDataViaCore(data_f.c_str(), mX, mY, core);
                ++coresLoaded;
            }
        }
    }
    DebugPrint("Boot process for board %s finished %ld cores loaded\n",
    board->Name().c_str(), coresLoaded);
    return coresLoaded;
}

//------------------------------------------------------------------------------

unsigned TMoth::NameDist(PMsg_p* mTask_Info)
// receive a broadcast core info block from the NameServer.
{
    CMsg_p task_info(*mTask_Info);
    string TaskName;
    task_info.Get(0, TaskName);
    map<string, TaskInfo_t*>::iterator T;
    if ((T=TaskMap.find(TaskName)) == TaskMap.end())
    {
        DebugPrint("Inserting new task %s from NameDist\n", TaskName.c_str());
        TaskMap[TaskName] = new TaskInfo_t(TaskName);
    }
    vector<pair<unsigned,P_addr_t>> cores;
    task_info.Get(cores);
    DebugPrint("Task %s has %d cores\n",TaskName.c_str(),cores.size());
    // set up the cores
    for (vector<pair<unsigned,P_addr_t>>::iterator core = cores.begin(); core != cores.end(); core++)
    TaskMap[TaskName]->insertCore(core->first, core->second);
    DebugPrint("%d cores inserted into TaskInfo structure for %s\n",cores.size(),TaskName.c_str());
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::NameRecl(PMsg_p* mTask_Info)
// remove (recall) a core info block from the Mothership.
{
    string TaskName;
    mTask_Info->Get(0, TaskName);
    map<string, TaskInfo_t*>::iterator T;
    if ((T=TaskMap.find(TaskName)) == TaskMap.end()) // task exists?
    {
        Post(607, TaskName); // No. Nothing to do.
        return 0;
    }
    switch (TaskMap[TaskName]->status)
    {
    case TaskInfo_t::TASK_IDLE:
    case TaskInfo_t::TASK_BOOT:
    case TaskInfo_t::TASK_STOP:
    case TaskInfo_t::TASK_END:
        break;
    case TaskInfo_t::TASK_RDY:
        {
            Post(813, TaskName, "recalled", "TASK_RDY");
            return 1;
        }
    case TaskInfo_t::TASK_BARR:
    case TaskInfo_t::TASK_RUN:
        CmStop(TaskName); // stop any running tasks before recalling them.
        break;
    default:
        {
            TaskMap[TaskName]->status = TaskInfo_t::TASK_ERR;
            Post(812, TaskName);
            return 1;
        }
    }
    system((string("rm -r -f ")+TaskMap[TaskName]->BinPath).c_str());
    delete T->second; // get rid of its TaskInfo object
    TaskMap.erase(T); // and then remove it from the task map
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::NameTdir(const string& task, const string& dir)
// set the path where binaries for a given task are to be found
{
    map<string, TaskInfo_t*>::iterator T;
    if ((T=TaskMap.find(task)) == TaskMap.end())
    {
        DebugPrint("Inserting new task %s from NameTdir\n", task.c_str());
        TaskMap[task] = new TaskInfo_t(task);
    }
    TaskMap[task]->BinPath = dir;
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::OnCmnd(PMsg_p * Z, unsigned cIdx)
// Handler for a task command sent (probably) from the user.
{
    // get the task that the command is going to operate on
    string task;
    Z->Get(0,task);
    unsigned key = Z->Key();
    if (key == PMsg_p::KEY(Q::CMND,Q::LOAD         ))
    return CmLoad(task);
    if (key ==  PMsg_p::KEY(Q::CMND,Q::RUN         ))
    return CmRun(task);
    if (key ==  PMsg_p::KEY(Q::CMND,Q::STOP        ))
    return CmStop(task);
    else
    Post(510,"Control",int2str(Urank));
    return 0;
}

//------------------------------------------------------------------------------

void* TMoth::Twig(void* par)
// the Twig processing deals with receiving packets from the managed Tinsel cores.
// Ideally, Twig, OnIdle, and OnTinselOut would reside in a separate *process*.
// For the moment we run Twig in a separate *thread* because to run a separate
// process would require identifying the matching Branch's rank, which could be
// done, using a variety of methods, but none of them trivial, and so that's
// 'for later'.
{
    TMoth* parent = static_cast<TMoth*>(par);
    //const uint32_t szFlit = (1<<TinselLogBytesPerFlit);
    //char recv_buf[p_pkt_size()]; // buffer for one packet at a time
    char *recv_buf = new char[p_pkt_size()]; // buffer for one packet at a time
    void* p_recv_buf = static_cast<void*>(recv_buf);
    FILE* OutFile;
    char Line[4*P_PKT_MAX_SIZE];
    fpos_t readPos;
    fpos_t writePos;
    if ( (OutFile = fopen("./DebugOutput.txt", "a+")) )
    {
        fsetpos(OutFile, &readPos);
        fsetpos(OutFile, &writePos);
    }
    while (parent->ForwardPkts) // until told otherwise,
    {
        // receive all available traffic. Should this be done or only one packet
        // and then try again for MPI? We don't expect MPI traffic to be intensive
        // and tinsel messsages might be time-critical.
        while (parent->canRecv())
        {
            DebugPrint("Message received from a Device\n");
            parent->recv(recv_buf);
            
            P_Pkt_t* pkt = static_cast<P_Pkt_t*>(p_recv_buf);
            P_Pkt_Hdr_t* hdr = &(pkt->header);      //static_cast<P_Pkt_Hdr_t*>(p_recv_buf);
            
            /*
            //Temporary packet dumping for debug.
            uint32_t* dump = static_cast<uint32_t*>(p_recv_buf);
            std::cout << std::hex;
            for(int iM = 0; iM < 16; iM++)
            {
                std::cout << std::setfill('0') << std::setw(8) << *dump << " ";
                dump++;
            }
            std::cout << std::dec << std::endl;
            */
            
            
            if((hdr->swAddr & P_SW_MOTHERSHIP_MASK) 
                    && !(hdr->swAddr & P_SW_CNC_MASK))
            {   // Mothership bit set, CNC bit unset - bound for External.
                // TODO: Send to rework and send to UserIO.
                DebugPrint("Message is bound for external device");
                DebugPrint("SW:%#010x Pin:%#010x\n", hdr->swAddr, hdr->pinAddr);
                
                /*
                P_Pkt_Hdr_t* m_hdr = static_cast<P_Pkt_Hdr_t*>(p_recv_buf);
                DebugPrint("Message is bound for external device %d\n", m_hdr->destDeviceAddr);
                if (parent->TwigExtMap[m_hdr->destDeviceAddr] == 0)
                parent->TwigExtMap[m_hdr->destDeviceAddr] = new deque<P_Pkt_t>;
                if (m_hdr->packetLenBytes > szFlit)
                parent->recvMsg(recv_buf+szFlit, m_hdr->packetLenBytes-szFlit);
                parent->TwigExtMap[m_hdr->destDeviceAddr]->push_back(*(static_cast<P_Pkt_t*>(p_recv_buf)));
                */
                
            }
            else if ((hdr->swAddr & P_SW_MOTHERSHIP_MASK) 
                    && (hdr->swAddr & P_SW_CNC_MASK))
            {   // Mothership bit set, CNC bit set - bound for Mothership
                DebugPrint("Message is bound for Supervisor");
                DebugPrint("SW:%#010x Pin:%#010x\n", hdr->swAddr, hdr->pinAddr);
                
                
                
                if (parent->OnTinselOut(pkt))
                {
                    parent->Post(530, int2str(parent->Urank));
                }
                
                
                
                
            }
            else
            {   // We have received something that we should not - barf!
                DebugPrint("Mothership: packet received without MS bit set: ");
                DebugPrint("SW:%#010x Pin:%#010x\n", hdr->swAddr, hdr->pinAddr);
                //TODO: Barf, we should never get here.
            }

        }
        
        // Capture anything happening on the DebugLink - which is text output directed at a file.
        bool updated = false;
        if (OutFile)
        {
            fsetpos(OutFile, &writePos);
            while (parent->pollStdOut(OutFile)) updated = true;
            if (updated)
            {
                DebugPrint("Received a debug output packet\n");
            }
            fgetpos(OutFile, &writePos);
        }
        else
        {
            while (parent->pollStdOut()); // or possibly only dumped to the local console
        }
        // output the debug output buffer, which has to be done immediately because we are in a separate thread
        if (OutFile && updated)
        {
            fflush(OutFile);
            fsetpos(OutFile, &readPos);
            while (!feof(OutFile)) parent->Post(600, string(fgets(Line, 4*P_PKT_MAX_SIZE, OutFile)));
            fgetpos(OutFile, &readPos);
        }
    }
    printf("Exiting Twig thread\n");
    delete[] recv_buf;
    pthread_exit(par);
    return par;
}

//------------------------------------------------------------------------------

void TMoth::OnIdle()
// idle processing deals with forwarding packets from the managed Tinsel cores.
{
    // queues may be changing but we can deal with a static snapshot of the actual
    // queue because OnIdle will execute periodically.
//    WALKMAP(uint32_t,deque<P_Pkt_t>*,TwigExtMap,D)
//    {
//        int NameSrvComm = RootCIdx();
//        // int NameSrvComm = NameSCIdx();
//        PMsg_p W(Comms[NameSrvComm]);   // return packets to be routed via the NameServer's comm
//        W.Key(Q::TINS);                 // it'll be a Tinsel packet
//        W.Tgt(pPmap[NameSrvComm]->U.Root);     // temporary: dump external packets to root
//        //W.Tgt(pPmap[NameSrvComm]->U.NameServer);     // directed to the NameServer (or UserIO, when we have it)
//        W.Src(Urank);                   // coming from us
//        /* well, this is awkward: the PMsg_p type has a Put method for vectors of objects,
//        which is what we want. Our message should have a vector of P_Pkt_t's. But as things
//        stand, the packets are trapped in a deque (because we want our twig process to
//        be able to append to the vector of things to send). Which means copying them out
//        into a vector. Again, this would be fine if we could copy them directly into a
//        vector in the PMsg_p, but the interface doesn't allow it - it expects to copy
//        from vector to vector. So we seem to be stuck with this silly bucket brigade
//        approach. NOT the most efficient way to move packetss.
//    */
//        vector<P_Pkt_t> packet;
//        while (D->second->size())
//        {
//            packet.push_back(D->second->front());
//            D->second->pop_front();
//        }
//        W.Put<P_Pkt_t>(0,&packet);      // stuff the Tinsel packets into the packet
//        W.Send();                       // and away it goes.
//    }
}

//------------------------------------------------------------------------------

unsigned TMoth::OnName(PMsg_p * Z, unsigned cIdx)
// This handles what happens when the NameServer dumps a name subblock to the
// mothership
{
    unsigned key = Z->Key();
    if (key == PMsg_p::KEY(Q::NAME,Q::DIST         ))
    {
        DebugPrint("NameDist command received\n");
        return NameDist(Z);
    }
    if (key == PMsg_p::KEY(Q::NAME,Q::RECL         ))
    {
        DebugPrint("NameRecl command received\n");
        return NameRecl(Z);
    }
    if (key ==  PMsg_p::KEY(Q::NAME,Q::TDIR         ))
    {
        string task,dir;
        Z->Get(0,task);
        Z->Get(1,dir);
        DebugPrint("NameTdir command received: task %s, directory %s\n", task.c_str(), dir.c_str());
        return NameTdir(task,dir);
    }
    else
    Post(510,"Name",int2str(Urank));
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::OnExit(PMsg_p * Z, unsigned cIdx)
// This is what happens when a user command to stop happens
{
    // We are going away. Shut down any active tasks.
    WALKMAP(string, TaskInfo_t*, TaskMap, tsk)
    {
        if ((tsk->second->status == TaskInfo_t::TASK_BOOT) || (tsk->second->status == TaskInfo_t::TASK_END)) continue;
        if (tsk->second->status == TaskInfo_t::TASK_BARR) CmRun(tsk->first);
        if (tsk->second->status == TaskInfo_t::TASK_RUN)  CmStop(tsk->first);
    }
    // stop accepting Tinsel packets
    if (twig_running) StopTwig();
    return CommonBase::OnExit(Z,cIdx); // exit through CommonBase handler
}

//------------------------------------------------------------------------------

unsigned TMoth::OnSuper(PMsg_p * Z, unsigned cIdx)
// Main hook for user-defined supervisor commands
{
    // set up a return message if we are going to send some reply back
    PMsg_p W(Comms[cIdx]);
    W.Key(Q::SUPR);
    W.Src(Z->Tgt());
    int superReturn = 0;
    if ((superReturn = (*SupervisorCall)(Z,&W)) > 0) // Execute. Send a reply if one is expected
    {
        if (!cIdx && (Z->Tgt() == Urank) && (Z->Src() == Urank)) OnTinsel(&W, 0); // either to Tinsels,
        else W.Send(Z->Src());  // or to some external or internal process.
    }
    if (superReturn < 0) Post(530, int2str(Urank));
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::OnSyst(PMsg_p * Z, unsigned cIdx)
// Handler for system commands sent by the user or operator
{
    unsigned key = Z->Key();
    if (key == PMsg_p::KEY(Q::SYST,Q::HARD        ))
    {
        vector<string> args;
        Z->Get<string>(0,args);
        if (SystHW(args)) // A system hardware command executes an external process.
        {
            string cmd;
            WALKVECTOR(string,args,arg)
            {
                cmd+=(*arg);
                cmd+=(' ');
            }
            Post(520,int2str(Urank),cmd);
            return 0;
        }
        return 0;
    }
    if (key == PMsg_p::KEY(Q::SYST,Q::KILL        ))
    return SystKill(); // Kill brutally shuts us down by exiting immediately
    if (key == PMsg_p::KEY(Q::SYST,Q::SHOW        ))
    return SystShow();
    if (key == PMsg_p::KEY(Q::SYST,Q::TOPO        ))
    return SystTopo();
    else
    Post(524,int2str(Z->Key()));
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::OnTinsel(PMsg_p * Z, unsigned cIdx)
// Handler for direct packets to be injected into the network from an external source
{
    vector<P_Super_Pkt_t> pkts; // packets are packed in Tinsel packet format
    Z->Get(0, pkts);      // We assume they're directly placed in the message
    WALKVECTOR(P_Super_Pkt_t, pkts, pkt) // and they're sent blindly
    {
        uint32_t flits = pkt->len >> TinselLogBytesPerFlit;
        if (flits == 0) ++flits;
        // if we have to we can run OnIdle to empty receive buffers
        send(pkt->hwAddr, flits, &(pkt->pkt), true);
    }
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::OnTinselOut(P_Pkt_t* pkt)
// Deals with what happens when a Tinsel packet is received. Generally we
// repack the packet for delivery to the Supervisor handler and deal with
// it there. The Supervisor can do one of 2 things: A) process it itself,
// possibly generating another message; B) immediately export it over MPI to
// the user Executive or other external process.
{
    DebugPrint("Processing a command packet from Tinsel\n");
    
    P_Pkt_Hdr_t* hdr = &(pkt->header);
    uint32_t opcode = ((hdr->swAddr & P_SW_OPCODE_MASK) >> P_SW_OPCODE_SHIFT);
    
    
    // handle the kill req from a tinsel core, generally means an assert failed.
    if (opcode == P_CNC_KILL)
    {
        return SystKill();
    }
    
    // Handler Log packet
    if (opcode == P_CNC_LOG)
    {
        DebugPrint("Received a handler_log packet from device\n");
        LogHandler(pkt);
    }
    
    else if (opcode == P_CNC_INSTR)
    {
        DebugPrint("Received an instrumentation packet from device\n");
        InstrumentationHandler(pkt);
    }
    
    else
    {
        DebugPrint("Message from device is a Supervisor call. Redirecting\n");
        
        // Bung the packet in a vector "because"
        std::vector<P_Super_Pkt_t> pkts; 
        P_Super_Pkt_t sPkt = {0, (1<<TinselLogBytesPerFlit*TinselMaxFlitsPerMsg), *pkt};
        pkts.push_back(sPkt);
        
        // Populate a PMessage
        PMsg_p W(Comms[0]);     // Create a new message on the local comm
        W.Key(Q::SUPR);         // it'll be a Supervisor message
        W.Src(Urank);           // coming from the us
        W.Tgt(Urank);           // and directed at us
        W.Put<P_Super_Pkt_t>(0,&pkts);  // stuff the Tinsel packet into the message
        
        return OnSuper(&W, 0);
        // W.Send();                        // away it goes.
        // return 0;
    }
    return 0;
}

//------------------------------------------------------------------------------

void TMoth::StopTwig()
// Cleanly shut down (or try to) the Twig process. This occurs whenever we
// end a task in preparation for shutting down, or exiting.
{
    if (!twig_running) return;
    ForwardPkts = false;            // notify the Twig to shut down
    pthread_join(Twig_thread,NULL); // wait for it to do so
    if (SuperHandle)                // then unload its Supervisor
    {
        // clean up memory first
        int (*SupervisorExit)() = reinterpret_cast<int (*)()>(dlsym(SuperHandle, "SupervisorExit"));
        if (!SupervisorExit || (*SupervisorExit)() || dlclose(SuperHandle))
        {
            Post(534,int2str(Urank));
            SuperHandle = 0;          // even if we errored invalidate the Supervisor
        }
        SupervisorCall = 0;
    }
    twig_running = false;
}

//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Instrumentation Methods
//------------------------------------------------------------------------------

// Instrumentation initialisation
void TMoth::InstrumentationInit(void)
{
    // TODO: during the Mothership re-write, this should be parameterised.
    
    // Create ~/.orchestrator/instrumentation/ if it does not exist
    system("mkdir --parents ~/.orchestrator/instrumentation");
    
    // Remove any existing instrumentation files
    system("rm -rf ~/.orchestrator/instrumentation/instrumentation_thread*.csv");
}

// Gracefully tear down the Instrumentation map
void TMoth::InstrumentationEnd(void)
{
    WALKMAP(uint32_t,TM_Instrumentation*,InstrMap,I)
        delete I->second;
}


// Handle an instrumentation packet
unsigned TMoth::InstrumentationHandler(P_Pkt_t* pkt)
{
    TM_Instrumentation* instr;
    uint32_t srcAddr = pkt->header.pinAddr;
    
    // Pointer to the Instrumentation
    P_Instr_Pkt_Pyld_t* instrPkt = reinterpret_cast<P_Instr_Pkt_Pyld_t*>(pkt->payload);
    
    std::ofstream tFile;        // Thread instrumentation file
    
    // Set Filename
    const char* home = getenv("HOME");
    std::ostringstream fName;
    fName << home << "/.orchestrator/instrumentation/instrumentation_thread_" << srcAddr << ".csv";
    
    TM_InstrMap_t::iterator MSearch = InstrMap.find(srcAddr);
    if(MSearch == InstrMap.end())
    { // Instrumentation for a new thread.
        // Form a new instrumentation record
        instr = new TM_Instrumentation();
        instr->totalTime = 0;
        instr->txCount = 0;
        instr->rxCount = 0;
        
        /* This falls over for large plates, so we open every time.
        // Set Filename
        std::ostringstream fName;
        fName << "~/.orchestrator/instrumentation/instrumentation_thread_" << srcAddr << ".csv";
        
        // Open file
        instr->tFile.open(fName.str(), std::ofstream::out);
        */
        
        // Open file and overwrite contents
        tFile.open(fName.str(), std::ofstream::out);
        
        // Check it is open
        if(tFile.fail())    //instr->tFile.fail()) // Check that the file opened
        {   // if it didn't, tell logserver, delete the entry and return
            Post(541, fName.str(), POETS::getSysErrorString(errno));
            
            delete instr;
            return 1;
        }
        
        // Write the CSV header
        tFile << "ThreadID, cIDX, Time, cycles, deltaT, ";
        tFile << "RX, OnRX, TX, SupTX, OnTX, Idle, OnIdle, Blocked, ";
#if TinselEnablePerfCount == true
        tFile << "CacheMiss, CacheHit, CacheWB, CPUIdle, ";
#endif
        tFile << "RX/s, TX/s, Sup/s" << std::endl;
         
        tFile << srcAddr << ", 0, 0, 0, 0, ";
        tFile << "0, 0, 0, 0, 0, 0, 0, 0, ";
#if TinselEnablePerfCount == true
        tFile << "0, 0, 0, 0, ";
#endif
        tFile << "0, 0, 0" << std::endl;
        
        // Add the map entry
        InstrMap.insert(TM_InstrMap_t::value_type(srcAddr, instr));
    }
    else
    { // Instrumentation for an existing thread.
        instr = MSearch->second;
        
        // Open the file in append mode
        tFile.open(fName.str(), std::ofstream::out | std::ofstream::app);
        
        if(tFile.fail()) // Check that the file opened
        {   // if it didn't, tell logserver, delete the entry and return
            Post(542, fName.str(), POETS::getSysErrorString(errno));
            
            delete instr;
            InstrMap.erase(srcAddr);
            return 1;
        }
    }
    
    // TODO: parameterise this - this needs to be tied back to the task.
    double deltaT;
    deltaT = static_cast<double>(instrPkt->cycles)/P_INSTR_INTERVAL;        // Convert cycles to seconds
    
    // Update the instrumentation entry
    instr->totalTime += deltaT;
    instr->txCount += instrPkt->txCnt;
    instr->rxCount += instrPkt->rxCnt;
    
    
    // Write the raw instrumentation
    tFile << srcAddr << ", ";                    // HW address
    tFile << instrPkt->cIDX << ", ";             // Index of the packet
    tFile << instr->totalTime << ", ";           // Total Time
    tFile << instrPkt->cycles << ", ";           // Cycle difference
    tFile << deltaT << ", ";                     // Change in time
    
    tFile << instrPkt->rxCnt << ", ";            // Number of packets received
    tFile << instrPkt->rxHanCnt << ", ";         // Number of times application OnReceive handler called
    
    tFile << instrPkt->txCnt << ", ";            // Number of packets sent
    tFile << instrPkt->supCnt << ", ";           // Number of packets sent to Supervisor
    tFile << instrPkt->txHanCnt << ", ";         // Number of times application OnSend handler called
    
    tFile << instrPkt->idleCnt << ", ";          // Number of times SoftswitchOnIdle called
    tFile << instrPkt->idleHanCnt << ", ";       // Number of times application OnCompute called
    
    tFile << instrPkt->blockCnt << ", ";          // Number of times send has been blocked
    
#if TinselEnablePerfCount == true      
    tFile << instrPkt->missCount << ", ";        // Cache miss count since last instrumentation
    tFile << instrPkt->hitCount << ", ";         // Cache hit count since last instrumentation
    tFile << instrPkt->writebackCount << ", ";   // Cache writeback count since last instrumentation
    tFile << instrPkt->CPUIdleCount << ", ";     // CPU Idle count since last instrumentation
#endif 
    
    // Write the calculated instrumentation values
    tFile << instrPkt->rxCnt/deltaT << ", ";     // RX per second
    tFile << instrPkt->txCnt/deltaT << ", ";     // TX per second
    tFile << instrPkt->supCnt/deltaT;            // Sup TX per second
    tFile << std::endl;
    
    return 0;
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// LogMessage Handlers
//------------------------------------------------------------------------------

// Gracefully tear down the logpacket map 
void TMoth::LogHandlerEnd(void)
{
    WALKMAP(uint32_t,TM_LogPacket*,LogPktMap,L)
        delete L->second;
}

// Handle a log packet
unsigned TMoth::LogHandler(P_Pkt_t* pkt)
{
    /* TODO: This needs to be more robust. This does not handle all edge cases.
     * Assumes that things arrive (mostly) in order, e.g. the last packet
     * cannot arrive first.
     */
    
    TM_LogPacket* logPkt;
    uint32_t srcAddr = pkt->header.pinAddr;
    
    P_Log_Pkt_Pyld_t* pyld = reinterpret_cast<P_Log_Pkt_Pyld_t*>(pkt->payload);
    
    
    TM_LogPktMap_t::iterator MSearch = LogPktMap.find(srcAddr);
    if(MSearch == LogPktMap.end())
    {   // First packet of a new log message
        logPkt = new TM_LogPacket();
        
        logPkt->logPktCnt = 0;
        logPkt->logPktMax = 0;
        
        LogPktMap.insert(TM_LogPktMap_t::value_type(srcAddr, logPkt));
    }
    else
    {
        logPkt = MSearch->second;
    }
    
    // Drop the packet into the map.
    memcpy(&(logPkt->logPktBuf[pyld->seq]), pyld, p_pkt_pyld_size);
    
    // Update the log counters
    logPkt->logPktCnt++;
    if(logPkt->logPktMax < pyld->seq) logPkt->logPktMax = pyld->seq;
    
    
    // Received the last log packet. Re-assemble & print it.
    if(logPkt->logPktCnt == (logPkt->logPktMax +1))
    {  
        char logStr[(p_logpkt_pyld_size << P_LOG_MAX_LOGPKT_FRAG)+1];
    
#ifdef TRIVIAL_LOG_HANDLER
        // Call the trivial log handler.
        TrivialLogHandler(logPkt, logStr);
#else
        strcpy(logStr, "ERROR: No Log Handler Defined!");   // (in)sanity check
#endif   

        // Post to the log server
        Post(601, int2str(srcAddr), int2str(srcAddr), string(logStr));
        
        // Cleanup the message
        LogPktMap.erase(srcAddr);
        delete logPkt;
    }
    return 0;
}

// Trivial log message handler
unsigned TMoth::TrivialLogHandler(TM_LogPacket* logPkt, char* logPtr)
{
    //char* logPtr = logStr;
    P_Log_Pkt_Pyld_t* pyld;
    
    // Re-assemble the full log message string
    for(unsigned int i = 0; i < logPkt->logPktCnt; i++)
    {        
        pyld = &(logPkt->logPktBuf[logPkt->logPktMax]);
     
        memcpy(logPtr, pyld->payload, p_logpkt_pyld_size);
        
        logPtr += p_logpkt_pyld_size;                
        logPkt->logPktMax--;
    }
    
    return 0;
}
//------------------------------------------------------------------------------



unsigned TMoth::SystHW(const vector<string>& args)
// Execute some command directly on the Mothership itself. Unlike a Supervisor
// message, which triggers the Supervisor function but doesn't change the
// functionality of the Mothership, this can directly interact with the
// running Mothership to do certain tasks
{
    stringstream cmd(ios_base::out | ios_base::ate);
    WALKCVECTOR(string, args, arg)
    cmd << *arg;
    return system(cmd.str().c_str());
}

//------------------------------------------------------------------------------

unsigned TMoth::SystKill()
// Kill some process in the local MPI universe. We had better be sure there is
// no traffic destined for this process after such a brutal command!
{
    return 1;
}

//------------------------------------------------------------------------------

unsigned TMoth::SystShow()
// Report the list of processes (active Motherships) on this comm
{
    return 0;
}

//------------------------------------------------------------------------------

unsigned TMoth::SystTopo()
// Report this Mothership's local topology information (boards/cores/connections)
{
    return 0;
}
//==============================================================================
