#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <iostream>
#include <cstring>

namespace cay {

int sext ( int x, int bit ) { 
    return (x<<(31-bit))>>(31-bit);
}

struct BasicInstruction {
    int ready, naive;
    int type, dest, src, value; 
    /* 
        type : 10 -> branch ; 0 -> register ; 1 -> memory
        dest : 10 -> pc     ; 0 -> idx      ; 1 -> idx
        src  : 10 -> /      ; 0 -> value    ; 1 -> value
    */

    BasicInstruction (): ready(0), naive(0) {}
    BasicInstruction ( int _type, int _dest ): type(_type), dest(_dest), ready(0), naive(0) {}
    BasicInstruction ( int _type, int _dest, int _value ): type(_type), dest(_dest), ready(0), naive(0) {
        if ( type==2 ) src=_value;
        else value=_value;
    }
    BasicInstruction ( int _type, int _dest, int _value, int _ready ): 
        type(_type), dest(_dest), value(_value), ready(_ready), naive(1) {}

    BasicInstruction& operator= ( const BasicInstruction& obj ) {
        if ( this==&obj ) return *this;
        ready=obj.ready;
        naive=obj.naive;
        type=obj.type;
        dest=obj.dest;
        src=obj.src;
        value=obj.value;
        return *this;
    }

    void print () const {
        std::cerr <<"********"<< type <<' '<< dest <<' '<< naive <<std::endl;
    }
};
struct ALUInstruction {
    int busy, opt, vj, vk, qj, qk, dest;
    
    ALUInstruction (): busy(0), qj(-1), qk(-1) {}
    ALUInstruction& operator= ( const ALUInstruction& obj ) {
        if ( &obj==this ) return *this;
        busy=obj.busy;
        opt=obj.opt;
        vj=obj.vj;
        vk=obj.vk;
        qj=obj.qj;
        qk=obj.qk;
        dest=obj.dest;
        return *this;
    }
};
struct MemoryInstruction {
    int ready, type, count;
    int vj, vk, qj, qk, sign, bit, dest, offset;

    MemoryInstruction (): ready(0), count(3), vj(0), qj(-1), vk(0), qk(-1) {}
    MemoryInstruction& operator= ( const MemoryInstruction& obj ) {
        if ( &obj==this ) return *this;
        ready=obj.ready;
        type=obj.type;
        count=obj.count;
        vj=obj.vj;
        vk=obj.vk;
        qj=obj.qj;
        qk=obj.qk;
        dest=obj.dest;
        sign=obj.sign;
        bit=obj.bit;
        offset=obj.offset;
        return *this;
    }
};

template < class T >
class queue {
  public:
    class iterator;

    queue () { head=tail=0; }

    const T operator[] ( const int idx ) const { return q[idx]; }
    T& operator[] ( const int idx ) { return q[idx]; }

    queue& operator= ( const queue& obj ) {
        if ( &obj==this ) return *this;
        for ( int i=0 ; i<SIZE ; i++ ) q[i]=obj[i];
        head=obj.head, tail=obj.tail;
        return *this;
    }
    queue& operator= ( const queue&& obj ) {
        if ( &obj==this ) return *this;
        for ( int i=0 ; i<SIZE ; i++ ) q[i]=obj[i];
        head=obj.head, tail=obj.tail;
        return *this;
    }

    bool full () const { return next(tail)==head; }
    int push ( const T& x ) {
        if ( full() ) throw "Push fail";
        q[tail=next(tail)]=x;
        return tail;
    }
    bool empty() const { return head==tail; }
    bool pop () { 
        if ( empty() ) return 0;
        head=next(head);
        return 1;
    }

    void clear () { 
        memset(q, 0, sizeof(q));
        head=tail=0;
    }

    T front () const { 
        if ( empty() ) throw "Queue is empty";
        return q[next(head)]; 
    }

    int next ( int x ) const { return (x+1)%SIZE; }
    const int begin () const { return next(head); }
    const int end () const { return next(tail); }

  private:
    static const int SIZE=32;
    T q[SIZE];
    int head, tail;
};

}

#endif