#include <fstream>
#include <stdint.h>


struct message
{
    uint32_t type;
    uint32_t time;
	uint32_t len;
};

template<typename T>
inline void read_binary(std::istream& input, T* value, size_t size = sizeof(T))
{
    input.read(reinterpret_cast<char*>(value), size);
}

template<typename T>
inline void write_binary(std::ostream& output, T* value, size_t size = sizeof(T))
{
    output.write(reinterpret_cast<char*>(value), size);
}

std::istream& operator >> (std::istream& input, message& m);
std::ostream& operator << (std::ostream& output, message& m);
