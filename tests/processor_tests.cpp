#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Plugin.h"
#include <cassert>
#include <iostream>
int main(){
    juce::ScopedJuceInitialiser_GUI init;CurvePilot p;
    assert(p.acceptsMidi()&&!p.producesMidi()&&p.getParameters().size()==32&&CurvePilot::laneCount==4);
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
    {std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());editor->setSize(560,560);editor->setSize(720,820);auto click=[&](const juce::String& title){std::function<juce::Button*(juce::Component&)> find=[&](juce::Component& c)->juce::Button*{if(auto* b=dynamic_cast<juce::Button*>(&c))if(b->getButtonText()==title)return b;for(auto* child:c.getChildren())if(auto* b=find(*child))return b;return nullptr;};auto* b=find(*editor);assert(b&&b->onClick);b->onClick();};click("Decrire / Code");click("Courbe");{auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream out(juce::File::getCurrentWorkingDirectory().getChildFile("curve-mini-interface.png"));juce::PNGImageFormat png;assert(out.openedOk()&&png.writeImageToStream(snapshot,out));}assert(restored.isManual(1)&&!restored.isManual(0)&&restored.getLane(1).port=="usb-c");}
    juce::AudioBuffer<float> audio(2,128);audio.clear();audio.setSample(0,0,1);juce::MidiBuffer input;input.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)100),0);restored.processBlock(audio,input);assert(audio.getMagnitude(0,128)==0&&input.isEmpty());
    auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("curve-mini-test",".mid",false);
    assert(p.exportMidi(1,file));juce::FileInputStream stream(file);juce::MidiFile midi;assert(midi.readFrom(stream)&&midi.getNumTracks()==1&&midi.getTimeFormat()==960);
    assert(midi.getLastTimestamp()==p.get(1,"bars")*4*960);for(int i=0;i<midi.getTrack(0)->getNumEvents();++i){auto msg=midi.getTrack(0)->getEventPointer(i)->message;assert(msg.isController()||msg.isEndOfTrackMetaEvent());if(msg.isController())assert(msg.getChannel()==3&&msg.getControllerNumber()==75);}
    stream.setPosition(0);file.deleteFile();
    // V2.1 sessions preserve drawings/settings, never load their hosted slots.
    juce::ValueTree legacy("CURVEPILOT_SESSION"),parameters("PARAMETERS"),content("CONTENT"),lane("LANE"),slot("SLOT");
    parameters.addChild(p.state.copyState(),-1,nullptr);legacy.addChild(parameters,-1,nullptr);
    lane.setProperty("index",1,nullptr);lane.setProperty("name","Ancien filtre",nullptr);
    auto drawing=p.getLane(1).curve;juce::MemoryBlock blob(drawing.y.data(),drawing.y.size()*sizeof(float));lane.setProperty("curve",blob.toBase64Encoding(),nullptr);content.addChild(lane,-1,nullptr);slot.setProperty("description","invalid hosted plugin",nullptr);content.addChild(slot,-1,nullptr);legacy.addChild(content,-1,nullptr);
    juce::MemoryBlock old;juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),old);restored.setStateInformation(old.getData(),(int)old.getSize());
    assert(restored.getLane(1).curve.y==drawing.y&&restored.getLane(1).port.isEmpty()&&restored.get(1,"on")==0&&!restored.sending);
    auto state=restored.state.copyState();state.getChildWithProperty("id","lane2_bars").setProperty("value",-5.0,nullptr);parameters.removeAllChildren(nullptr);parameters.addChild(state,-1,nullptr);juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),old);restored.setStateInformation(old.getData(),(int)old.getSize());assert(restored.get(1,"bars")==.25f);
    std::cout<<"PASS: points, clipboard preserves destination, undo preserves new routing, locks, MIDI takeover, state, editor, silent audio and CC-only SMF export\n";
}
