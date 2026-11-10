#pragma once
#include <JuceHeader.h>
#include <map>
#include "MidiExport.h"
class Pilot final:public juce::AudioProcessor,private juce::Timer,private juce::AudioProcessorValueTreeState::Listener {
public:
    Pilot();~Pilot()override;
    const juce::String getName()const override{return "Curve Mini Prototype 0.2";}
    void prepareToPlay(double,int)override{}
    void releaseResources()override{running=false;}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
    bool isBusesLayoutSupported(const BusesLayout& b)const override{return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
    // Physical controllers are mapped by Live to our host parameters (Remote).
    // No raw MIDI driver or parameter locks in processBlock.
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
    void setDefaultPort(const juce::String& id);
    void learn(int lane);
    void arm(bool);
    void setEnabled(int lane,bool enabled);
    void setAddress(int lane,int channel,int cc);
    void setPort(int lane,const juce::String& id);
    void useCurve(int lane,bool curve);
    bool isManual(int lane)const;
    int manualValue(int lane)const;
    bool exportLane(int lane,const juce::File& file);
    static juce::String valueId(int lane){return "lane"+juce::String(lane+1)+"_value";}
    juce::AudioProcessorValueTreeState parameters;
    juce::CriticalSection control;
    std::array<pilot::Lane,pilot::lanes> lanes;
    bool armed=false;
    double standaloneTempo=120;
    juce::String selectedPort,status="Choisir une sortie MIDI, puis Connecter.";
    double displayQuarter=0,displayBeats=4;
private:
    friend struct PilotTestAccess;
    static juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout();
    std::array<juce::String,pilot::lanes> valueIds;
    std::array<std::atomic<bool>,pilot::lanes> manualRequested{};
    std::array<std::atomic<float>*,pilot::lanes> values{};
    std::map<std::string,std::unique_ptr<juce::MidiOutput>> outputs;
    juce::MidiOutput* outputFor(int lane);
    void resetLast();
    void parameterChanged(const juce::String&,float)override;
    std::atomic<unsigned> sequence{0};
    std::atomic<double> quarter{0},tempo{120},beats{4},stamp{0};
    std::atomic<int> timeNumerator{4},timeDenominator{4};
    std::atomic<bool> running{false};
    double localQuarter=0,lastTick=0;
    void timerCallback()override;
};
