#pragma once
#include "MidiModel.h"
namespace pilot {
// SMF0, one controller lane, no notes or tempo changes. Musical sampling at
// 1/96 quarter note, deduplicated after 7-bit quantisation. End-of-track marks
// the exact loop length, including a constant curve's otherwise silent tail.
inline std::vector<uint8_t> midiFile(const Lane& lane,int numerator=4,int denominator=4){
    if(lane.channel<1||lane.channel>16||lane.cc<0||lane.cc>127||!std::isfinite(lane.bars)||lane.bars<.25||lane.bars>32
       ||numerator<1||numerator>64||denominator<1||denominator>64||(denominator&(denominator-1)))
        throw std::invalid_argument("Adresse, duree ou signature MIDI invalide");
    constexpr int ppq=960;
    const int end=(int)std::lround(lane.bars*numerator*4.0/denominator*ppq);
    std::vector<uint8_t> track;
    auto variable=[&](uint32_t n){uint8_t bytes[4];int count=0;bytes[count++]=(uint8_t)(n&127);while((n>>=7)!=0)bytes[count++]=(uint8_t)((n&127)|128);while(count)track.push_back(bytes[--count]);};
    int exponent=0;for(int d=denominator;d>1;d>>=1)++exponent;
    track={0,0xff,0x58,4,(uint8_t)numerator,(uint8_t)exponent,24,8};
    int last=-1,lastTick=0;
    auto sample=[&](int tick){
        const int v=value(lane.drawing.at((double)tick/end),lane.minimum,lane.maximum);
        if(v==last)return;
        variable((uint32_t)(tick-lastTick));track.push_back((uint8_t)(0xb0+lane.channel-1));
        track.push_back((uint8_t)lane.cc);track.push_back((uint8_t)v);last=v;lastTick=tick;
    };
    for(int tick=0;tick<end;tick+=10)sample(tick);
    sample(end-1);variable((uint32_t)(end-lastTick));track.insert(track.end(),{0xff,0x2f,0});
    std::vector<uint8_t> file={'M','T','h','d',0,0,0,6,0,0,0,1,3,0xc0,'M','T','r','k'};
    for(int shift=24;shift>=0;shift-=8)file.push_back((uint8_t)((uint32_t)track.size()>>shift));
    file.insert(file.end(),track.begin(),track.end());return file;
}
}
