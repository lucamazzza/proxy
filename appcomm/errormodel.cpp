#include "errormodel.h"

namespace errormodel {

bool AppCommError::hasError(){
    return true;
}

QString AppCommError::toString(){
    return "";
}

}
