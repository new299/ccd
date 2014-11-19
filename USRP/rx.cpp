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

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(string(""));

    usrp->set_clock_source("internal"); // set clock

    //always select the subdevice first, the channel mapping affects the other settings
    usrp->set_rx_subdev_spec(string("A:A"));

    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;
    double rate = 1000000; // trying 1MSPS
    usrp->set_rx_rate(rate);

    //set the center frequency
    double freq=0;
    uhd::tune_request_t tune_request(freq);
    usrp->set_rx_freq(tune_request);

    //usrp->set_rx_gain(5);
    //usrp->set_rx_bandwidth(100);

//    ant = "";
//    usrp->set_rx_antenna(ant);

//   cout << usrp->get_mboard_sensor("lo_locked", 0).to_bool() << endl;

    sleep(1);

/////////////// RECEIVE CODE

    //create a receive streamer
    uhd::stream_args_t stream_args("sc16","");
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;
    std::vector<std::complex<short> > buffer(50000);

    //setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.num_samps = 0;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    rx_stream->issue_stream_cmd(stream_cmd);

    for(;;) {

        size_t num_rx_samps = rx_stream->recv(&buffer.front(), buffer.size(), md, 3.0, false);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) { cerr << "timeout"  << endl; }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){ cerr << "overflow" << endl; }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)    { cerr <<  md.strerror() << endl; }

        cout << "data size: " << num_rx_samps << " first: " << (&buffer.front())[0] << endl;
    }

    // Will never get here
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);
}
