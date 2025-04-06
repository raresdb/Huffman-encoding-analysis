#ifndef __CODEBUILDER__H__
#define __CODEBUILDER__H__

struct Code
{
    unsigned char code;
    short size;
    
    Code(char code, short size);
};


class CodeBuilder
{
private:
    unsigned char encoded;
    short size;

public:
    unsigned char addCode(Code &code);
};


#endif /* __CODEBUILDER__H__ */