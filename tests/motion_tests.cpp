#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../Source/MotionLanguage.h"
#include <cassert>
#include <iostream>
int main(){
 auto d=motion::translate("Ouverture sèche haute, fermeture de trois quarts, va-et-vient rapide très serré, puis réouverture complète et fermeture lente",4,0);
 assert(d.data.bars==4);assert(d.data.y.front()==0);assert(d.data.y.back()==0);
 for(float y:d.data.y)assert(std::isfinite(y)&&y>=0&&y<=1);
 auto explicitTime=motion::translate("Ouvre jusqu'à 80 % pendant 2 temps puis ferme sur 2 temps",1,0);
 assert(std::abs(explicitTime.data.y[1024]-.8)<1e-5);assert(explicitTime.data.y.back()==0);
 auto wave=motion::translate("Oscille autour de 50 % amplitude 10 % 8 cycles",4,.5);
 assert(wave.explanation.find("8 oscillations")!=std::string::npos);
 auto saw=motion::translate("Dents de scie jusqu'a 90 % 8 cycles",4,.1);
 assert(saw.script.find("saw from 10")!=std::string::npos);assert(std::abs(saw.data.y.back()-.1)<1e-5);
 auto half=motion::translate("Ferme de trois quarts",1,.8);assert(std::abs(half.data.y.back()-.2)<1e-5);
 auto decimal=motion::translate("Ouvre a 62,5 % pendant 1 mesure",1,0);assert(std::abs(decimal.data.y.back()-.625)<1e-5);
 auto hold=motion::translate("Maintiens pendant 1 temps",1,.4);for(float y:hold.data.y)assert(std::abs(y-.4)<1e-6);
 assert(motion::translate("Ouvre rapidement puis ferme lentement",4,0).script==motion::translate("Ouvre rapidement puis ferme lentement",4,0).script);
 for(auto bad:{"", "fais un son spatial", "ouvre vers Mars", "ouvre a 120 %", "ouvre a -10 %", "ouvre lentement rapidement", "oscille", "ouvre pendant 8 mesures", "ouvre a 30 % pendant 4 mesures puis ferme", "ouvre a nan %", "system rm", "ouvre viteee", "ouvre a 20 % 0 cycles"}){
  bool failed=false;try{motion::translate(bad,4,0);}catch(const std::exception&){failed=true;}if(!failed)std::cerr<<"Unexpected success: "<<bad<<'\n';assert(failed);
 }
 std::cout<<"PASS: French movements, accents, durations, fractions, waves, saw, bounds, errors, deterministic DSL\n";
}
