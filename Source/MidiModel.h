#pragma once
#include "Curve.h"
namespace pilot {
constexpr int lanes=12;
inline int controller(int lane){return 20+lane;}
inline int value(float curveValue,float minimum,float maximum){
    return (int)std::lround(127*std::clamp(minimum+(maximum-minimum)*curveValue,0.0f,1.0f));
}
inline double phase(double quarter,double beatsPerBar,double bars){
    return quarter/(std::max(.125,beatsPerBar)*std::max(.25,bars));
}
struct Lane {
    explicit Lane(int index=0):cc(controller(index)){}
    bool enabled=false,manual=false;
    int channel=1,cc;
    // Empty = use the instance's default port. Identifiers, not device names.
    std::string port;
    float minimum=0,maximum=1;
    double bars=4;
    curve::Data drawing=curve::preset(0,4);
    int last=-1;
};
inline std::string destination(const Lane& lane,const std::string& defaultPort){return lane.port.empty()?defaultPort:lane.port;}
inline bool conflict(const std::array<Lane,lanes>& lines,const std::string& defaultPort){
    for(int i=0;i<lanes;++i)for(int j=i+1;j<lanes;++j){
        const auto& a=lines[(size_t)i];const auto& b=lines[(size_t)j];
        if(a.enabled&&b.enabled&&a.channel==b.channel&&a.cc==b.cc&&destination(a,defaultPort)==destination(b,defaultPort))return true;
    }
    return false;
}
inline bool fresh(double ageMs){return std::isfinite(ageMs)&&ageMs>=0&&ageMs<250;}
}
