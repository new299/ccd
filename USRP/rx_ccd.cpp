#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>
#include <SDL/SDL.h>
#include <pthread.h>

using namespace std;
    
std::vector<std::complex<short> > buffer;
uhd::usrp::multi_usrp::sptr usrp;

void *recv_thread(void *) {
    //create a receive streamer
    uhd::stream_args_t stream_args("sc16","");
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;


    //setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.num_samps = 0;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    rx_stream->issue_stream_cmd(stream_cmd);

    for(;;) {
      size_t num_rx_samps = rx_stream->recv(&buffer.front(), buffer.size(), md, 3.0, false);
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) { cout << "timeout"  << endl; }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){ cout << "overflow" << endl; }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE)    { cout <<  md.strerror() << endl; }
    }

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    usrp = uhd::usrp::multi_usrp::make(string(""));

    usrp->set_clock_source("internal"); // set clock

    //always select the subdevice first, the channel mapping affects the other settings
    usrp->set_rx_subdev_spec(string("A:A"));

    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;
    double rate = 4000000; // trying 1MSPS
    usrp->set_rx_rate(rate);

    //set the center frequency
    double freq=0;
    uhd::tune_request_t tune_request(freq);
    usrp->set_rx_freq(tune_request);

    usrp->set_rx_gain(9);
    usrp->set_rx_bandwidth(10);

//    ant = "";
//    usrp->set_rx_antenna(ant);

//   cout << usrp->get_mboard_sensor("lo_locked", 0).to_bool() << endl;

    sleep(1);

    // Init SDL
    SDL_Surface *screen;
 
    if(SDL_Init(SDL_INIT_VIDEO)<0) {
      cout << "Failed SDL_Init " << SDL_GetError() << endl;
      return 1;
    }
 
    screen=SDL_SetVideoMode(800,600,32,SDL_ANYFORMAT);
    if(screen==NULL) {
      cout << "Failed SDL_SetVideoMode: " << SDL_GetError() << endl;
      SDL_Quit();
      return 1;
    }

/////////////// RECEIVE CODE

    buffer = std::vector<std::complex<short> >(rate/30);

    pthread_t r;
    pthread_create(&r, NULL, &recv_thread, NULL);

    int ix =200;
    int iy =125;
int v_off=0;
    int maxv=1;
    int omaxv=1;
    int minv=1;
    int ominv=1;
    unsigned int threshold=128;
    int tdist=0;
    int tdistthres=128;
    for(;;) {

        omaxv=maxv;
        maxv=1;
        ominv=minv;
        minv=99999999;

        //cout << "P2" << endl;
        //cout << x << " " << y << endl;
        //cout << "1000 0" << endl;
       SDL_Flip(screen);
       SDL_LockSurface(screen);
        int pix=0;
    unsigned int x=0;
    unsigned int y=0;
        for(int n=0;n<buffer.size();n++) { 
        
          int v = buffer[n].real();
          if(v < 0) v = 0-v;
          if(pix==0) cout << "vA: " << v << endl;
          if(pix==0) cout << "omaxv: " << omaxv << endl;
          if(v > maxv) maxv=v;
          if(v < minv) minv=v;
          v = (((double)v-ominv)/(double)(omaxv-ominv))*255.0;

          if((v > threshold) && (tdist > tdistthres)) {x=0;y++;tdist=0;}
                       else {x++;tdist++;}

          if(x>=800) x=0;
          if(y>=600) y=0;
// cout << x << " " << y << endl;
v+=50;
if(v>255) v=255;
          if(pix==0) cout << "vC: " << v << endl;
          unsigned int pixel= v | (v << 8) | (v << 16) | (v << 24);
          if(pix==0) cout << "vD: " << pixel << endl;
          int bpp = screen->format->BytesPerPixel;
          Uint8 *p = (Uint8 *)screen->pixels + y * screen->pitch + x * bpp;
          *(Uint32 *)p = pixel;


          SDL_Event event;
          while(SDL_PollEvent(&event)) {
            if(event.key.keysym.sym == SDLK_a     ) { ix-=1; cout << ix << " " << iy << endl;}
            if(event.key.keysym.sym == SDLK_s     ) { ix+=1; }
            if(event.key.keysym.sym == SDLK_e     ) { iy+=1; }
            if(event.key.keysym.sym == SDLK_d     ) { iy-=1; }
            if(event.key.keysym.sym == SDLK_o     ) { v_off++; }
            if(event.key.keysym.sym == SDLK_c     ) { SDL_FillRect(screen,NULL,0);  }
            if(event.key.keysym.sym == SDLK_t     ) { threshold++;  }
            if(event.key.keysym.sym == SDLK_g     ) { threshold--;  }
            if(event.key.keysym.sym == SDLK_y     ) { tdistthres++;  }
            if(event.key.keysym.sym == SDLK_h     ) { tdistthres--;  }
          } 
          pix++;
        }
       SDL_UnlockSurface(screen);
 
  //      cout << "data size: " << num_rx_samps << " first: " << (&buffer.front())[0] << endl;
    }

    // Will never get here
}

