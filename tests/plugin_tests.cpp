#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Plugin.h"
#include <cassert>
#include <iostream>
struct PilotTestAccess {
    static void tick(Pilot& p){p.timerCallback();}
    static bool playing(Pilot& p){return p.running.load();}
    static double position(Pilot& p){return p.quarter.load();}
    static bool disconnected(Pilot& p){return p.outputs.empty();}
};
struct Transport:juce::AudioPlayHead {
    PositionInfo position;
    juce::Optional<PositionInfo> getPosition()const override{return position;}
};
static void setValue(Pilot& p,int lane,float value){
    auto* parameter=p.parameters.getParameter(Pilot::valueId(lane));parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
static juce::MemoryBlock binary(const juce::XmlElement& xml){juce::MemoryBlock b;juce::AudioProcessor::copyXmlToBinary(xml,b);return b;}
int main(){
    juce::ScopedJuceInitialiser_GUI init;
    Pilot p;
    assert(p.getParameters().size()==12);
    for(int i=0;i<pilot::lanes;++i){assert(p.manualValue(i)==64&&p.lanes[(size_t)i].cc==20+i);}
    setValue(p,0,95);PilotTestAccess::tick(p);assert(p.isManual(0)&&!p.isManual(1)&&p.manualValue(0)==95);
    p.useCurve(0,true);PilotTestAccess::tick(p);assert(!p.isManual(0));
    setValue(p,1,0); // Save even before timer consumes the takeover request.
    p.setEnabled(0,true);p.setEnabled(1,true);p.setAddress(0,2,74);p.setAddress(1,3,75);
    p.setPort(0,"missing-usb-a");p.setPort(1,"missing-usb-b");p.lanes[0].bars=2.25;p.lanes[0].minimum=.8f;p.lanes[0].maximum=.2f;p.lanes[0].drawing.y[23]=.123f;
    juce::MemoryBlock saved;p.getStateInformation(saved);
    Pilot restored;restored.setStateInformation(saved.getData(),(int)saved.getSize());PilotTestAccess::tick(restored);
    assert(!restored.armed&&PilotTestAccess::disconnected(restored));
    assert(restored.isManual(1)&&!restored.isManual(0)&&restored.manualValue(0)==95&&restored.manualValue(1)==0);
    assert(restored.lanes[0].enabled&&restored.lanes[0].cc==74&&restored.lanes[0].channel==2&&restored.lanes[0].port=="missing-usb-a");
    assert(restored.lanes[1].port=="missing-usb-b"&&restored.lanes[0].drawing.y[23]==.123f&&restored.lanes[0].bars==2.25);
    // Reopening the editor must not turn restored curve lanes into manual lanes.
    {std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());PilotTestAccess::tick(restored);assert(!restored.isManual(0)&&restored.isManual(1));}
    restored.arm(true);assert(!restored.armed); // Restore never reconnects drivers.
    juce::XmlElement legacy("CURVE_PILOT_MIDI_01");auto* lane=legacy.createNewChildElement("LANE");lane->setAttribute("on",true);lane->setAttribute("bars",8);
    auto old=binary(legacy);restored.setStateInformation(old.getData(),(int)old.getSize());PilotTestAccess::tick(restored);
    for(int i=0;i<pilot::lanes;++i){assert(restored.lanes[(size_t)i].cc==20+i&&restored.lanes[(size_t)i].channel==1&&!restored.isManual(i)&&restored.manualValue(i)==64);}
    assert(restored.lanes[0].enabled&&restored.lanes[0].bars==8&&!restored.lanes[1].enabled);
    lane->setAttribute("channel",999);lane->setAttribute("cc",-30);lane->setAttribute("bars",-1);lane->setAttribute("min",2.0);lane->setAttribute("max",-1.0);
    auto bad=binary(legacy);restored.setStateInformation(bad.getData(),(int)bad.getSize());
    assert(restored.lanes[0].channel==16&&restored.lanes[0].cc==0&&restored.lanes[0].bars==.25&&restored.lanes[0].minimum==1&&restored.lanes[0].maximum==0);
    const char invalid[]="not a preset";restored.setStateInformation(invalid,sizeof(invalid));assert(restored.lanes[0].channel==16);
    Transport transport;transport.position.setIsPlaying(true);transport.position.setPpqPosition(8.0);transport.position.setBpm(120.0);
    p.setPlayHead(&transport);juce::AudioBuffer<float> audio(2,64);audio.clear();audio.setSample(0,0,1);juce::MidiBuffer midi;
    p.processBlock(audio,midi);assert(PilotTestAccess::playing(p)&&PilotTestAccess::position(p)==8&&audio.getSample(0,0)==0);
    transport.position.setPpqPosition(0.0);p.processBlock(audio,midi);assert(PilotTestAccess::position(p)==0);
    transport.position=juce::AudioPlayHead::PositionInfo{};transport.position.setIsPlaying(true);
    p.processBlock(audio,midi);assert(!PilotTestAccess::playing(p)); // No stale PPQ reuse.
    p.releaseResources();assert(!PilotTestAccess::playing(p));p.setPlayHead(nullptr);
    // Read the generated .mid through JUCE's independent SMF reader as well.
    auto bytes=pilot::midiFile(p.lanes[0],7,8);juce::MemoryInputStream stream(bytes.data(),bytes.size(),false);juce::MidiFile file;
    assert(file.readFrom(stream)&&file.getNumTracks()==1&&file.getTimeFormat()==960);
    assert(file.getLastTimestamp()==7560.0);
    std::cout<<"PASS: APVTS takeover, independent lanes, editor reopen, current/legacy state, disarmed restore, input validation, transport jump/missing PPQ, JUCE MIDI reader\n";
}
