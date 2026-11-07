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
        const ScopedLock lock(p.control);auto& l=p.lanes[(size_t)lane];Path path;
        for(int x=0;x<getWidth();++x){const float phase=(float)x/jmax(1,getWidth()-1);const float value=l.drawing.y[(size_t)jlimit(0,curve::resolution,roundToInt(phase*curve::resolution))];const float y=4+(1-value)*(getHeight()-8);if(x==0)path.startNewSubPath(0,y);else path.lineTo((float)x,y);}
        g.setColour(accent(lane));g.strokePath(path,PathStrokeType(2));
        if(p.armed&&l.enabled){const float x=(float)(pilot::phase(p.displayQuarter,p.displayBeats,l.bars)-std::floor(pilot::phase(p.displayQuarter,p.displayBeats,l.bars)))*getWidth();g.setColour(ink.withAlpha(.6f));g.drawVerticalLine(roundToInt(x),0,(float)getHeight());}
    }
    void mouseDown(const MouseEvent& e)override{last=-1;draw(e);}
    void mouseDrag(const MouseEvent& e)override{draw(e);}
    void draw(const MouseEvent& e){
        const int n=jlimit(0,curve::resolution,roundToInt(e.position.x/jmax(1,getWidth()-1)*curve::resolution));const float y=jlimit(0.0f,1.0f,1-(e.position.y-4)/jmax(1,getHeight()-8));
        {const ScopedLock lock(p.control);auto& data=p.lanes[(size_t)lane].drawing.y;
        if(last<0)data[(size_t)n]=y;else for(int k=jmin(last,n);k<=jmax(last,n);++k)data[(size_t)k]=previous+(y-previous)*(n==last?1.0f:(float)(k-last)/(n-last));}
        last=n;previous=y;repaint();
    }
};
struct Row:Component {
    Pilot& p;int lane;Label title;ToggleButton on{"Active"};TextButton assign{"Assigner"};ComboBox shape;
    std::array<Slider,3> knobs;std::array<Label,3> labels;Graph graph;
    Row(Pilot& processor,int index):p(processor),lane(index),graph(p,index){
        title.setText("LIGNE "+String(index+1)+"   /   CC "+String(pilot::controller(index)),dontSendNotification);title.setColour(Label::textColourId,accent(index));addAndMakeVisible(title);
        addAndMakeVisible(on);on.onClick=[this]{const ScopedLock lock(p.control);p.lanes[(size_t)lane].enabled=on.getToggleState();p.lanes[(size_t)lane].last=-1;};
        addAndMakeVisible(assign);assign.onClick=[this]{p.learn(lane);};
        shape.addItemList({"Vague","Dent montante","Dent descendante","Triangle","Ouverture douce","Escalier"},1);shape.setTextWhenNothingSelected("Forme...");addAndMakeVisible(shape);
        shape.onChange=[this]{const ScopedLock lock(p.control);p.lanes[(size_t)lane].drawing=curve::preset(shape.getSelectedId()-1,p.lanes[(size_t)lane].bars);graph.repaint();};
        const char* names[]={"Minimum %","Maximum %","Mesures"};
        for(int k=0;k<3;++k){auto& knob=knobs[(size_t)k];knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);knob.setTextBoxStyle(Slider::TextBoxBelow,false,72,20);knob.setRange(k==2?.25:0,k==2?32:100,k==2?.25:1);knob.setColour(Slider::rotarySliderFillColourId,accent(index));addAndMakeVisible(knob);labels[(size_t)k].setText(names[k],dontSendNotification);labels[(size_t)k].setJustificationType(Justification::centred);addAndMakeVisible(labels[(size_t)k]);knob.onValueChange=[this,k]{const ScopedLock lock(p.control);auto& l=p.lanes[(size_t)lane];const auto v=knobs[(size_t)k].getValue();if(k==0)l.minimum=(float)v/100;else if(k==1)l.maximum=(float)v/100;else l.bars=v;};}
        addAndMakeVisible(graph);sync();
    }
    void sync(){const ScopedLock lock(p.control);const auto& l=p.lanes[(size_t)lane];on.setToggleState(l.enabled,dontSendNotification);const double values[]={l.minimum*100,l.maximum*100,l.bars};for(int k=0;k<3;++k)if(!knobs[(size_t)k].isMouseButtonDown())knobs[(size_t)k].setValue(values[k],dontSendNotification);graph.repaint();}
    void paint(Graphics& g)override{g.setColour(surface);g.fillRoundedRectangle(getLocalBounds().toFloat(),10);}
    void resized()override{auto r=getLocalBounds().reduced(12);auto head=r.removeFromTop(30);title.setBounds(head.removeFromLeft(180));on.setBounds(head.removeFromLeft(85));assign.setBounds(head.removeFromLeft(100).reduced(3));shape.setBounds(head.reduced(5,0));r.removeFromTop(8);auto controls=r.removeFromRight(270);for(int k=0;k<3;++k){auto c=controls.removeFromLeft(90);labels[(size_t)k].setBounds(c.removeFromTop(20));knobs[(size_t)k].setBounds(c);}graph.setBounds(r.reduced(0,3));}
};
struct Editor:AudioProcessorEditor,private Timer {
    Pilot& p;Label title,hint,status;ComboBox ports;TextButton refresh{"Actualiser"},connectButton{"Connecter"},send{"ENVOI OFF"};Slider tempo;
    Viewport view;Component stack;std::vector<std::unique_ptr<Row>> rows;Array<MidiDeviceInfo> devices;
    Editor(Pilot& processor):AudioProcessorEditor(processor),p(processor){
        title.setText("CURVE MINI  /  PROTOTYPE 0.1",dontSendNotification);title.setColour(Label::textColourId,accent(0));addAndMakeVisible(title);
        hint.setText("12 courbes -> MIDI -> macros Ableton   |   Aucun instrument heberge",dontSendNotification);hint.setColour(Label::textColourId,muted);addAndMakeVisible(hint);
        for(auto* c:std::initializer_list<Component*>{&ports,&refresh,&connectButton,&send,&tempo,&status,&view})addAndMakeVisible(c);
        ports.setTextWhenNothingSelected("Port MIDI virtuel...");refresh.onClick=[this]{scan();};connectButton.onClick=[this]{int i=ports.getSelectedId()-1;if(isPositiveAndBelow(i,devices.size()))p.connect(devices[i].identifier);};send.onClick=[this]{const ScopedLock lock(p.control);p.arm(!p.armed);};
        tempo.setRange(20,300,1);{const ScopedLock lock(p.control);tempo.setValue(p.standaloneTempo);}tempo.setTextValueSuffix(" BPM");tempo.onValueChange=[this]{const ScopedLock lock(p.control);p.standaloneTempo=tempo.getValue();};tempo.setVisible(JUCEApplicationBase::isStandaloneApp());
        for(int i=0;i<pilot::lanes;++i){auto row=std::make_unique<Row>(p,i);stack.addAndMakeVisible(*row);rows.push_back(std::move(row));}view.setViewedComponent(&stack,false);view.setScrollBarsShown(true,false);
        scan();setResizable(true,true);setResizeLimits(820,500,2400,1800);setSize(1100,780);startTimerHz(15);
    }
    ~Editor()override{stopTimer();view.setViewedComponent(nullptr,false);}
    void scan(){const ScopedLock lock(p.control);devices=MidiOutput::getAvailableDevices();ports.clear(dontSendNotification);for(int i=0;i<devices.size();++i){ports.addItem(devices[i].name,i+1);if(devices[i].identifier==p.selectedPort)ports.setSelectedId(i+1,dontSendNotification);}}
    void timerCallback()override{
        {const ScopedLock lock(p.control);send.setButtonText(p.armed?"ENVOI ON":"ENVOI OFF");send.setColour(TextButton::buttonColourId,p.armed?Colour(0xff137c69):surface);status.setText(p.status,dontSendNotification);}
        for(auto& row:rows)if(row->getBounds().intersects(view.getViewArea()))row->sync();
    }
    void paint(Graphics& g)override{g.fillAll(background);}
    void resized()override{auto r=getLocalBounds().reduced(16);title.setBounds(r.removeFromTop(30));hint.setBounds(r.removeFromTop(24));auto bar=r.removeFromTop(36);send.setBounds(bar.removeFromRight(110).reduced(2));connectButton.setBounds(bar.removeFromRight(100).reduced(2));refresh.setBounds(bar.removeFromRight(100).reduced(2));if(tempo.isVisible())tempo.setBounds(bar.removeFromRight(170));ports.setBounds(bar.reduced(2));status.setBounds(r.removeFromBottom(28));r.removeFromTop(10);view.setBounds(r);stack.setSize(jmax(760,view.getMaximumVisibleWidth()),pilot::lanes*170);for(int i=0;i<pilot::lanes;++i)rows[(size_t)i]->setBounds(0,i*170,stack.getWidth(),160);}
};
}
AudioProcessorEditor* Pilot::createEditor(){return new Editor(*this);}
