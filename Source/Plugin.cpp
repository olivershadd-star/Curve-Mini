#include "Plugin.h"
#include <set>
using namespace juce;
AudioProcessorValueTreeState::ParameterLayout Pilot::parameterLayout(){
    AudioProcessorValueTreeState::ParameterLayout layout;
    for(int i=0;i<pilot::lanes;++i)
        layout.add(std::make_unique<AudioParameterInt>(ParameterID{valueId(i),1},"Ligne "+String(i+1)+" Valeur",0,127,64));
    return layout;
}
Pilot::Pilot():AudioProcessor(BusesProperties().withOutput("Silence - controle MIDI externe",AudioChannelSet::stereo(),true)),
    parameters(*this,nullptr,"PARAMETERS",parameterLayout()){
    for(int i=0;i<pilot::lanes;++i){
        lanes[(size_t)i]=pilot::Lane(i);valueIds[(size_t)i]=valueId(i);
        values[(size_t)i]=parameters.getRawParameterValue(valueIds[(size_t)i]);
        manualRequested[(size_t)i].store(false);parameters.addParameterListener(valueIds[(size_t)i],this);
    }
    startTimer(10);
}
Pilot::~Pilot(){stopTimer();for(const auto& id:valueIds)parameters.removeParameterListener(id,this);}
void Pilot::parameterChanged(const String& id,float){
    // Called by the host, possibly on the audio thread: atomics only.
    for(int i=0;i<pilot::lanes;++i)if(id==valueIds[(size_t)i]){manualRequested[(size_t)i].store(true);return;}
}
void Pilot::processBlock(AudioBuffer<float>& audio,MidiBuffer& midi){
    audio.clear();midi.clear();
    sequence.fetch_add(1,std::memory_order_acq_rel);
    bool play=false;
    if(auto* head=getPlayHead())if(auto p=head->getPosition()){
        bool hasPosition=false;play=p->getIsPlaying();
        if(auto v=p->getPpqPosition())if(std::isfinite(*v)){quarter.store(*v);hasPosition=true;}
        if(auto v=p->getBpm())if(std::isfinite(*v)&&*v>0)tempo.store(*v);
        if(auto v=p->getTimeSignature())if(v->denominator>0&&v->numerator>0){
            beats.store(v->numerator*4.0/v->denominator);timeNumerator.store(v->numerator);timeDenominator.store(v->denominator);
        }
        play=play&&hasPosition;
    }
    running.store(play);stamp.store(Time::getMillisecondCounterHiRes());
    sequence.fetch_add(1,std::memory_order_release);
}
void Pilot::resetLast(){for(auto& lane:lanes)lane.last=-1;}
MidiOutput* Pilot::outputFor(int index){
    auto it=outputs.find(pilot::destination(lanes[(size_t)index],selectedPort.toStdString()));
    return it==outputs.end()?nullptr:it->second.get();
}
void Pilot::setDefaultPort(const String& id){
    const ScopedLock lock(control);if(selectedPort==id)return;
    armed=false;outputs.clear();selectedPort=id;resetLast();status="Sortie modifiee. Cliquer Connecter puis ENVOI.";
}
void Pilot::connect(const String& id){
    const ScopedLock lock(control);armed=false;outputs.clear();selectedPort=id;resetLast();
    std::set<std::string> ids;
    for(const auto& lane:lanes){auto port=pilot::destination(lane,id.toStdString());if(!port.empty())ids.insert(port);}
    int failed=0;
    for(const auto& port:ids){auto out=MidiOutput::openDevice(String(port));if(out)outputs.emplace(port,std::move(out));else ++failed;}
    if(failed>0)status=String(failed)+" sortie(s) indisponible(s). Verifier les machines / loopMIDI, puis reconnecter.";
    else status=outputs.empty()?"Choisir une sortie MIDI avant de connecter.":"Sorties connectees. Activer les lignes puis ENVOI.";
}
void Pilot::arm(bool on){
    const ScopedLock lock(control);armed=false;resetLast();
    if(!on){status="ENVOI arrete (courbes et potards).";return;}
    if(outputs.empty()){status="Connecter les sorties avant ENVOI.";return;}
    if(pilot::conflict(lanes,selectedPort.toStdString())){status="Deux lignes actives ont la meme sortie / canal / CC. Changer une adresse.";return;}
    for(int i=0;i<pilot::lanes;++i)if(lanes[(size_t)i].enabled&&!outputFor(i)){
        status="Sortie manquante pour la ligne "+String(i+1)+". Verifier et Connecter.";return;
    }
    armed=true;localQuarter=0;lastTick=Time::getMillisecondCounterHiRes();
    status="ENVOI actif. Courbes en lecture ; potards manuels disponibles aussi a l'arret.";
}
void Pilot::setEnabled(int index,bool enabled){
    if(!isPositiveAndBelow(index,pilot::lanes))return;
    const ScopedLock lock(control);lanes[(size_t)index].enabled=enabled;lanes[(size_t)index].last=-1;
    if(armed&&pilot::conflict(lanes,selectedPort.toStdString())){armed=false;status="Adresse MIDI deja utilisee : ENVOI arrete.";}
    if(armed&&enabled&&!outputFor(index)){armed=false;status="Sortie non connectee : cliquer Connecter puis ENVOI.";}
}
void Pilot::setAddress(int index,int channel,int cc){
    if(!isPositiveAndBelow(index,pilot::lanes))return;
    const ScopedLock lock(control);auto& lane=lanes[(size_t)index];
    channel=jlimit(1,16,channel);cc=jlimit(0,127,cc);if(lane.channel==channel&&lane.cc==cc)return;
    armed=false;lane.channel=channel;lane.cc=cc;lane.last=-1;status="Adresse modifiee. Verifier la destination puis reactiver ENVOI.";
}
void Pilot::setPort(int index,const String& id){
    if(!isPositiveAndBelow(index,pilot::lanes))return;
    const ScopedLock lock(control);auto& lane=lanes[(size_t)index];if(lane.port==id.toStdString())return;
    armed=false;outputs.clear();lane.port=id.toStdString();resetLast();status="Sortie modifiee. Cliquer Connecter puis ENVOI.";
}
void Pilot::useCurve(int index,bool curve){
    if(!isPositiveAndBelow(index,pilot::lanes))return;
    const ScopedLock lock(control);manualRequested[(size_t)index].store(false);lanes[(size_t)index].manual=!curve;lanes[(size_t)index].last=-1;
}
bool Pilot::isManual(int index)const{return lanes[(size_t)index].manual||manualRequested[(size_t)index].load();}
int Pilot::manualValue(int index)const{return jlimit(0,127,roundToInt(values[(size_t)index]->load()));}
void Pilot::learn(int index){
    const ScopedLock lock(control);armed=false;
    if(!isPositiveAndBelow(index,pilot::lanes)){status="Ligne MIDI invalide.";return;}
    auto* output=outputFor(index);if(!output){status="Connecter la sortie de cette ligne avant l'assignation.";return;}
    const auto& lane=lanes[(size_t)index];output->sendMessageNow(MidiMessage::controllerEvent(lane.channel,lane.cc,64));
    status="Canal "+String(lane.channel)+" / CC "+String(lane.cc)+" envoye seul. Quitter le mode MIDI de Live puis reactiver ENVOI.";
}
void Pilot::timerCallback(){
    const double now=Time::getMillisecondCounterHiRes();const ScopedLock lock(control);
    const double dt=lastTick>0?jlimit(0.0,.25,(now-lastTick)/1000):0;lastTick=now;
    for(int i=0;i<pilot::lanes;++i)if(manualRequested[(size_t)i].exchange(false)){
        lanes[(size_t)i].manual=true; // Keep last: unchanged physical values need no resend.
    }
    if(!armed)return;
    bool curvePlaying=false;double q=0,b=4;
    if(JUCEApplicationBase::isStandaloneApp()){
        localQuarter+=dt*standaloneTempo/60;q=localQuarter;curvePlaying=true;
    }else{
        const auto before=sequence.load(std::memory_order_acquire);
        if(!(before&1u)){
            const bool play=running.load();const double time=stamp.load(),t=tempo.load();q=quarter.load();b=beats.load();
            curvePlaying=sequence.load(std::memory_order_acquire)==before&&play&&pilot::fresh(now-time);
            if(curvePlaying)q+=(now-time)*t/60000;
        }
    }
    if(curvePlaying){displayQuarter=q;displayBeats=b;}
    for(int i=0;i<pilot::lanes;++i){auto& lane=lanes[(size_t)i];
        if(!lane.enabled){lane.last=-1;continue;}
        if(!lane.manual&&!curvePlaying){lane.last=-1;continue;}
        auto* output=outputFor(i);if(!output)continue;
        const int cc=lane.manual?manualValue(i):pilot::value(lane.drawing.at(pilot::phase(q,b,lane.bars)),lane.minimum,lane.maximum);
        if(cc!=lane.last){output->sendMessageNow(MidiMessage::controllerEvent(lane.channel,lane.cc,cc));lane.last=cc;}
    }
}
bool Pilot::exportLane(int index,const File& file){
    if(!isPositiveAndBelow(index,pilot::lanes))return false;
    pilot::Lane lane;int n=4,d=4;
    {const ScopedLock lock(control);lane=lanes[(size_t)index];if(!JUCEApplicationBase::isStandaloneApp()){n=timeNumerator.load();d=timeDenominator.load();}}
    try{
        const auto bytes=pilot::midiFile(lane,n,d);const bool ok=file.replaceWithData(bytes.data(),bytes.size());
        const ScopedLock lock(control);status=ok?"Courbe exportee : "+file.getFileName()+". Importer le fichier sur une piste MIDI de Live.":"Export impossible : verifier le dossier et les droits d'ecriture.";return ok;
    }catch(const std::exception& e){const ScopedLock lock(control);status="Export impossible : "+String(e.what());return false;}
}
void Pilot::getStateInformation(MemoryBlock& out){
    const ScopedLock lock(control);XmlElement root("CURVE_PILOT_MIDI_01");root.setAttribute("version",2);root.setAttribute("port",selectedPort);root.setAttribute("tempo",standaloneTempo);
    root.addChildElement(parameters.copyState().createXml().release());
    for(int i=0;i<pilot::lanes;++i){const auto& lane=lanes[(size_t)i];auto* x=root.createNewChildElement("LANE");
        x->setAttribute("on",lane.enabled);x->setAttribute("manual",isManual(i));x->setAttribute("channel",lane.channel);x->setAttribute("cc",lane.cc);x->setAttribute("port",String(lane.port));
        x->setAttribute("min",(double)lane.minimum);x->setAttribute("max",(double)lane.maximum);x->setAttribute("bars",lane.bars);
        MemoryBlock curveData(lane.drawing.y.data(),lane.drawing.y.size()*sizeof(float));x->setAttribute("curve",curveData.toBase64Encoding());
    }
    copyXmlToBinary(root,out);
}
void Pilot::setStateInformation(const void* data,int size){
    auto root=getXmlFromBinary(data,size);if(!root||!root->hasTagName("CURVE_PILOT_MIDI_01"))return;
    const ScopedLock lock(control);armed=false;outputs.clear();selectedPort=root->getStringAttribute("port");
    auto finite=[](double x,double fallback){return std::isfinite(x)?x:fallback;};
    standaloneTempo=jlimit(20.0,300.0,finite(root->getDoubleAttribute("tempo",120),120));
    // Legacy states have no PARAMETERS node. Reset values rather than reusing
    // an unrelated preset's pot positions, then restore the saved host values.
    for(int i=0;i<pilot::lanes;++i){lanes[(size_t)i]=pilot::Lane(i);auto* parameter=parameters.getParameter(valueIds[(size_t)i]);parameter->setValueNotifyingHost(parameter->convertTo0to1(64));}
    if(auto* xml=root->getChildByName("PARAMETERS")){
        auto state=ValueTree::fromXml(*xml);if(state.isValid())parameters.replaceState(state);
    }
    int i=0;
    for(auto* x:root->getChildIterator())if(x->hasTagName("LANE")&&i<pilot::lanes){
        auto& lane=lanes[(size_t)i++];lane.enabled=x->getBoolAttribute("on");lane.manual=x->getBoolAttribute("manual");
        lane.channel=jlimit(1,16,x->getIntAttribute("channel",1));lane.cc=jlimit(0,127,x->getIntAttribute("cc",pilot::controller(i-1)));lane.port=x->getStringAttribute("port").toStdString();
        lane.minimum=(float)jlimit(0.0,1.0,finite(x->getDoubleAttribute("min"),0));lane.maximum=(float)jlimit(0.0,1.0,finite(x->getDoubleAttribute("max",1),1));lane.bars=jlimit(.25,32.0,finite(x->getDoubleAttribute("bars",4),4));
        MemoryBlock block;if(block.fromBase64Encoding(x->getStringAttribute("curve"))&&block.getSize()==lane.drawing.y.size()*sizeof(float)){
            std::memcpy(lane.drawing.y.data(),block.getData(),block.getSize());for(auto& y:lane.drawing.y)y=std::isfinite(y)?jlimit(0.0f,1.0f,y):.5f;
        }
    }
    for(auto& request:manualRequested)request.store(false);
    localQuarter=0;lastTick=0;status="Reglages restaures. Verifier les sorties, cliquer Connecter puis ENVOI.";
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Pilot();}
