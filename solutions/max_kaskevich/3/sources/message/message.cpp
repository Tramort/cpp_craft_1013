#include "message.h"


std::istream& operator >> (std::istream& input, message& m)
{
    read_binary(input, &m.type);
    read_binary(input, &m.time);
    read_binary(input, &m.len);
    return input;
}

std::ostream& operator << (std::ostream& output, message& m)
{
    write_binary(output, &m.type);
    write_binary(output, &m.time);
    write_binary(output, &m.len);
    return output;
}