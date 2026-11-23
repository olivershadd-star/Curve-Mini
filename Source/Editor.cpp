#include "Plugin.h"
#include <set>
#include "MotionLanguage.h"
#include "WindowPlacement.h"
using namespace juce;
namespace {
using SA=AudioProcessorValueTreeState::SliderAttachment;
using BA=AudioProcessorValueTreeState::ButtonAttachment;
const Colour bg(0xff141819),surface(0xff202729),ink(0xffedf2f0),muted(0xff9baaa7),edge(0xff35413e);
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
    int grid=16;bool snap=true;int brush=0;
    CurveGraph(CurvePilot& proc,int l):p(proc),lane(l),colour(laneColour(l)){setMouseCursor(MouseCursor::CrosshairCursor);}
    void sync(){if(dragging)return;auto a=p.getLane(lane);data=a.curve;points=a.points;pointMode=a.pointMode;locked=a.locked;selected.clear();repaint();}
    Rectangle<float> plot()const{return getLocalBounds().toFloat().reduced(28,22);}
    Point<float> screen(const curve::Point& pt)const{auto r=plot();return {r.getX()+(float)pt.x*r.getWidth(),r.getBottom()-pt.y*r.getHeight()};}
    void paint(Graphics& g)override{
        auto r=plot();rounded(g,getLocalBounds().toFloat(),Colour(0xff121619),6);
        for(int i=0;i<=grid;++i){g.setColour(i%4==0?Colour(0xff354044):Colour(0xff222b2f));float x=r.getX()+r.getWidth()*i/grid;g.drawVerticalLine(roundToInt(x),r.getY(),r.getBottom());}
        for(int i=0;i<=8;++i){g.setColour(Colour(0xff293237));float y=r.getY()+r.getHeight()*i/8;g.drawHorizontalLine(roundToInt(y),r.getX(),r.getRight());}
        g.setColour(muted);g.setFont(Font(FontOptions(10)));g.drawText("127",0,roundToInt(r.getY())-5,25,14,Justification::centredRight);g.drawText("0",0,roundToInt(r.getBottom())-8,25,14,Justification::centredRight);
        g.drawText("0",roundToInt(r.getX()),getHeight()-18,35,14,Justification::centredLeft);g.drawText(String(p.get(lane,"bars"),2)+" mesures",roundToInt(r.getRight())-110,getHeight()-18,110,14,Justification::centredRight);
        Path path;for(int n=0;n<=curve::resolution;++n){float x=r.getX()+(float)n/curve::resolution*r.getWidth(),y=r.getBottom()-data.y[(size_t)n]*r.getHeight();if(n==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        auto fill=path;fill.lineTo(r.getRight(),r.getBottom());fill.lineTo(r.getX(),r.getBottom());fill.closeSubPath();g.setColour(colour.withAlpha(.065f));g.fillPath(fill);g.setColour(locked?muted:ink);g.strokePath(path,PathStrokeType(1.7f));
        if(pointMode&&!preview){for(size_t i=0;i<points.size();++i){auto pos=screen(points[i]);g.setColour(selected.count((int)i)?colour:ink);g.fillEllipse(pos.x-3.5f,pos.y-3.5f,7,7);
            if(i+1<points.size()){double t=(points[i].x+points[i+1].x)*.5;auto h=screen({t,data.y[(size_t)jlimit(0,curve::resolution,roundToInt(t*curve::resolution))]});g.setColour(colour);g.drawEllipse(h.x-3,h.y-3,6,6,1.3f);}}
        }
        if(selecting){g.setColour(colour.withAlpha(.1f));g.fillRect(selection);g.setColour(colour);g.drawRect(selection,1);}
        if(!preview){double phase=p.visualBars.load()/p.get(lane,"bars")+p.get(lane,"offset")*.01;phase-=std::floor(phase);float x=r.getX()+(float)phase*r.getWidth();g.setColour(colour.withAlpha(.7f));g.drawLine(x,r.getY(),x,r.getBottom(),1);}
    }
    void mouseDown(const MouseEvent& e)override{
        if(locked||preview)return;dragIndex=-1;handle=-1;lastX=-1;selecting=false;start=e.position;
        if(pointMode){dragIndex=hit(e.position);
            if(e.mods.isPopupMenu()){if(dragIndex>=0)showSegmentMenu(dragIndex);return;}
            if(e.mods.isAltDown()&&dragIndex>0&&dragIndex<(int)points.size()-1){points.erase(points.begin()+dragIndex);p.replacePoints(lane,points);sync();return;}
            if(dragIndex<0)for(size_t i=0;i+1<points.size();++i){double x=(points[i].x+points[i+1].x)*.5;auto pos=screen({x,data.y[(size_t)roundToInt(x*curve::resolution)]});if(pos.getDistanceFrom(e.position)<10){handle=(int)i;initialTension=points[i].tension;break;}}
            if(handle<0&&dragIndex<0){if(e.mods.isShiftDown()){selecting=true;selection={start.x,start.y,0,0};selected.clear();}else{auto pt=toPoint(e.position,e.mods);points.push_back(pt);points=curve::canonicalPoints(points);for(size_t i=0;i<points.size();++i)if(std::abs(points[i].x-pt.x)<1e-6){dragIndex=(int)i;break;}selected.clear();}}
            if(dragIndex>=0){if(!selected.count(dragIndex)){if(!e.mods.isShiftDown())selected.clear();selected.insert(dragIndex);}original=points;}
        }
        dragging=true;if(!pointMode)draw(e);rebuild();
    }
    void mouseDrag(const MouseEvent& e)override{
        if(!dragging)return;
        if(selecting){selection=Rectangle<float>(start,e.position);selected.clear();for(size_t i=0;i<points.size();++i)if(selection.contains(screen(points[i])))selected.insert((int)i);repaint();return;}
        if(!pointMode){draw(e);return;}
        if(handle>=0){points[(size_t)handle].tension=std::clamp(initialTension+(start.y-e.position.y)/150.0,-1.0,1.0);}
        else if(dragIndex>=0){auto a=toPoint(start,e.mods),b=toPoint(e.position,e.mods);double dx=b.x-a.x,dy=b.y-a.y;
            double lo=-1,hi=1,lowY=-1,highY=1;
            for(int i:selected){auto& q=original[(size_t)i];lowY=std::max(lowY,-(double)q.y);highY=std::min(highY,1.0-q.y);
                if(i==0||i==(int)original.size()-1){lo=std::max(lo,0.0);hi=std::min(hi,0.0);}
                else{lo=std::max(lo,(selected.count(i-1)?0.0:original[(size_t)i-1].x+1e-6)-q.x);hi=std::min(hi,(selected.count(i+1)?1.0:original[(size_t)i+1].x-1e-6)-q.x);}}
            dx=std::clamp(dx,lo,hi);dy=std::clamp(dy,lowY,highY);
            for(int i:selected){points[(size_t)i]=original[(size_t)i];points[(size_t)i].x+=dx;points[(size_t)i].y+=(float)dy;}
        }rebuild();
    }
    void mouseUp(const MouseEvent&)override{if(!dragging)return;dragging=false;if(selecting){selecting=false;repaint();return;}if(pointMode)p.replacePoints(lane,points);else{data.bars=p.get(lane,"bars");p.replaceCurve(lane,data);}repaint();}
    void mouseDoubleClick(const MouseEvent& e)override{if(locked||preview||!pointMode)return;for(size_t i=0;i+1<points.size();++i){double x=(points[i].x+points[i+1].x)*.5;auto h=screen({x,data.y[(size_t)roundToInt(x*curve::resolution)]});if(h.getDistanceFrom(e.position)<10){points[i].tension=0;p.replacePoints(lane,points);sync();break;}}}
    void mouseWheelMove(const MouseEvent&,const MouseWheelDetails& w)override{grid=jlimit(2,64,w.deltaY>0?grid*2:grid/2);repaint();}
    void transformSelection(bool stretch,bool positive){if(locked||preview||!pointMode||selected.size()<2)return;auto first=*selected.begin(),last=*selected.rbegin();double centre=(points[(size_t)first].x+points[(size_t)last].x)*.5;double left=first==0?0:points[(size_t)first-1].x+1e-6,right=last+1==(int)points.size()?1:points[(size_t)last+1].x-1e-6;
        for(int i:selected){auto& q=points[(size_t)i];if(stretch){if(i!=0&&i+1!=(int)points.size())q.x=std::clamp(centre+(q.x-centre)*(positive?1.1:.9),left,right);}else q.y=(float)curve::clamp(q.y+(positive?.08:-.08)*(q.x-centre));}
        points=curve::canonicalPoints(points);p.replacePoints(lane,points);rebuild();}
private:
    int dragIndex=-1,handle=-1,lastX=-1;float lastY=.5;double initialTension=0;bool selecting=false;Point<float> start;Rectangle<float> selection;std::set<int> selected;std::vector<curve::Point> original;
    int hit(Point<float> pos)const{for(size_t i=0;i<points.size();++i)if(screen(points[i]).getDistanceFrom(pos)<10)return (int)i;return -1;}
    curve::Point toPoint(Point<float> position,ModifierKeys mods){auto r=plot();double x=curve::clamp((position.x-r.getX())/jmax(1.f,r.getWidth())),y=curve::clamp((r.getBottom()-position.y)/jmax(1.f,r.getHeight()));if(snap&&!mods.isShiftDown()){x=std::round(x*grid)/grid;y=std::round(y*16)/16;}return {x,(float)y};}
    void rebuild(){if(pointMode)data=curve::fromPoints(points,p.get(lane,"bars"));repaint();}
    void showSegmentMenu(int i){PopupMenu menu;const char* names[]={"Courbe","Courbe en S","Palier","Escalier","Onde","Triangle","Impulsion"};for(int k=0;k<7;++k)menu.addItem(k+1,names[k],i+1<(int)points.size(),points[(size_t)i].shape==k);menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this),[safe=Component::SafePointer<CurveGraph>(this),i](int result){if(!safe||result<=0||i>=(int)safe->points.size())return;safe->points[(size_t)i].shape=result-1;safe->p.replacePoints(safe->lane,safe->points);safe->sync();});}
    void draw(const MouseEvent& e){auto pt=toPoint(e.position,e.mods);int x=jlimit(0,curve::resolution,roundToInt(pt.x*curve::resolution));if(brush){int cell=std::min(grid-1,(int)(pt.x*grid));int begin=cell*curve::resolution/grid,end=(cell+1)*curve::resolution/grid;for(int n=begin;n<=end;++n){double t=(double)(n-begin)/(end-begin);double v=brush==1?t:brush==2?1-std::abs(2*t-1):.5-.5*std::cos(2*curve::pi*t);data.y[(size_t)n]=(float)(pt.y*v);} }
        else if(lastX<0)data.y[(size_t)x]=pt.y;else for(int n=jmin(lastX,x);n<=jmax(lastX,x);++n){float t=x==lastX?1.f:(float)(n-lastX)/(x-lastX);data.y[(size_t)n]=lastY+t*(pt.y-lastY);}lastX=x;lastY=pt.y;repaint();}
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
        if(getWidth()>=110){g.setColour(ink);g.drawFittedText(l.name,35,8,getWidth()-44,20,Justification::centredLeft,1);}
        Path path;for(int k=0;k<=64;++k){float x=12+(getWidth()-24)*k/64.f,y=58-l.curve.y[(size_t)(k*32)]*24;if(k==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        g.setColour(laneColour(lane).withAlpha(p.get(lane,"on")>=.5f?1.f:.4f));g.strokePath(path,PathStrokeType(1.3f));
    }
};
class Editor final:public AudioProcessorEditor,private Timer {
    CurvePilot& p;Theme theme;TooltipWindow tips{this,650};
    std::array<std::unique_ptr<CurveTab>,CurvePilot::laneCount> tabs;
    Label title,status,heading,routeHelp,manualLabel,manualHelp,nameLabel,portLabel,channelLabel,ccLabel,modeLabel;
    TextButton send{"ENVOI OFF"},connect{"Connecter"},session{"Session"},fit{"Placer a gauche"};
    TextButton curvePage{"Courbe"},routePage{"Destination"},motionPage{"Decrire / Code"};
    Viewport viewport;Component editBody,routeBody;std::unique_ptr<MotionPanel> motionPanel;
    ComboBox gridChoice,brushChoice;ToggleButton snapGrid{"Grille"};TextButton stretchOut{"Etirer +"},stretchIn{"Etirer -"},tiltUp{"Incliner +"},tiltDown{"Incliner -"};Label drawHelp;
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
        setLookAndFeel(&theme);selected=jlimit(0,CurvePilot::laneCount-1,p.selectedLaneUI);
        for(auto* c:std::initializer_list<Component*>{&title,&send,&connect,&session,&fit,&curvePage,&routePage,&motionPage,&status,&viewport,&play,&stop,&bpm})addAndMakeVisible(c);
        label(title,"CURVE MINI / 8",20,ink);label(status,p.status,12);
        for(int i=0;i<CurvePilot::laneCount;++i){tabs[(size_t)i]=std::make_unique<CurveTab>(p,i);addAndMakeVisible(*tabs[(size_t)i]);tabs[(size_t)i]->onClick=[this,i]{select(i);};}
        viewport.setViewedComponent(&editBody,false);viewport.setScrollBarsShown(true,false);
        for(auto* c:std::initializer_list<Component*>{&gridChoice,&brushChoice,&snapGrid,&stretchOut,&stretchIn,&tiltUp,&tiltDown,&drawHelp,&heading,&graph,&active,&free,&point,&copy,&paste,&undo,&lock,&shape,&advanced,&invert,&vary,&smooth})editBody.addAndMakeVisible(c);
        for(auto* c:std::initializer_list<Component*>{&nameLabel,&name,&portLabel,&ports,&channelLabel,&channel,&ccLabel,&cc,&modeLabel,&destination,&routeHelp,&manualLabel,&manual,&manualHelp,&assign,&resume,&midiExport,&refreshPorts})routeBody.addAndMakeVisible(c);
        connect.onClick=[this]{p.connectOutputs();};send.onClick=[this]{const ScopedLock guard(p.mutex);p.arm(!p.sending);};fit.onClick=[this]{initialFit=false;placementAttempts=0;fitToLeft();};
        connect.setTooltip("Ouvre une seule fois chaque sortie configuree pour les huit courbes.");
        curvePage.onClick=[this]{showPage(0);};routePage.onClick=[this]{showPage(1);};motionPage.onClick=[this]{showPage(2);};
        const Component::SafePointer<Editor> safeEditor(this);
        session.onClick=[safeEditor]{if(!safeEditor)return;PopupMenu menu;menu.addItem(1,"Exporter la session");menu.addItem(2,"Importer une session");menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&safeEditor->session),[safeEditor](int result){if(safeEditor&&result)safeEditor->fileDialog(result);});};
        free.onClick=[this]{p.setPointMode(selected,false);sync();};point.onClick=[this]{p.setPointMode(selected,true);sync();};
        copy.onClick=[this]{p.copyCurve(selected);};paste.onClick=[this]{p.pasteCurve(selected);sync();};undo.onClick=[this]{p.undoCurve(selected);sync();};lock.onClick=[this]{p.toggleLock(selected);sync();};
        shape.addItemList({"Vague","Rampe +","Rampe -","Triangle","Douce","Paliers"},1);shape.setTextWhenNothingSelected("Forme...");shape.onChange=[this]{if(shape.getSelectedId()>0){p.replaceCurve(selected,curve::preset(shape.getSelectedId()-1,p.get(selected,"bars")));shape.setSelectedId(0,dontSendNotification);sync();}};
        for(int n:{4,8,16,32,64})gridChoice.addItem(String(n)+" divisions",n);gridChoice.setSelectedId(16,dontSendNotification);gridChoice.onChange=[this]{graph.grid=gridChoice.getSelectedId();graph.repaint();};
        snapGrid.setToggleState(true,dontSendNotification);snapGrid.onClick=[this]{graph.snap=snapGrid.getToggleState();};
        brushChoice.addItemList({"Trait libre","Motif rampe","Motif triangle","Motif sinus"},1);brushChoice.setSelectedId(1,dontSendNotification);brushChoice.onChange=[this]{graph.brush=brushChoice.getSelectedId()-1;if(graph.brush){p.setPointMode(selected,false);sync();}};
        stretchOut.onClick=[this]{graph.transformSelection(true,true);};stretchIn.onClick=[this]{graph.transformSelection(true,false);};tiltUp.onClick=[this]{graph.transformSelection(false,true);};tiltDown.onClick=[this]{graph.transformSelection(false,false);};
        label(drawHelp,"Points : poignee = courbure / clic droit = forme / Maj + glisser = selection / Alt + clic = supprimer",11);
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
        for(int k=0;k<CurvePilot::laneCount;++k)tabs[(size_t)k]->setToggleState(k==i,dontSendNotification);sync();showPage(page);
    }
    void scan(){devices=MidiOutput::getAvailableDevices();sync();}
    void sync(){
        syncing=true;auto l=p.getLane(selected);label(heading,String(selected+1).paddedLeft('0',2)+" / "+l.name,19,ink);
        if(!name.hasKeyboardFocus(true))name.setText(l.name,false);destination.setSelectedId(l.hardware?2:1,dontSendNotification);channel.setSelectedId(l.channel,dontSendNotification);cc.setValue(l.cc,dontSendNotification);
        ports.clear(dontSendNotification);ports.addItem("Sortie MIDI vers Ableton",1);int choice=l.port.isEmpty()?1:0;for(int i=0;i<devices.size();++i){ports.addItem(devices[i].name,i+2);if(devices[i].identifier==l.port)choice=i+2;}if(choice==0){ports.addItem("Indisponible : "+l.port,10000);choice=10000;}ports.setSelectedId(choice,dontSendNotification);
        for(auto& slider:sliders)slider.setColour(Slider::thumbColourId,laneColour(selected));manual.setColour(Slider::thumbColourId,laneColour(selected));active.setColour(ToggleButton::tickColourId,laneColour(selected));free.setToggleState(!l.pointMode,dontSendNotification);point.setToggleState(l.pointMode,dontSendNotification);lock.setButtonText(l.locked?"Deverrouiller":"Verrouiller");
        label(routeHelp,l.hardware?"Le canal choisit la partie de la machine. Le CC choisit le parametre.":"Sortie MIDI vers Ableton : router cette piste vers le VST cible, puis utiliser son MIDI Learn pour associer le CC.",12);assign.setVisible(!l.hardware&&l.port.isNotEmpty());graph.sync();seen=p.revision.load();syncing=false;resized();
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
        auto r=getLocalBounds().reduced(16);auto header=r.removeFromTop(38);session.setBounds(header.removeFromRight(75).reduced(2,7));send.setBounds(header.removeFromRight(105).reduced(2,7));connect.setBounds(header.removeFromRight(95).reduced(2,7));title.setBounds(header);r.removeFromTop(6);
        auto curves=r.removeFromTop(60);int tabWidth=curves.getWidth()/CurvePilot::laneCount;for(int i=0;i<CurvePilot::laneCount;++i)tabs[(size_t)i]->setBounds(curves.removeFromLeft(tabWidth));r.removeFromTop(6);
        auto nav=r.removeFromTop(30);curvePage.setBounds(nav.removeFromLeft(80));nav.removeFromLeft(4);routePage.setBounds(nav.removeFromLeft(105));nav.removeFromLeft(4);motionPage.setBounds(nav.removeFromLeft(125));fit.setBounds(nav.removeFromRight(125));
        status.setBounds(r.removeFromBottom(28));if(JUCEApplicationBase::isStandaloneApp()){auto row=r.removeFromBottom(32);play.setBounds(row.removeFromLeft(85));stop.setBounds(row.removeFromLeft(100));bpm.setBounds(row);}
        r.removeFromTop(15);viewport.setBounds(r);int w=jmax(480,viewport.getMaximumVisibleWidth());editBody.setSize(w,expanded?1040:750);routeBody.setSize(w,700);if(motionPanel)motionPanel->setSize(w,650);
        auto e=editBody.getLocalBounds().reduced(6);auto head=e.removeFromTop(38);active.setBounds(head.removeFromRight(90));heading.setBounds(head);e.removeFromTop(12);auto modes=e.removeFromTop(30);free.setBounds(modes.removeFromLeft(110));modes.removeFromLeft(6);point.setBounds(modes.removeFromLeft(80));modes.removeFromLeft(12);shape.setBounds(modes);e.removeFromTop(10);
        auto tools=e.removeFromTop(28);for(auto* button:{&copy,&paste,&undo,&lock})button->setBounds(tools.removeFromLeft((w-12)/4).reduced(2,0));e.removeFromTop(14);auto gridRow=e.removeFromTop(28);gridChoice.setBounds(gridRow.removeFromLeft(125));snapGrid.setBounds(gridRow.removeFromLeft(80));brushChoice.setBounds(gridRow);e.removeFromTop(8);
        auto groupRow=e.removeFromTop(26);for(auto* button:{&stretchOut,&stretchIn,&tiltUp,&tiltDown})button->setBounds(groupRow.removeFromLeft((w-12)/4).reduced(2,0));e.removeFromTop(8);
        graph.setBounds(e.removeFromTop(330));drawHelp.setBounds(e.removeFromTop(30));e.removeFromTop(20);auto primary=e.removeFromTop(62);int col=primary.getWidth()/3;for(int k=0;k<3;++k){auto area=primary.removeFromLeft(col);labels[(size_t)k].setBounds(area.removeFromTop(20));sliders[(size_t)k].setBounds(area.reduced(3,0));}e.removeFromTop(14);advanced.setBounds(e.removeFromTop(30));e.removeFromTop(15);
        for(int k=3;k<8;++k){labels[(size_t)k].setVisible(expanded);sliders[(size_t)k].setVisible(expanded);}if(expanded)for(int row=0;row<3;++row){auto band=e.removeFromTop(60);for(int x=0;x<2;++x){int k=3+row*2+x;if(k>=8)break;auto area=band.removeFromLeft((w-12)/2);labels[(size_t)k].setBounds(area.removeFromTop(19));sliders[(size_t)k].setBounds(area.reduced(3,0));}}
        auto actions=e.removeFromTop(32);for(auto* button:{&invert,&vary,&smooth}){button->setVisible(expanded);if(expanded)button->setBounds(actions.removeFromLeft((w-12)/3).reduced(2,0));}
        auto d=routeBody.getLocalBounds().reduced(10);auto field=[&](Label& lab,Component& control){lab.setBounds(d.removeFromTop(22));control.setBounds(d.removeFromTop(32));d.removeFromTop(12);};field(modeLabel,destination);field(nameLabel,name);field(portLabel,ports);
        refreshPorts.setBounds(d.removeFromTop(28).removeFromRight(175));d.removeFromTop(12);auto addresses=d.removeFromTop(55);auto ch=addresses.removeFromLeft(addresses.getWidth()/2-8);addresses.removeFromLeft(16);channelLabel.setBounds(ch.removeFromTop(22));channel.setBounds(ch);ccLabel.setBounds(addresses.removeFromTop(22));cc.setBounds(addresses);
        routeHelp.setBounds(d.removeFromTop(60));assign.setBounds(d.removeFromTop(32));d.removeFromTop(24);manualLabel.setBounds(d.removeFromTop(30));manual.setBounds(d.removeFromTop(35));resume.setBounds(d.removeFromTop(32));manualHelp.setBounds(d.removeFromTop(65));d.removeFromTop(16);midiExport.setBounds(d.removeFromTop(34));
    }
};
}
AudioProcessorEditor* CurvePilot::createEditor(){return new Editor(*this);}
