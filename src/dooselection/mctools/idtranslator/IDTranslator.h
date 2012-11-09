#ifndef IDTRANSLATOR_H
#define IDTRANSLATOR_H

#include <string>
#include <cmath>

#include "TROOT.h"


namespace IDTranslator{
  std::string convertMCID(Float_t ID, bool real_names, bool only_abs);
  int checkparticle (Float_t decaymatrixelement);
  std::string makedecaystring (Float_t decaymatrix[][25], int rows, int columns, int row, int column, std::string decaystring, bool real_names, bool only_abs);
}

#endif //IDTRANSLATOR_H