#include "TaskInfo.h"

const unsigned char TaskInfo_t::TASK_IDLE;
const unsigned char TaskInfo_t::TASK_BOOT;
const unsigned char TaskInfo_t::TASK_RDY;
const unsigned char TaskInfo_t::TASK_BARR;
const unsigned char TaskInfo_t::TASK_RUN;
const unsigned char TaskInfo_t::TASK_STOP;
const unsigned char TaskInfo_t::TASK_END;
const unsigned char TaskInfo_t::TASK_ERR;
const map<unsigned char,string> TaskInfo_t::Task_Status({{TaskInfo_t::TASK_IDLE,"TASK_IDLE"},{TaskInfo_t::TASK_BOOT,"TASK_BOOT"},{TaskInfo_t::TASK_RDY,"TASK_RDY"},{TaskInfo_t::TASK_BARR,"TASK_BARR"},{TaskInfo_t::TASK_RUN,"TASK_RUN"},{TaskInfo_t::TASK_STOP,"TASK_STOP"},{TaskInfo_t::TASK_END,"TASK_END"},{TaskInfo_t::TASK_ERR,"TASK_ERR"}});

TaskInfo_t::TaskInfo_t(string name) : TaskName(name), BinPath(), CoreMap(), VCoreMap()
{
    status = TASK_IDLE;
}

TaskInfo_t::~TaskInfo_t()
{
    delete VirtualBox;
    WALKMAP(P_board*,vector<BinPair_t>*,binaries,bBins)
      delete bBins->second;
}

// sets the core value for a virtual core, provided this core number exists
bool TaskInfo_t::setCore(uint32_t vCore, P_core* core)
{
     if (VCoreMap.find(vCore) == VCoreMap.end()) return false; // virtual core exists?
     if (CoreMap.find(core) == CoreMap.end()) // new physical core?
     {
        // Get a P_addr_t for this core.
        P_addr coreSoftAddress;
        core->get_hardware_address()->
            populate_a_software_address(&coreSoftAddress);
        insertCore(vCore, coreSoftAddress); // add a new core in the usual way
        return true;
     }
     CoreMap[core] = vCore;     // otherwise set both maps
     VCoreMap[vCore] = core;
     return true;
}

// sets the virtual core index for a core, provided this core has been mapped.
bool TaskInfo_t::setCore(P_core* core, uint32_t vCore)
{
     if (CoreMap.find(core) == CoreMap.end()) return false; // core exists?
     VCoreMap[vCore] = core;      // set both maps.
     CoreMap[core] = vCore;
     return true;
}

// absolute setter for a core - inserts if none exists, overwrites if it is already in the map
void TaskInfo_t::insertCore(uint32_t vCore, P_addr_t coreID)
{
    if (VirtualBox == 0) // no box yet. Assume this core insertion defines our box number.
    {
        VirtualBox = new P_box(0);
        VirtualBox->AutoName(TaskName+"_Box_");
        VirtualBox->get_hardware_address()->set_box(coreID.A_box);
        printf("Inserting VirtualBox %s\n",VirtualBox->Name().c_str());
        fflush(stdout);
    }
    if (coreID.A_box != VirtualBox->get_hardware_address()->get_box()) return; // not our box. Ignore.
    P_board* VirtualBoard = 0;
    WALKVECTOR(P_board*, VirtualBox->P_boardv, B)
    {
        if ((*B)->get_hardware_address()->get_board() == coreID.A_board)
        {
            printf("Using VirtualBoard %s\n",(*B)->Name().c_str());
            fflush(stdout);
            VirtualBoard = (*B);
            break;
        }
    }
    if (!VirtualBoard) // no existing board matches the core. Create a new one.
    {
        VirtualBoard = new P_board(0);
        VirtualBoard->parent = VirtualBox;
        VirtualBox->contain(coreID.A_board, VirtualBoard);
        VirtualBoard->AutoName(VirtualBox->Name()+"_Board_");
        printf("Inserting VirtualBoard %s\n",VirtualBoard->Name().c_str());
        fflush(stdout);
    }
    P_mailbox* VirtualMailbox = 0;
    WALKPDIGRAPHNODES(AddressComponent, P_mailbox*,
                      unsigned, P_link*,
                      unsigned, P_port*, VirtualBoard->G, MB)
    {
        if (VirtualBoard->G.NodeData(MB)->get_hardware_address()->get_mailbox() == coreID.A_mailbox)
        {
            printf("Using VirtualMailbox %s\n",
                   VirtualBoard->G.NodeData(MB)->Name().c_str());
            fflush(stdout);
            VirtualMailbox = VirtualBoard->G.NodeData(MB);
            break;
        }
    }
    if (!VirtualMailbox) // no existing mailbox matches the core. Create a new one.
    {
        VirtualMailbox = new P_mailbox(0);
        VirtualMailbox->parent = VirtualBoard;
        VirtualBoard->contain(coreID.A_mailbox, VirtualMailbox);
        VirtualMailbox->AutoName(VirtualBoard->Name()+"_Mailbox_");
        printf("Inserting VirtualMailbox %s\n",VirtualMailbox->Name().c_str());
        fflush(stdout);
    }

    softMap_t::iterator C = VCoreMap.find(vCore);
    if (C != VCoreMap.end())
    {
        // core already exists. No need to add.
        if ((C->second->get_hardware_address()->get_box() == VirtualBox->get_hardware_address()->get_box()) &&
            (C->second->get_hardware_address()->get_board() == VirtualBoard->get_hardware_address()->get_board()) &&
            (C->second->get_hardware_address()->get_mailbox() == VirtualMailbox->get_hardware_address()->get_mailbox()) &&
            (C->second->get_hardware_address()->get_core() == coreID.A_core)) return;
       // A mapped core with the same virtual number is already in the table; get rid of it.
       CoreMap.erase(C->second);
       removeCore(C->second);
    }
    // insert the new core with its appropriate address fields.
    P_core* VirtualCore = new P_core(0);
    VirtualCore->parent = VirtualMailbox;
    VirtualMailbox->contain(coreID.A_core, VirtualCore);
    VirtualCore->AutoName(VirtualMailbox->Name()+"_Core_");
    // generate the threads for the core.
    P_thread* VirtualThread;
    for (unsigned thread = 0; thread <= coreID.A_thread; thread++)
    {
        VirtualThread = new P_thread(0);
        VirtualThread->parent = VirtualCore;
        VirtualCore->contain(coreID.A_thread, VirtualThread);
        VirtualThread->AutoName(VirtualCore->Name()+"_Thread_");
    }
    // then map to the task.
    VCoreMap[vCore] = VirtualCore;
    CoreMap[VirtualCore] = vCore;
}

