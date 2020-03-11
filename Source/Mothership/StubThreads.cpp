#include "ThreadComms.h"

/* A temporary file to hold stubbed thread logic. */

void* ThreadComms::mpi_input_broker(void* mothershipArg)
{
    Mothership* mothership = (Mothership*)mothershipArg;
    mothership->MPISpinner();
}

void* ThreadComms::mpi_cnc_resolver(void* mothershipArg)
{
    Mothership* mothership = (Mothership*)mothershipArg;
    while(1);
}

void* ThreadComms::mpi_application_resolver(void* mothershipArg)
{
    Mothership* mothership = (Mothership*)mothershipArg;
    while(1);
}

void* ThreadComms::backend_output_broker(void* mothershipArg)
{
    Mothership* mothership = (Mothership*)mothershipArg;
    while(1);
}

void* ThreadComms::backend_input_broker(void* mothershipArg)
{
    Mothership* mothership = (Mothership*)mothershipArg;
    while(1);
}

void* ThreadComms::debug_input_broker(void* mothershipArg)
{
    Mothership* mothership = (Mothership*)mothershipArg;
    while(1);
}