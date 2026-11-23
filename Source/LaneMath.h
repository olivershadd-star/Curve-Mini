#pragma once
// Pure lane maths shared by processBlock (host MIDI), the sender thread (hardware / loopMIDI ports)
// and the MIDI export. No JUCE dependency, so it is unit-tested without building the plugin.
#include "Curve.h"
#include <algorithm>
#include <cmath>
namespace lanemath {
// Same units as the host parameters: min/max/depth/offset in percent, bars in measures, smooth in ms, value 0..127.
struct Params {float min=0,max=100,bars=4,offset=0,smooth=20,depth=100,value=64;};
// Curve value (0..1) -> lane output (0..1): depth around the middle, then min/max range.
inline float shape(float curveValue,const Params& p){
    const float v=.5f+p.depth*.01f*(curveValue-.5f);
    return std::clamp((p.min+(p.max-p.min)*v)*.01f,0.f,1.f);
}
// Output (0..1) for a transport position given in quarter notes.
inline float target(const curve::Data& c,double quarter,double beatsPerBar,const Params& p){
    return shape(c.at(quarter/(beatsPerBar*p.bars)+p.offset*.01),p);
}
// One-pole smoothing. `initial` seeds the filter the first time (or after `valid` was reset).
inline float smooth(float& filtered,bool& valid,float wanted,double dtSeconds,float ms,float initial){
    if(!valid){filtered=initial;valid=true;}
    const float alpha=ms<=0?1.f:(float)(1-std::exp(-dtSeconds/(ms*.001)));
    filtered+=alpha*(wanted-filtered);
    return filtered;
}
inline int toCC(float v){return std::clamp((int)std::lround(v*127.f),0,127);}
}
