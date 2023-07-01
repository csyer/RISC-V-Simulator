#ifndef DECLARATION_HPP
#define DECLARATION_HPP

#include "register_group.hpp"
#include "utility.hpp"

namespace cay {

class LoadStoreBuffer;
class ReorderBuffer;
class ReservationStation;

const int SIZE=32;

class LoadStoreBuffer {
  public:
    LoadStoreBuffer ();

    MemoryInstruction& operator[] ( const int );
    const MemoryInstruction operator[] ( const int ) const;

    bool full ();
    void pushLoad ( int, int, int, int, int, char*, RegisterGroup&, ReorderBuffer& );
    int pushStore ( int, int, int, int, char*, RegisterGroup&, ReorderBuffer& );
    void update ( int );
    void updateRely ( int, int );
    void excute ( char*, ReorderBuffer&, ReservationStation&, RegisterGroup& );
    void clear ();
    void flush ();
  private:
    queue<MemoryInstruction> buffer, next_buffer;
};
class ReorderBuffer {
  public:
    int stall, Jstall, Jstall_offset;
    ReorderBuffer ();

    BasicInstruction& operator[] ( const int );
    const BasicInstruction operator[] ( const int) const;

    bool full ();
    void update ( int , int );
    int push ( const BasicInstruction& );
    void excute ( ReservationStation&, LoadStoreBuffer&, RegisterGroup& );
    void clear ();
    void flush ();
  private:
    queue<BasicInstruction> buffer, next_buffer;
};
class ReservationStation {
  public:
    ReservationStation ();

    ALUInstruction& operator[] ( const int );
    const ALUInstruction operator[] ( const int ) const;

    bool full () const;
    void updateRely ( int, int );
    void excute ( ReorderBuffer&, LoadStoreBuffer&, RegisterGroup& );
    int insertBranch ( int, int, int, int, RegisterGroup&, ReorderBuffer& );
    int insertImm ( int, int, int, int, RegisterGroup&, ReorderBuffer& );
    int insert ( int, int, int, int, RegisterGroup&, ReorderBuffer& );
    void clear ();
    void flush ();
  private:
    ALUInstruction station[SIZE],
                   next_station[SIZE];
};

}

#endif