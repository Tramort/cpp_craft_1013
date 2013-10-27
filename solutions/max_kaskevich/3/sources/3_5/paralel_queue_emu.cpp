#include "message.h"
#include <stdint.h>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>


namespace fs = boost::filesystem;


static const uint32_t max_data_size = 2048;

struct message_stats
{
    boost::mutex mtx_;
    uint32_t     seconds;
    int64_t      last_second;
    uint32_t     msg_count;
    message_stats() : mtx_(),
                      seconds(0),
                      last_second(-1),
                      msg_count(0)
    {}

    void add_stat(uint32_t time)
    {
        boost::lock_guard<boost::mutex> lock(mtx_);
        ++msg_count;
        if(time > last_second)
        {
            last_second = time;
            ++seconds;
        }
    }

    void write(std::ostream& output, uint32_t type)
    {
        double avg = (msg_count
            ? msg_count /  seconds
            : 0.0);
        write_binary(output, &type);
        write_binary(output, &avg);
    }
};


typedef std::shared_ptr< message_stats > msg_stats_ptr;
typedef std::map< uint32_t, msg_stats_ptr > msg_stats_map;
msg_stats_map msg_stats;
boost::mutex mtx;


msg_stats_ptr get_stats(uint32_t type)
{
    msg_stats_map::iterator it = msg_stats.find(type);
    if(it == msg_stats.end())
    {
        boost::lock_guard<boost::mutex> lock(mtx);
        auto res = msg_stats.emplace(type, msg_stats_ptr());
        if(res.second)
        {
            res.first->second.reset(new message_stats());
        }
        return res.first->second;
    }
    else
    {
        return it->second;
    }
}


void proc(const std::string& input_path)
{
    std::ifstream input(input_path, std::ios::binary);
    if(!input)
    {
        return;
    }

    std::map< uint32_t, uint32_t > msg_size_in_time;

    uint32_t cur_time = 0;
    message m;
    while(input >> m)
    {
        // clear size info when new time coming
        if (m.time > cur_time)
        {
            msg_size_in_time.clear();
            cur_time = m.time;
        }

        auto res = msg_size_in_time.emplace(m.type, m.len + 12);
        if (!res.second)
        {
            res.first->second += m.len + 12;
        }

        auto m_stat = get_stats(m.type);

        if(msg_size_in_time[m.type] < max_data_size)
        {
            m_stat->add_stat(m.time);
        }

        // miss msg
        if(m.len)
        {
            input.seekg(m.len, std::ios::cur);
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
        if(!boost::regex_match(cur_path.filename().string(), input_regex))
        {
            continue;
        }

        threads.create_thread(boost::bind(proc, cur_path.string()));

    }
    threads.join_all();

    std::ofstream output(BINARY_DIR "/output.txt", std::ios::binary);
    for(msg_stats_map::value_type& m_stat_pair: msg_stats)
    {
        m_stat_pair.second->write(output, m_stat_pair.first);
    }
}
