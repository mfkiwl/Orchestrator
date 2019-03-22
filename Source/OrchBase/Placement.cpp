//------------------------------------------------------------------------------

#include "Placement.h"

//==============================================================================

Placement::Placement(OrchBase * _p):iterator(0),par(_p),pCon(0),pD_graph(0)
{
}

//------------------------------------------------------------------------------

Placement::~Placement()
{
    if (iterator == 0) delete iterator;
}

//------------------------------------------------------------------------------

void Placement::DoLink()
{
}

//------------------------------------------------------------------------------

void Placement::Dump(FILE * fp)
{
fprintf(fp,"Placement+++++++++++++++++++++++++++++++++++\n");
fprintf(fp,"Me,Parent      %#018lx,%#018lx\n",
        (uint64_t) this, (uint64_t) par);
fprintf(fp,"Placement-----------------------------------\n");
fflush(fp);
}

//------------------------------------------------------------------------------

void Placement::Init()
{
    if (iterator != 0) delete iterator;
    iterator = new HardwareIterator(par->pE);
}

//------------------------------------------------------------------------------

bool Placement::Place(P_task * pT)
// Place a task.
{
P_thread* pTh = iterator->get_thread();

WALKVECTOR(P_devtyp*,pT->pP_typdcl->P_devtypv,dT)
{
    if ((*dT)->pOnRTS) // don't need to place if it's a supervisor - easily
                       // identified by lack of RTS handler
    {
        // get all the devices of this type
        vector<P_device*> dVs = pT->pD->DevicesOfType(*dT);
        unsigned int devMem = (*dT)->MemPerDevice();
        if (devMem > BYTES_PER_THREAD)
        {
            // even a single device is too big to fit
            par->Post(810, (*dT)->Name(), int2str(devMem),
                      int2str(BYTES_PER_THREAD));
            return true;
        }
        // place according to constraints found
        if (!pCon) pCon = new Constraints();
        if (pCon->Constraintm.find("DevicesPerThread") == pCon->Constraintm.end())
            pCon->Constraintm["DevicesPerThread"] = min(BYTES_PER_THREAD/devMem,
                                                        MAX_DEVICES_PER_THREAD);
        if (pCon->Constraintm.find("ThreadsPerCore") == pCon->Constraintm.end())
            pCon->Constraintm["ThreadsPerCore"] = THREADS_PER_CORE;

        // For each device.....
        for (unsigned devIdx = 0; devIdx < dVs.size(); devIdx++)
        {
            // if we have packed the existing thread,
            if (devIdx != 0 && !(devIdx%pCon->Constraintm["DevicesPerThread"]))
            {
                // get the next thread
                pTh = iterator->next_thread();

                // check to see that we don't run out of room - i.e. that we
                // increment past box space and this isn't the last device to
                // place
                if (iterator->has_wrapped() &&
                    !((devIdx == (dVs.size()-1)) &&
                      ((dT+1) == pT->pP_typdcl->P_devtypv.end())))
                {
                    // out of room. Abandon placement.
                    par->Post(163, pT->Name());
                    return true;
                }
            }
            // insert the device's internal address (thread index)
            dVs[devIdx]->addr.SetDevice(
                devIdx%pCon->Constraintm["DevicesPerThread"]);
            // And link thread and device
            Xlink(dVs[devIdx],pTh);
        }

        // jump to the next core: each core will only have one device type. We
        // do not need to jump if the devices exactly fit on an integral number
        // of cores because in that situation the previous GetNext() function
        // will have incremented the core for us.
        if (dVs.size()%(pCon->Constraintm["DevicesPerThread"]*pCon->Constraintm["ThreadsPerCore"]))
        {
            if (((dT+1) != pT->pP_typdcl->P_devtypv.end()))
            {
                // get the first thread on the next core, and abandon if we've
                // filled up the hardware.
                iterator->next_core();
                pTh = iterator->get_thread();
                if (iterator->has_wrapped())
                {
                    // out of room. Abandon placement.
                    par->Post(163, pT->Name());
                    return true;
                }
            }
        }
        // current tinsel architecture shares I-memory between pairs of cores,
        // so for a new device type, if the postincremented core number is odd,
        // we need to increment again to get an even boundary.
        if (SHARED_INSTR_MEM &&
            iterator->get_core()->get_hardware_address()->get_core() & 0x1)
        {
            // get the first thread on the next core, and abandon if we've
            // filled up the hardware.
            iterator->next_core();
            pTh = iterator->get_thread();
            if (iterator->has_wrapped())
            {
                // out of room. Abandon placement.
                par->Post(163, pT->Name());
                return true;
            }
        }
    }
}
return false;
}

//------------------------------------------------------------------------------

void Placement::Xlink(P_device * pDe,P_thread * pTh)
// Actually link a real device to a real thread
{
printf("XLinking device %s with id %d to thread %s\n",
        pDe->FullName().c_str(), pDe->addr.A_device, pTh->FullName().c_str());
fflush(stdout);
pDe->pP_thread = pTh;                  // Device to thread
pTh->P_devicel.push_back(pDe);         // Thread to device
// The supervisor is already attached to the task; now it needs to be linked to
// the topology. I can't but think there's a cooler way......
pDe->par->par->pSup->Attach(pTh->parent->parent->parent);
}

//==============================================================================
