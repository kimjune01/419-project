//
//  Saver.hpp
//  OpenCV3Import
//
//  Created by June Kim on 2016-11-21.
//  Copyright © 2016 Soulcast-team. All rights reserved.
//

#ifndef FileIO_hpp
#define FileIO_hpp

#include <stdio.h>
#include "GRT/GRT.h"
#include "SpellEnum.hpp"


using namespace GRT;

class FileIO {
    
    
    
public:    
    void appendGesture(VectorFloat frameFloats, Spell spell  );
    void load();
};

#endif /* Saver_hpp */
