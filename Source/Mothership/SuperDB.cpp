#include "SuperDB.h"

/* Loads a supervisor into the database, returning true on success and false on
 * failure. If there is a failure, errorMessage is written with the contents of
 * the error. */
bool SuperDB::load_supervisor(std::string appName, std::string path,
                              std::string* errorMessage)
{
    /* Firstly, check whether or not a supervisor has already been loaded for
     * this application. */
    SuperIt superIt = supervisors.find(appName);
    if (superIt != supervisors.end())
    {
        *errorMessage = dformat(
            "A supervisor has already been loaded to application '%s'.",
            appName.c_str());
        return false;
    }

    /* Otherwise, load up. */
    supervisors.insert(std::pair<std::string, SuperHolder>
                       (appName, SuperHolder(path)));

    /* Check for errors. */
    if (supervisors[appName].so == NULL)  /* As per the specification... */
    {
        *errorMessage = dlerror();
        return false;
    }

    return true;
}

/* Unloads a supervisor from the database, returning true on success and false
 * if no such supervisor exists. */
bool SuperDB::unload_supervisor(std::string appName)
{
    /* Check whether or not a supervisor has already been loaded for this
     * application. */
    SuperIt superIt = supervisors.find(appName);
    if (superIt != supervisors.end())
    {
        return false;
    }

    /* Otherwise, unload away (via destructor). */
    supervisors.erase(superIt);
    return true;
}

/* Prints a diagnostic line for each supervisor. The argument is the stream to
 * dump to. */
void SuperDB::dump(ofstream* stream)
{
    *stream << "SuperDB dump:\n";
    if (supers.empty())
    {
        *stream << "No supervisor devices defined.\n";
    }
    for (SuperIt superIt = supervisors.begin(); superIt != supervisors.end();
         superIt++)
    {
        superIt->second.dump(stream);
    }
}
