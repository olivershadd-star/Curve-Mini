#pragma once
#include "Curve.h"
namespace pilot {
constexpr int lanes=12;
// CC20..31 on channel 1: one stable address per lane, independent of target synth.
inline int controller(int lane){return 20+lane;}
inline int value(float curveValue,float minimum,float maximum){
    return (int)std::lround(127*std::clamp(minimum+(maximum-minimum)*curveValue,0.0f,1.0f));
}
inline double phase(double quarter,double beatsPerBar,double bars){
    return quarter/(std::max(.125,beatsPerBar)*std::max(.25,bars));
}
struct Lane {
    bool enabled=false;
    float minimum=0,maximum=1;
    double bars=4;
    curve::Data drawing=curve::preset(0,4);
    int last=-1;
};
inline bool fresh(double ageMs){return std::isfinite(ageMs)&&ageMs>=0&&ageMs<250;}
}
