#ifndef __ORCHESTRATOR_SOURCE_COMMON_HARDWAREMODEL_POETSBOARD_H
#define __ORCHESTRATOR_SOURCE_COMMON_HARDWAREMODEL_POETSBOARD_H

/* Describes the concept of a "POETS Board" in the model of the POETS
   hardware stack.

   The POETS Board represents an FPGA board in the POETS Engine, and contains
   mailboxes.

   See the hardware model documentation for further information POETS
   boards. */

#include <sstream>

#include "AddressableItem.h"
#include "dumpchan.h"
#include "NameBase.h"
#include "OwnershipException.h"
#include "P_box.h"
#include "P_mailbox.h"
#include "pdigraph.hpp"

#define MAXIMUM_BREAKER_LENGTH 80
#define CALLBACK static void

/* Facilitate out-of-order includes. */
struct P_box;
struct P_mailbox;

class P_board: public AddressableItem, public NameBase, protected DumpChan
{
public:
    P_board(std::string name);

    /* Destruction */
    ~P_board();
    void clear();

    /* Boards live in boxes; this is the box that this board lives in. Note
       that boards also live in the engine by direct reference, but this is
       not captured in the board construct.

       Boards use boxes as their NameBase parent. */
    P_box* parent = NULL;
    void on_being_contained_hook(P_box* container);

    /* Boards contain mailboxes as a graph, mapped by their hardware address
       components. */
    pdigraph<AddressComponent, P_mailbox*,
        unsigned int, float,
        unsigned int, unsigned int> G;
    void contain(AddressComponent addressComponent, P_mailbox* mailbox);
    void connect(AddressComponent start, AddressComponent end, float weight,
                 bool oneWay=false);

    unsigned int dram;
    unsigned int supervisorMemory;
    float costBoardMailbox;

    void dump(FILE* = stdout);

private:
    /* Keys for arcs in the graph, incremented when items are connected. */
    unsigned int arcKey;
};

#endif