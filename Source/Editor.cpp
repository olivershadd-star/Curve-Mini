#include "Plugin.h"
using namespace juce;
namespace {
const Colour background(0xff101521),surface(0xff1b2435),ink(0xffe5ecf6),muted(0xff9eabc0);
Colour accent(int i){return Colour::fromHSV((float)i/16+.42f,.65f,.95f,1);}
struct Graph:Component {
    Pilot& p;int lane,last=-1;float previous=0;
    Graph(Pilot& processor,int index):p(processor),lane(index){setMouseCursor(MouseCursor::CrosshairCursor);}
    void paint(Graphics& g)override{
        g.fillAll(background);g.setColour(surface.brighter(.15f));
        for(int i=1;i<8;++i)g.drawVerticalLine(getWidth()*i/8,0,(float)getHeight());
        for(int i=1;i<4;++i)g.drawHorizontalLine(getHeight()*i/4,0,(float)getWidth());
        const ScopedLock lock(p.control);const auto& line=p.lanes[(size_t)lane];Path path;
        for(int x=0;x<getWidth();++x){
            const float phase=(float)x/jmax(1,getWidth()-1);
            const float value=line.drawing.y[(size_t)jlimit(0,curve::resolution,roundToInt(phase*curve::resolution))];
            const float y=4+(1-value)*(getHeight()-8);if(x==0)path.startNewSubPath(0,y);else path.lineTo((float)x,y);
        }
        g.setColour(accent(lane));g.strokePath(path,PathStrokeType(2));
        if(p.armed&&line.enabled&&!p.isManual(lane)){
            const double phase=pilot::phase(p.displayQuarter,p.displayBeats,line.bars);
            g.setColour(ink.withAlpha(.6f));g.drawVerticalLine(roundToInt((float)(phase-std::floor(phase))*getWidth()),0,(float)getHeight());
        }
    }
    void mouseDown(const MouseEvent& e)override{last=-1;draw(e);}
    void mouseDrag(const MouseEvent& e)override{draw(e);}
    void draw(const MouseEvent& e){
        const int n=jlimit(0,curve::resolution,roundToInt(e.position.x/jmax(1,getWidth()-1)*curve::resolution));
        const float y=jlimit(0.0f,1.0f,1-(e.position.y-4)/jmax(1,getHeight()-8));
        {const ScopedLock lock(p.control);auto& data=p.lanes[(size_t)lane].drawing.y;
            if(last<0)data[(size_t)n]=y;
            else for(int k=jmin(last,n);k<=jmax(last,n);++k)data[(size_t)k]=previous+(y-previous)*(n==last?1.0f:(float)(k-last)/(n-last));
        }
        last=n;previous=y;repaint();
    }
};
// The "missing" entry preserves restored identifiers; never silently select a
// different device when a Windows USB port is absent or has changed identity.
void fillPorts(ComboBox& box,const Array<MidiDeviceInfo>& devices,const String& selected,const String& defaultText){
    box.clear(dontSendNotification);box.addItem(defaultText,1);int choice=selected.isEmpty()?1:0;
    for(int i=0;i<devices.size();++i){box.addItem(devices[i].name,i+2);if(devices[i].identifier==selected)choice=i+2;}
    if(choice==0){box.addItem("Indisponible : "+selected,10000);choice=10000;}
    box.setSelectedId(choice,dontSendNotification);
}
struct Row:Component {
    Pilot& p;int lane;Label title,sent;ToggleButton on{"Active"};
    TextButton assign{"Assigner Live"},mode{"COURBE"},exportButton{"Exporter MIDI"};
    ComboBox shape,channel,cc,port;
    std::array<Slider,4> knobs;std::array<Label,4> labels;Graph graph;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> valueAttachment;
    std::unique_ptr<FileChooser> chooser;
    Array<MidiDeviceInfo> devices;String shownPort;
    Row(Pilot& processor,int index):p(processor),lane(index),graph(p,index){
        title.setText("LIGNE "+String(index+1),dontSendNotification);title.setColour(Label::textColourId,accent(index));
        for(auto* c:std::initializer_list<Component*>{&title,&sent,&on,&assign,&mode,&exportButton,&shape,&channel,&cc,&port,&graph})addAndMakeVisible(c);
        on.onClick=[this]{p.setEnabled(lane,on.getToggleState());};
        assign.onClick=[this]{p.learn(lane);};
        assign.setTooltip("Pour les macros de Live : envoie une seule fois le CC de cette ligne a 64 et coupe ENVOI. Inutile pour une machine qui connait deja ses CC.");
        mode.onClick=[this]{const ScopedLock lock(p.control);p.useCurve(lane,p.isManual(lane));sync();};
        mode.setTooltip("COURBE suit Live en lecture. MANUEL utilise le potard 0-127, meme a l'arret. Tourner ce potard prend la main ; cliquer ici pour reprendre la courbe.");
        exportButton.onClick=[this]{exportMidi();};
        exportButton.setTooltip("Exporte un tour de la courbe, avec minimum/maximum, canal et CC, sans notes. Importer le .mid dans un clip MIDI de Live.");
        for(int i=1;i<=16;++i)channel.addItem("Canal "+String(i),i);
        for(int i=0;i<=127;++i)cc.addItem("CC "+String(i),i+1);
        cc.setTooltip("Numero envoye a la machine. CC120 a CC127 sont des commandes de mode de canal.");
        auto address=[this]{p.setAddress(lane,channel.getSelectedId(),cc.getSelectedId()-1);};channel.onChange=address;cc.onChange=address;
        port.onChange=[this]{const int id=port.getSelectedId();if(id==1)p.setPort(lane,{});else if(isPositiveAndBelow(id-2,devices.size()))p.setPort(lane,devices[id-2].identifier);};
        shape.addItemList({"Vague","Dent montante","Dent descendante","Triangle","Ouverture douce","Escalier"},1);
        shape.setTextWhenNothingSelected("Forme...");
        shape.onChange=[this]{const ScopedLock lock(p.control);auto& line=p.lanes[(size_t)lane];line.drawing=curve::preset(shape.getSelectedId()-1,line.bars);graph.repaint();};
        const char* names[]={"Minimum %","Maximum %","Mesures","Manuel 0-127"};
        for(int k=0;k<4;++k){auto& knob=knobs[(size_t)k];
            knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);knob.setTextBoxStyle(Slider::TextBoxBelow,false,72,20);
            knob.setRange(k==2?.25:0,k==2?32:(k==3?127:100),k==2?.25:1);
            knob.setColour(Slider::rotarySliderFillColourId,accent(index));addAndMakeVisible(knob);
            labels[(size_t)k].setText(names[k],dontSendNotification);labels[(size_t)k].setJustificationType(Justification::centred);addAndMakeVisible(labels[(size_t)k]);
            if(k<3)knob.onValueChange=[this,k]{const ScopedLock lock(p.control);auto& line=p.lanes[(size_t)lane];const double v=knobs[(size_t)k].getValue();if(k==0)line.minimum=(float)v/100;else if(k==1)line.maximum=(float)v/100;else line.bars=v;};
        }
        knobs[3].setTooltip("Assignable au controleur externe via Configure puis Ctrl+M dans Live. Sa valeur manuelle est independante du minimum/maximum de la courbe.");
        valueAttachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.parameters,Pilot::valueId(lane),knobs[3]);
        scan({});sync();
    }
    void scan(const Array<MidiDeviceInfo>& list){
        devices=list;const ScopedLock lock(p.control);shownPort=String(p.lanes[(size_t)lane].port);fillPorts(port,devices,shownPort,"Sortie globale");
    }
    void sync(){
        const ScopedLock lock(p.control);const auto& line=p.lanes[(size_t)lane];
        on.setToggleState(line.enabled,dontSendNotification);channel.setSelectedId(line.channel,dontSendNotification);cc.setSelectedId(line.cc+1,dontSendNotification);
        if(shownPort!=String(line.port)){shownPort=String(line.port);fillPorts(port,devices,shownPort,"Sortie globale");}
        const bool manual=p.isManual(lane);mode.setButtonText(manual?"MANUEL":"COURBE");mode.setColour(TextButton::buttonColourId,manual?Colour(0xff80572b):surface);
        sent.setText(line.last<0?"Valeur : --":"Valeur : "+String(line.last),dontSendNotification);
        const double values[]={line.minimum*100,line.maximum*100,line.bars};
        for(int k=0;k<3;++k)if(!knobs[(size_t)k].isMouseButtonDown())knobs[(size_t)k].setValue(values[k],dontSendNotification);
        graph.repaint();
    }
    void exportMidi(){
        if(chooser)return;
        int channelNumber=1,controllerNumber=20;
        {const ScopedLock lock(p.control);channelNumber=p.lanes[(size_t)lane].channel;controllerNumber=p.lanes[(size_t)lane].cc;}
        const auto name="CurveMini_L"+String(lane+1)+"_Ch"+String(channelNumber)+"_CC"+String(controllerNumber)+".mid";
        chooser=std::make_unique<FileChooser>("Exporter un tour de courbe MIDI",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(name),"*.mid");
        chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,
            [safe=Component::SafePointer<Row>(this)](const FileChooser& dialog){
                if(!safe)return;const auto file=dialog.getResult();if(file!=File{})safe->p.exportLane(safe->lane,file);
                // Defer destruction until the FileChooser's callback has returned.
                MessageManager::callAsync([safe]{if(safe)safe->chooser.reset();});
            });
    }
    void paint(Graphics& g)override{g.setColour(surface);g.fillRoundedRectangle(getLocalBounds().toFloat(),10);}
    void resized()override{
        auto r=getLocalBounds().reduced(12);auto head=r.removeFromTop(28);
        title.setBounds(head.removeFromLeft(90));on.setBounds(head.removeFromLeft(70));mode.setBounds(head.removeFromLeft(100).reduced(2));
        assign.setBounds(head.removeFromLeft(110).reduced(2));exportButton.setBounds(head.removeFromLeft(120).reduced(2));sent.setBounds(head);
        r.removeFromTop(4);auto address=r.removeFromTop(28);channel.setBounds(address.removeFromLeft(100).reduced(2));cc.setBounds(address.removeFromLeft(90).reduced(2));
        shape.setBounds(address.removeFromRight(170).reduced(2));port.setBounds(address.reduced(2));
        r.removeFromTop(8);auto controls=r.removeFromRight(360);
        for(int k=0;k<4;++k){auto c=controls.removeFromLeft(90);labels[(size_t)k].setBounds(c.removeFromTop(20));knobs[(size_t)k].setBounds(c);}
        graph.setBounds(r.reduced(0,3));
    }
};
struct Editor:AudioProcessorEditor,private Timer {
    Pilot& p;Label title,hint,status;ComboBox ports;TextButton refresh{"Actualiser"},connectButton{"Connecter"},send{"ENVOI OFF"};Slider tempo;
    Viewport view;Component stack;std::vector<std::unique_ptr<Row>> rows;Array<MidiDeviceInfo> devices;String shownPort;
    TooltipWindow tips{this,700};
    Editor(Pilot& processor):AudioProcessorEditor(processor),p(processor){
        title.setText("CURVE MINI  /  PROTOTYPE 0.2",dontSendNotification);title.setColour(Label::textColourId,accent(0));addAndMakeVisible(title);
        hint.setText("Sortie globale ou par ligne | Canal + CC libres | Potards assignables dans Live | Export .mid",dontSendNotification);
        hint.setColour(Label::textColourId,muted);addAndMakeVisible(hint);
        for(auto* c:std::initializer_list<Component*>{&ports,&refresh,&connectButton,&send,&tempo,&status,&view})addAndMakeVisible(c);
        ports.onChange=[this]{const int id=ports.getSelectedId();if(id==1)p.setDefaultPort({});else if(isPositiveAndBelow(id-2,devices.size()))p.setDefaultPort(devices[id-2].identifier);};
        refresh.onClick=[this]{scan();};connectButton.onClick=[this]{const ScopedLock lock(p.control);p.connect(p.selectedPort);};
        send.onClick=[this]{const ScopedLock lock(p.control);p.arm(!p.armed);};
        tempo.setRange(20,300,1);{const ScopedLock lock(p.control);tempo.setValue(p.standaloneTempo);}tempo.setTextValueSuffix(" BPM");
        tempo.onValueChange=[this]{const ScopedLock lock(p.control);p.standaloneTempo=tempo.getValue();};tempo.setVisible(JUCEApplicationBase::isStandaloneApp());
        for(int i=0;i<pilot::lanes;++i){auto row=std::make_unique<Row>(p,i);stack.addAndMakeVisible(*row);rows.push_back(std::move(row));}
        view.setViewedComponent(&stack,false);view.setScrollBarsShown(true,false);
        scan();setResizable(true,true);setResizeLimits(820,500,2400,1800);setSize(1180,800);startTimerHz(15);
    }
    ~Editor()override{stopTimer();view.setViewedComponent(nullptr,false);}
    void scan(){
        devices=MidiOutput::getAvailableDevices();const ScopedLock lock(p.control);shownPort=p.selectedPort;
        fillPorts(ports,devices,shownPort,"Aucune sortie globale");for(auto& row:rows)row->scan(devices);
    }
    void timerCallback()override{
        {const ScopedLock lock(p.control);send.setButtonText(p.armed?"ENVOI ON":"ENVOI OFF");send.setColour(TextButton::buttonColourId,p.armed?Colour(0xff137c69):surface);
            status.setText(p.status,dontSendNotification);
            if(shownPort!=p.selectedPort){shownPort=p.selectedPort;fillPorts(ports,devices,shownPort,"Aucune sortie globale");}
        }
        for(auto& row:rows)if(row->getBounds().intersects(view.getViewArea()))row->sync();
    }
    void paint(Graphics& g)override{g.fillAll(background);}
    void resized()override{
        auto r=getLocalBounds().reduced(16);title.setBounds(r.removeFromTop(30));hint.setBounds(r.removeFromTop(24));auto bar=r.removeFromTop(36);
        send.setBounds(bar.removeFromRight(110).reduced(2));connectButton.setBounds(bar.removeFromRight(100).reduced(2));refresh.setBounds(bar.removeFromRight(100).reduced(2));
        if(tempo.isVisible())tempo.setBounds(bar.removeFromRight(170));ports.setBounds(bar.reduced(2));status.setBounds(r.removeFromBottom(28));r.removeFromTop(10);view.setBounds(r);
        constexpr int rowHeight=224;stack.setSize(jmax(760,view.getMaximumVisibleWidth()),pilot::lanes*rowHeight);
        for(int i=0;i<pilot::lanes;++i)rows[(size_t)i]->setBounds(0,i*rowHeight,stack.getWidth(),rowHeight-10);
    }
};
}
AudioProcessorEditor* Pilot::createEditor(){return new Editor(*this);}
