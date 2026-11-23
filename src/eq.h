// Equation object(s)

#ifndef HAVE_EQ_H
#define HAVE_EQ_H

#include "jansson.h"
#include <vector>

#include "in.h"

enum eqOp { INIT, CONST, IN, ADD, SUB, MUL, DIV, IF, MIN, MAX, AVG, AND, OR, XOR, NOT, ME, EQ, LE, NE, MORE, LESS };

typedef struct s_eqVarDescr
{
    std::string descr;
    in* i;
    int idx;
} eqVarDescr;

class eqNode;
// Creates the equation tree, and corresponding eqVarDescr, based on a json.
eqNode* eqFromJson(json_t*, std::vector<eqVarDescr>&);

class eqNode
{
public:
	eqNode();
	~eqNode();
    float eval(const std::vector<float>&);
private:
    eqOp op = CONST;
    float cval = 0.0;   // Constant for CONST op
    unsigned idx = 0;   // Index for IN op
    std::vector<eqNode*> children;
    friend eqNode* eqFromJson(json_t*, std::vector<eqVarDescr>&);    
};


#endif // HAVE_EQ_H
