#include "Plugin.h"
#include "MotionLanguage.h"
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
class Editor final:public AudioProcessorEditor,private Timer,private ListBoxModel {
    CurvePilot& p;Theme theme;TooltipWindow tips{this,700};
    Label title,subtitle,status,heading,routeHeading,routeHelp,manualLabel,manualHelp;
    TextButton connect{"Connecter"},send{"ENVOI OFF"},refreshPorts{"Actualiser"},save{"Exporter session"},load{"Importer session"};
    ListBox list{"Courbes",this};Viewport editViewport,routeViewport;Component editBody,routeBody;
    CurveGraph graph;ToggleButton active{"Ligne active"};TextButton free{"Main levee"},point{"Points"},copy{"Copier"},paste{"Coller"},undo{"Annuler"},lock{"Verrouiller"};
    ComboBox shape;std::array<Slider,8> sliders;std::array<Label,8> labels;
    TextButton advanced{"Reglages avances"},invert{"Inverser"},vary{"Varier"},smooth{"Lisser"},describe{"Decrire / Code"};
    Label nameLabel,portLabel,channelLabel,ccLabel,modeLabel;TextEditor name;ComboBox destination,ports,channel;Slider cc,manual;
    TextButton assign{"Assigner dans Live"},resume{"Reprendre la courbe"},midiExport{"Exporter courbe MIDI"};
    std::array<std::unique_ptr<SA>,6> attachments;std::unique_ptr<SA> valueAttachment;std::unique_ptr<BA> activeAttachment;
    Component::SafePointer<DialogWindow> motionWindow;
    std::unique_ptr<FileChooser> chooser;Array<MidiDeviceInfo> devices;int selected=0,seen=-1;bool syncing=false,expanded=false,initialSizeChecked=false;
    std::array<Label,5> routeLabels;
    TextButton play{"Lecture"},stop{"Stop / debut"};Slider bpm;
public:
    Editor(CurvePilot& owner):AudioProcessorEditor(owner),p(owner),graph(p,0){
        setLookAndFeel(&theme);selected=p.selectedLaneUI;
        for(auto* c:std::initializer_list<Component*>{&title,&subtitle,&status,&connect,&send,&save,&load,&list,&editViewport,&routeViewport,&play,&stop,&bpm})addAndMakeVisible(c);
        label(title,"CURVE MINI",23,ink);label(subtitle,"Courbes / Ableton / MIDI",12);label(status,p.status,12);
        list.setRowHeight(66);list.setColour(ListBox::backgroundColourId,bg);list.setOutlineThickness(0);
        editViewport.setViewedComponent(&editBody,false);routeViewport.setViewedComponent(&routeBody,false);editViewport.setScrollBarsShown(true,false);routeViewport.setScrollBarsShown(true,false);
        for(auto* c:std::initializer_list<Component*>{&heading,&graph,&active,&free,&point,&copy,&paste,&undo,&lock,&shape,&advanced,&invert,&vary,&smooth,&describe})editBody.addAndMakeVisible(c);
        for(auto* c:std::initializer_list<Component*>{&routeHeading,&routeHelp,&manualLabel,&manualHelp,&nameLabel,&name,&portLabel,&ports,&channelLabel,&channel,&ccLabel,&cc,&modeLabel,&destination,&manual,&assign,&resume,&midiExport,&refreshPorts})routeBody.addAndMakeVisible(c);
        label(routeHeading,"DESTINATION",13,ink);label(nameLabel,"Nom de la ligne",12);label(portLabel,"Sortie MIDI",12);label(channelLabel,"Canal",12);label(ccLabel,"CC de sortie",12);label(modeLabel,"Piloter",12);
        label(manualLabel,"COMMANDE MANUELLE / 0-127",12,ink);label(manualHelp,"Assignable a un controleur dans Live via Configure, puis Ctrl+M. Un mouvement prend la main sur cette ligne.",12);manualHelp.setMinimumHorizontalScale(1);
        label(routeHelp,"",12);routeHelp.setMinimumHorizontalScale(1);
        destination.addItem("Macro / parametre Ableton",1);destination.addItem("Machine MIDI",2);
        for(int ch=1;ch<=16;++ch)channel.addItem(String(ch),ch);
        cc.setRange(0,127,1);cc.setSliderStyle(Slider::IncDecButtons);cc.setTextBoxStyle(Slider::TextBoxLeft,false,60,25);
        manual.setSliderStyle(Slider::LinearHorizontal);manual.setTextBoxStyle(Slider::TextBoxRight,false,48,26);manual.setRange(0,127,1);
        name.setSelectAllWhenFocused(true);name.onFocusLost=[this]{commitDestination();};name.onReturnKey=[this]{commitDestination();};
        for(auto* combo:{&destination,&ports,&channel})combo->onChange=[this]{commitDestination();};cc.onValueChange=[this]{commitDestination();};
        connect.onClick=[this]{p.connectOutputs();};refreshPorts.onClick=[this]{scan();};send.onClick=[this]{const ScopedLock guard(p.mutex);p.arm(!p.sending);};
        free.onClick=[this]{p.setPointMode(selected,false);sync();};point.onClick=[this]{p.setPointMode(selected,true);sync();};
        copy.onClick=[this]{p.copyCurve(selected);};paste.onClick=[this]{p.pasteCurve(selected);sync();};undo.onClick=[this]{p.undoCurve(selected);sync();};lock.onClick=[this]{p.toggleLock(selected);sync();};
        shape.addItemList({"Vague","Rampe +","Rampe -","Triangle","Douce","Paliers"},1);shape.setTextWhenNothingSelected("Choisir une forme...");shape.onChange=[this]{if(shape.getSelectedId()>0){p.replaceCurve(selected,curve::preset(shape.getSelectedId()-1,p.get(selected,"bars")));shape.setSelectedId(0,dontSendNotification);sync();}};
        const char* titles[]={"Minimum %","Maximum %","Mesures","Phase %","Lissage ms","Profondeur %","Variation %","Lisser dessin %"};
        for(int k=0;k<8;++k){editBody.addAndMakeVisible(sliders[(size_t)k]);editBody.addAndMakeVisible(labels[(size_t)k]);label(labels[(size_t)k],titles[k],12);auto& s=sliders[(size_t)k];s.setSliderStyle(Slider::LinearHorizontal);s.setTextBoxStyle(Slider::TextBoxRight,false,50,24);if(k>=6){s.setRange(0,100,1);s.setValue(k==6?5:30);}}
        advanced.onClick=[this]{expanded=!expanded;advanced.setButtonText(expanded?"Masquer les reglages avances":"Reglages avances");resized();};
        invert.onClick=[this]{auto l=p.getLane(selected);l.curve.bars=p.get(selected,"bars");for(auto& y:l.curve.y)y=1-y;p.replaceCurve(selected,l.curve);sync();};
        vary.onClick=[this]{p.applyVariation(selected,(float)sliders[6].getValue()*.01f);sync();};smooth.onClick=[this]{p.smoothCurve(selected,(float)sliders[7].getValue()*.01f);sync();};
        describe.onClick=[this]{DialogWindow::LaunchOptions options;auto* panel=new MotionPanel(p,selected);panel->setLookAndFeel(&theme);options.content.setOwned(panel);options.dialogTitle="Decrire / Code — ligne "+String(selected+1);options.dialogBackgroundColour=surface;options.useNativeTitleBar=true;options.resizable=true;if(motionWindow){motionWindow->toFront(true);return;}motionWindow=options.launchAsync();};
        assign.onClick=[this]{p.assign(selected);};resume.onClick=[this]{p.resumeCurve(selected);sync();};midiExport.onClick=[this]{fileDialog(0);};save.onClick=[this]{fileDialog(1);};load.onClick=[this]{fileDialog(2);};
        bpm.setRange(20,300,.1);bpm.setValue(p.standaloneBpm.load());bpm.setSliderStyle(Slider::LinearHorizontal);bpm.setTextBoxStyle(Slider::TextBoxRight,false,65,24);bpm.setTextValueSuffix(" BPM");bpm.onValueChange=[this]{p.standaloneBpm=bpm.getValue();};play.onClick=[this]{p.standalonePlaying=!p.standalonePlaying.load();};stop.onClick=[this]{p.standalonePlaying=false;p.transportReset=true;};
        bool standalone=JUCEApplicationBase::isStandaloneApp();play.setVisible(standalone);stop.setVisible(standalone);bpm.setVisible(standalone);
        setResizable(true,true);setResizeLimits(820,560,3200,2200);setSize(p.editorWidth,p.editorHeight);scan();list.selectRow(selected);select(selected);startTimerHz(15);
    }
    ~Editor()override{stopTimer();if(motionWindow)delete motionWindow.getComponent();{const ScopedLock guard(p.mutex);p.editorWidth=getWidth();p.editorHeight=getHeight();p.selectedLaneUI=selected;}editViewport.setViewedComponent(nullptr,false);routeViewport.setViewedComponent(nullptr,false);setLookAndFeel(nullptr);}
    // Original V2.1 behaviour: fit the first native window to the logical
    // desktop area, so Windows 200/300% scaling does not hide its edges.
    void parentHierarchyChanged()override{if(!initialSizeChecked&&getPeer()!=nullptr){initialSizeChecked=true;if(auto* d=Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds()))setSize(jmax(820,jmin(getWidth(),d->userArea.getWidth()-20)),jmax(560,jmin(getHeight(),d->userArea.getHeight()-70)));}}
    int getNumRows()override{return CurvePilot::laneCount;}
    void paintListBoxItem(int row,Graphics& g,int w,int h,bool chosen)override{
        auto l=p.getLane(row);if(chosen){g.setColour(surface);g.fillRoundedRectangle(3,3,(float)w-6,(float)h-6,8);}g.setColour(laneColour(row));g.setFont(Font(FontOptions(12)));g.drawText(String(row+1).paddedLeft('0',2),12,12,26,20,Justification::centredLeft);
        g.setColour(ink.withAlpha(p.get(row,"on")>=.5f?1.f:.55f));g.drawFittedText(l.name.isEmpty()?"Courbe "+String(row+1):l.name,44,10,w-52,23,Justification::centredLeft,1);
        g.setColour(muted);g.setFont(Font(FontOptions(10)));g.drawText(l.hardware?"Canal "+String(l.channel)+" / CC "+String(l.cc):"Macro Ableton / CC "+String(l.cc),44,33,w-50,20,Justification::centredLeft);
    }
    void selectedRowsChanged(int row)override{if(row>=0)select(row);}
    void select(int row){
        selected=row;graph.lane=row;
        activeAttachment.reset();valueAttachment.reset();for(auto& a:attachments)a.reset();
        activeAttachment=std::make_unique<BA>(p.state,id(row,"on"),active);
        const char* keys[]={"min","max","bars","offset","smooth","depth"};
        for(int k=0;k<6;++k)attachments[(size_t)k]=std::make_unique<SA>(p.state,id(row,keys[k]),sliders[(size_t)k]);
        valueAttachment=std::make_unique<SA>(p.state,id(row,"value"),manual);sync();
    }
    void scan(){devices=MidiOutput::getAvailableDevices();sync();}
    void sync(){
        syncing=true;for(auto& slider:sliders)slider.setColour(Slider::thumbColourId,laneColour(selected));manual.setColour(Slider::thumbColourId,laneColour(selected));active.setColour(ToggleButton::tickColourId,laneColour(selected));auto l=p.getLane(selected);label(heading,String(selected+1).paddedLeft('0',2)+"  /  "+l.name,21,ink);
        if(!name.hasKeyboardFocus(true))name.setText(l.name,false);destination.setSelectedId(l.hardware?2:1,dontSendNotification);channel.setSelectedId(l.channel,dontSendNotification);cc.setValue(l.cc,dontSendNotification);
        ports.clear(dontSendNotification);ports.addItem("Choisir une sortie...",1);int choice=l.port.isEmpty()?1:0;for(int i=0;i<devices.size();++i){ports.addItem(devices[i].name,i+2);if(devices[i].identifier==l.port)choice=i+2;}if(choice==0){ports.addItem("Indisponible : "+l.port,10000);choice=10000;}ports.setSelectedId(choice,dontSendNotification);
        free.setToggleState(!l.pointMode,dontSendNotification);point.setToggleState(l.pointMode,dontSendNotification);lock.setButtonText(l.locked?"Deverrouiller":"Verrouiller");
        label(routeHelp,l.hardware?"Le canal choisit la partie de la machine ; le CC choisit son parametre.":"Choisir un port virtuel avec Remote active dans Live. Ctrl+M : choisir la macro, puis Assigner dans Live.",12);
        assign.setVisible(!l.hardware);graph.sync();seen=p.revision.load();syncing=false;resized();list.repaint();
    }
    void commitDestination(){
        if(syncing)return;auto l=p.getLane(selected);String port=l.port;int i=ports.getSelectedId();if(i==1)port={};else if(isPositiveAndBelow(i-2,devices.size()))port=devices[i-2].identifier;
        p.configureDestination(selected,destination.getSelectedId()==2,port,channel.getSelectedId(),roundToInt(cc.getValue()),name.getText());sync();
    }
    void fileDialog(int action){
        if(chooser)return;int lane=selected;String filter=action==0?"*.mid":action==1?"*.curvemini":"*.curvemini;*.curvepilot";
        chooser=std::make_unique<FileChooser>(action==0?"Exporter un tour de courbe MIDI":action==1?"Exporter la session":"Importer une session",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(action==0?"Courbe "+String(lane+1)+".mid":"Curve Mini.curvemini"),filter);
        chooser->launchAsync((action==2?FileBrowserComponent::openMode:FileBrowserComponent::saveMode)|FileBrowserComponent::canSelectFiles|(action==2?0:FileBrowserComponent::warnAboutOverwriting),[safe=Component::SafePointer<Editor>(this),action,lane](const FileChooser& dialog){
            if(!safe)return;auto file=dialog.getResult();if(file!=File{}){if(action==0)safe->p.exportMidi(lane,file);else if(action==1)safe->p.exportSession(file);else safe->p.importSession(file);safe->sync();}
            MessageManager::callAsync([safe]{if(safe)safe->chooser.reset();});
        });
    }
    void timerCallback()override{
        if(seen!=p.revision.load())sync();graph.repaint();list.repaint();paste.setEnabled(p.hasClipboard());
        const ScopedLock guard(p.mutex);status.setText(p.status,dontSendNotification);status.setTooltip(p.status);send.setButtonText(p.sending?"ENVOI ON":"ENVOI OFF");send.setColour(TextButton::buttonColourId,p.sending?Colour(0xff186856):Colour(0xff28354b));
        bool manualMode=p.isManual(selected);resume.setEnabled(manualMode);manualLabel.setText(manualMode?"MANUEL / COURBE SUSPENDUE":"COMMANDE MANUELLE / 0-127",dontSendNotification);play.setButtonText(p.standalonePlaying?"Pause":"Lecture");
    }
    void paint(Graphics& g)override{g.fillAll(bg);g.setColour(edge);g.drawHorizontalLine(82,20,(float)getWidth()-20);g.drawVerticalLine(list.getRight()+10,98,(float)getHeight()-45);g.setColour(surface.withAlpha(.45f));g.fillRoundedRectangle(routeViewport.getBounds().toFloat(),10);}
    void resized()override{
        auto r=getLocalBounds().reduced(20);auto header=r.removeFromTop(50);title.setBounds(header.removeFromLeft(180));load.setBounds(header.removeFromRight(130).reduced(3,8));save.setBounds(header.removeFromRight(130).reduced(3,8));send.setBounds(header.removeFromRight(108).reduced(3,8));connect.setBounds(header.removeFromRight(108).reduced(3,8));subtitle.setBounds(header);
        r.removeFromTop(22);status.setBounds(r.removeFromBottom(26));auto transport=r.removeFromBottom(JUCEApplicationBase::isStandaloneApp()?34:0);play.setBounds(transport.removeFromLeft(95));stop.setBounds(transport.removeFromLeft(110));bpm.setBounds(transport.removeFromLeft(220));
        list.setBounds(r.removeFromLeft(getWidth()<1100?150:185));r.removeFromLeft(20);auto right=r.removeFromRight(getWidth()<1100?245:265);routeViewport.setBounds(right);r.removeFromRight(18);editViewport.setBounds(r);
        const int w=jmax(320,editViewport.getMaximumVisibleWidth());editBody.setSize(w,expanded?910:650);auto e=editBody.getLocalBounds().reduced(6);auto head=e.removeFromTop(44);active.setBounds(head.removeFromRight(115));heading.setBounds(head);
        e.removeFromTop(16);auto modes=e.removeFromTop(30);free.setBounds(modes.removeFromLeft(100));modes.removeFromLeft(5);point.setBounds(modes.removeFromLeft(72));modes.removeFromLeft(12);shape.setBounds(modes);
        e.removeFromTop(10);auto tools=e.removeFromTop(28);for(auto* button:{&copy,&paste,&undo,&lock}){button->setBounds(tools.removeFromLeft((w-26)/4).reduced(2,0));}
        e.removeFromTop(18);graph.setBounds(e.removeFromTop(300));e.removeFromTop(22);auto primary=e.removeFromTop(62);int col=primary.getWidth()/3;for(int k=0;k<3;++k){auto area=primary.removeFromLeft(col);labels[(size_t)k].setBounds(area.removeFromTop(20));sliders[(size_t)k].setBounds(area.reduced(2,0));}
        e.removeFromTop(18);auto advancedRow=e.removeFromTop(30);describe.setBounds(advancedRow.removeFromRight(125).reduced(2,0));advanced.setBounds(advancedRow.reduced(2,0));e.removeFromTop(14);
        for(int k=3;k<8;++k){labels[(size_t)k].setVisible(expanded);sliders[(size_t)k].setVisible(expanded);}
        if(expanded)for(int row=0;row<3;++row){auto band=e.removeFromTop(61);for(int x=0;x<2;++x){int k=3+row*2+x;if(k>=8)break;auto area=band.removeFromLeft((w-12)/2);labels[(size_t)k].setBounds(area.removeFromTop(19));sliders[(size_t)k].setBounds(area.reduced(2,0));}}
        auto actions=e.removeFromTop(34);for(auto* button:{&invert,&vary,&smooth}){button->setVisible(expanded);if(expanded)button->setBounds(actions.removeFromLeft((w-12)/3).reduced(2,0));}
        routeBody.setSize(jmax(200,routeViewport.getMaximumVisibleWidth()),800);auto d=routeBody.getLocalBounds().reduced(12,4);routeHeading.setBounds(d.removeFromTop(30));d.removeFromTop(12);
        auto field=[&](Label& lab,Component& control){lab.setBounds(d.removeFromTop(22));control.setBounds(d.removeFromTop(32));d.removeFromTop(14);};field(modeLabel,destination);field(nameLabel,name);field(portLabel,ports);
        refreshPorts.setBounds(d.removeFromTop(28));d.removeFromTop(15);auto addresses=d.removeFromTop(58);auto ch=addresses.removeFromLeft(addresses.getWidth()/2-5);addresses.removeFromLeft(10);channelLabel.setBounds(ch.removeFromTop(22));channel.setBounds(ch);ccLabel.setBounds(addresses.removeFromTop(22));cc.setBounds(addresses);
        routeHelp.setBounds(d.removeFromTop(78));assign.setBounds(d.removeFromTop(32));d.removeFromTop(24);manualLabel.setBounds(d.removeFromTop(36));manual.setBounds(d.removeFromTop(36));resume.setBounds(d.removeFromTop(32));manualHelp.setBounds(d.removeFromTop(92));d.removeFromTop(15);midiExport.setBounds(d.removeFromTop(36));
    }
};
}
AudioProcessorEditor* CurvePilot::createEditor(){return new Editor(*this);}
