#include "Plugin.h"
using namespace juce;
Pilot::Pilot():AudioProcessor(BusesProperties().withOutput("Silence - controle MIDI externe",AudioChannelSet::stereo(),true)){
    startTimer(10);
}
Pilot::~Pilot(){stopTimer();}
void Pilot::processBlock(AudioBuffer<float>& audio,MidiBuffer& midi){
    // No hosted plugins, no audio processing, no MIDI driver calls or UI locks.
    audio.clear();midi.clear();
    sequence.fetch_add(1,std::memory_order_acq_rel);
    bool play=false;
    if(auto* head=getPlayHead())if(auto p=head->getPosition()){
        bool hasPosition=false;
        play=p->getIsPlaying();
        if(auto v=p->getPpqPosition())if(std::isfinite(*v)){quarter.store(*v);hasPosition=true;}
        if(auto v=p->getBpm())if(std::isfinite(*v)&&*v>0)tempo.store(*v);
        if(auto v=p->getTimeSignature())if(v->denominator>0&&v->numerator>0)beats.store(v->numerator*4.0/v->denominator);
        play=play&&hasPosition;
    }
    running.store(play);stamp.store(Time::getMillisecondCounterHiRes());
    sequence.fetch_add(1,std::memory_order_release);
}
void Pilot::connect(const String& id){
    const ScopedLock lock(control);armed=false;output.reset();selectedPort=id;
    if(id.isNotEmpty())output=MidiOutput::openDevice(id);
    for(auto& lane:lanes)lane.last=-1;
    status=output?"Port connecte. Assigner les macros, puis activer ENVOI.":"Port indisponible. Ouvrir loopMIDI et actualiser la liste.";
}
void Pilot::arm(bool on){const ScopedLock lock(control);armed=on&&output!=nullptr;for(auto& l:lanes)l.last=-1;localQuarter=0;lastTick=Time::getMillisecondCounterHiRes();status=armed?"ENVOI actif - lecture Ableton requise (ou horloge autonome).":"ENVOI arrete.";}
void Pilot::learn(int index){
    const ScopedLock lock(control);armed=false;
    if(index<0||index>=pilot::lanes){status="Ligne MIDI invalide.";return;}
    if(!output){status="Connecter le port MIDI avant l'assignation.";return;}
    output->sendMessageNow(MidiMessage::controllerEvent(1,pilot::controller(index),64));
    status="CC "+String(pilot::controller(index))+" envoye seul. Sortir du mode MIDI d'Ableton, puis reactiver ENVOI.";
}
void Pilot::timerCallback(){
    const double now=Time::getMillisecondCounterHiRes();
    const ScopedLock lock(control);
    const double dt=lastTick>0?jlimit(0.0,.25,(now-lastTick)/1000):0;lastTick=now;
    if(!armed||!output)return;
    double q=0,b=4;
    if(JUCEApplicationBase::isStandaloneApp()){
        localQuarter+=dt*standaloneTempo/60;q=localQuarter;
    }else{
        const auto before=sequence.load(std::memory_order_acquire);
        if(before&1u)return;
        const bool play=running.load();const double time=stamp.load(),t=tempo.load();q=quarter.load();b=beats.load();
        if(sequence.load(std::memory_order_acquire)!=before||!play||!pilot::fresh(now-time))return;
        q+=(now-time)*t/60000;
    }
    displayQuarter=q;displayBeats=b;
    for(int i=0;i<pilot::lanes;++i){auto& l=lanes[(size_t)i];if(!l.enabled){l.last=-1;continue;}
        const float v=l.drawing.at(pilot::phase(q,b,l.bars));const int cc=pilot::value(v,l.minimum,l.maximum);
        if(cc!=l.last){output->sendMessageNow(MidiMessage::controllerEvent(1,pilot::controller(i),cc));l.last=cc;}
    }
}
void Pilot::getStateInformation(MemoryBlock& out){
    const ScopedLock lock(control);XmlElement root("CURVE_PILOT_MIDI_01");root.setAttribute("port",selectedPort);root.setAttribute("tempo",standaloneTempo);
    for(const auto& l:lanes){auto* x=root.createNewChildElement("LANE");x->setAttribute("on",l.enabled);x->setAttribute("min",(double)l.minimum);x->setAttribute("max",(double)l.maximum);x->setAttribute("bars",l.bars);MemoryBlock curveData(l.drawing.y.data(),l.drawing.y.size()*sizeof(float));x->setAttribute("curve",curveData.toBase64Encoding());}
    copyXmlToBinary(root,out);
}
void Pilot::setStateInformation(const void* data,int size){
    auto root=getXmlFromBinary(data,size);if(!root||!root->hasTagName("CURVE_PILOT_MIDI_01"))return;
    const ScopedLock lock(control);armed=false;output.reset();selectedPort=root->getStringAttribute("port");
    auto finite=[](double x,double fallback){return std::isfinite(x)?x:fallback;};
    standaloneTempo=jlimit(20.0,300.0,finite(root->getDoubleAttribute("tempo",120),120));
    for(auto& l:lanes)l=pilot::Lane{};int i=0;
    for(auto* x:root->getChildIterator())if(x->hasTagName("LANE")&&i<pilot::lanes){auto& l=lanes[(size_t)i++];l.enabled=x->getBoolAttribute("on");l.minimum=(float)jlimit(0.0,1.0,finite(x->getDoubleAttribute("min"),0));l.maximum=(float)jlimit(0.0,1.0,finite(x->getDoubleAttribute("max",1),1));l.bars=jlimit(.25,32.0,finite(x->getDoubleAttribute("bars",4),4));MemoryBlock d;if(d.fromBase64Encoding(x->getStringAttribute("curve"))&&d.getSize()==l.drawing.y.size()*sizeof(float)){std::memcpy(l.drawing.y.data(),d.getData(),d.getSize());for(auto& y:l.drawing.y)y=std::isfinite(y)?jlimit(0.0f,1.0f,y):.5f;}}
    status="Reglages restaures. Verifier le port, cliquer Connecter puis ENVOI.";
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Pilot();}
