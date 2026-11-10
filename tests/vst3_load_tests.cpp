#include <JuceHeader.h>
#include <iostream>
#include <stdexcept>
using namespace juce;
static void require(bool ok,const String& error){if(!ok)throw std::runtime_error(error.toStdString());}
int main(int argc,char** argv){
    ScopedJuceInitialiser_GUI init;
    try{
        require(argc==2||argc==3,"Usage: vst3_load_tests <complete .vst3 bundle> [--legacy-diagnostic]");
        const bool legacy=argc==3&&String::fromUTF8(argv[2])=="--legacy-diagnostic";
        const File bundle=File::getCurrentWorkingDirectory().getChildFile(String::fromUTF8(argv[1]));
        require(bundle.exists(),"VST3 introuvable : "+bundle.getFullPathName());
        VST3PluginFormat format;OwnedArray<PluginDescription> types;
        format.findAllTypesForFile(types,bundle.getFullPathName());
        require(types.size()==1,"Le module VST3 ne fournit pas exactement un instrument chargeable");
        const auto& description=*types[0];
        std::cout<<"Factory loaded: "<<description.name<<", instrument="<<description.isInstrument<<", ID="<<description.createIdentifierString()<<std::endl;
        require(description.isInstrument,"Le VST3 doit etre declare comme instrument silencieux");
        String error;auto instance=format.createInstanceFromDescription(description,48000,512,error);
        require(instance!=nullptr,"Creation VST3 refusee : "+error);
        std::cout<<"MIDI input event bus: "<<instance->acceptsMidi()<<std::endl;
        if(!legacy)require(instance->acceptsMidi(),"Instrument VST3 sans bus MIDI entrant");
        require(instance->getParameters().size()>=12,"Parametres de controle absents du VST3");
        instance->setRateAndBufferSizeDetails(48000,512);instance->prepareToPlay(48000,512);
        for(int pass=0;pass<3;++pass){
            std::unique_ptr<AudioProcessorEditor> editor(instance->createEditorIfNeeded());
            require(editor!=nullptr,"Impossible d'ouvrir l'editeur VST3");
            require(editor->getWidth()>0&&editor->getHeight()>0,"Dimensions de l'editeur invalides");
            // Attach to a real native HWND, exercising the VST3 view's attach/
            // detach path rather than only constructing the processor's UI.
            editor->addToDesktop(ComponentPeer::windowIsTemporary);editor->setVisible(true);
            MessageManager::getInstance()->runDispatchLoopUntil(40);
            editor->setVisible(false);editor->removeFromDesktop();
        }
        AudioBuffer<float> buffer(jmax(2,instance->getTotalNumOutputChannels()),512);MidiBuffer midi;
        for(int i=0;i<4;++i){buffer.clear();buffer.setSample(0,0,1);midi.addEvent(MidiMessage::noteOn(1,60,(uint8)100),0);instance->processBlock(buffer,midi);require(buffer.getMagnitude(0,buffer.getNumSamples())==0,"Sortie non silencieuse");}
        MemoryBlock state;instance->getStateInformation(state);require(state.getSize()>0,"Etat VST3 vide");
        instance->releaseResources();instance.reset();
        auto reopened=format.createInstanceFromDescription(description,44100,128,error);
        require(reopened!=nullptr,"Recreation VST3 refusee : "+error);reopened->setStateInformation(state.getData(),(int)state.getSize());
        reopened->setRateAndBufferSizeDetails(44100,128);reopened->prepareToPlay(44100,128);reopened->releaseResources();reopened.reset();
        std::cout<<"PASS: real packaged VST3 factory, instrument instance, 12 parameters, native editor attach/detach, process, saved-state reload and destruction"<<std::endl;
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<std::endl;return 1;}
}
