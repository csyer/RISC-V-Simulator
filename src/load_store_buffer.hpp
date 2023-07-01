#ifndef LOAD_STORE_BUFFER_HPP
#define LOAD_STORE_BUFFER_HPP

#include "declaration.hpp"
#include "register_group.hpp"
#include "utility.hpp"

namespace cay {

LoadStoreBuffer::LoadStoreBuffer () {}

MemoryInstruction& LoadStoreBuffer::operator[] ( const int idx ) { return next_buffer[idx]; }
const MemoryInstruction LoadStoreBuffer::operator[] ( const int idx ) const { return next_buffer[idx]; }

bool LoadStoreBuffer::full () { return buffer.full(); }

void LoadStoreBuffer::pushLoad ( int rs, int offset, int dest, int bit, int sign, char* mem, RegisterGroup& Reg, ReorderBuffer& RoB ) { // Load
    MemoryInstruction ins;
    ins.dest=dest;
    ins.type=0;
    ins.sign=sign;
    ins.bit=bit;
    ins.offset=offset;
    int rly=Reg.rely[rs];
    if ( ~rly ) {
        if ( !RoB[rly].ready ) ins.vj=rs, ins.qj=rly;
        else ins.vj=RoB[rly].value, ins.qj=-1;
        // std::cerr <<"rely "<< rly <<std::endl;
    }
    else ins.ready=1, ins.qj=-1, ins.vj=Reg.at(rs);
    ins.qk=-1;
    next_buffer.push(ins);
}
int LoadStoreBuffer::pushStore ( int rs1, int rs2, int offset, int bit, char* mem, RegisterGroup& Reg, ReorderBuffer& RoB ) { // Store
    MemoryInstruction ins;
    ins.type=1;
    ins.bit=bit;
    ins.offset=offset;
    int rly=Reg.rely[rs1];
    if ( ~rly ) {
        if ( !RoB[rly].ready ) ins.qj=rly;
        else ins.qj=-1, ins.vj=RoB[rly].value;
        // std::cerr <<"exist rely "<< ins.qj <<std::endl;
    }
    else ins.vj=Reg.at(rs1), ins.qj=-1;
    rly=Reg.rely[rs2];
    if ( ~rly ) {
        if ( !RoB[rly].ready ) ins.qk=rly;
        else ins.qk=-1, ins.vk=RoB[rly].value;
    }
    else ins.vk=Reg.at(rs2), ins.qk=-1;
    return next_buffer.push(ins);
}

void LoadStoreBuffer::update ( int dest ) { next_buffer[dest].ready=1; }
void LoadStoreBuffer::updateRely ( int dest, int value ) {
    // std::cerr <<"LSB updateRely "<< dest <<' '<< value <<std::endl;
    for ( int i=buffer.begin() ; i!=buffer.end() ; i=buffer.next(i) ) {
        if ( buffer[i].qj==dest ) {
            next_buffer[i].vj=value;
            next_buffer[i].qj=-1;
        }
        if ( buffer[i].qk==dest ) {
            next_buffer[i].vk=value;
            next_buffer[i].qk=-1;
        }
        if ( !buffer[i].type && !(~buffer[i].qj) && !(~buffer[i].qk) )
            next_buffer[i].ready=1;
    }
}

void LoadStoreBuffer::excute ( char* mem, ReorderBuffer& RoB, ReservationStation& RS, RegisterGroup& Reg ) {
    if ( buffer.empty() ) return ;
    // for ( int i=buffer.begin() ; i!=buffer.end() ; i++ ) 
    //     std::cerr << buffer[i].dest <<' ';
    // std::cerr <<std::endl;
    MemoryInstruction ins=buffer.front();
    if ( ~ins.qj && RoB[ins.qj].ready ) ins.vj=RoB[ins.qj].value, ins.qj=-1;
    if ( ~ins.qk && RoB[ins.qk].ready ) ins.vk=RoB[ins.qk].value, ins.qk=-1;
    if ( !(~ins.qj) && !(~ins.qk) || ins.ready ) { // commit
        // std::cerr <<"is ready"<<std::endl;
        int id=buffer.begin();
        --ins.count;
        if ( ins.count!=0 ) {
            next_buffer[id].count--;
            return ;
        }
        // std::cerr <<"count down end"<<std::endl;
        if ( ins.type==0 ) {
            int addr=ins.vj+ins.offset, value=0;
            unsigned int _value=0;
            if ( ins.bit==31 ) _value=*reinterpret_cast<unsigned int const*>(mem+addr);
            if ( ins.bit==15 ) _value=*reinterpret_cast<unsigned short const*>(mem+addr);
            if ( ins.bit==7 ) _value=*reinterpret_cast<unsigned char const*>(mem+addr);
            if ( !ins.sign ) value=sext((int)_value, ins.bit);
            else value=_value;
            // std::cerr <<"load "<< addr <<' '<< value <<" to "<< ins.dest <<' '<< ins.offset <<std::endl;
            RoB.update(ins.dest, value);
            RS.updateRely(ins.dest, value);
            this->updateRely(ins.dest, value);
            // std::cerr <<"break"<<std::endl;
        }
        if ( ins.type==1 ) {
            int value=ins.vk, addr=ins.vj+ins.offset;
            // std::cerr <<"store "<< value <<" in "<< addr <<' '<< ins.bit <<std::endl;
            // std::cerr << ins.dest <<std::endl;
            // std::cerr << std::hex << ins.vj <<std::endl;
            if ( ins.bit==31 ) *reinterpret_cast<unsigned int*>(mem+addr)=(unsigned int)value;
            if ( ins.bit==15 ) *reinterpret_cast<unsigned short*>(mem+addr)=(unsigned int)value;
            if ( ins.bit==7 ) *reinterpret_cast<unsigned char*>(mem+addr)=(unsigned int)value;
            RoB.update(ins.dest, 0);
            // std::cerr << std::dec << "break"<<std::endl;
        }
        next_buffer.pop();
    }
}

void LoadStoreBuffer::clear () { 
    buffer.clear();
    next_buffer.clear();
}
void LoadStoreBuffer::flush () { buffer=next_buffer; }

}

#endif