// remove a core from the map (by virtual ID)
void TaskInfo_t::deleteCore(uint32_t vCore)
{
     softMap_t::iterator C;
     if ((C = VCoreMap.find(vCore)) == VCoreMap.end()) return; // core isn't mapped
     P_core* core = C->second;
     CoreMap.erase(C->second); // remove from both maps
     VCoreMap.erase(C);
     removeCore(core);          // and then remove from the VirtualBox
}

// remove a core from the map (by physical core)
void TaskInfo_t::deleteCore(P_core* core)
{
     hardMap_t::iterator C;
     if ((C = CoreMap.find(core)) == CoreMap.end()) return; // core isn't mapped
     VCoreMap.erase(C->second); // remove from both maps
     CoreMap.erase(C);
     removeCore(core);           // and then remove from the VirtualBox.
}

// remove a core from the VirtualBox
void TaskInfo_t::removeCore(P_core* core)
{
     vector<P_core*>::iterator C = core->par->P_corev.begin();
     while (C != core->par->P_corev.end())
     {
           if ((*C) == core)
           {
	      core->par->P_corev.erase(C); // very inefficient but rare: remove old mapped core
	      break;                       // and end the search
	   }
     }
     delete core;                          // once removed destroy the object.
}

vector<P_core*>& TaskInfo_t::CoresForTask()
{
   if (!cores.size())
   {
      WALKVECTOR(P_board*,VirtualBox->P_boardv,board)
        cores.insert(cores.end(),(*board)->P_corev.begin(),(*board)->P_corev.end());
   }
   return cores;
}
vector<P_thread*>& TaskInfo_t::ThreadsForTask()
{
   if (!threads.size())
   {
      WALKVECTOR(P_board*,VirtualBox->P_boardv,board)
      {
	WALKVECTOR(P_core*,(*board)->P_corev,core)
	  threads.insert(threads.end(),(*core)->P_threadv.begin(),(*core)->P_threadv.end());
      }
   }
   return threads;
}
vector<P_device*>& TaskInfo_t::DevicesForTask()
{
   if (!devices.size())
   {
      WALKVECTOR(P_board*,VirtualBox->P_boardv,board)
      {
	WALKVECTOR(P_core*,(*board)->P_corev,core)
	{
	  WALKVECTOR(P_thread*,(*core)->P_threadv,thread)
	    devices.insert(devices.end(),(*thread)->P_devicel.begin(),(*thread)->P_devicel.end());
	}
      }
   }
   return devices;
}

vector<BinPair_t>& TaskInfo_t::BinariesForBoard(P_board* board)
{
   if (binaries.find(board) == binaries.end())
   {
      BinPair_t core_bins;
      binaries[board] = new vector<BinPair_t>;
      WALKVECTOR(P_core*,board->P_corev,core)
      {
	core_bins.instr = (*core)->pCoreBin;
	core_bins.data = (*core)->pDataBin;
	binaries[board]->push_back(core_bins);
      }
   }
   return *binaries[board];
}
