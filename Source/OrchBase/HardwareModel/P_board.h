#ifndef __ORCHESTRATOR_SOURCE_COMMON_HARDWAREMODEL_P_BOARD_H
#define __ORCHESTRATOR_SOURCE_COMMON_HARDWAREMODEL_P_BOARD_H

/* Describes the concept of a "POETS Board" in the model of the POETS
 * hardware stack.
 *
 * The POETS Board represents an FPGA board in the POETS Engine, and contains
 * mailboxes.
 *
 * See the hardware model documentation for further information POETS
 * boards. */

#include <sstream>

#include "AddressableItem.h"
#include "DumpUtils.h"
#include "NameBase.h"
#include "OwnershipException.h"
#include "P_box.h"
#include "P_link.h"
#include "P_mailbox.h"
#include "P_port.h"
#include "pdigraph.hpp"

/* Facilitate out-of-order includes. */
class P_box;
class P_mailbox;

class P_board: public AddressableItem, public NameBase, public DumpChan
{
public:
    P_board(std::string name);

    /* Destruction */
    ~P_board();
    void clear();

    /* Boards live in boxes; this is the box that this board lives in. Note
     * that boards also live in the engine by direct reference, but this is
     * not captured in the board construct.
     *
     * Boards use boxes as their NameBase parent. */
    P_box* parent = NULL;
    void on_being_contained_hook(P_box* container);

    /* Boards contain mailboxes as a graph, mapped by their hardware address
     * components. */
    pdigraph<AddressComponent, P_mailbox*,
             unsigned, P_link*,
             unsigned, P_port*> G;
    void contain(AddressComponent addressComponent, P_mailbox* mailbox);
    void connect(AddressComponent start, AddressComponent end, float weight,
                 bool oneWay=false);

    /* Boards can be assigned supervisors, by offset. The supervisors
     * themselves are stored on the box level. The board actually contains
     * indeces to the vector on the box level. */
    std::vector<unsigned> sup_offv;

    unsigned int dram;
    unsigned int supervisorMemory;
    float costBoardMailbox;

    void Dump(FILE* = stdout);

private:
    /* Keys for arcs and ports in the graph, incremented when items are
     * connected. */
    unsigned int arcKey;
    unsigned int portKey;
};

#endif
