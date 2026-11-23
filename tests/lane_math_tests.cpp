#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../Source/LaneMath.h"
#include <cassert>
#include <iostream>
// Reference copies of the formulas as they were written, twice, in Plugin.cpp before LaneMath.h existed.
static float oldTarget(const curve::Data& c,double quarter,double beats,float mn,float mx,float bars,float offset,float depth){
    double phase=quarter/(beats*bars)+offset*.01;float v=.5f+depth*.01f*(c.at(phase)-.5f);
    return std::clamp((mn+(mx-mn)*v)*.01f,0.f,1.f);
}
int main(){
    auto c=curve::preset(0,4);
    // Identical to the previous inline formula over a grid of settings.
    for(float depth:{0.f,35.f,100.f})for(float mn:{0.f,10.f,40.f})for(float mx:{60.f,90.f,100.f})for(float off:{0.f,25.f,99.f})for(float bars:{.25f,1.f,4.f,32.f})
        for(double q=0;q<130;q+=.37){lanemath::Params p;p.min=mn;p.max=mx;p.depth=depth;p.offset=off;p.bars=bars;
            assert(lanemath::target(c,q,4.0,p)==oldTarget(c,q,4.0,mn,mx,bars,off,depth));}
    // Output always inside 0..1, even for inverted ranges or phases that wrap to exactly 1.0.
    {lanemath::Params p;p.min=80;p.max=20;for(double q:{-1e-18,0.0,1e9,-3.3}){float v=lanemath::target(c,q,3.5,p);assert(v>=0&&v<=1);}}
    // Smoothing: first call seeds, ms<=0 follows the target instantly, otherwise converges monotonically.
    {float f=0;bool valid=false;assert(lanemath::smooth(f,valid,.8f,.01,20,.2f)>.2f&&valid);
     float g=0;bool vg=false;assert(lanemath::smooth(g,vg,.7f,.01,0,.1f)==.7f);
     float h=0;bool vh=false;float prev=0;lanemath::smooth(h,vh,1.f,.01,200,0.f);prev=h;
     for(int i=0;i<500;++i){float now=lanemath::smooth(h,vh,1.f,.01,200,0.f);assert(now>=prev&&now<=1.f);prev=now;}
     assert(prev>.999f);}
    // CC conversion clamps and rounds.
    assert(lanemath::toCC(0)==0&&lanemath::toCC(1)==127&&lanemath::toCC(.5f)==64&&lanemath::toCC(-1)==0&&lanemath::toCC(2)==127);
    std::cout<<"PASS: shared lane maths identical to previous formula, bounded, smoothing, CC rounding\n";
}
