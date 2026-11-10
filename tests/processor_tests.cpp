#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Plugin.h"
#include <cassert>
#include <iostream>
int main(){
    juce::ScopedJuceInitialiser_GUI init;CurvePilot p;
    assert(p.acceptsMidi()&&!p.producesMidi()&&p.getParameters().size()==96);
    auto original=p.getLane(0).curve;
    p.replacePoints(0,{{0,0},{.5,1},{1,0}});assert(p.getLane(0).pointMode&&p.getLane(0).curve.y[1024]==1);
    p.copyCurve(0);p.configureDestination(1,true,"usb-b",2,74,"Filtre");p.pasteCurve(1);
    assert(p.getLane(1).curve.y==p.getLane(0).curve.y&&p.getLane(1).port=="usb-b"&&p.getLane(1).cc==74);
    p.replaceCurve(1,curve::preset(1,2));p.configureDestination(1,true,"usb-c",3,75,"Resonance");p.undoCurve(1);
    assert(p.getLane(1).curve.y==p.getLane(0).curve.y&&p.getLane(1).port=="usb-c"&&p.getLane(1).cc==75);
    p.toggleLock(1);auto locked=p.getLane(1).curve;p.replaceCurve(1,original);assert(p.getLane(1).curve.y==locked.y);p.toggleLock(1);
    p.set(1,"value",90);assert(p.isManual(1)&&!p.isManual(0));p.resumeCurve(1);assert(!p.isManual(1));
    p.set(1,"value",91);p.set(1,"on",1);juce::MemoryBlock saved;p.getStateInformation(saved);
    CurvePilot restored;restored.setStateInformation(saved.getData(),(int)saved.getSize());assert(!restored.sending&&restored.isManual(1)&&restored.get(1,"value")==91);
    assert(restored.getLane(1).port=="usb-c"&&restored.getLane(1).curve.y==p.getLane(1).curve.y&&restored.getLane(1).cc==75);
    restored.arm(true);assert(!restored.sending);
    // An editor must not change modes or destinations by creating attachments.
    {std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());editor->setSize(900,620);editor->setSize(1380,880);{auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream out(juce::File::getCurrentWorkingDirectory().getChildFile("curve-mini-interface.png"));juce::PNGImageFormat png;assert(out.openedOk()&&png.writeImageToStream(snapshot,out));}assert(restored.isManual(1)&&!restored.isManual(0)&&restored.getLane(1).port=="usb-c");}
    juce::AudioBuffer<float> audio(2,128);audio.clear();audio.setSample(0,0,1);juce::MidiBuffer input;input.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),0);restored.processBlock(audio,input);assert(audio.getMagnitude(0,128)==0&&input.isEmpty());
    auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("curve-mini-test",".mid",false);
    assert(p.exportMidi(1,file));juce::FileInputStream stream(file);juce::MidiFile midi;assert(midi.readFrom(stream)&&midi.getNumTracks()==1&&midi.getTimeFormat()==960);
    assert(midi.getLastTimestamp()==p.get(1,"bars")*4*960);for(int i=0;i<midi.getTrack(0)->getNumEvents();++i){auto msg=midi.getTrack(0)->getEventPointer(i)->message;assert(msg.isController()||msg.isEndOfTrackMetaEvent());if(msg.isController())assert(msg.getChannel()==3&&msg.getControllerNumber()==75);}
    stream.setPosition(0);file.deleteFile();
    std::cout<<"PASS: points, clipboard preserves destination, undo preserves new routing, locks, MIDI takeover, state, editor, silent audio and CC-only SMF export\n";
}
