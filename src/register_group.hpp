#ifndef REGISTER_GROUP_HPP
#define REGISTER_GROUP_HPP

namespace cay {

class RegisterGroup {
  public:
    int rely[32], next_rely[32];
    int pc, next_pc;
    
    RegisterGroup () {
        for ( int i=0 ; i<32 ; i++ ) { 
            rely[i]=next_rely[i]=-1;
            reg[i]=next_reg[i]=0;
        }
    }

    int& operator[] ( const int idx ) { return next_reg[idx]; }
    const int operator[] ( const int idx ) const { return next_reg[idx]; }

    int& at ( const int idx ) { return reg[idx]; }
    const int at ( const int idx ) const { return reg[idx]; }

    void update ( int dest ) {
        for ( int i=0 ; i<SIZE ; i++ ) 
            if ( rely[i]==dest && next_rely[i]==dest ) next_rely[i]=-1;
        return ;
    }
    void clear () { 
        for ( int i=0 ; i<SIZE ; i++ ) 
            rely[i]=next_rely[i]=-1;
        return ;
    }
    void flush () { 
        pc=next_pc;
        for ( int i=0 ; i<SIZE ; i++ ) {
            reg[i]=next_reg[i];
            rely[i]=next_rely[i];
        }
    }
  private:
    static const int SIZE=32;
    int reg[32];
    int next_reg[33]; 
};

}

#endif