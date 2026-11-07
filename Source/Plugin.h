#pragma once
#include <JuceHeader.h>
#include "MidiModel.h"
class Pilot final:public juce::AudioProcessor,private juce::Timer {
public:
    Pilot();~Pilot()override;
    const juce::String getName()const override{return "Curve Mini Prototype 0.1";}
    void prepareToPlay(double,int)override{}
    void releaseResources()override{running=false;}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
    bool isBusesLayoutSupported(const BusesLayout& b)const override{return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
    bool acceptsMidi()const override{return false;}
    bool producesMidi()const override{return false;}
    double getTailLengthSeconds()const override{return 0;}
    bool hasEditor()const override{return true;}
    juce::AudioProcessorEditor* createEditor()override;
    int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}
    void setCurrentProgram(int)override{}
    const juce::String getProgramName(int)override{return {};}
    void changeProgramName(int,const juce::String&)override{}
    void getStateInformation(juce::MemoryBlock&)override;
    void setStateInformation(const void*,int)override;
    void connect(const juce::String& id);
    void learn(int lane);
    void arm(bool);
    juce::CriticalSection control;
    std::array<pilot::Lane,pilot::lanes> lanes;
    bool armed=false;
    double standaloneTempo=120;
    juce::String selectedPort,status="Choisir un port MIDI virtuel, puis Connecter.";
    double displayQuarter=0,displayBeats=4;
private:
    std::unique_ptr<juce::MidiOutput> output;
    std::atomic<unsigned> sequence{0};
    std::atomic<double> quarter{0},tempo{120},beats{4},stamp{0};
    std::atomic<bool> running{false};
    double localQuarter=0,lastTick=0;
    void timerCallback()override;
};
