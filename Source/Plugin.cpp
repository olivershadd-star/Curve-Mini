#include "Plugin.h"
using namespace juce;
namespace {String pid(int i,const String& key){return "lane"+String(i+1)+"_"+key;}}
AudioProcessorValueTreeState::ParameterLayout CurvePilot::layout(){
    AudioProcessorValueTreeState::ParameterLayout p;
    for(int i=0;i<laneCount;++i){String title="Lane "+String(i+1)+" ";
        p.add(std::make_unique<AudioParameterBool>(ParameterID{pid(i,"on"),1},title+"On",false));
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{pid(i,"min"),1},title+"Min %",NormalisableRange<float>(0,100,.01f),0));
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{pid(i,"max"),1},title+"Max %",NormalisableRange<float>(0,100,.01f),100));
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{pid(i,"bars"),1},title+"Length bars",NormalisableRange<float>(.25f,32,.25f),4));
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{pid(i,"offset"),1},title+"Phase %",NormalisableRange<float>(0,100,.01f),0));
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{pid(i,"smooth"),1},title+"Smooth ms",NormalisableRange<float>(0,500,1,.5f),20));
        p.add(std::make_unique<AudioParameterFloat>(ParameterID{pid(i,"depth"),1},title+"Depth %",NormalisableRange<float>(0,100,.01f),100));
    }
    for(int i=0;i<laneCount;++i)p.add(std::make_unique<AudioParameterInt>(ParameterID{pid(i,"value"),1},"Ligne "+String(i+1)+" Valeur MIDI",0,127,64));
    return p;
}
float CurvePilot::get(int i,const String& key)const{return state.getRawParameterValue(pid(i,key))->load();}
void CurvePilot::set(int i,const String& key,float v){if(auto* p=state.getParameter(pid(i,key))){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();}}
CurvePilot::Lane CurvePilot::getLane(int l){const ScopedLock lock(mutex);return lanes[(size_t)l];}
void CurvePilot::replaceCurve(int l,const curve::Data& d,const String& script){const ScopedLock lock(mutex);auto& a=lanes[(size_t)l];if(a.locked){status="Ligne verrouillee.";return;}previous[(size_t)l]=std::make_unique<Lane>(a);a.curve=d;a.script=script;a.motionText={};a.points.clear();a.pointMode=false;set(l,"bars",(float)d.bars);revision++;}
void CurvePilot::undoCurve(int l){const ScopedLock lock(mutex);auto& a=lanes[(size_t)l];if(a.locked||!previous[(size_t)l])return;auto old=*previous[(size_t)l];previous[(size_t)l]=std::make_unique<Lane>(a);a.curve=old.curve;a.points=old.points;a.pointMode=old.pointMode;a.script=old.script;a.motionText=old.motionText;set(l,"bars",(float)a.curve.bars);revision++;}
void CurvePilot::toggleLock(int l){const ScopedLock lock(mutex);lanes[(size_t)l].locked=!lanes[(size_t)l].locked;revision++;}
void CurvePilot::applyVariation(int l,float amount){auto a=getLane(l);replaceCurve(l,curve::vary(a.curve,amount,(uint32_t)Random::getSystemRandom().nextInt()));}
bool CurvePilot::applyScript(int l,const String& script,String& error){try{auto d=curve::parse(script.toStdString());if(d.bars<.25||d.bars>32||std::abs(d.bars*4-std::round(d.bars*4))>1e-6)throw std::runtime_error("Duree : 0.25 a 32 mesures, par pas de 0.25.");if(getLane(l).locked)throw std::runtime_error("Ligne verrouillee.");replaceCurve(l,d,script);return true;}catch(const std::exception& e){error=String::fromUTF8(e.what());return false;}}
void CurvePilot::replacePoints(int l,std::vector<curve::Point> pts){
    const ScopedLock lock(mutex);auto& a=lanes[(size_t)l];if(a.locked){status="Ligne verrouillee.";return;}
    pts=curve::canonicalPoints(std::move(pts));auto d=curve::fromPoints(pts,get(l,"bars"));
    previous[(size_t)l]=std::make_unique<Lane>(a);a.curve=d;a.points=std::move(pts);a.pointMode=true;a.script={};a.motionText={};revision++;
}
void CurvePilot::setPointMode(int l,bool enabled){
    const ScopedLock lock(mutex);auto& a=lanes[(size_t)l];if(a.locked)return;
    previous[(size_t)l]=std::make_unique<Lane>(a);
    if(enabled&&a.points.empty())a.points=curve::simplify(a.curve,.006);
    a.pointMode=enabled;
    if(enabled)a.curve=curve::fromPoints(a.points,get(l,"bars"));
    revision++;
}
void CurvePilot::smoothCurve(int l,float strength){auto a=getLane(l);a.curve.bars=get(l,"bars");replaceCurve(l,curve::smoothDrawing(a.curve,strength));}
void CurvePilot::copyCurve(int l){const ScopedLock lock(mutex);clipboard=lanes[(size_t)l];clipboard->curve.bars=get(l,"bars");status="Courbe "+String(l+1)+" copiee (destination non copiee).";}
void CurvePilot::pasteCurve(int l){const ScopedLock lock(mutex);auto& a=lanes[(size_t)l];if(!clipboard){status="Copier une courbe d'abord.";return;}if(a.locked){status="Ligne verrouillee.";return;}
    previous[(size_t)l]=std::make_unique<Lane>(a);a.curve=clipboard->curve;a.points=clipboard->points;a.pointMode=clipboard->pointMode;a.script=clipboard->script;a.motionText=clipboard->motionText;set(l,"bars",(float)a.curve.bars);revision++;
}
bool CurvePilot::hasClipboard()const{const ScopedLock lock(mutex);return clipboard.has_value();}
void CurvePilot::exportSession(const File& f){MemoryBlock blob;getStateInformation(blob);bool ok=f.replaceWithData(blob.getData(),blob.getSize());const ScopedLock lock(mutex);status=ok?"Session exportee.":"Echec export session.";}
bool CurvePilot::importSession(const File& f){MemoryBlock blob;if(!f.loadFileAsData(blob))return false;auto xml=getXmlFromBinary(blob.getData(),(int)blob.getSize());if(!xml||!(xml->hasTagName("CURVE_MINI_SESSION")||xml->hasTagName("CURVEPILOT_SESSION")))return false;setStateInformation(blob.getData(),(int)blob.getSize());return true;}
CurvePilot::CurvePilot():AudioProcessor(BusesProperties().withInput("Audio Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),state(*this,nullptr,"CurvePilot",layout()){
    for(int i=0;i<laneCount;++i){lanes[(size_t)i].cc=20+i;lanes[(size_t)i].name="Courbe "+String(i+1);valueIds[(size_t)i]=pid(i,"value");manualRequests[(size_t)i]=false;state.addParameterListener(valueIds[(size_t)i],this);}startTimerHz(100);
}
CurvePilot::~CurvePilot(){stopTimer();for(auto& id:valueIds)state.removeParameterListener(id,this);}
bool CurvePilot::isBusesLayoutSupported(const BusesLayout& b)const{return (b.getMainInputChannelSet().isDisabled()||b.getMainInputChannelSet()==AudioChannelSet::stereo())&&b.getMainOutputChannelSet()==AudioChannelSet::stereo();}
void CurvePilot::parameterChanged(const String& id,float){for(int i=0;i<laneCount;++i)if(id==valueIds[(size_t)i]){manualRequests[(size_t)i]=true;return;}}
bool CurvePilot::isManual(int i)const{return lanes[(size_t)i].manual||manualRequests[(size_t)i].load();}
void CurvePilot::resumeCurve(int i){const ScopedLock lock(mutex);manualRequests[(size_t)i]=false;lanes[(size_t)i].manual=false;lanes[(size_t)i].filterValid=false;lanes[(size_t)i].last=-1;revision++;}
void CurvePilot::resetOutputValues(){for(auto& l:lanes){l.last=-1;l.filterValid=false;}}
void CurvePilot::configureDestination(int i,bool hardware,const String& port,int channel,int cc,const String& name){
    const ScopedLock lock(mutex);auto& l=lanes[(size_t)i];channel=jlimit(1,16,channel);cc=jlimit(0,127,cc);
    const bool changed=l.hardware!=hardware||l.port!=port||l.channel!=channel||l.cc!=cc;
    if(!changed){if(l.name!=name){l.name=name;revision++;}return;}
    sending=false;if(l.port!=port)outputs.clear();l.hardware=hardware;l.port=port;l.channel=channel;l.cc=cc;l.name=name;resetOutputValues();status="Destination modifiee. Connecter les sorties puis activer ENVOI.";revision++;
}
void CurvePilot::connectOutputs(){
    const ScopedLock lock(mutex);sending=false;outputs.clear();resetOutputValues();StringArray ids;for(auto& l:lanes)if(l.port.isNotEmpty())ids.addIfNotAlreadyThere(l.port);
    int failed=0;for(auto& id:ids){auto out=MidiOutput::openDevice(id);if(out)outputs.emplace(id,std::move(out));else failed++;}
    status=failed?String(failed)+" sortie(s) indisponible(s). Verifier les connexions.":outputs.empty()?"Choisir une sortie dans Destination.":"Sorties connectees. Activer ENVOI pour envoyer.";
}
bool CurvePilot::routesValid(){
    for(int i=0;i<laneCount;++i)if(get(i,"on")>=.5f){auto& a=lanes[(size_t)i];
        if(a.port.isEmpty()||outputs.find(a.port)==outputs.end()){status="Sortie de la ligne "+String(i+1)+" non connectee.";return false;}
        for(int j=0;j<i;++j){auto& b=lanes[(size_t)j];if(get(j,"on")>=.5f&&a.port==b.port&&a.channel==b.channel&&a.cc==b.cc){status="Meme port / canal / CC sur deux lignes actives : "+String(j+1)+" et "+String(i+1);return false;}}
    }return true;
}
void CurvePilot::arm(bool on){const ScopedLock lock(mutex);sending=false;resetOutputValues();if(on&&outputs.empty()){status="Connecter les sorties avant ENVOI.";return;}if(on&&!routesValid())return;sending=on;status=on?"ENVOI actif. Courbes pendant la lecture ; manuel disponible a l'arret.":"ENVOI arrete.";}
void CurvePilot::assign(int i){const ScopedLock lock(mutex);sending=false;auto& l=lanes[(size_t)i];auto it=outputs.find(l.port);if(it==outputs.end()){status="Connecter la sortie de cette ligne avant Assigner.";return;}it->second->sendMessageNow(MidiMessage::controllerEvent(l.channel,l.cc,64));status="Une seule commande envoyee. Quitter Ctrl+M dans Live, puis reactiver ENVOI.";}
void CurvePilot::processBlock(AudioBuffer<float>& audio,MidiBuffer& midi){
    audio.clear();midi.clear();transportSequence.fetch_add(1);bool play=false;
    if(auto* h=getPlayHead())if(auto pos=h->getPosition()){
        const auto q=pos->getPpqPosition();play=pos->getIsPlaying()&&q&&std::isfinite(*q);if(q&&std::isfinite(*q))hostQuarter=*q;
        if(auto v=pos->getBpm())if(std::isfinite(*v)&&*v>0)hostTempo=*v;
        if(auto v=pos->getTimeSignature())if(v->numerator>0&&v->denominator>0)hostBeats=v->numerator*4.0/v->denominator;
    }
    hostPlaying=play;hostStamp=Time::getMillisecondCounterHiRes();transportSequence.fetch_add(1);
}
void CurvePilot::timerCallback(){
    const double now=Time::getMillisecondCounterHiRes();const ScopedLock lock(mutex);
    const double dt=lastTick>0?jlimit(0.0,.25,(now-lastTick)/1000):0;lastTick=now;
    for(int i=0;i<laneCount;++i)if(manualRequests[(size_t)i].exchange(false)){lanes[(size_t)i].manual=true;revision++;}
    double q=0,b=4;bool play=false;
    if(JUCEApplicationBase::isStandaloneApp()){if(transportReset.exchange(false))localQuarter=0;play=standalonePlaying.load();if(play)localQuarter+=dt*standaloneBpm.load()/60;q=localQuarter;}
    else{auto seq=transportSequence.load();if(!(seq&1u)){q=hostQuarter.load();b=hostBeats.load();double age=now-hostStamp.load();play=hostPlaying.load()&&age>=0&&age<250;const auto t=hostTempo.load();play=play&&seq==transportSequence.load();if(play)q+=age*t/60000;}}
    if(play)visualBars=q/b;
    if(!sending)return;if(!routesValid()){sending=false;return;}
    for(int i=0;i<laneCount;++i){auto& l=lanes[(size_t)i];if(get(i,"on")<.5f||(!l.manual&&!play)){l.last=-1;l.filterValid=false;continue;}
        float wanted=get(i,"value")/127;
        if(!l.manual){double phase=q/(b*get(i,"bars"))+get(i,"offset")*.01;float v=l.curve.at(phase);v=.5f+get(i,"depth")*.01f*(v-.5f);wanted=jlimit(0.0f,1.0f,(get(i,"min")+(get(i,"max")-get(i,"min"))*v)*.01f);
            float ms=get(i,"smooth");if(!l.filterValid){l.filtered=l.last>=0?l.last/127.f:wanted;l.filterValid=true;}float alpha=ms<=0?1.f:(float)(1-std::exp(-dt/(ms*.001)));l.filtered+=alpha*(wanted-l.filtered);wanted=l.filtered;
        }else l.filterValid=false;
        int value=jlimit(0,127,roundToInt(wanted*127));if(value!=l.last){outputs.at(l.port)->sendMessageNow(MidiMessage::controllerEvent(l.channel,l.cc,value));l.last=value;}
    }
}
bool CurvePilot::exportMidi(int i,const File& file){
    MidiMessageSequence seq;double length;int channel,cc;Lane l;float minimum,maximum,depth,offset,smooth;double beats;
    {const ScopedLock lock(mutex);l=lanes[(size_t)i];length=get(i,"bars");channel=l.channel;cc=l.cc;minimum=get(i,"min")*.01f;maximum=get(i,"max")*.01f;depth=get(i,"depth")*.01f;offset=get(i,"offset")*.01f;smooth=get(i,"smooth");beats=JUCEApplicationBase::isStandaloneApp()?4:hostBeats.load();}
    constexpr int ppq=960;int end=jmax(1,roundToInt(length*beats*ppq));int last=-1;float filtered=0;bool valid=false;
    const double bpm=JUCEApplicationBase::isStandaloneApp()?standaloneBpm.load():hostTempo.load();
    auto add=[&](int tick){float v=l.curve.at((double)tick/end+offset);float wanted=jlimit(0.f,1.f,minimum+(maximum-minimum)*(.5f+depth*(v-.5f)));
        if(!valid){filtered=wanted;valid=true;}else{float alpha=smooth<=0?1.f:(float)(1-std::exp(-(10.0/ppq*60/bpm)/(smooth*.001)));filtered+=alpha*(wanted-filtered);}
        int value=jlimit(0,127,roundToInt(filtered*127));if(value!=last){auto m=MidiMessage::controllerEvent(channel,cc,value);m.setTimeStamp(tick);seq.addEvent(m);last=value;}};
    for(int tick=0;tick<end;tick+=10)add(tick);if((end-1)%10)add(end-1);
    auto finish=MidiMessage::endOfTrack();finish.setTimeStamp(end);seq.addEvent(finish);MidiFile midi;midi.setTicksPerQuarterNote(ppq);midi.addTrack(seq);
    MemoryOutputStream bytes;bool ok=midi.writeTo(bytes)&&file.replaceWithData(bytes.getData(),bytes.getDataSize());const ScopedLock lock(mutex);status=ok?"Courbe MIDI exportee. Importer le .mid dans un clip Live ; regler le canal de sortie de la piste.":"Echec de l'export MIDI.";return ok;
}
void CurvePilot::getStateInformation(MemoryBlock& out){
    const ScopedLock lock(mutex);ValueTree save("CURVE_MINI_SESSION");save.addChild(state.copyState(),-1,nullptr);ValueTree data("CONTENT");data.setProperty("width",editorWidth,nullptr);data.setProperty("height",editorHeight,nullptr);data.setProperty("selected",selectedLaneUI,nullptr);data.setProperty("standaloneBpm",standaloneBpm.load(),nullptr);
    for(int i=0;i<laneCount;++i){auto& l=lanes[(size_t)i];ValueTree v("LANE");v.setProperty("index",i,nullptr);v.setProperty("name",l.name,nullptr);v.setProperty("port",l.port,nullptr);v.setProperty("channel",l.channel,nullptr);v.setProperty("cc",l.cc,nullptr);v.setProperty("hardware",l.hardware,nullptr);v.setProperty("manual",isManual(i),nullptr);v.setProperty("locked",l.locked,nullptr);v.setProperty("pointMode",l.pointMode,nullptr);v.setProperty("script",l.script,nullptr);v.setProperty("motionText",l.motionText,nullptr);
        MemoryBlock blob(l.curve.y.data(),l.curve.y.size()*sizeof(float));v.setProperty("curve",blob.toBase64Encoding(),nullptr);for(auto pt:l.points){ValueTree p("POINT");p.setProperty("x",pt.x,nullptr);p.setProperty("y",pt.y,nullptr);v.addChild(p,-1,nullptr);}data.addChild(v,-1,nullptr);
    }save.addChild(data,-1,nullptr);copyXmlToBinary(*save.createXml(),out);
}
void CurvePilot::setStateInformation(const void* bytes,int size){
    auto xml=getXmlFromBinary(bytes,size);if(!xml||(!xml->hasTagName("CURVE_MINI_SESSION")&&!xml->hasTagName("CURVEPILOT_SESSION")))return;
    const bool legacy=xml->hasTagName("CURVEPILOT_SESSION");auto tree=ValueTree::fromXml(*xml);auto parameters=legacy?tree.getChildWithName("PARAMETERS").getChild(0).createCopy():tree.getChildWithName("CurvePilot").createCopy();if(!parameters.isValid())return;
    const ScopedLock lock(mutex);sending=false;outputs.clear();
    for(auto* param:getParameters())if(auto* ranged=dynamic_cast<RangedAudioParameter*>(param)){
        auto v=parameters.getChildWithProperty("id",ranged->paramID);if(!v.isValid()){v=ValueTree("PARAM");v.setProperty("id",ranged->paramID,nullptr);parameters.addChild(v,-1,nullptr);}
        double value=(double)v.getProperty("value",ranged->convertFrom0to1(ranged->getDefaultValue()));
        if(!std::isfinite(value))value=ranged->convertFrom0to1(ranged->getDefaultValue());
        const auto& range=ranged->getNormalisableRange();v.setProperty("value",range.snapToLegalValue(jlimit(range.start,range.end,(float)value)),nullptr);
    }state.replaceState(parameters);
    auto content=tree.getChildWithName("CONTENT");editorWidth=jlimit(820,3200,(int)content.getProperty("width",1380));editorHeight=jlimit(560,2200,(int)content.getProperty("height",880));selectedLaneUI=jlimit(0,11,(int)content.getProperty("selected",0));double bpm=(double)content.getProperty("standaloneBpm",120);standaloneBpm=std::isfinite(bpm)?jlimit(20.,300.,bpm):120;
    for(int i=0;i<laneCount;++i){lanes[(size_t)i]=Lane{};lanes[(size_t)i].cc=20+i;lanes[(size_t)i].name="Courbe "+String(i+1);previous[(size_t)i].reset();}
    for(auto v:content)if(v.hasType("LANE")){int i=(int)v["index"];if(i<0||i>=laneCount)continue;auto& l=lanes[(size_t)i];l.name=v["name"].toString();l.port=legacy?String{}:v["port"].toString();l.channel=jlimit(1,16,(int)v.getProperty("channel",16));l.cc=jlimit(0,127,(int)v.getProperty("cc",20+i));l.hardware=(bool)v["hardware"];l.manual=!legacy&&(bool)v["manual"];l.locked=(bool)v["locked"];l.pointMode=(bool)v["pointMode"];l.script=v["script"].toString();l.motionText=v["motionText"].toString();l.curve.bars=get(i,"bars");
        MemoryBlock blob;if(blob.fromBase64Encoding(v["curve"].toString())&&blob.getSize()==l.curve.y.size()*sizeof(float)){std::memcpy(l.curve.y.data(),blob.getData(),blob.getSize());for(auto& y:l.curve.y)y=std::isfinite(y)?jlimit(0.f,1.f,y):.5f;}
        for(auto pt:v)if(pt.hasType("POINT"))l.points.push_back({(double)pt["x"],(float)pt["y"]});try{if(!l.points.empty())l.points=curve::canonicalPoints(l.points);else l.pointMode=false;}catch(...){l.points.clear();l.pointMode=false;}if(legacy)set(i,"on",0);
    }
    for(auto& request:manualRequests)request=false;standalonePlaying=false;localQuarter=0;lastTick=0;clipboard.reset();status=legacy?"Courbes de Curve Pilot importees. Les VST ne sont pas charges ; reassigner les destinations.":"Session restauree. Verifier les destinations, connecter puis activer ENVOI.";revision++;
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new CurvePilot();}
