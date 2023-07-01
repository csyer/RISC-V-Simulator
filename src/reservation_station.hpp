#ifndef RESERVATION_STATION_HPP
#define RESERVATION_STATION_HPP

#include "declaration.hpp"
#include "register_group.hpp"
#include "utility.hpp"

namespace cay {

int ADD ( int x, int y ) { return x+y; }                            // 0
int SUB ( int x, int y ) { return x-y; }                            // 1
int LL ( int x, int y ) { return x<<(y&0b11111); }                  // 2
int LT ( int x, int y ) { return x<y; }                             // 3
int LTU ( int x, int y ) { return unsigned(x)<unsigned(y); }        // 4
int XOR ( int x, int y ) { return x^y; }                            // 5
int RL ( int x, int y ) { return unsigned(x)>>(y&0b11111); }        // 6
int RA ( int x, int y ) { return x>>(y&0b11111); }                  // 7
int OR ( int x, int y ) { return x|y; }                             // 8
int AND ( int x, int y ) { return x&y; }                            // 9
int EQ ( int x, int y ) { return x==y; }                            // 10
int NE ( int x, int y ) { return x!=y; }                            // 11
int GE ( int x, int y ) { return x>=y; }                            // 12
int GEU ( int x, int y ) { return unsigned(x)>=unsigned(y); }       // 13
int (*FUNC[])(int, int)={ADD, SUB, LL, LT, LTU, XOR, RL, RA, OR, AND, EQ, NE, GE, GEU};

ReservationStation::ReservationStation () {}

ALUInstruction& ReservationStation::operator[] ( const int idx ) { return next_station[idx]; }
const ALUInstruction ReservationStation::operator[] ( const int idx ) const { return next_station[idx]; }

bool ReservationStation::full () const {
    for ( int i=0 ; i<SIZE ; i++ ) 
        if ( !station[i].busy ) return 0;
    return 1;
}

void ReservationStation::updateRely ( int dest, int value ) {
    for ( int i=0 ; i<SIZE ; i++ ) 
        if ( station[i].busy ) {
            if ( station[i].qj==dest )
                next_station[i].qj=-1, next_station[i].vj=value;
            if ( station[i].qk==dest ) 
                next_station[i].qk=-1, next_station[i].vk=value;
            // std::cerr <<"   update "<< i <<' '<< next_station[i].qj <<std::endl;
        }
}
void ReservationStation::excute ( ReorderBuffer& RoB, LoadStoreBuffer& LSB, RegisterGroup& Reg ) {
    for ( int i=0 ; i<SIZE ; i++ ) {
        if ( ~station[i].qj && RoB[station[i].qj].ready ) 
            next_station[i].vj=RoB[station[i].qj].value, next_station[i].qj=-1;
        if ( ~station[i].qk && RoB[station[i].qk].ready ) 
            next_station[i].vk=RoB[station[i].qk].value, next_station[i].qk=-1;
        if ( station[i].busy && !(~station[i].qj) && !(~station[i].qk) ) {
            // std::cerr <<"    update busy "<< i <<std::endl;
            next_station[i].busy=0;
            int dest=station[i].dest,
                value=FUNC[station[i].opt](station[i].vj, station[i].vk);
            // std::cerr <<"    update "<< dest <<' '<< value <<std::endl;
            RoB.update(dest, value);
            LSB.updateRely(dest, value);
            this->updateRely(dest, value);
            break;
        }
    }
    return ;
}

int ReservationStation::insertBranch ( int opt, int dest, int rs1, int rs2, RegisterGroup& Reg, ReorderBuffer& RoB ) {
    for ( int i=0, rly ; i<SIZE ; i++ ) 
        if ( !station[i].busy ) {
            ALUInstruction& ins=next_station[i];
            ins.busy=1;
            ins.opt=opt;
            rly=Reg.rely[rs1];
            if ( ~rly ) {
                if ( !RoB[rly].ready ) ins.qj=rly;
                else ins.qj=-1, ins.vj=RoB[rly].value;
            }
            else ins.vj=Reg.at(rs1), ins.qj=-1;
            rly=Reg.rely[rs2];
            if ( ~rly ) {
                if ( !RoB[rly].ready ) ins.qk=rly;
                else ins.qk=-1, ins.vk=RoB[rly].value;
            }
            else ins.vk=Reg.at(rs2), ins.qk=-1;
            // std::cerr <<"rely "<< ins.qj <<' '<< ins.qk <<' '<< dest <<std::endl;
            ins.dest=dest;
            return i;
        }
    throw "RS is full";
}
int ReservationStation::insertImm ( int opt, int dest, int rs1, int imm, RegisterGroup& Reg, ReorderBuffer& RoB ) {
    for ( int i=0, rly ; i<SIZE ; i++ ) 
        if ( !station[i].busy ) {
            ALUInstruction& ins=next_station[i];
            ins.busy=1;
            ins.opt=opt;
            rly=Reg.rely[rs1];
            if ( ~rly ) {
                if ( !RoB[rly].ready ) ins.qj=rly;
                else ins.qj=-1, ins.vj=RoB[rly].value;
            }
            else ins.vj=Reg.at(rs1), ins.qj=-1;
            ins.vk=imm;
            ins.qk=-1;
            ins.dest=dest;
            // std::cerr <<"rely "<< rly <<' '<< ins.qk <<' '<< FUNC[0](ins.vj, ins.vk) <<std::endl;
            return i;
        }
    throw "RS is full";
}
int ReservationStation::insert ( int opt, int dest, int rs1, int rs2, RegisterGroup& Reg, ReorderBuffer& RoB ) {
    for ( int i=0, rly ; i<SIZE ; i++ ) 
        if ( !station[i].busy ) {
            ALUInstruction& ins=next_station[i];
            ins.busy=1;
            ins.opt=opt;
            rly=Reg.rely[rs1];
            if ( ~rly ) {
                if ( !RoB[rly].ready ) ins.qj=rly;
                else ins.qj=-1, ins.vj=RoB[rly].value;
            }
            else ins.vj=Reg.at(rs1), ins.qj=-1;
            rly=Reg.rely[rs2];
            if ( ~rly ) {
                if ( !RoB[rly].ready ) ins.qk=rly;
                else ins.qk=-1, ins.vk=RoB[rly].value;
            }
            else ins.vk=Reg.at(rs2), ins.qk=-1;
            // std::cerr <<"rely "<< ins.vj <<' '<< ins.vk <<' '<< dest <<std::endl;
            ins.dest=dest;
            return i;
        }
    throw "RS is full";
}

void ReservationStation::clear () { 
    for ( int i=0 ; i<SIZE ; i++ ) 
        station[i].busy=next_station[i].busy=0;
}

void ReservationStation::flush () { 
    for ( int i=0 ; i<SIZE ; i++ ) 
        station[i]=next_station[i];
    return ;
}

}

#endif