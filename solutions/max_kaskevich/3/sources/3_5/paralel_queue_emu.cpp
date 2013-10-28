#include "message.h"
#include <stdint.h>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <numeric>


namespace fs = boost::filesystem;


static const uint32_t max_data_size = 2048;

typedef std::map< uint32_t, uint32_t > msg_count_map;
typedef std::map< uint32_t, msg_count_map > msg_stats_map;
msg_stats_map global_stats;
boost::mutex mtx;

void proc(const std::string& input_path)
{
    std::ifstream input(input_path, std::ios::binary);
    if(!input)
    {
        return;
    }

    std::map< uint32_t, uint32_t > msg_size_in_time;
    msg_stats_map local_stats;
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

        local_stats[m.type][m.time];
        

        if(msg_size_in_time[m.type] < max_data_size)
        {
            ++local_stats[m.type][m.time];
        }

        // miss msg
        if(m.len)
        {
            input.seekg(m.len, std::ios::cur);
        }
    }

    boost::lock_guard<boost::mutex> lock(mtx);
    for(auto& by_type_pair: local_stats)
    {
        for(auto& by_time_pair: by_type_pair.second)
        {
            global_stats[by_type_pair.first][by_time_pair.first] += by_time_pair.second;
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
    for(auto& m_stat_pair: global_stats)
    {
        write_binary(output, const_cast<uint32_t*>(&m_stat_pair.first));
        double all_msg = std::accumulate(m_stat_pair.second.cbegin(), m_stat_pair.second.cend(), 0,
            [](uint32_t prev, const msg_count_map::value_type& p) -> uint32_t {return prev + p.second;});
        double avg = (all_msg ? all_msg / m_stat_pair.second.size() : 0.0);
        write_binary(output, &avg);
    }
}
