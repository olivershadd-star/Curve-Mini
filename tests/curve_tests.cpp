#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../Source/Curve.h"
#include <cassert>
#include <iostream>
#include <limits>
int main(){
 auto d=curve::parse("duration: 8 bars\nstart: 20%\nramp to 70% over 2 bars smooth\nwave around 70% depth 10% cycles 2 over 2 bars sine\nramp to 15% over 1 bars linear\nsaw from 15% to 60% cycles 3 over 2 bars linear\nramp to 20% over 1 bars out\n");
 assert(d.bars==8);assert(std::abs(d.y[0]-.2)<1e-6);assert(std::abs(d.y[512]-.7)<1e-6);assert(std::abs(d.y.back()-.2)<1e-6);
 for(float v:d.y)assert(std::isfinite(v)&&v>=0&&v<=1);
 assert(std::abs(d.at(-.25)-d.at(.75))<1e-6);assert(d.at(0)==d.at(1));
 // Phases that wrap to exactly 1.0 or are not finite must stay inside the table.
 {const double edge[]={-1e-18,-1e-300,1-1e-17,1.0,-1.0,1e300,-1e300,std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
  for(double ph:edge){float v=d.at(ph);assert(std::isfinite(v)&&v>=0&&v<=1);}
  assert(std::abs(d.at(-1e-18)-d.y[curve::resolution])<1e-6);}
 auto a=curve::vary(d,.1f,123), b=curve::vary(d,.1f,123);assert(a.y==b.y);
 for(int s=0;s<6;++s){auto p=curve::preset(s,4);for(float v:p.y)assert(v>=0&&v<=1);}
 for(auto bad:{"", "duration: 4 bars\nstart: 10%\nramp to 70% over 2 bars smooth", "duration: nan bars", "start: 101%", "ramp to 20% over 4 bars nonsense", "import os", "wave around 70% depth 10% cycles 2 over 4 bars sine", "duration: 4 bars\nstart: 90%\nwave around 90% depth 30% cycles 2 over 4 bars sine", "duration: 4 bars\nhold over -1 bars"}){
  bool failed=false;try{curve::parse(bad);}catch(...){failed=true;}assert(failed);
 }

 // Point editing: ordering, duplicates, endpoint insertion and linear interpolation.
 auto points=curve::canonicalPoints({{1,.2f},{.5,.9f},{0,.2f},{.5,.8f}});
 assert(points.size()==3);assert(points[1].y==.8f);
 auto pointed=curve::fromPoints(points,4);
 assert(std::abs(pointed.y[1024]-.8f)<1e-6);
 assert(std::abs(pointed.y[512]-.5f)<1e-6);
 auto single=curve::fromPoints({{.3,.7f}},2);
 for(float v:single.y)assert(std::abs(v-.7f)<1e-6);
 auto empty=curve::fromPoints({},4);assert(empty.y[500]==.5f);
 auto simplified=curve::simplify(d,.006);
 auto rebuilt=curve::fromPoints(simplified,d.bars);
 for(int n=0;n<=curve::resolution;++n)assert(std::abs(rebuilt.y[(size_t)n]-d.y[(size_t)n])<=.00601);
 curve::Data jitter;for(int n=0;n<=curve::resolution;++n)jitter.y[(size_t)n]=.5f+(n%2?.1f:-.1f);
 auto polished=curve::smoothDrawing(jitter,1);
 assert(polished.y.front()==jitter.y.front());assert(polished.y.back()==jitter.y.back());
 double variationBefore=0,variationAfter=0;
 for(int n=1;n<curve::resolution;++n){variationBefore+=std::abs(jitter.y[(size_t)n]-jitter.y[(size_t)n-1]);variationAfter+=std::abs(polished.y[(size_t)n]-polished.y[(size_t)n-1]);assert(polished.y[(size_t)n]>=0&&polished.y[(size_t)n]<=1);}
 assert(variationAfter<variationBefore*.1);
 assert(curve::smoothDrawing(d,0).y==d.y);
 // Segment types and tension: bounded data and exact endpoints at extremes.
 for(int shape=0;shape<=6;++shape)for(double tension:{-1.0,0.0,1.0}){
   auto shaped=curve::fromPoints({{0,0,tension,shape},{1,1}},4);
   assert(shaped.y.front()==0&&shaped.y.back()==1);
   for(float v:shaped.y)assert(std::isfinite(v)&&v>=0&&v<=1);
 }
 auto slow=curve::fromPoints({{0,0,1,0},{1,1}},4),fast=curve::fromPoints({{0,0,-1,0},{1,1}},4);
 assert(slow.y[1024]<.01&&fast.y[1024]>.99);
 std::cout<<"PASS: original V2.1 curves, DSL, endpoints, bounds, repeatability, presets, invalid input, points, simplification and drawing smoothing\n";
}
