#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <cstdio>
#include <iostream>

#include "instructions.hpp"
#include "reservation_station.hpp"
#include "reorder_buffer.hpp"
#include "load_store_buffer.hpp"
#include "utility.hpp"

namespace cay {

class Simulator {
  public:
    Simulator () { memset(mem, 0, sizeof(mem)); }
    void read () {
        char token[16];
        int addr=0;
        while ( ~scanf("%s", token) ) {
            if ( token[0]=='@' ) sscanf(token+1, "%x", &addr);
            else sscanf(token, "%hhx", mem+addr), ++addr;
        }
    }
    void run () {
        int clk=0;
        try {
            for ( ; ; ++clk ) {
                // std::cerr <<" pc = "<< Reg.pc <<std::endl;
                decoder.excute(mem, Reg, RoB, RS, LSB, Pre);
                // std::cerr <<"Decoder end"<<std::endl;
                RS.excute(RoB, LSB, Reg);
                // std::cerr <<"RS end"<<std::endl;
                LSB.excute(mem, RoB, RS, Reg);
                // std::cerr <<"LSB end"<<std::endl;
                RoB.excute(RS, LSB, Reg, Pre);
                // std::cerr <<"RoB end"<<std::endl;
                
                RoB.flush();
                // std::cerr <<"RoB flush"<<std::endl;
                RS.flush();
                // std::cerr <<"RS flush"<<std::endl;
                LSB.flush();
                // std::cerr <<"LSB flush"<<std::endl;
                Reg.flush();
                // std::cerr <<"REG flush"<<std::endl;
            }
        }
        catch ( int ret ) {
            printf("%u\n", unsigned(ret)&(0b11111111));
            return ;
        }
        catch ( const char* info ) {
            std::cerr << info <<std::endl;
        }
        return ;
    }
  private:
    char mem[1<<22];
    Decoder decoder;
    RegisterGroup Reg;
    ReorderBuffer RoB;
    ReservationStation RS;
    LoadStoreBuffer LSB;
    Predictor Pre;
};

}

#endif