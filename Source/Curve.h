#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace curve {
constexpr int resolution=2048;
constexpr double pi=3.14159265358979323846;
struct Data {
    std::array<float,resolution+1> y{};
    double bars=4;
    float at(double phase) const {
        if(!std::isfinite(phase))return y[0];
        phase-=std::floor(phase);double p=phase*resolution;int n=(int)p;
        // phase-floor(phase) can round to exactly 1.0 for tiny negative phases: keep n+1 inside the table.
        if(n>resolution-1)n=resolution-1;
        return y[(size_t)n]+(float)(p-n)*(y[(size_t)n+1]-y[(size_t)n]);
    }
};
inline double clamp(double x){return std::clamp(x,0.0,1.0);}
inline std::string trim(std::string s){auto a=s.find_first_not_of(" \t\r");if(a==std::string::npos)return {};return s.substr(a,s.find_last_not_of(" \t\r")-a+1);}
inline double number(const std::string& s){size_t used=0;double n=std::stod(s,&used);if(used!=s.size()||!std::isfinite(n))throw std::runtime_error("Nombre invalide : "+s);return n;}
inline double percent(std::string s){if(!s.empty()&&s.back()=='%')s.pop_back();double n=number(s);if(n<0||n>100)throw std::runtime_error("Pourcentage hors de 0..100");return n/100;}
inline double ease(double t,const std::string& mode){if(mode=="linear")return t;if(mode=="smooth")return t*t*(3-2*t);if(mode=="in")return t*t;if(mode=="out")return 1-(1-t)*(1-t);throw std::runtime_error("Courbure attendue : linear, smooth, in, out");}
struct Segment {double from,to,duration,cycles=1,depth=0;std::string type,mode="linear";};
// Non-executable declarative grammar. All parsing happens off the audio thread.
inline Data parse(const std::string& text){
    if(text.size()>32768)throw std::runtime_error("Texte trop long (32768 caracteres maximum)");
    std::istringstream input(text);std::string line;std::vector<Segment> segments;
    double current=.2,bars=4,total=0;int lineNo=0;bool durationSeen=false,startSeen=false;
    while(std::getline(input,line)){
        ++lineNo;auto hash=line.find('#');if(hash!=std::string::npos)line.resize(hash);line=trim(line);if(line.empty())continue;
        try {
            std::istringstream words(line);std::vector<std::string> w;std::string word;while(words>>word)w.push_back(word);
            auto positive=[&](const std::string& s){double n=number(s);if(n<=0||n>128)throw std::runtime_error("Duree attendue : >0 et <=128 mesures");return n;};
            if(w[0]=="duration:"){
                if(durationSeen||!segments.empty()||w.size()!=3||w[2]!="bars")throw std::runtime_error("Syntaxe : duration: 8 bars (une seule fois, avant les segments)");
                bars=positive(w[1]);durationSeen=true;continue;
            }
            if(w[0]=="start:"){
                if(startSeen||!segments.empty()||w.size()!=2)throw std::runtime_error("Syntaxe : start: 20% (avant les segments)");
                current=percent(w[1]);startSeen=true;continue;
            }
            Segment s;s.from=current;
            if(w[0]=="ramp"){
                if(w.size()!=7||w[1]!="to"||w[3]!="over"||w[5]!="bars")throw std::runtime_error("Syntaxe : ramp to 70% over 2 bars smooth");
                s.type="ramp";s.to=percent(w[2]);s.duration=positive(w[4]);s.mode=w[6];ease(.5,s.mode);
            }else if(w[0]=="hold"){
                if(w.size()!=4||w[1]!="over"||w[3]!="bars")throw std::runtime_error("Syntaxe : hold over 1 bars");
                s.type="hold";s.to=current;s.duration=positive(w[2]);
            }else if(w[0]=="wave"){
                if(w.size()!=11||w[1]!="around"||w[3]!="depth"||w[5]!="cycles"||w[7]!="over"||w[9]!="bars"||w[10]!="sine")throw std::runtime_error("Syntaxe : wave around 70% depth 10% cycles 2 over 2 bars sine");
                s.type="wave";s.to=percent(w[2]);s.depth=percent(w[4]);s.cycles=number(w[6]);s.duration=positive(w[8]);
                if(std::abs(s.to-current)>1e-6)throw std::runtime_error("Faire une ramp jusqu'au centre avant wave");
                if(s.to-s.depth<0||s.to+s.depth>1)throw std::runtime_error("La vague depasse 0..100%");
                if(s.cycles<1||s.cycles>64||s.cycles!=std::floor(s.cycles))throw std::runtime_error("cycles : entier de 1 a 64");
            }else if(w[0]=="saw"){
                if(w.size()!=11||w[1]!="from"||w[3]!="to"||w[5]!="cycles"||w[7]!="over"||w[9]!="bars"||w[10]!="linear")throw std::runtime_error("Syntaxe : saw from 15% to 60% cycles 3 over 2 bars linear");
                s.type="saw";s.from=percent(w[2]);s.to=percent(w[4]);s.cycles=number(w[6]);s.duration=positive(w[8]);
                if(std::abs(s.from-current)>1e-6)throw std::runtime_error("Faire une ramp jusqu'au depart avant saw");
                if(s.cycles<1||s.cycles>64||s.cycles!=std::floor(s.cycles))throw std::runtime_error("cycles : entier de 1 a 64");
            }else throw std::runtime_error("Instruction inconnue");
            if(segments.size()>=128)throw std::runtime_error("128 segments maximum");
            total+=s.duration;current=s.type=="saw"?s.from:s.to;segments.push_back(s);
        }catch(const std::exception& e){throw std::runtime_error("Ligne "+std::to_string(lineNo)+" : "+e.what());}
    }
    if(segments.empty())throw std::runtime_error("Ajouter au moins un segment");
    if(std::abs(total-bars)>1e-6)throw std::runtime_error("Somme des segments = "+std::to_string(total)+" mesures ; duration = "+std::to_string(bars));
    Data out;out.bars=bars;size_t index=0;double begin=0;
    for(int n=0;n<=resolution;++n){
        double pos=bars*n/resolution;while(index+1<segments.size()&&pos>=begin+segments[index].duration){begin+=segments[index].duration;++index;}
        const auto& s=segments[index];double t=clamp((pos-begin)/s.duration),v=s.from;
        if(s.type=="ramp")v=s.from+(s.to-s.from)*ease(t,s.mode);
        else if(s.type=="wave")v=s.to+s.depth*std::sin(2*pi*s.cycles*t);
        else if(s.type=="saw"){double x=t*s.cycles;v=s.from+(s.to-s.from)*(x-std::floor(x));}
        out.y[(size_t)n]=(float)clamp(v);
    }
    return out;
}
inline Data preset(int shape,double bars){Data d;d.bars=bars;for(int n=0;n<=resolution;++n){double t=(double)n/resolution,v=.5;
    switch(shape){case 0:v=.5-.5*std::cos(2*pi*t);break;case 1:v=t;break;case 2:v=1-t;break;case 3:v=1-std::abs(2*t-1);break;case 4:v=t*t*(3-2*t);break;case 5:v=std::floor(t*8)/8;break;default:break;}d.y[(size_t)n]=(float)v;}return d;}
