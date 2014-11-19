//
// Copyright 2010-2011,2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

using namespace std;

template<typename samp_type> void recv_to_file(
    uhd::usrp::multi_usrp::sptr usrp,
    const std::string &cpu_format,
    const std::string &wire_format,
    const std::string &file,
    size_t samps_per_buff,
    unsigned long long num_requested_samples,
    double time_requested = 0.0,
    bool bw_summary = false,
    bool stats = false,
    bool null = false,
    bool enable_size_map = false,
    bool continue_on_bad_packet = false
){
    unsigned long long num_total_samps = 0;
    //create a receive streamer
    uhd::stream_args_t stream_args(cpu_format,wire_format);
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;
    std::vector<samp_type> buff(samps_per_buff);
    bool overflow_message = true;

    //setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)?
        uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
        uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
    );
    stream_cmd.num_samps = num_requested_samples;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    rx_stream->issue_stream_cmd(stream_cmd);

    for(;;) {

        size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0, enable_size_map);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << "Timeout while streaming" << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
 //           if (overflow_message) {
 //               overflow_message = false;
                std::cerr << "overflow" << endl;
 //           }
            continue;
        }

        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
            cerr <<  md.strerror();
            continue;
        }

        num_total_samps += num_rx_samps;

        cout << "data size: " << num_rx_samps << " first: " << (&buff.front())[0] << endl;

    }

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

}

/*
typedef boost::function<uhd::sensor_value_t (const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(uhd::usrp::multi_usrp::sptr &usrp, std::vector<std::string> sensor_names, const char* sensor_name, get_sensor_fn_t get_sensor_fn, double setup_time){
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name) == sensor_names.end())
        return false;

    std::cout << "Waiting for " << sensor_name << endl;
    std::cout.flush();

    while (true) {
       uhd::usrp::multi_usrp::get_mboard_sensor(usrp, _1, 0).to_bool();
        if(b) {
          break;
        }
        sleep(1);
    }
    return true;
}
*/

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, file, type, ant, subdev, ref, wirefmt;
    size_t total_num_samps, spb;
    double rate, freq, gain, bw, total_time, setup_time;

    //setup the program options

    bool bw_summary = false;
    bool stats = false;
    bool null = false;
    bool enable_size_map = false;
    bool continue_on_bad_packet = false;

    //create a usrp device
    std::cout << std::endl;
    std::cout << "Creating the usrp device with: " << args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    //Lock mboard clocks
    usrp->set_clock_source(ref);

    //always select the subdevice first, the channel mapping affects the other settings
    usrp->set_rx_subdev_spec(string("A"));

    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;
    rate = 1000000; // trying 1MSPS

    std::cout << "Setting RX Rate: %f Msps..." << (rate/1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << "Actual RX Rate: %f Msps..." << (usrp->get_rx_rate()/1e6) << std::endl << std::endl;

    //set the center frequency
    freq=0;
    uhd::tune_request_t tune_request(freq);
    usrp->set_rx_freq(tune_request);

    //set the rf gain
    gain = 1;
    std::cout << "Setting RX Gain: %f dB..." << gain << std::endl;
    usrp->set_rx_gain(gain);
    std::cout << "Actual RX Gain: %f dB..." << usrp->get_rx_gain() << std::endl << std::endl;

    bw = 100;
    //set the IF filter bandwidth
    std::cout << "Setting RX Bandwidth: %f MHz..." << bw << std::endl;
    usrp->set_rx_bandwidth(bw);
    std::cout << "Actual RX Bandwidth: %f MHz..." << usrp->get_rx_bandwidth() << std::endl << std::endl;

    //set the antenna
    ant = "RXA";
    usrp->set_rx_antenna(ant);

    //check Ref and LO Lock detect
    //check_locked_sensor(usrp, usrp->get_rx_sensor_names(0), "lo_locked", setup_time);

    recv_to_file<std::complex<short> >(usrp, "sc16", wirefmt, file, spb, total_num_samps, total_time, bw_summary, stats, null, enable_size_map, continue_on_bad_packet);

    return EXIT_SUCCESS;
}
