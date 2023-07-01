#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "declaration.hpp"
#include "register_group.hpp"
#include "utility.hpp"

namespace cay {

class Decoder {
  public:
    Decoder () {}

    void excute ( char* mem, 
                  RegisterGroup& Reg, 
                  ReorderBuffer& RoB, 
                  ReservationStation& RS, 
                  LoadStoreBuffer& LSB,
                  Predictor& Pre ) {
        if ( RoB.full() || RoB.stall ) return ;

        if ( ~RoB.Jstall ) {
            // std::cerr <<"stall in "<< RoB.Jstall <<std::endl;
            if ( RoB[RoB.Jstall].ready ) {
                // std::cerr <<"get "<< RoB[RoB.Jstall].value+RoB.Jstall_offset <<std::endl;
                Reg.pc=Reg.next_pc=(RoB[RoB.Jstall].value+RoB.Jstall_offset)&~1;
                RoB.Jstall=-1;
            }
            else return ;
        }
        
        int code=*reinterpret_cast<unsigned int const*>(mem+Reg.pc);

        static int cnt=0;
        // std::cerr << Reg.pc <<' ';
        // std::cerr << std::hex << (unsigned)code ;
        // std::cerr << std::dec <<std::endl;

        int opcode=get_code(code, 0, 6);
        if ( code==0x0ff00513 ) { // 
            // std::cerr <<"LI"<<std::endl;
            RoB.push(BasicInstruction(-1, 0, 0, 1));
            RoB.stall=1;
            return ;
        }
        else if ( opcode==0b0110111 ) { // LUI: x[rd]=sext(immediate[31:12]<<12);
            int imm=get_code(code, 12, 31), 
                rd=get_code(code, 7, 11);
            // std::cerr <<"LUI "<< rd <<' '<< (signed int)(imm<<12) <<std::endl;

            int id=RoB.push(BasicInstruction(0, rd, imm<<12, 1));
            if ( rd!=0 ) Reg.next_rely[rd]=id;
            Reg.next_pc+=4;
        }
        else if ( opcode==0b0010111 ) { // AUIPC: x[rd]=pc+sext(immediate[31:12]<<12);
            int imm=get_code(code, 12, 31), 
                rd=get_code(code, 7, 11);

            // std::cerr <<"AUIPC "<< rd <<' '<< Reg.pc+(imm<<12) <<std::endl;

            int id=RoB.push(BasicInstruction(0, rd, Reg.pc+(imm<<12), 1));
            if ( rd!=0 ) Reg.next_rely[rd]=id;
            Reg.next_pc+=4;
        }
        else if ( opcode==0b1101111 ) { // JAL: x[rd]=pc+4, pc+=sext(offset);
            int rd=get_code(code, 7, 11), offset=0;

            offset|=(get_code(code, 31, 31)<<20);
            offset|=(get_code(code, 21, 30)<<1);
            offset|=(get_code(code, 20, 20)<<11);
            offset|=(get_code(code, 12, 19)<<12);

            // std::cerr <<"JAL "<< rd <<' '<< sext(offset, 20) <<std::endl;
            // std::cerr << get_code(code, 31, 31) <<' '<< code <<std::endl;
            Reg.next_pc+=sext(offset, 20);
            int id=RoB.push(BasicInstruction(0, rd, Reg.pc+4, 1));
            if ( rd!=0 ) Reg.next_rely[rd]=id;
        }
        else if ( opcode==0b1100111 ) { // JALR: t=pc+4, pc=(x[rs1]+sext(offset))&~1, x[rd]=t;
            // std::cerr <<"JALR"<<std::endl;
            int offset=get_code(code, 20, 31),
                rd=get_code(code, 7, 11),
                rs1=get_code(code, 15, 19);
            
            int id=RoB.push(BasicInstruction(0, rd, Reg.pc+4, 1));
            if ( rd!=0 ) Reg.next_rely[rd]=id;

            // std::cerr <<"JALR "<< rs1 <<' '<< Reg.rely[rs1] <<' '<< Reg.pc <<std::endl;

            if ( ~Reg.rely[rs1] ) {
                RoB.Jstall=Reg.rely[rs1];
                RoB.Jstall_offset=sext(offset, 11);
                return ;
            }
            else Reg.next_pc=(Reg.at(rs1)+sext(offset, 11))&~1;
        }
        else if ( opcode==0b1100011 ) {
            // std::cerr <<"BRANCH"<<std::endl;
            int rs1=get_code(code, 15, 19),
                rs2=get_code(code, 20, 24),
                idcode=get_code(code, 12, 14),
                dest;
            int offset=0;
            offset|=(get_code(code, 8, 11)<<1);
            offset|=(get_code(code, 25, 30)<<5);
            offset|=(get_code(code, 7, 7)<<11);
            offset|=(get_code(code, 30, 30)<<12);

            if ( idcode==0b000 ) { // BEQ: if ( x[rs1]==x[rs2] ) pc+=sext(offset);
                if ( Pre.predict(Reg.pc) ) {
                    Reg.next_pc+=sext(offset, 12);
                    dest=RoB.push(BasicInstruction(2, Reg.pc+4, 1, Reg.pc));
                }
                else {
                    Reg.next_pc+=4;
                    dest=RoB.push(BasicInstruction(2, Reg.pc+sext(offset, 12), 0, Reg.pc));
                }
                RS.insertBranch(10, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b001 ) { // BNE: if ( x[rs1]!=x[rs2] ) pc+=sext(offset);
                if ( Pre.predict(Reg.pc) ) {
                    Reg.next_pc+=sext(offset, 12);
                    dest=RoB.push(BasicInstruction(2, Reg.pc+4, 1, Reg.pc));
                }
                else {
                    Reg.next_pc+=4;
                    dest=RoB.push(BasicInstruction(2, Reg.pc+sext(offset, 12), 0, Reg.pc));
                }
                // std::cerr <<"BNE "<< rs1 <<' '<< rs2 <<' '<< Reg.pc+sext(offset, 12) <<std::endl;
                RS.insertBranch(11, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b100 ) { // BLT: if ( x[rs1]<x[rs2] ) pc+=sext(offset)
                if ( Pre.predict(Reg.pc) ) {
                    Reg.next_pc+=sext(offset, 12);
                    dest=RoB.push(BasicInstruction(2, Reg.pc+4, 1, Reg.pc));
                }
                else {
                    Reg.next_pc+=4;
                    dest=RoB.push(BasicInstruction(2, Reg.pc+sext(offset, 12), 0, Reg.pc));
                }
                RS.insertBranch(3, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b101 ) { // BGE: if ( x[rs1]>=x[rs2] ) pc+=sext(offset)
                if ( Pre.predict(Reg.pc) ) {
                    Reg.next_pc+=sext(offset, 12);
                    dest=RoB.push(BasicInstruction(2, Reg.pc+4, 1, Reg.pc));
                }
                else {
                    Reg.next_pc+=4;
                    dest=RoB.push(BasicInstruction(2, Reg.pc+sext(offset, 12), 0, Reg.pc));
                }
                RS.insertBranch(12, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b110 ) { // BLTU: if ( unsigned(x[rs1])<x[rs2] ) pc+=sext(offset);
                if ( Pre.predict(Reg.pc) ) {
                    Reg.next_pc+=sext(offset, 12);
                    dest=RoB.push(BasicInstruction(2, Reg.pc+4, 1, Reg.pc));
                }
                else {
                    Reg.next_pc+=4;
                    dest=RoB.push(BasicInstruction(2, Reg.pc+sext(offset, 12), 0, Reg.pc));
                }
                RS.insertBranch(4, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b111 ) { // BGEU: if ( unsigned(x[rs1])>=x[rs2] ) pc+=sext(offset);
                if ( Pre.predict(Reg.pc) ) {
                    Reg.next_pc+=sext(offset, 12);
                    dest=RoB.push(BasicInstruction(2, Reg.pc+4, 1, Reg.pc));
                }
                else {
                    Reg.next_pc+=4;
                    dest=RoB.push(BasicInstruction(2, Reg.pc+sext(offset, 12), 0, Reg.pc));
                }
                RS.insertBranch(13, dest, rs1, rs2, Reg, RoB);
            }
            else throw "Invalid Branch Instruction";
        }
        else if ( opcode==0b0000011 ) { 
            // std::cerr <<"LOAD"<<std::endl;
            if ( LSB.full() ) return ;
            int rd=get_code(code, 7, 11), 
                rs1=get_code(code, 15, 19), 
                offset=get_code(code, 20, 31),
                idcode=get_code(code, 12, 14),
                dest=0;
            if ( idcode==0b000 ) { // LB: x[rd]=sext(M[x[rs1]+sext(offset)][7:0]);
                dest=RoB.push(BasicInstruction(1, rd));
                LSB.pushLoad(rs1, sext(offset, 11), dest, 7, 0, mem, Reg, RoB);
            }
            else if ( idcode==0b001 ) { // LH: x[rd]=sext(M[x[rs1]+sext(offset)][15:0]);
                dest=RoB.push(BasicInstruction(1, rd));
                LSB.pushLoad(rs1, sext(offset, 11), dest, 15, 0, mem, Reg, RoB);
            }
            else if ( idcode==0b010 ) { // LW: x[rd]=sext(M[x[rs1]+sext(offset)][31:0]);
                dest=RoB.push(BasicInstruction(1, rd));
                LSB.pushLoad(rs1, sext(offset, 11), dest, 31, 0, mem, Reg, RoB);
            }
            else if ( idcode==0b100 ) { // LBU: x[rd]=M[x[rs1]+sext(offset)][7:0];
                dest=RoB.push(BasicInstruction(1, rd));
                // std::cerr <<"LBU "<< rd <<' '<< rs1 <<' '<< dest <<std::endl;
                LSB.pushLoad(rs1, sext(offset, 11), dest, 7, 1, mem, Reg, RoB);
            }
            else if ( idcode==0b101 ) { // LHU: x[rd]=M[x[rs1]+sext(offset)][15:0];
                dest=RoB.push(BasicInstruction(1, rd));
                LSB.pushLoad(rs1, sext(offset, 11), dest, 15, 1, mem, Reg, RoB);
            }
            else throw "Invalid Load Instruction";
            if ( rd!=0 ) Reg.next_rely[rd]=dest;
            Reg.next_pc+=4;
        }
        else if ( opcode==0b0100011 ) {
            // std::cerr <<"STORE"<<std::endl;
            if ( LSB.full() ) return ;
            int offset=sext((get_code(code, 25, 31)<<5)|get_code(code, 7, 11), 11), 
                rs1=get_code(code, 15, 19), 
                rs2=get_code(code, 20, 24),
                idcode=get_code(code, 12, 14);
            if ( idcode==0b000 ) { // SB: M[x[rs1]+sext(offset)]=x[rs2][7:0];
                int id=LSB.pushStore(rs1, rs2, offset, 7, mem, Reg, RoB);
                int dest=RoB.push(BasicInstruction(3, id));
                LSB[id].dest=dest;
            }
            else if ( idcode==0b001 ) { // SH: M[x[rs1]+sext(offset)]=x[rs2][15:0];
                int id=LSB.pushStore(rs1, rs2, offset, 15, mem, Reg, RoB);
                int dest=RoB.push(BasicInstruction(3, id));
                LSB[id].dest=dest;
            }
            else if ( idcode==0b010 ) { // SW: M[x[rs1]+sext(offset)]=x[rs2][31:0];
                // std::cerr <<"SW "<< rs1 <<' '<< rs2 <<' '<< offset <<std::endl;
                int id=LSB.pushStore(rs1, rs2, offset, 31, mem, Reg, RoB);
                int dest=RoB.push(BasicInstruction(3, id));
                LSB[id].dest=dest;
                // std::cerr <<"dest = "<< dest <<std::endl;
            }
            else throw "Invalid Store Instruction";
            Reg.next_pc+=4;
        }
        else if ( opcode==0b0010011 ) {
            // std::cerr <<"ALUI"<<std::endl;
            if ( RS.full() ) return ;
            int imm=sext(get_code(code, 20, 31), 11), 
                shamt=get_code(code, 20, 24),
                rs1=get_code(code, 15, 19), 
                rd=get_code(code, 7, 11),
                idcode=get_code(code, 12, 14),
                dest=0;
            if ( idcode==0b000 ) { // ADDI: x[rd]=x[rs1]+sext(immediate);
                // std::cerr <<"ADDI "<< rd <<' '<< rs1 <<' '<< imm <<std::endl;
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(0, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b010 ) { // SLTI: x[rd]=(x[rs1]<sext(immediate));
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(3, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b011 ) { // SLTIU: x[rd]=(unsigned(x[rs1])<sext(immediate));
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(4, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b100 ) { // XORI: x[rd]=x[rs1]^sext(immediate);
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(5, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b110 ) { // ORI: x[rd]=x[rs1]|sext(immediate);
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(8, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b111 ) { // ANDI: x[rd]=x[rs1]&sext(immediate);
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(9, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b001 ) { // SLLI: x[rd]=(x[rs1]<<shamt);
                if ( shamt>>5 ) throw "Invalid SLLI Instruction";
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insertImm(2, dest, rs1, imm, Reg, RoB);
            }
            else if ( idcode==0b101 ) { 
                if ( shamt>>5 ) throw "Invalid SRI Instruction";
                if ( get_code(code, 25, 31)==0b0000000 ) { // SRLI: x[rd]=(x[rs1]>>shamt);
                    dest=RoB.push(BasicInstruction(0, rd));
                    RS.insertImm(6, dest, rs1, imm, Reg, RoB);
                }
                else if ( get_code(code, 25, 31)==0b0100000 ) { // SRAI: x[rd]=sext(x[rs1]>>shamt);
                    dest=RoB.push(BasicInstruction(0, rd));
                    RS.insertImm(7, dest, rs1, imm, Reg, RoB);
                }
                else throw "Invalid SRI Instruction";
            }
            else throw "Invalid ALUI Instruction";
            if ( rd!=0 ) Reg.next_rely[rd]=dest;
            Reg.next_pc+=4;
        }
        else if ( opcode==0b0110011 ) { 
            // std::cerr <<"ALU"<<std::endl;
            if ( RS.full() ) return ;
            int rd=get_code(code, 7, 11), 
                rs1=get_code(code, 15, 19), 
                rs2=get_code(code, 20, 24),
                idcode=get_code(code, 12, 14),
                dest=0;
            if ( idcode==0b000 ) { 
                if ( get_code(code, 25, 31)==0b0000000 ) { // ADD: x[rd]=x[rs1]+x[rs2];
                    dest=RoB.push(BasicInstruction(0, rd));
                    RS.insert(0, dest, rs1, rs2, Reg, RoB);
                }
                else if ( get_code(code, 25, 31)==0b0100000 ) { // SUB: x[rd]=x[rs1]-x[rs2];
                    dest=RoB.push(BasicInstruction(0, rd));
                    RS.insert(1, dest, rs1, rs2, Reg, RoB);
                    // std::cerr <<"SUB "<< rs1 <<' '<< rs2 <<' '<< id <<std::endl;
                }
                else throw "Invalid ADD/SUB Instruction";
            }
            else if ( idcode==0b001 ) { // SLL: x[rd]=(x[rs1]<<(x[rs2]&0b11111));
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insert(2, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b010 ) { // SLT: x[rd]=(x[rs1]<x[rs2]);
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insert(3, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b011 ) { // SLTU: x[rd]=(unsigned(x[rs1])<x[rs2]);
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insert(4, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b100 ) { // XOR: x[rd]=x[rs1]^x[rs2];
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insert(5, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b101 ) { 
                if ( get_code(code, 25, 31)==0b0000000 ) { // SRL: x[rd]=x[rs1]>>(x[rs2]&0b11111);
                    dest=RoB.push(BasicInstruction(0, rd));
                    RS.insert(6, dest, rs1, rs2, Reg, RoB);
                }
                else if ( get_code(code, 25, 31)==0b0100000 ) { // SRA: x[rd]=sext(x[rs1]>>(x[rs2]&0b11111));
                    dest=RoB.push(BasicInstruction(0, rd));
                    RS.insert(7, dest, rs1, rs2, Reg, RoB);
                }
                else throw "Invalid SR Instruction";
            }
            else if ( idcode==0b110 ) { // OR: x[rd]=x[rs1]|x[rs2];
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insert(8, dest, rs1, rs2, Reg, RoB);
            }
            else if ( idcode==0b111 ) { // AND: x[rd]=x[rs1]&x[rs2];
                dest=RoB.push(BasicInstruction(0, rd));
                RS.insert(8, dest, rs1, rs2, Reg, RoB);
            }
            else throw "Invalid ALU Instruction";
            if ( rd!=0 ) Reg.next_rely[rd]=dest;
            Reg.next_pc+=4;
        }
        else throw "Invalid Instruction";
    }
    
  private:
    int get_code ( int code, int l, int r ) {
        unsigned int u_code=code;
        return (int)((u_code&((1ull<<(r+1))-1))>>l);
    }
};

}

#endif