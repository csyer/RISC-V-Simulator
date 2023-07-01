#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

#include "declaration.hpp"
#include "register_group.hpp"
#include "utility.hpp"

namespace cay {

ReorderBuffer::ReorderBuffer (): stall(0), Jstall(-1), Jstall_offset(0) {}

BasicInstruction& ReorderBuffer::operator[] ( const int idx ) { return buffer[idx]; }
const BasicInstruction ReorderBuffer::operator[] ( const int idx ) const { return buffer[idx]; }

bool ReorderBuffer::full () { return buffer.full(); }

void ReorderBuffer::update ( int dest, int value ) {
    BasicInstruction& ins=next_buffer[dest];
    ins.ready=1;
    ins.value=value;
}

int ReorderBuffer::push ( const BasicInstruction& ins ) {
    return next_buffer.push(ins); 
}
void ReorderBuffer::excute ( ReservationStation& RS, LoadStoreBuffer& LSB, RegisterGroup& Reg, Predictor& Pre ) {
    if ( buffer.empty() ) return ;
    static int cnt=0;
    BasicInstruction ins=buffer.front();
    if ( ins.ready ) { // commit
        int id=buffer.begin();
        next_buffer.pop();
        if ( ins.type==-1 ) {
            throw Reg.at(10);
        }
        if ( ins.type==0 && ins.dest!=0 ) Reg[ins.dest]=ins.value;
        if ( ins.type==1 && ins.dest!=0 ) Reg[ins.dest]=ins.value;
        if ( ins.type==2 ) {
            if ( ins.value==ins.src ) Pre.hit(ins.pc);
            else {
                Pre.fail(ins.pc);
                next_buffer.clear();
                RS.clear();
                LSB.clear();
                Reg.pc=Reg.next_pc=ins.dest;
                Reg.clear();
                stall=0;
                Jstall=-1;
                Jstall_offset=0;
            }
            return ;
        }
        Reg.update(id);
        LSB.updateRely(id, ins.value);
        if ( ins.naive ) {
            RS.updateRely(id, ins.value);
        }
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