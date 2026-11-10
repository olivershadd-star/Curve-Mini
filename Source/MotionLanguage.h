#pragma once
#include "Curve.h"
#include <iomanip>
#include <locale>
#include <map>
#include <optional>
#include <set>
namespace motion {
struct Result {curve::Data data;std::string script,explanation;};
inline std::string normalise(std::string text){
    const std::pair<const char*,const char*> accents[]={{"é","e"},{"è","e"},{"ê","e"},{"ë","e"},{"É","e"},{"à","a"},{"â","a"},{"À","a"},{"î","i"},{"ï","i"},{"ô","o"},{"ö","o"},{"ù","u"},{"û","u"},{"ü","u"},{"ç","c"},{"œ","oe"},{"’"," "}};
    for(auto pair:accents){size_t pos=0;while((pos=text.find(pair.first,pos))!=std::string::npos){text.replace(pos,std::string(pair.first).size(),pair.second);pos+=std::string(pair.second).size();}}
    for(size_t k=0;k<text.size();++k){auto c=(unsigned char)text[k];if(c>='A'&&c<='Z')text[k]=(char)(c-'A'+'a');if(text[k]==','&&k>0&&k+1<text.size()&&std::isdigit((unsigned char)text[k-1])&&std::isdigit((unsigned char)text[k+1]))text[k]='.';else if(text[k]==','||text[k]==';'||text[k]=='\n'||text[k]=='\r'||(text[k]=='.'&&!(k>0&&k+1<text.size()&&std::isdigit((unsigned char)text[k-1])&&std::isdigit((unsigned char)text[k+1]))))text[k]='|';else if(text[k]=='\''||(text[k]=='-'&&!(k+1<text.size()&&std::isdigit((unsigned char)text[k+1])))||text[k]==':')text[k]=' ';}
    return text;
}
inline std::string num(double v){std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(12)<<v;return s.str();}
inline Result translate(const std::string& text,double bars,float initial){
    if(text.size()>4096)throw std::runtime_error("Description trop longue (4096 caracteres maximum).");
    if(!std::isfinite(bars)||bars<.25||bars>32||std::abs(bars*4-std::round(bars*4))>1e-6)throw std::runtime_error("Choisir une duree de 0.25 a 32 mesures, par pas de 0.25.");
    if(!std::isfinite(initial)||initial<0||initial>1)throw std::runtime_error("Depart attendu entre 0 et 100 %.");
    std::istringstream input(normalise(text));std::vector<std::string> words;std::string word;
    while(input>>word){size_t at=0,next;while((next=word.find('|',at))!=std::string::npos){if(next>at)words.push_back(word.substr(at,next-at));words.push_back("|");at=next+1;}if(at<word.size())words.push_back(word.substr(at));}
    if(words.empty())throw std::runtime_error("Decrire un mouvement : ouvre, ferme, maintiens, oscille, dents de scie.");
    const std::map<std::string,std::string> verbs={{"ouvre","up"},{"ouvrir","up"},{"ouverture","up"},{"monte","up"},{"monter","up"},{"reouvre","up"},{"reouverture","up"},{"ferme","down"},{"fermer","down"},{"fermeture","down"},{"descends","down"},{"descend","down"},{"descendre","down"},{"maintiens","hold"},{"maintenir","hold"},{"maintien","hold"},{"reste","hold"},{"oscille","wave"},{"osciller","wave"},{"oscillation","wave"},{"oscillations","wave"},{"va","wave"},{"dents","saw"},{"dent","saw"}};
    const std::map<std::string,double> numbers={{"zero",0},{"un",1},{"une",1},{"deux",2},{"trois",3},{"quatre",4},{"cinq",5},{"six",6},{"sept",7},{"huit",8},{"neuf",9},{"dix",10},{"douze",12},{"seize",16}};
    const std::set<std::string> fillers={"a","au","aux","de","du","des","le","la","les","l","d","en","et","vient","vient","scie","vers","jusqu","pour","cent","%","fois","cycles","cycle","vitesse","une","un","tres","autour","amplitude","profondeur","pendant","sur","temps","mesure","mesures","moitie","quart","quarts","tiers","complet","complete","completement","maximum","max","minimum","min","haut","haute","bas","basse","rapide","rapidement","lent","lente","lentement","sec","seche","brutal","brutale","immediat","immediate","instantane","instantanee","progressif","progressive","progressivement","doux","douce","doucement","courbe","courbee","lineaire","droit","droite","serre","serree","serrees","serres","petit","petite","petites","petits","leger","legere","legerement","large","larges","ample","fort","forte","triangle","sinus","sinusoidale","carree","escalier","cisaille","cisaillee"};
    struct Step {std::string type,mode="linear";std::vector<std::string> w;double duration=-1,weight=1,target=0,depth=.1,cycles=4;bool relative=false,center=false;};
    std::vector<Step> steps;
    for(const auto& token:words){if(token=="|"||token=="puis"||token=="ensuite")continue;auto verb=verbs.find(token);if(verb!=verbs.end()){if(steps.size()>=32)throw std::runtime_error("32 mouvements maximum.");Step s;s.type=verb->second;steps.push_back(s);continue;}if(steps.empty())throw std::runtime_error("Commencer par ouvre, ferme, maintiens, oscille ou dents de scie.");steps.back().w.push_back(token);}
    if(steps.empty())throw std::runtime_error("Aucun mouvement reconnu.");
    double explicitTotal=0,weights=0;
    for(auto& s:steps){auto has=[&](const std::string& value){return std::find(s.w.begin(),s.w.end(),value)!=s.w.end();};
        if(has("carree")||has("escalier")||has("triangle"))throw std::runtime_error("Cette forme n'est pas encore traduite. Utiliser oscille ou dents de scie, ou le menu Forme.");
        bool fast=has("rapide")||has("rapidement"),slow=has("lent")||has("lente")||has("lentement"),instant=has("sec")||has("seche")||has("brutal")||has("brutale")||has("immediat")||has("immediate")||has("instantane")||has("instantanee");
        if((fast&&slow)||(instant&&slow))throw std::runtime_error("Vitesses contradictoires dans un meme mouvement.");
        s.weight=instant?.03:(fast?.5:(slow?3:1));if(s.type=="wave"||s.type=="saw")s.weight=1;
        if(has("courbe")||has("courbee")||has("doux")||has("douce")||has("doucement")||has("progressif")||has("progressive")||has("progressivement"))s.mode="smooth";
        s.target=s.type=="up"?1:0;if(has("haut")||has("haute"))s.target=.8;if(has("bas")||has("basse"))s.target=.2;
        if(has("moitie"))s.target=.5;else if(has("tiers"))s.target=1.0/3;else if(has("quart")||has("quarts"))s.target=has("trois")?.75:.25;
        if(has("maximum")||has("max")||has("complet")||has("complete")||has("completement"))s.target=s.type=="down"?0:1;
        if(has("minimum")||has("min"))s.target=0;
        if(has("cisaille")||has("cisaillee")){s.type="saw";}
        s.relative=!has("jusqu")&&!has("vers")&&(has("de")||has("du"))&&(has("quart")||has("quarts")||has("tiers")||has("moitie"));
        if(has("serre")||has("serree")||has("serrees")||has("serres")||has("petit")||has("petite")||has("petites")||has("petits")||has("leger")||has("legere")||has("legerement"))s.depth=.05;
        if(has("large")||has("larges")||has("ample"))s.depth=.25;
        s.cycles=fast?8:(slow?2:4);if(fast&&has("tres"))s.cycles=16;
        for(size_t k=0;k<s.w.size();++k){const auto& t=s.w[k];bool numeric=false;double v=0;auto n=numbers.find(t);if(n!=numbers.end()){numeric=true;v=n->second;}else{std::string value=t;if(!value.empty()&&value.back()=='%')value.pop_back();if(!value.empty()&&(std::isdigit((unsigned char)value[0])||value[0]=='-'||value[0]=='+')){try{v=curve::number(value);numeric=true;}catch(...){throw std::runtime_error("Nombre non reconnu : "+t);}}}
            if(numeric){auto next=k+1<s.w.size()?s.w[k+1]:"";auto previous=k>0?s.w[k-1]:"";
                if(next=="temps"||next=="mesure"||next=="mesures"){if(s.duration>=0)throw std::runtime_error("Une seule duree par mouvement.");if(v<=0||v>128)throw std::runtime_error("Duree positive attendue.");s.duration=next=="temps"?v/4:v;}
                else if(next=="cycles"||next=="cycle"||next=="fois"){if(v<1||v>64||v!=std::floor(v))throw std::runtime_error("Choisir 1 a 64 cycles entiers.");s.cycles=v;}
                else if(t.back()=='%'||next=="%"||next=="pour"){if(v<0||v>100)throw std::runtime_error("Pourcentage attendu entre 0 et 100.");if(previous=="amplitude"||previous=="profondeur"){s.depth=v*.01;}else{s.target=v*.01;s.center=true;if(previous=="de"&&!has("autour")&&!has("jusqu"))s.relative=true;}}
                else if(next!="quart"&&next!="quarts"&&next!="tiers"&&t!="un"&&t!="une")throw std::runtime_error("Preciser % / temps / mesures / cycles apres "+t);
            }else if(fillers.find(t)==fillers.end())throw std::runtime_error("Mot non compris : '"+t+"'. Decrire un mouvement de bouton ; voir les exemples.");
        }
        if(s.type=="wave"&&has("autour")&&!s.center)throw std::runtime_error("Preciser le centre : oscille autour de 50 %.");
        if(s.duration>=0)explicitTotal+=s.duration;else weights+=s.weight;
    }
    if(explicitTotal>bars+1e-8)throw std::runtime_error("Les durees depassent la longueur choisie.");
    double remaining=bars-explicitTotal;if(weights>0&&remaining<=1e-8)throw std::runtime_error("Il ne reste pas de temps pour les mouvements sans duree.");
    std::ostringstream dsl,summary;dsl.imbue(std::locale::classic());summary.imbue(std::locale::classic());dsl<<"duration: "<<num(bars)<<" bars\nstart: "<<num(initial*100)<<"%\n";double current=initial;
    for(size_t i=0;i<steps.size();++i){auto& s=steps[i];double duration=s.duration>=0?s.duration:remaining*s.weight/weights;
        summary<<i+1<<". ";
        if(s.type=="up"||s.type=="down"){double target=s.relative?curve::clamp((s.type=="up"?current+(1-current)*s.target:current*(1-s.target))):s.target;dsl<<"ramp to "<<num(target*100)<<"% over "<<num(duration)<<" bars "<<s.mode<<"\n";summary<<"Position "<<num(current*100)<<" -> "<<num(target*100)<<" %";current=target;}
        else if(s.type=="hold"){dsl<<"hold over "<<num(duration)<<" bars\n";summary<<"Maintien a "<<num(current*100)<<" %";}
        else {double center=s.center?s.target:current;
            if(s.type=="wave"&&std::abs(center-current)>1e-6){double transition=duration*.1;dsl<<"ramp to "<<num(center*100)<<"% over "<<num(transition)<<" bars smooth\n";duration-=transition;current=center;summary<<"Transition au centre, puis ";}
            if(s.type=="wave"){double depth=std::min(s.depth,std::min(current,1-current));if(depth<1e-6)throw std::runtime_error("Une oscillation au minimum ou maximum n'a pas de place : ajouter autour de 50 % ou une ouverture avant.");dsl<<"wave around "<<num(current*100)<<"% depth "<<num(depth*100)<<"% cycles "<<num(s.cycles)<<" over "<<num(duration)<<" bars sine\n";summary<<s.cycles<<" oscillations autour de "<<num(current*100)<<" %, amplitude "<<num(depth*100)<<" %";}
            else {double target=s.center?s.target:1;if(std::abs(target-current)<1e-6)throw std::runtime_error("Les dents de scie ont besoin de deux positions differentes.");dsl<<"saw from "<<num(current*100)<<"% to "<<num(target*100)<<"% cycles "<<num(s.cycles)<<" over "<<num(duration)<<" bars linear\n";summary<<s.cycles<<" dents de scie "<<num(current*100)<<" -> "<<num(target*100)<<" %";}
        }
        summary<<" / "<<num(s.duration>=0?s.duration:remaining*s.weight/weights)<<" mesures\n";
    }
    if(weights==0&&remaining>1e-8){dsl<<"hold over "<<num(remaining)<<" bars\n";summary<<"Temps restant : maintien final de "<<num(remaining)<<" mesures.\n";}
    summary<<"Sans duree explicite, rapide/lent repartit le temps ; oscillations : rapide/lent choisit le nombre de cycles.\n";
    Result result;result.script=dsl.str();result.data=curve::parse(result.script);result.explanation=summary.str();return result;
}
}
