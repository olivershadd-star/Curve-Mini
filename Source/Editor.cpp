#include "Plugin.h"
#include "MotionLanguage.h"
#include "WindowPlacement.h"
using namespace juce;
namespace {
using SA=AudioProcessorValueTreeState::SliderAttachment;
using BA=AudioProcessorValueTreeState::ButtonAttachment;
const Colour bg(0xff10141f),surface(0xff192131),ink(0xffedf2ff),muted(0xff91a2be),edge(0xff2a364c);
Colour laneColour(int i){static const uint32 colours[]={0xff42dfbd,0xff69b4ff,0xffcc91ff,0xffffae70,0xffff7799,0xffe6d575,0xff7ce2ed,0xffa4db83,0xff999cff,0xffee9acb,0xff80bfc2,0xffe5b192};return Colour(colours[i%12]);}
String id(int lane,const String& name){return "lane"+String(lane+1)+"_"+name;}
void label(Label& l,const String& text,float size=12,Colour colour=muted){l.setText(text,dontSendNotification);l.setFont(Font(FontOptions(size)));l.setColour(Label::textColourId,colour);}
void rounded(Graphics& g,Rectangle<float> r,Colour colour,float radius=12){g.setColour(colour);g.fillRoundedRectangle(r,radius);}
class Theme final:public LookAndFeel_V4 {
public:
    Theme(){setColour(ResizableWindow::backgroundColourId,bg);setColour(Label::textColourId,ink);setColour(TextButton::textColourOffId,ink);setColour(TextButton::textColourOnId,ink);setColour(TextButton::buttonColourId,Colour(0xff28354b));setColour(TextButton::buttonOnColourId,Colour(0xff32556a));setColour(ComboBox::backgroundColourId,surface);setColour(ComboBox::textColourId,ink);setColour(ComboBox::outlineColourId,edge);setColour(PopupMenu::backgroundColourId,surface);setColour(PopupMenu::textColourId,ink);setColour(TextEditor::backgroundColourId,Colour(0xff101927));setColour(TextEditor::textColourId,ink);setColour(TextEditor::outlineColourId,edge);setColour(ScrollBar::thumbColourId,Colour(0xff536981));setColour(Slider::textBoxBackgroundColourId,Colours::transparentBlack);setColour(Slider::textBoxTextColourId,ink);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);setColour(Slider::trackColourId,Colour(0xff33435d));setColour(Slider::thumbColourId,laneColour(0));}
    void drawButtonBackground(Graphics& g,Button& b,const Colour& colour,bool over,bool down)override{auto r=b.getLocalBounds().toFloat().reduced(.7f);rounded(g,r,colour.brighter(down?.2f:(over?.1f:0)),7);}
    Font getTextButtonFont(TextButton&,int h)override{return Font(FontOptions(jmin(13.0f,h*.49f)));}
};
class CurveGraph final:public Component {
public:
    CurvePilot& p;int lane;curve::Data data=curve::preset(0,4);std::vector<curve::Point> points;
    bool pointMode=false,locked=false,preview=false,dragging=false;Colour colour;
    CurveGraph(CurvePilot& proc,int l):p(proc),lane(l),colour(laneColour(l)){setMouseCursor(MouseCursor::CrosshairCursor);}
    void sync(){if(dragging)return;auto a=p.getLane(lane);data=a.curve;points=a.points;pointMode=a.pointMode;locked=a.locked;repaint();}
    void paint(Graphics& g)override{
        auto area=getLocalBounds().toFloat();rounded(g,area,Colour(0xff101928),9);auto r=area.reduced(9,10);
        g.setColour(Colour(0xff27354a));for(int i=0;i<=8;++i)g.drawLine(r.getX()+r.getWidth()*i/8,r.getY(),r.getX()+r.getWidth()*i/8,r.getBottom(),i%2==0?1.0f:.5f);for(int i=0;i<=4;++i)g.drawLine(r.getX(),r.getY()+r.getHeight()*i/4,r.getRight(),r.getY()+r.getHeight()*i/4,.5f);
        Path path;for(int n=0;n<=curve::resolution;++n){float x=r.getX()+(float)n/curve::resolution*r.getWidth(),y=r.getBottom()-data.y[(size_t)n]*r.getHeight();if(n==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        auto fill=path;fill.lineTo(r.getRight(),r.getBottom());fill.lineTo(r.getX(),r.getBottom());fill.closeSubPath();g.setGradientFill(ColourGradient(colour.withAlpha(.20f),0,r.getY(),colour.withAlpha(.015f),0,r.getBottom(),false));g.fillPath(fill);
        g.setColour(locked?muted:colour);g.strokePath(path,PathStrokeType(1.8f));
        if(pointMode&&!preview)for(auto pt:points){float x=r.getX()+(float)pt.x*r.getWidth(),y=r.getBottom()-pt.y*r.getHeight();g.setColour(bg);g.fillEllipse(x-4,y-4,8,8);g.setColour(colour);g.drawEllipse(x-4,y-4,8,8,1.5f);}
        if(!preview){double phase=p.visualBars.load()/p.get(lane,"bars")+p.get(lane,"offset")*.01;phase-=std::floor(phase);float x=r.getX()+(float)phase*r.getWidth();g.setColour(ink.withAlpha(.75f));g.drawLine(x,r.getY(),x,r.getBottom(),1);}
    }
    void mouseDown(const MouseEvent& e)override{
        if(locked||preview)return;
        dragging=true;dragIndex=-1;lastX=-1;
        if(pointMode){auto pt=toPoint(e.position);float best=12;auto r=getLocalBounds().toFloat().reduced(9,10);for(size_t k=0;k<points.size();++k){Point<float> pos(r.getX()+(float)points[k].x*r.getWidth(),r.getBottom()-points[k].y*r.getHeight());float distance=pos.getDistanceFrom(e.position);if(distance<best){best=distance;dragIndex=(int)k;}}
            if(e.mods.isRightButtonDown()||e.mods.isAltDown()){if(dragIndex>0&&dragIndex<(int)points.size()-1)points.erase(points.begin()+dragIndex);dragIndex=-1;}
            else if(dragIndex<0){points.push_back(pt);points=curve::canonicalPoints(points);for(size_t k=0;k<points.size();++k)if(std::abs(points[k].x-pt.x)<1e-6){dragIndex=(int)k;break;}}
            data=curve::fromPoints(points,p.get(lane,"bars"));repaint();
        }else draw(e);
    }
    void mouseDrag(const MouseEvent& e)override{
        if(!dragging)return;
        if(pointMode){if(dragIndex<0)return;auto pt=toPoint(e.position);size_t k=(size_t)dragIndex;if(k==0)pt.x=0;else if(k+1==points.size())pt.x=1;else {double gap=std::min(1.0/curve::resolution,(points[k+1].x-points[k-1].x)*.25);pt.x=std::clamp(pt.x,points[k-1].x+gap,points[k+1].x-gap);}points[k]=pt;data=curve::fromPoints(points,p.get(lane,"bars"));repaint();}else draw(e);
    }
    void mouseUp(const MouseEvent&)override{if(!dragging)return;dragging=false;if(pointMode)p.replacePoints(lane,points);else{data.bars=p.get(lane,"bars");p.replaceCurve(lane,data);}sync();}

private:
    int dragIndex=-1,lastX=-1;float lastY=.5;
    curve::Point toPoint(Point<float> position){auto r=getLocalBounds().toFloat().reduced(9,10);return {curve::clamp((position.x-r.getX())/jmax(1.0f,r.getWidth())),(float)curve::clamp((r.getBottom()-position.y)/jmax(1.0f,r.getHeight()))};}
    void draw(const MouseEvent& e){auto pt=toPoint(e.position);int x=jlimit(0,curve::resolution,roundToInt(pt.x*curve::resolution));if(lastX<0)data.y[(size_t)x]=pt.y;else for(int n=jmin(lastX,x);n<=jmax(lastX,x);++n){float t=x==lastX?1.0f:(float)(n-lastX)/(x-lastX);data.y[(size_t)n]=lastY+t*(pt.y-lastY);}lastX=x;lastY=pt.y;repaint();}
};
class MotionPanel final:public Component {
    CurvePilot& p;int lane;ComboBox mode;TextEditor text,interpretation;Label message;TextButton preview{"Previsualiser"},apply{"Appliquer"};CurveGraph graph;std::optional<curve::Data> parsed;String compiled,original;
public:
    MotionPanel(CurvePilot& proc,int i):p(proc),lane(i),graph(p,i){
        for(auto* c:std::initializer_list<Component*>{&mode,&text,&interpretation,&message,&preview,&apply,&graph})addAndMakeVisible(c);
        mode.addItem("Decrire le mouvement (local)",1);mode.addItem("Code avance",2);mode.setSelectedId(1,dontSendNotification);
        text.setMultiLine(true);text.setReturnKeyStartsNewLine(true);interpretation.setMultiLine(true);interpretation.setReadOnly(true);graph.preview=true;graph.sync();
        auto fill=[this]{auto l=p.getLane(lane);text.setText(mode.getSelectedId()==1?l.motionText:l.script,false);parsed.reset();apply.setEnabled(false);};mode.onChange=fill;fill();
        text.onTextChange=[this]{parsed.reset();apply.setEnabled(false);};
        preview.onClick=[this]{try{original=text.getText();
            if(mode.getSelectedId()==1){auto result=motion::translate(original.toStdString(),p.get(lane,"bars"),p.getLane(lane).curve.y.front());compiled=String::fromUTF8(result.script.c_str());parsed=result.data;interpretation.setText(String::fromUTF8(result.explanation.c_str()),false);}
            else{compiled=original;parsed=curve::parse(compiled.toStdString());if(parsed->bars<.25||parsed->bars>32||std::abs(parsed->bars*4-std::round(parsed->bars*4))>1e-6)throw std::runtime_error("Duree : 0.25 a 32 mesures, pas de 0.25.");interpretation.setText("Code valide. Verifier l'apercu avant d'appliquer.",false);}
            graph.data=*parsed;graph.repaint();message.setText("Proposition : la courbe actuelle reste inchangee.",dontSendNotification);apply.setEnabled(true);
        }catch(const std::exception& e){parsed.reset();apply.setEnabled(false);message.setText(String::fromUTF8(e.what()),dontSendNotification);}};
        apply.onClick=[this]{if(!parsed||original!=text.getText())return;String error;if(p.applyScript(lane,compiled,error)){if(mode.getSelectedId()==1){const ScopedLock lock(p.mutex);p.lanes[(size_t)lane].motionText=original;}message.setText("Courbe appliquee.",dontSendNotification);}else message.setText(error,dontSendNotification);};
        setSize(740,650);
    }
    void resized()override{auto r=getLocalBounds().reduced(22);mode.setBounds(r.removeFromTop(34));r.removeFromTop(12);text.setBounds(r.removeFromTop(125));r.removeFromTop(10);interpretation.setBounds(r.removeFromTop(80));r.removeFromTop(12);auto bottom=r.removeFromBottom(78);message.setBounds(bottom.removeFromTop(35));preview.setBounds(bottom.removeFromLeft(150));bottom.removeFromLeft(10);apply.setBounds(bottom.removeFromLeft(130));graph.setBounds(r.reduced(0,8));}
};
class CurveTab final:public Button {
    CurvePilot& p;int lane;
public:
    CurveTab(CurvePilot& proc,int i):Button("Courbe "+String(i+1)),p(proc),lane(i){}
    void paintButton(Graphics& g,bool over,bool)override{
        auto r=getLocalBounds().toFloat().reduced(2);g.setColour(getToggleState()?surface:Colour(0xff131b28));g.fillRoundedRectangle(r,8);
        if(getToggleState()||over){g.setColour(laneColour(lane).withAlpha(.7f));g.drawRoundedRectangle(r,8,1);}
        auto l=p.getLane(lane);g.setColour(laneColour(lane));g.setFont(Font(FontOptions(12)));g.drawText(String(lane+1).paddedLeft('0',2),10,8,23,20,Justification::centredLeft);
        g.setColour(ink);g.drawFittedText(l.name,35,8,getWidth()-44,20,Justification::centredLeft,1);
        Path path;for(int k=0;k<=64;++k){float x=12+(getWidth()-24)*k/64.f,y=58-l.curve.y[(size_t)(k*32)]*24;if(k==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        g.setColour(laneColour(lane).withAlpha(p.get(lane,"on")>=.5f?1.f:.4f));g.strokePath(path,PathStrokeType(1.3f));
    }
};
class Editor final:public AudioProcessorEditor,private Timer {
    CurvePilot& p;Theme theme;TooltipWindow tips{this,650};
    std::array<std::unique_ptr<CurveTab>,4> tabs;
    Label title,status,heading,routeHelp,manualLabel,manualHelp,nameLabel,portLabel,channelLabel,ccLabel,modeLabel;
    TextButton send{"ENVOI OFF"},connect{"Connecter"},session{"Session"},fit{"Placer a gauche"};
    TextButton curvePage{"Courbe"},routePage{"Destination"},motionPage{"Decrire / Code"};
    Viewport viewport;Component editBody,routeBody;std::unique_ptr<MotionPanel> motionPanel;
    CurveGraph graph;ToggleButton active{"Active"};TextButton free{"Main levee"},point{"Points"},copy{"Copier"},paste{"Coller"},undo{"Annuler"},lock{"Verrouiller"};ComboBox shape;
    std::array<Slider,8> sliders;std::array<Label,8> labels;std::array<std::unique_ptr<SA>,6> attachments;std::unique_ptr<SA> valueAttachment;std::unique_ptr<BA> activeAttachment;
    TextButton advanced{"Reglages avances"},invert{"Inverser"},vary{"Varier"},smooth{"Lisser"};
    TextEditor name;ComboBox destination,ports,channel;Slider cc,manual;
    TextButton assign{"Assigner la macro dans Live"},resume{"Reprendre la courbe"},midiExport{"Exporter la courbe MIDI"},refreshPorts{"Actualiser les ports"};
    TextButton play{"Lecture"},stop{"Stop / debut"};Slider bpm;
    std::unique_ptr<FileChooser> chooser;Array<MidiDeviceInfo> devices;
    int selected=0,page=0,seen=-1,placementAttempts=0;bool syncing=false,expanded=false,initialFit=false;
public:
    Editor(CurvePilot& owner):AudioProcessorEditor(owner),p(owner),graph(p,0){
        setLookAndFeel(&theme);selected=jlimit(0,3,p.selectedLaneUI);
        for(auto* c:std::initializer_list<Component*>{&title,&send,&connect,&session,&fit,&curvePage,&routePage,&motionPage,&status,&viewport,&play,&stop,&bpm})addAndMakeVisible(c);
        label(title,"CURVE MINI / 4",20,ink);label(status,p.status,12);
        for(int i=0;i<4;++i){tabs[(size_t)i]=std::make_unique<CurveTab>(p,i);addAndMakeVisible(*tabs[(size_t)i]);tabs[(size_t)i]->onClick=[this,i]{select(i);};}
        viewport.setViewedComponent(&editBody,false);viewport.setScrollBarsShown(true,false);
        for(auto* c:std::initializer_list<Component*>{&heading,&graph,&active,&free,&point,&copy,&paste,&undo,&lock,&shape,&advanced,&invert,&vary,&smooth})editBody.addAndMakeVisible(c);
        for(auto* c:std::initializer_list<Component*>{&nameLabel,&name,&portLabel,&ports,&channelLabel,&channel,&ccLabel,&cc,&modeLabel,&destination,&routeHelp,&manualLabel,&manual,&manualHelp,&assign,&resume,&midiExport,&refreshPorts})routeBody.addAndMakeVisible(c);
        connect.onClick=[this]{p.connectOutputs();};send.onClick=[this]{const ScopedLock guard(p.mutex);p.arm(!p.sending);};fit.onClick=[this]{initialFit=false;placementAttempts=0;fitToLeft();};
        connect.setTooltip("Ouvre une seule fois chaque sortie configuree pour les quatre courbes.");
        curvePage.onClick=[this]{showPage(0);};routePage.onClick=[this]{showPage(1);};motionPage.onClick=[this]{showPage(2);};
        session.onClick=[this]{PopupMenu menu;menu.addItem(1,"Exporter la session");menu.addItem(2,"Importer une session");menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&session),[safe=Component::SafePointer<Editor>(this)](int result){if(safe&&result)safe->fileDialog(result);});};
        free.onClick=[this]{p.setPointMode(selected,false);sync();};point.onClick=[this]{p.setPointMode(selected,true);sync();};
        copy.onClick=[this]{p.copyCurve(selected);};paste.onClick=[this]{p.pasteCurve(selected);sync();};undo.onClick=[this]{p.undoCurve(selected);sync();};lock.onClick=[this]{p.toggleLock(selected);sync();};
        shape.addItemList({"Vague","Rampe +","Rampe -","Triangle","Douce","Paliers"},1);shape.setTextWhenNothingSelected("Forme...");shape.onChange=[this]{if(shape.getSelectedId()>0){p.replaceCurve(selected,curve::preset(shape.getSelectedId()-1,p.get(selected,"bars")));shape.setSelectedId(0,dontSendNotification);sync();}};
        const char* titles[]={"Minimum %","Maximum %","Mesures","Phase %","Lissage ms","Profondeur %","Variation %","Lisser dessin %"};
        for(int k=0;k<8;++k){auto& slider=sliders[(size_t)k];editBody.addAndMakeVisible(slider);editBody.addAndMakeVisible(labels[(size_t)k]);label(labels[(size_t)k],titles[k],12);slider.setSliderStyle(Slider::LinearHorizontal);slider.setTextBoxStyle(Slider::TextBoxRight,false,50,24);if(k>=6){slider.setRange(0,100,1);slider.setValue(k==6?5:30);}}
        advanced.onClick=[this]{expanded=!expanded;advanced.setButtonText(expanded?"Masquer les reglages avances":"Reglages avances");resized();};
        invert.onClick=[this]{auto l=p.getLane(selected);l.curve.bars=p.get(selected,"bars");for(auto& y:l.curve.y)y=1-y;p.replaceCurve(selected,l.curve);sync();};vary.onClick=[this]{p.applyVariation(selected,(float)sliders[6].getValue()*.01f);sync();};smooth.onClick=[this]{p.smoothCurve(selected,(float)sliders[7].getValue()*.01f);sync();};
        for(auto* lab:{&nameLabel,&portLabel,&channelLabel,&ccLabel,&modeLabel})lab->setColour(Label::textColourId,muted);
        nameLabel.setText("Nom de la courbe",dontSendNotification);portLabel.setText("Sortie MIDI",dontSendNotification);channelLabel.setText("Canal",dontSendNotification);ccLabel.setText("CC de sortie",dontSendNotification);modeLabel.setText("Destination",dontSendNotification);
        label(manualLabel,"COMMANDE MANUELLE / 0-127",13,ink);label(manualHelp,"Assignable a un potard physique dans Live : Configure, puis Ctrl+M. Le mouvement suspend cette courbe ; Reprendre rend la main au dessin.",12);manualHelp.setMinimumHorizontalScale(1);routeHelp.setMinimumHorizontalScale(1);
        destination.addItem("Macro / parametre Ableton",1);destination.addItem("Machine MIDI externe",2);for(int ch=1;ch<=16;++ch)channel.addItem(String(ch),ch);
        cc.setRange(0,127,1);cc.setSliderStyle(Slider::IncDecButtons);cc.setTextBoxStyle(Slider::TextBoxLeft,false,60,26);
        manual.setRange(0,127,1);manual.setSliderStyle(Slider::LinearHorizontal);manual.setTextBoxStyle(Slider::TextBoxRight,false,50,26);
        name.onFocusLost=[this]{commitDestination();};name.onReturnKey=[this]{commitDestination();};for(auto* combo:{&destination,&ports,&channel})combo->onChange=[this]{commitDestination();};cc.onValueChange=[this]{commitDestination();};
        refreshPorts.onClick=[this]{scan();};assign.onClick=[this]{p.assign(selected);};resume.onClick=[this]{p.resumeCurve(selected);sync();};midiExport.onClick=[this]{fileDialog(0);};
        bpm.setRange(20,300,.1);bpm.setValue(p.standaloneBpm.load());bpm.setSliderStyle(Slider::LinearHorizontal);bpm.setTextBoxStyle(Slider::TextBoxRight,false,70,24);bpm.setTextValueSuffix(" BPM");bpm.onValueChange=[this]{p.standaloneBpm=bpm.getValue();};play.onClick=[this]{p.standalonePlaying=!p.standalonePlaying.load();};stop.onClick=[this]{p.standalonePlaying=false;p.transportReset=true;};
        bool standalone=JUCEApplicationBase::isStandaloneApp();for(auto* c:std::initializer_list<Component*>{&play,&stop,&bpm})c->setVisible(standalone);
        setResizable(true,true);setResizeLimits(560,560,1600,2000);setSize(p.editorWidth,p.editorHeight);scan();select(selected);startTimerHz(15);
    }
    ~Editor()override{stopTimer();viewport.setViewedComponent(nullptr,false);motionPanel.reset();{const ScopedLock guard(p.mutex);p.editorWidth=getWidth();p.editorHeight=getHeight();p.selectedLaneUI=selected;}setLookAndFeel(nullptr);}
    void parentHierarchyChanged()override{fitToLeft();}
    void fitToLeft(){
        if(initialFit||getPeer()==nullptr)return;initialFit=true;
        if(auto* display=Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())){
            auto area=display->userArea;setSize(jmax(560,jmin(720,jmin(getWidth(),roundToInt(area.getWidth()*.52)))),jmax(560,jmin(getHeight(),area.getHeight()-75)));
            if(auto* window=findParentComponentOfClass<DocumentWindow>())window->setTopLeftPosition(area.getX()+10,area.getY()+10);
        }
    }
    void showPage(int next){
        viewport.setViewedComponent(nullptr,false);page=next;
        if(page==2){motionPanel=std::make_unique<MotionPanel>(p,selected);motionPanel->setLookAndFeel(&theme);viewport.setViewedComponent(motionPanel.get(),false);}
        else viewport.setViewedComponent(page==0?&editBody:&routeBody,false);
        curvePage.setToggleState(page==0,dontSendNotification);routePage.setToggleState(page==1,dontSendNotification);motionPage.setToggleState(page==2,dontSendNotification);viewport.setViewPosition(0,0);resized();
    }
    void select(int i){
        selected=i;graph.lane=i;activeAttachment.reset();valueAttachment.reset();for(auto& a:attachments)a.reset();
        activeAttachment=std::make_unique<BA>(p.state,id(i,"on"),active);const char* keys[]={"min","max","bars","offset","smooth","depth"};for(int k=0;k<6;++k)attachments[(size_t)k]=std::make_unique<SA>(p.state,id(i,keys[k]),sliders[(size_t)k]);valueAttachment=std::make_unique<SA>(p.state,id(i,"value"),manual);
        for(int k=0;k<4;++k)tabs[(size_t)k]->setToggleState(k==i,dontSendNotification);sync();showPage(page);
    }
    void scan(){devices=MidiOutput::getAvailableDevices();sync();}
    void sync(){
        syncing=true;auto l=p.getLane(selected);label(heading,String(selected+1).paddedLeft('0',2)+" / "+l.name,19,ink);
        if(!name.hasKeyboardFocus(true))name.setText(l.name,false);destination.setSelectedId(l.hardware?2:1,dontSendNotification);channel.setSelectedId(l.channel,dontSendNotification);cc.setValue(l.cc,dontSendNotification);
        ports.clear(dontSendNotification);ports.addItem("Choisir une sortie...",1);int choice=l.port.isEmpty()?1:0;for(int i=0;i<devices.size();++i){ports.addItem(devices[i].name,i+2);if(devices[i].identifier==l.port)choice=i+2;}if(choice==0){ports.addItem("Indisponible : "+l.port,10000);choice=10000;}ports.setSelectedId(choice,dontSendNotification);
        for(auto& slider:sliders)slider.setColour(Slider::thumbColourId,laneColour(selected));manual.setColour(Slider::thumbColourId,laneColour(selected));active.setColour(ToggleButton::tickColourId,laneColour(selected));free.setToggleState(!l.pointMode,dontSendNotification);point.setToggleState(l.pointMode,dontSendNotification);lock.setButtonText(l.locked?"Deverrouiller":"Verrouiller");
        label(routeHelp,l.hardware?"Le canal choisit la partie de la machine. Le CC choisit le parametre.":"Utiliser un port virtuel avec Remote active dans Live. Ctrl+M : choisir une macro du rack, puis cliquer Assigner la macro dans Live.",12);assign.setVisible(!l.hardware);graph.sync();seen=p.revision.load();syncing=false;resized();
    }
    void commitDestination(){if(syncing)return;auto l=p.getLane(selected);String port=l.port;int chosen=ports.getSelectedId();if(chosen==1)port={};else if(isPositiveAndBelow(chosen-2,devices.size()))port=devices[chosen-2].identifier;p.configureDestination(selected,destination.getSelectedId()==2,port,channel.getSelectedId(),roundToInt(cc.getValue()),name.getText());sync();}
    void fileDialog(int action){
        if(chooser)return;int lane=selected;chooser=std::make_unique<FileChooser>(action==0?"Exporter courbe MIDI":action==1?"Exporter session":"Importer session",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(action==0?"Courbe "+String(lane+1)+".mid":"Curve Mini.curvemini"),action==0?"*.mid":action==1?"*.curvemini":"*.curvemini;*.curvepilot");
        chooser->launchAsync((action==2?FileBrowserComponent::openMode:FileBrowserComponent::saveMode)|FileBrowserComponent::canSelectFiles|(action==2?0:FileBrowserComponent::warnAboutOverwriting),[safe=Component::SafePointer<Editor>(this),action,lane](const FileChooser& dialog){if(!safe)return;auto file=dialog.getResult();if(file!=File{}){if(action==0)safe->p.exportMidi(lane,file);else if(action==1)safe->p.exportSession(file);else safe->p.importSession(file);safe->sync();}MessageManager::callAsync([safe]{if(safe)safe->chooser.reset();});});
    }
    void timerCallback()override{
        fitToLeft();if(placementAttempts<10&&getPeer()){if(positionCurveWindowLeft(getPeer()->getNativeHandle()))placementAttempts=10;else ++placementAttempts;}
        if(seen!=p.revision.load())sync();graph.repaint();for(auto& tab:tabs)tab->repaint();paste.setEnabled(p.hasClipboard());
        const ScopedLock guard(p.mutex);p.selectedLaneUI=selected;p.editorWidth=getWidth();p.editorHeight=getHeight();status.setText(p.status,dontSendNotification);status.setTooltip(p.status);send.setButtonText(p.sending?"ENVOI ON":"ENVOI OFF");send.setColour(TextButton::buttonColourId,p.sending?Colour(0xff186856):Colour(0xff28354b));bool manualMode=p.isManual(selected);resume.setEnabled(manualMode);manualLabel.setText(manualMode?"MANUEL / COURBE SUSPENDUE":"COMMANDE MANUELLE / 0-127",dontSendNotification);play.setButtonText(p.standalonePlaying?"Pause":"Lecture");
    }
    void paint(Graphics& g)override{g.fillAll(bg);g.setColour(edge);g.drawHorizontalLine(73,16,(float)getWidth()-16);}
    void resized()override{
        auto r=getLocalBounds().reduced(16);auto header=r.removeFromTop(44);session.setBounds(header.removeFromRight(75).reduced(2,7));send.setBounds(header.removeFromRight(105).reduced(2,7));connect.setBounds(header.removeFromRight(95).reduced(2,7));title.setBounds(header);r.removeFromTop(14);
        auto curves=r.removeFromTop(72);int tabWidth=curves.getWidth()/4;for(int i=0;i<4;++i)tabs[(size_t)i]->setBounds(curves.removeFromLeft(tabWidth));r.removeFromTop(12);
        auto nav=r.removeFromTop(30);curvePage.setBounds(nav.removeFromLeft(80));nav.removeFromLeft(4);routePage.setBounds(nav.removeFromLeft(105));nav.removeFromLeft(4);motionPage.setBounds(nav.removeFromLeft(125));fit.setBounds(nav.removeFromRight(125));
        status.setBounds(r.removeFromBottom(28));if(JUCEApplicationBase::isStandaloneApp()){auto row=r.removeFromBottom(32);play.setBounds(row.removeFromLeft(85));stop.setBounds(row.removeFromLeft(100));bpm.setBounds(row);}
        r.removeFromTop(15);viewport.setBounds(r);int w=jmax(480,viewport.getMaximumVisibleWidth());editBody.setSize(w,expanded?900:610);routeBody.setSize(w,700);if(motionPanel)motionPanel->setSize(w,650);
        auto e=editBody.getLocalBounds().reduced(6);auto head=e.removeFromTop(38);active.setBounds(head.removeFromRight(90));heading.setBounds(head);e.removeFromTop(12);auto modes=e.removeFromTop(30);free.setBounds(modes.removeFromLeft(110));modes.removeFromLeft(6);point.setBounds(modes.removeFromLeft(80));modes.removeFromLeft(12);shape.setBounds(modes);e.removeFromTop(10);
        auto tools=e.removeFromTop(28);for(auto* button:{&copy,&paste,&undo,&lock})button->setBounds(tools.removeFromLeft((w-12)/4).reduced(2,0));e.removeFromTop(14);graph.setBounds(e.removeFromTop(275));e.removeFromTop(20);auto primary=e.removeFromTop(62);int col=primary.getWidth()/3;for(int k=0;k<3;++k){auto area=primary.removeFromLeft(col);labels[(size_t)k].setBounds(area.removeFromTop(20));sliders[(size_t)k].setBounds(area.reduced(3,0));}e.removeFromTop(14);advanced.setBounds(e.removeFromTop(30));e.removeFromTop(15);
        for(int k=3;k<8;++k){labels[(size_t)k].setVisible(expanded);sliders[(size_t)k].setVisible(expanded);}if(expanded)for(int row=0;row<3;++row){auto band=e.removeFromTop(60);for(int x=0;x<2;++x){int k=3+row*2+x;if(k>=8)break;auto area=band.removeFromLeft((w-12)/2);labels[(size_t)k].setBounds(area.removeFromTop(19));sliders[(size_t)k].setBounds(area.reduced(3,0));}}
        auto actions=e.removeFromTop(32);for(auto* button:{&invert,&vary,&smooth}){button->setVisible(expanded);if(expanded)button->setBounds(actions.removeFromLeft((w-12)/3).reduced(2,0));}
        auto d=routeBody.getLocalBounds().reduced(10);auto field=[&](Label& lab,Component& control){lab.setBounds(d.removeFromTop(22));control.setBounds(d.removeFromTop(32));d.removeFromTop(12);};field(modeLabel,destination);field(nameLabel,name);field(portLabel,ports);
        refreshPorts.setBounds(d.removeFromTop(28).removeFromRight(175));d.removeFromTop(12);auto addresses=d.removeFromTop(55);auto ch=addresses.removeFromLeft(addresses.getWidth()/2-8);addresses.removeFromLeft(16);channelLabel.setBounds(ch.removeFromTop(22));channel.setBounds(ch);ccLabel.setBounds(addresses.removeFromTop(22));cc.setBounds(addresses);
        routeHelp.setBounds(d.removeFromTop(60));assign.setBounds(d.removeFromTop(32));d.removeFromTop(24);manualLabel.setBounds(d.removeFromTop(30));manual.setBounds(d.removeFromTop(35));resume.setBounds(d.removeFromTop(32));manualHelp.setBounds(d.removeFromTop(65));d.removeFromTop(16);midiExport.setBounds(d.removeFromTop(34));
    }
};
}
AudioProcessorEditor* CurvePilot::createEditor(){return new Editor(*this);}
