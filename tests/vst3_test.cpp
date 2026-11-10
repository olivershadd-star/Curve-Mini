#include <JuceHeader.h>
#include <iostream>
int main(int argc,char** argv){
    juce::ScopedJuceInitialiser_GUI init;
    if(argc!=2)return 2;
    juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile(descriptions,juce::String::fromUTF8(argv[1]));
    if(descriptions.size()!=1||!descriptions[0]->isInstrument)return 3;
    juce::String error;auto plugin=format.createInstanceFromDescription(*descriptions[0],48000,256,error);
    if(!plugin||!plugin->acceptsMidi()||plugin->getParameters().size()<96){std::cerr<<error<<std::endl;return 4;}
    {std::unique_ptr<juce::AudioProcessorEditor> ui(plugin->createEditorIfNeeded());if(!ui)return 5;ui->addToDesktop(juce::ComponentPeer::windowIsTemporary);ui->setVisible(true);juce::MessageManager::getInstance()->runDispatchLoopUntil(80);ui->removeFromDesktop();}
    plugin->setRateAndBufferSizeDetails(48000,256);plugin->prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);audio.clear();audio.setSample(0,0,1);juce::MidiBuffer midi;plugin->processBlock(audio,midi);
    if(audio.getMagnitude(0,256)!=0)return 6;
    juce::MemoryBlock state;plugin->getStateInformation(state);plugin->releaseResources();plugin.reset();
    auto reopened=format.createInstanceFromDescription(*descriptions[0],44100,128,error);if(!reopened||state.getSize()==0)return 7;reopened->setStateInformation(state.getData(),(int)state.getSize());
    std::cout<<"PASS: packaged VST3 loads, exposes MIDI/parameters, native editor opens, audio remains silent, saved state reopens\n";return 0;
}
