#ifndef PREDICTOR_HPP
#define PREDICTOR_HPP

#include <cstring>

namespace cay {

class Predictor {
  public:
    Predictor () { memset(pre, 0, sizeof(pre)); }
    bool predict ( int pc ) {
        // return 0;
        return pre[pc%SIZE]>>1;
    }

    void hit ( int pc ) {
        if ( pre[pc%SIZE]!=0b11 ) ++pre[pc%SIZE];
    }
    void fail ( int pc ) {
        if ( pre[pc%SIZE]!=0b00 ) --pre[pc%SIZE];
    }
  private:
    static const int SIZE=1024;
    char pre[SIZE];
};

}

#endif