#ifndef PREDICTOR_HPP
#define PREDICTOR_HPP

#include <cstring>

namespace cay {

class Predictor {
  public:
    Predictor () { memset(pre, 0, sizeof(pre)); }
    bool predict ( int pc ) {
        return pre[pc/BLK]>>1;
    }

    void hit ( int pc ) {
        if ( pre[pc/BLK]!=0b11 ) ++pre[pc/BLK];
    }
    void fail ( int pc ) {
        if ( pre[pc/BLK]!=0b00 ) --pre[pc/BLK];
    }
  private:
    static const int SIZE=(1<<12), BLK=(1<<12);
    char pre[SIZE];
};

}

#endif