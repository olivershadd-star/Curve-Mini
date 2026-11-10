#pragma once
#include <JuceHeader.h>
#include "Curve.h"
#include <map>
#include <optional>
class CurvePilot final:public juce::AudioProcessor,private juce::Timer,private juce::AudioProcessorValueTreeState::Listener {
public:
    static constexpr int laneCount=12;
    struct Lane {
        curve::Data curve=curve::preset(0,4);
        juce::String name,script,motionText,port;
        int channel=16,cc=20;
        bool locked=false,pointMode=false,hardware=false,manual=false;
        std::vector<curve::Point> points;
        int last=-1;float filtered=.5f;bool filterValid=false;
    };
    CurvePilot();~CurvePilot()override;
    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    mutable juce::CriticalSection mutex;
    std::array<Lane,laneCount> lanes;
    std::atomic<double> visualBars{0};
    std::atomic<bool> hostPlaying{false};
    std::atomic<int> revision{0};
    juce::String status="Choisir une destination, connecter, puis activer l'envoi.";
    int editorWidth=1380,editorHeight=880,selectedLaneUI=0;
    bool sending=false;
    std::atomic<bool> standalonePlaying{false},transportReset{false};
    std::atomic<double> standaloneBpm{120};
    Lane getLane(int);
    void replaceCurve(int,const curve::Data&,const juce::String& script={});
    void replacePoints(int,std::vector<curve::Point>);
    void setPointMode(int,bool);void smoothCurve(int,float);
    void copyCurve(int);void pasteCurve(int);bool hasClipboard()const;
    void toggleLock(int);void applyVariation(int,float);void undoCurve(int);
    float get(int,const juce::String&)const;void set(int,const juce::String&,float);
    bool applyScript(int,const juce::String&,juce::String&);
    void configureDestination(int,bool,const juce::String&,int,int,const juce::String&);
    void connectOutputs();void arm(bool);void assign(int);void resumeCurve(int);
    bool isManual(int)const;
    bool exportMidi(int,const juce::File&);
    void exportSession(const juce::File&);bool importSession(const juce::File&);
    const juce::String getName()const override{return "Curve Mini";}
    bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}
    double getTailLengthSeconds()const override{return 0;}
    bool isBusesLayoutSupported(const BusesLayout&)const override;
    void prepareToPlay(double,int)override{}void releaseResources()override{hostPlaying=false;}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
    bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
    int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}
    const juce::String getProgramName(int)override{return {}; }void changeProgramName(int,const juce::String&)override{}
    void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
private:
    std::array<std::unique_ptr<Lane>,laneCount> previous;
    std::optional<Lane> clipboard;
    std::array<std::atomic<bool>,laneCount> manualRequests{};
    std::array<juce::String,laneCount> valueIds;
    std::map<juce::String,std::unique_ptr<juce::MidiOutput>> outputs;
    std::atomic<unsigned> transportSequence{0};
    std::atomic<double> hostQuarter{0},hostTempo{120},hostBeats{4},hostStamp{0};
    double localQuarter=0,lastTick=0;
    void timerCallback()override;void parameterChanged(const juce::String&,float)override;
    bool routesValid();void resetOutputValues();
};
