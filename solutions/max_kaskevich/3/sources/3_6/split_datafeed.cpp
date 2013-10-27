#include <stdint.h>
#include <fstream>
#include "message.h"
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <map>



struct message2_6
{
    char stock_name[8];
    char date_time[8];

    double price;        
    double vwap;

    uint32_t volume;

    double f1; 
    double t1;
    double f2; 
    double f3;
    double f4;
}; 

std::istream& operator >> (std::istream& input, message2_6& m)
{
    read_binary(input, m.stock_name, sizeof(m.stock_name));
    read_binary(input, m.date_time, sizeof(m.date_time));
    read_binary(input, &m.price);
    read_binary(input, &m.vwap);
    read_binary(input, &m.volume);
    read_binary(input, &m.f2);
    return input;
}

std::ostream& operator << (std::ostream& output, message2_6& m)
{
    write_binary(output, m.stock_name, sizeof(m.stock_name));
    output << '\0';

    uint32_t year, month, day;
    std::sscanf(m.date_time, "%4d%2d%2d", &year, &month, &day);
    uint32_t all_days = (year - 1) * 372 + (month - 1) * 31 + day;
    write_binary(output, &all_days);

    write_binary(output, &m.vwap);
    write_binary(output, &m.volume);
    write_binary(output, &m.f1);
    write_binary(output, &m.f4);
    write_binary(output, &m.f3);
    return output;
}

int main( int argc, char* argv[] )
{
    std::ifstream input(BINARY_DIR "/input.txt", std::ios::binary);
    if(!input)
    {
        return -1;
    }

    std::map<std::string, std::ofstream> outputs;
    message2_6 m;
    while (input >> m)
    {
        std::ofstream& output = outputs[m.stock_name];
        if (!output.is_open())
        {
            output.open((boost::format(BINARY_DIR "/output_%1%.txt") % m.stock_name).str());
        }
        output && output << m;
    }

}