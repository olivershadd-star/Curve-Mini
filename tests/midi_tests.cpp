#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../Source/MidiModel.h"
#include <cassert>
#include <iostream>
#include <limits>
int main(){
    for(int i=0;i<pilot::lanes;++i){assert(pilot::controller(i)==20+i);pilot::Lane l;assert(!l.enabled);}
    assert(pilot::value(0,0,1)==0&&pilot::value(1,0,1)==127);
    assert(pilot::value(0,1,0)==127&&pilot::value(1,1,0)==0);
    assert(pilot::value(.5f,.25f,.75f)==64);
    assert(pilot::value(4,0,1)==127&&pilot::value(-3,0,1)==0);
    assert(pilot::phase(8,4,4)==.5&&pilot::phase(6,3,4)==.5);
    assert(pilot::phase(1,.5,4)==.5); // 1/8 time signature
    assert(pilot::phase(16,4,4)==1&&pilot::phase(0,4,4)==0); // loop jump
    assert(!pilot::fresh(std::numeric_limits<double>::quiet_NaN()));
    assert(pilot::fresh(0)&&pilot::fresh(249)&&!pilot::fresh(250)&&!pilot::fresh(-1));
    for(int shape=0;shape<6;++shape){auto data=curve::preset(shape,4);for(int k=-100;k<=1000;++k){auto v=data.at(k/77.0);assert(std::isfinite(v)&&v>=0&&v<=1);assert(pilot::value(v,0,1)>=0&&pilot::value(v,0,1)<=127);}}
    std::cout<<"PASS: stable CC addresses, disabled defaults, range/reverse, MIDI quantization, musical phase, stale transport, curve bounds\n";
}
