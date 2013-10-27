#include "message.h"
#include <fstream>
#include <stdint.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>


namespace fs = boost::filesystem;


enum msg_type: uint32_t
{
    MARKET_OPEN = 1u,
    TRADE = 2u,
    QUOTE = 3u,
    MARKET_CLOSE = 4u    
};


void proc(const std::string& input_path, const std::string& output_path)
{
    std::ifstream input(input_path, std::ios::binary);
    std::ofstream output(output_path, std::ios::binary);
    if(!input || !output)
    {
        return;
    }

    uint32_t T = 0;
    message m;
    while(input >> m)
    {
        if(m.type >= MARKET_OPEN && m.type <= MARKET_CLOSE && m.time + 2 > T )
        {
            if( !(output << m ) )
            {
                return;
            }
            // redirect msg to output
            if (input && m.len)
            {
                uint32_t len = m.len;
                static const size_t buf_size = 4096;
                char buf[buf_size];
                while (len > buf_size)
                {
                    input.read(buf, buf_size);
                    output.write(buf, buf_size);
                    len -= buf_size;
                }
                input.read(buf, len);
                output.write(buf, len);
            }

            T = std::max(T, m.time);
        }
        else
        {
            // miss msg
            if (input)
            {
                input.seekg(m.len, std::ios::cur);
            }
        }
    }
}

int main( int argc, char* argv[] )
{
    boost::system::error_code er;
    fs::path bin_dir(BINARY_DIR);
    fs::directory_iterator dir_it(bin_dir, er), end_it;
    if(er)
    {
        std::cerr << er.message() << std::endl;
        return er.value();
    }

    boost::thread_group threads;
    for(;dir_it != end_it; ++dir_it)
    {
        if(!fs::is_regular_file(dir_it->status()))
        {            
            continue;
        }
        fs::path cur_path = dir_it->path();

        boost::regex input_regex("input_([0-9]{3})\\.txt");
        boost::smatch match;
        if(!boost::regex_match(cur_path.filename().string(), match, input_regex))
        {
            continue;
        }

        threads.create_thread(boost::bind(proc, cur_path.string(),
            BINARY_DIR "/output_" + match[0].str() + ".txt"));
        
    }
    threads.join_all();
}
