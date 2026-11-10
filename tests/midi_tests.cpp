#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../Source/MidiExport.h"
#include <cassert>
#include <iostream>
#include <limits>
struct Event {int tick,status,cc,value;};
struct Parsed {std::vector<Event> events;int end=0,numerator=0,denominator=0;};
// Independent minimal SMF reader: reject notes, other controllers, and malformed
// lengths rather than only comparing our export to a second copy of its writer.
Parsed readMidi(const std::vector<uint8_t>& bytes){
    assert(bytes.size()>=22);assert(std::string(bytes.begin(),bytes.begin()+4)=="MThd");
    assert(bytes[7]==6&&bytes[9]==0&&bytes[11]==1&&bytes[12]==3&&bytes[13]==0xc0);
    assert(std::string(bytes.begin()+14,bytes.begin()+18)=="MTrk");
    uint32_t length=0;for(int i=18;i<22;++i)length=(length<<8)|bytes[(size_t)i];assert(length+22==bytes.size());
    size_t pos=22;int tick=0;Parsed result;bool ended=false;
    auto variable=[&]{int n=0;for(int i=0;i<4;++i){assert(pos<bytes.size());auto v=bytes[pos++];n=n*128+(v&127);if(!(v&128))return n;}assert(false);return 0;};
    while(pos<bytes.size()){
        tick+=variable();assert(pos<bytes.size());const int status=bytes[pos++];
        if(status==0xff){
            assert(pos<bytes.size());int type=bytes[pos++],size=variable();assert(pos+(size_t)size<=bytes.size());
            if(type==0x58){assert(size==4);result.numerator=bytes[pos];result.denominator=1<<bytes[pos+1];}
            else{assert(type==0x2f&&size==0&&pos==bytes.size());result.end=tick;ended=true;}
            pos+=(size_t)size;
        }else{assert((status&0xf0)==0xb0&&pos+2<=bytes.size());const int cc=bytes[pos++],value=bytes[pos++];assert(cc<128&&value<128);result.events.push_back({tick,status,cc,value});}
    }
    assert(ended&&!result.events.empty());return result;
}
int main(){
    for(int i=0;i<pilot::lanes;++i){pilot::Lane lane(i);assert(lane.cc==20+i&&lane.channel==1&&!lane.enabled&&!lane.manual&&lane.port.empty());}
    assert(pilot::value(0,0,1)==0&&pilot::value(1,0,1)==127);
    assert(pilot::value(0,1,0)==127&&pilot::value(1,1,0)==0);
    assert(pilot::value(.5f,.25f,.75f)==64);
    assert(pilot::value(4,0,1)==127&&pilot::value(-3,0,1)==0);
    assert(pilot::phase(8,4,4)==.5&&pilot::phase(6,3,4)==.5&&pilot::phase(1,.5,4)==.5);
    assert(pilot::phase(16,4,4)==1&&pilot::phase(0,4,4)==0);
    assert(!pilot::fresh(std::numeric_limits<double>::quiet_NaN()));
    assert(pilot::fresh(0)&&pilot::fresh(249)&&!pilot::fresh(250)&&!pilot::fresh(-1));
    std::array<pilot::Lane,pilot::lanes> lanes;
    lanes[0].enabled=lanes[1].enabled=true;assert(pilot::conflict(lanes,"usb"));
    lanes[1].channel=2;assert(!pilot::conflict(lanes,"usb"));
    lanes[1].channel=1;lanes[1].cc=71;assert(!pilot::conflict(lanes,"usb"));
    lanes[1].cc=20;lanes[1].port="other-usb";assert(!pilot::conflict(lanes,"usb"));
    lanes[1].port="usb";assert(pilot::conflict(lanes,"usb"));
    lanes[1].enabled=false;assert(!pilot::conflict(lanes,"usb"));
    for(int shape=0;shape<6;++shape){
        pilot::Lane lane;lane.channel=2;lane.cc=74;lane.bars=2.25;lane.minimum=.8f;lane.maximum=.2f;lane.drawing=curve::preset(shape,4);
        for(int k=-100;k<=1000;++k){auto v=lane.drawing.at(k/77.0);assert(std::isfinite(v)&&v>=0&&v<=1);}
        auto parsed=readMidi(pilot::midiFile(lane,7,8));assert(parsed.end==7560&&parsed.numerator==7&&parsed.denominator==8);
        assert(parsed.events.front().tick==0);int last=-1,previous=-1;
        for(const auto& event:parsed.events){assert(event.status==0xb1&&event.cc==74&&event.tick>previous&&event.tick<parsed.end&&event.value!=last);
            assert(event.value==pilot::value(lane.drawing.at((double)event.tick/parsed.end),.8f,.2f));last=event.value;previous=event.tick;}
    }
    pilot::Lane constant;constant.channel=16;constant.cc=127;constant.bars=32;constant.drawing.y.fill(.5f);
    auto parsed=readMidi(pilot::midiFile(constant));assert(parsed.events.size()==1&&parsed.end==122880&&parsed.events[0].value==64&&parsed.events[0].status==0xbf);
    constant.bars=.25;assert(readMidi(pilot::midiFile(constant,1,8)).end==120);
    constant.cc=0;assert(readMidi(pilot::midiFile(constant)).events[0].cc==0);
    auto rejects=[&](auto action){bool threw=false;try{action();}catch(const std::invalid_argument&){threw=true;}assert(threw);};
    constant.channel=0;rejects([&]{pilot::midiFile(constant);});constant.channel=1;constant.bars=std::numeric_limits<double>::infinity();rejects([&]{pilot::midiFile(constant);});
    constant.bars=4;rejects([&]{pilot::midiFile(constant,4,3);});
    std::cout<<"PASS: MIDI values, legacy addresses, transport, per-port/channel/CC conflicts, six exported curves, exact loop length, SMF structure and bounds\n";
}
