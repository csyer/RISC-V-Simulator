#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

#include "declaration.hpp"
#include "register_group.hpp"
#include "utility.hpp"

namespace cay {

ReorderBuffer::ReorderBuffer (): stall(0), Jstall(-1), Jstall_offset(0) {}

BasicInstruction& ReorderBuffer::operator[] ( const int idx ) { return next_buffer[idx]; }
const BasicInstruction ReorderBuffer::operator[] ( const int idx ) const { return next_buffer[idx]; }

bool ReorderBuffer::full () { return buffer.full(); }

void ReorderBuffer::update ( int dest, int value ) {
    BasicInstruction& ins=next_buffer[dest];
    ins.ready=1;
    ins.value=value;
    // std::cerr <<"  RoB update "<< dest <<' '<< value <<std::endl;
}

int ReorderBuffer::push ( const BasicInstruction& ins ) {
    // if ( next_buffer.end()==14 ) ins.print(); 
    return next_buffer.push(ins); 
}
void ReorderBuffer::excute ( ReservationStation& RS, LoadStoreBuffer& LSB, RegisterGroup& Reg ) {
    if ( buffer.empty() ) return ;
    static int cnt=0;
    BasicInstruction ins=buffer.front();
    // std::cout <<"stall "<< ins.dest <<std::endl;
    if ( ins.ready ) { // commit
        int id=buffer.begin();
        // std::cerr <<"commit "<< ++cnt <<' '<< id <<std::endl;
        ++cnt;
        if ( cnt%1000000==0 ) std::cerr << cnt <<std::endl;
        next_buffer.pop();
        if ( ins.type==-1 ) {
            // std::cerr <<"EXIT"<<std::endl;
            throw Reg.at(10);
        }
        if ( ins.type==0 && ins.dest!=0 ) Reg[ins.dest]=ins.value;//, std::cerr <<"  modify 0 "<< ins.dest <<' '<< ins.value <<std::endl;
        if ( ins.type==1 && ins.dest!=0 ) Reg[ins.dest]=ins.value;//, std::cerr <<"  modify 1 "<< ins.dest <<' '<< ins.value <<std::endl;
        if ( ins.type==2 ) {
            if ( ins.value==ins.src ) {
                // predictor hit
                // std::cerr <<"predictor hit\n";
            }
            else {
                // std::cerr <<"predictor fail\n";
                next_buffer.clear();
                RS.clear();
                LSB.clear();
                Reg.pc=Reg.next_pc=ins.dest;
                Reg.clear();
                stall=0;
                Jstall=-1;
                Jstall_offset=0;
            }
            // for ( int i=0 ; i<32 ; i++ ) 
            //     std::cerr << (int)Reg[i] <<' ';
            // std::cerr <<std::endl;
            return ;
        }
        Reg.update(id);
        if ( ins.naive ) {
            RS.updateRely(id, ins.value);
            LSB.updateRely(id, ins.value);
        }
        // for ( int i=0 ; i<32 ; i++ ) 
        //     std::cerr << (int)Reg[i] <<' ';
        // std::cerr <<std::endl;
    }
    else if ( ins.type==3 ) LSB.update(ins.dest);
}

void ReorderBuffer::clear () { 
    buffer.clear(); 
    next_buffer.clear();
}
void ReorderBuffer::flush () { buffer=next_buffer; }

}

#endif