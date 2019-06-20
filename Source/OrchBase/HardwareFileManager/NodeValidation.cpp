/* Defines node validation mechanism (see the accompanying
 * header for further information). */

#include "NodeValidation.h"

/* Returns whether the value at a node is a natural number. Arguments:
 *
 * - recordNode: Record parent node (for writing error message).
 * - valueNode: The UIF node that holds the value.
 * - variable: Variable name string (for writing error message).
 * - value: Value string (for writing error message).
 * - sectionName: The name of the section the record lives in (for writing
     error message).
 * - errorMessage: String to append the error message to, if any. */
bool complain_if_node_value_not_natural(
    UIF::Node* recordNode, UIF::Node* valueNode, std::string variable,
    std::string value, std::string sectionName, std::string* errorMessage)
{
    if (valueNode->qop != Lex::Sy_ISTR)
    {
        errorMessage->append(dformat(
            "L%u: Variable '%s' in section '%s' has value '%s', which is not "
            "a natural number.\n",
            recordNode->pos, variable.c_str(), sectionName.c_str(),
            value.c_str()));
        return false;
    }
    return true;
}

/* Returns whether the value at a node is a positive floating-point
 * number (or an integer). Arguments:
 *
 * - recordNode: Record parent node (for writing error message).
 * - valueNode: The UIF node that holds the value.
 * - variable: Variable name string (for writing error message).
 * - value: Value string (for writing error message).
 * - sectionName: The name of the section the record lives in (for writing
     error message).
 * - errorMessage: String to append the error message to, if any. */
bool complain_if_node_value_not_floating(
    UIF::Node* recordNode, UIF::Node* valueNode, std::string variable,
    std::string value, std::string sectionName, std::string* errorMessage)
{
    if (valueNode->qop != Lex::Sy_FSTR && valueNode->qop != Lex::Sy_ISTR)
    {
        errorMessage->append(dformat(
            "L%u: Variable '%s' in section '%s' has value '%s', which is not "
            "a positive floating-point number or a positive integer.\n",
            recordNode->pos, variable.c_str(), sectionName.c_str(),
            value.c_str()));
        return false;
    }
    return true;
}

/* Returns whether the value at a node, and all of its children, are natural
 * numbers. Arguments:
 *
 * - recordNode: Record parent node (for writing error message).
 * - valueNode: The UIF node that holds the value.
 * - variable: Variable name string (for writing error message).
 * - value: Value string (for writing error message).
 * - sectionName: The name of the section the record lives in (for writing
     error message).
 * - errorMessage: String to append the error message to, if any. */
bool complain_if_nodes_values_not_natural(
    UIF::Node* recordNode, UIF::Node* valueNode, std::string variable,
    std::string value, std::string sectionName, std::string* errorMessage)
{
    std::vector<UIF::Node*>::iterator valueNodeIterator;
    bool valid = true;
    for (valueNodeIterator=valueNode->leaf.begin();
         valueNodeIterator!=valueNode->leaf.end(); valueNodeIterator++)
    {
        valid &= complain_if_node_value_not_natural(
            recordNode, (*valueNodeIterator),
            variable, (*valueNodeIterator)->str.c_str(),
            sectionName.c_str(), errorMessage);
    }
    return valid;
}

/* Converts a float-like input string to an actual float. */
float str2float(std::string floatLike)
{
    float returnValue;
    sscanf(floatLike.c_str(), "%f", &returnValue);
    return returnValue;
}

/* Converts a float-like input string to an actual float. I know something like
 * this exists in flat, but I want something with a single argument so that I
 * can use std::transform to transform a vector of strings into a vector of
 * unsigneds. */
unsigned str2unsigned(std::string uintLike)
{
    unsigned returnValue;
    sscanf(uintLike.c_str(), "%u", &returnValue);
    return returnValue;
}
