#include "CodeBuilder.h"

Code::Code(char code, short size)
{
    this->code = code;
    this->size = size;
}

unsigned char CodeBuilder::addCode(Code &code)
{
    int available_bits = (8 - this->size);

    if (available_bits < code.size) {
        
    }
}