inline Data vary(Data d,float depth,uint32_t seed){uint32_t x=seed?seed:1;std::array<double,9> anchors{};for(auto& a:anchors){x^=x<<13;x^=x>>17;x^=x<<5;a=((double)x/4294967295.0*2-1)*depth;}anchors.back()=anchors.front();
    for(int n=0;n<=resolution;++n){double t=(double)n/resolution*8;int a=std::min(7,(int)t);double f=ease(t-a,"smooth");d.y[(size_t)n]=(float)clamp(d.y[(size_t)n]+anchors[(size_t)a]+f*(anchors[(size_t)a+1]-anchors[(size_t)a]));}return d;}
enum class SegmentShape { Curve, Smooth, Hold, Stairs, Wave, Triangle, Pulse };
struct Point {double x=0;float y=0;double tension=0;int shape=0;};
inline double segmentValue(double t, double tension, int shape){
    t=clamp(t); tension=std::clamp(tension,-1.0,1.0);
    const double exponent=std::pow(8.0,std::abs(tension));
    double u=tension>=0?std::pow(t,exponent):1-std::pow(1-t,exponent);
    switch(static_cast<SegmentShape>(shape)){
        case SegmentShape::Smooth: return u*u*(3-2*u);
        case SegmentShape::Hold: return t>=1?1:0;
        case SegmentShape::Stairs: return t>=1?1:std::floor(u*8)/8;
        case SegmentShape::Wave: return u+.12*std::sin(8*pi*u)*std::sin(pi*u);
        case SegmentShape::Triangle: return u+.12*(2/pi)*std::asin(std::sin(8*pi*u))*std::sin(pi*u);
        case SegmentShape::Pulse: return u+.12*(std::sin(8*pi*u)>=0?1:-1)*std::sin(pi*u);
        default: return u;
    }
}
inline std::vector<Point> canonicalPoints(std::vector<Point> points){
    for(auto& p:points){if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.tension))throw std::runtime_error("Point non fini");p.x=clamp(p.x);p.y=(float)clamp(p.y);p.tension=std::clamp(p.tension,-1.0,1.0);p.shape=std::clamp(p.shape,0,6);}
    std::stable_sort(points.begin(),points.end(),[](const auto& a,const auto& b){return a.x<b.x;});
    std::vector<Point> out;
    for(auto p:points){if(!out.empty()&&std::abs(out.back().x-p.x)<1e-6)out.back()=p;else out.push_back(p);}
    if(out.empty())out={{0,.5f},{1,.5f}};
    if(out.front().x>0)out.insert(out.begin(),{0,out.front().y});
    if(out.back().x<1)out.push_back({1,out.back().y});
    return out;
}
inline Data fromPoints(const std::vector<Point>& input,double bars){
    auto pts=canonicalPoints(input);Data d;d.bars=bars;size_t k=0;
    for(int n=0;n<=resolution;++n){double x=(double)n/resolution;while(k+2<pts.size()&&x>pts[k+1].x)++k;
        const auto& a=pts[k];const auto& b=pts[k+1];double f=segmentValue((x-a.x)/(b.x-a.x),a.tension,a.shape);d.y[(size_t)n]=(float)clamp(a.y+f*(b.y-a.y));}
    return d;
}
inline std::vector<Point> simplify(const Data& data,double tolerance){
    tolerance=std::clamp(tolerance,0.00001,1.0);std::array<bool,resolution+1> keep{};keep[0]=keep[resolution]=true;
    std::vector<std::pair<int,int>> pending{{0,resolution}};
    while(!pending.empty()){auto pair=pending.back();pending.pop_back();int left=pair.first,right=pair.second,index=-1;double error=tolerance;
        for(int n=left+1;n<right;++n){double estimate=data.y[(size_t)left]+(data.y[(size_t)right]-data.y[(size_t)left])*(double)(n-left)/(right-left);double e=std::abs(estimate-data.y[(size_t)n]);if(e>error){error=e;index=n;}}
        if(index>=0){keep[(size_t)index]=true;pending.push_back({left,index});pending.push_back({index,right});}
    }
    std::vector<Point> points;for(int n=0;n<=resolution;++n)if(keep[(size_t)n])points.push_back({(double)n/resolution,data.y[(size_t)n]});return points;
}
inline Data smoothDrawing(const Data& input,float strength){
    strength=(float)clamp(strength);if(strength==0)return input;
    int radius=1+(int)std::round(strength*40);Data out=input;
    for(int n=1;n<resolution;++n){double sum=0,weight=0;
        for(int j=-radius;j<=radius;++j){int index=std::clamp(n+j,0,resolution);double w=radius+1-std::abs(j);sum+=w*input.y[(size_t)index];weight+=w;}
        out.y[(size_t)n]=(float)clamp(input.y[(size_t)n]*(1-strength)+sum/weight*strength);}
    return out;
}

}
