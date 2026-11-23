#include "eq.h"

#include <cmath>

//#define DBG(...) printf(__VA_ARGS__);printf("\n");
#define DBG(...)

using namespace std;

eqNode::eqNode() {}

eqNode::~eqNode()
{
    for (auto c : children)
        delete c;
}

float eqNode::eval(const vector<float> &var)
{
    bool first = true;
    bool condition = false;
    bool rv = false;
    float tot = 0.0;
    float v = 0.0;
    float v2 = 0.0;
    switch(op)
    {
    case INIT:
        return NAN;
    case CONST:
        return cval;
    case IN:
        return var[idx];
    case ADD:
        for (auto c : children)
            tot += c->eval(var);
        return tot;
    case SUB:
        for (auto c: children)
        {
            if (first)
            {
                tot = c->eval(var);
                first = false;
            }
            else
                tot -= c->eval(var);
        }
        return tot;
    case MUL:
        for (auto c: children)
        {
            if (first)
            {
                tot = c->eval(var);
                first = false;
            }
            else
                tot *= c->eval(var);
        }
        return tot;
    case DIV:
        for (auto c: children)
        {
            if (first)
            {
                tot = c->eval(var);
                first = false;
            }
            else
                tot /= c->eval(var);
        }
        return tot;
    case IF:
        if (children.size() >=2)
        {
            condition = children[0]->eval(var) >= 0.5;
            if (condition)
                return children[1]->eval(var);
            else
                if (children.size() >= 3)
                    return children[2]->eval(var);
        }
        return NAN;
    case MIN:
        for (auto c: children)
        {
            if (first)
            {
                tot = c->eval(var);
                first = false;
            }
            else
            {
                v = c->eval(var);
                if (v < tot)
                    tot = v;
            }
        }
        return tot;
    case MAX:
        for (auto c: children)
        {
            if (first)
            {
                tot = c->eval(var);
                first = false;
            }
            else
            {
                v = c->eval(var);
                if (v > tot)
                    tot = v;
            }
        }
        return tot;
    case AVG:
        for (auto c: children)
            tot += c->eval(var);
        return tot/children.size();
    case AND:
        for (auto c: children)
        {
            v = c->eval(var);
            if (isnan(v)) return NAN;
            rv |= (v < 0.5);
        }
        return float(!rv);
    case OR:
        for (auto c: children)
        {
            v = c->eval(var);
            if (isnan(v)) return NAN;
            rv |= (v >= 0.5);
        }
        return float(rv);
    case XOR:
        for (auto c: children)
        {
            v = c->eval(var);
            if (isnan(v)) return NAN;
            rv ^= (v >= 0.5);
        }
        return float(rv);
    case NOT:
        v = children[0]->eval(var);
        if (isnan(v)) return NAN;
        return float(v < 0.5);
    case ME:
        v =  children[0]->eval(var);
        v2 = children[1]->eval(var);
        if (isnan(v) or isnan(v2)) return NAN;
        return float(v >= v2);
    case EQ:
        v =  children[0]->eval(var);
        v2 = children[1]->eval(var);
        if (isnan(v) or isnan(v2)) return NAN;
        return float(v == v2);
    case LE:
        v =  children[0]->eval(var);
        v2 = children[1]->eval(var);
        if (isnan(v) or isnan(v2)) return NAN;
        return float(v <= v2);
    case NE:
        v =  children[0]->eval(var);
        v2 = children[1]->eval(var);
        if (isnan(v) or isnan(v2)) return NAN;
        return float(v != v2);
    case MORE:
        v =  children[0]->eval(var);
        v2 = children[1]->eval(var);
        if (isnan(v) or isnan(v2)) return NAN;
        return float(v > v2);
    case LESS:
        v =  children[0]->eval(var);
        v2 = children[1]->eval(var);
        if (isnan(v) or isnan(v2)) return NAN;
        return float(v < v2);     
    }
    return NAN;
}

eqNode* eqFromJson(json_t *json, std::vector<eqVarDescr>& vardes)
{
    eqNode *node;
    DBG("eqFromJson(...) called.");
    if (json_is_true(json))
    {
        node = new eqNode();
        node->op = CONST;
        node->cval = 1.0;
        return node;
    }
    if (json_is_false(json))
    {
        node = new eqNode();
        node->op = CONST;
        node->cval = 0.0;
        return node;
    }         
    if (json_is_number(json))
    {
        node = new eqNode();
        node->op = CONST;
        node->cval = json_number_value(json);
        return node;
    }
    if (json_is_string(json))
    {
        const char *str = json_string_value(json);
        in* i = get_in(string(str));
        if (!i)
        {
            fprintf(stderr, "Failed to find in %s\n", str);
            return nullptr;
        } 
        int idx = vardes.size();        
        vardes.push_back({string(str), i, idx});
        node = new eqNode();
        node->op = IN;
        node->idx = idx;
        return node;
    }
    if (json_is_array(json))
    {
        size_t size = json_array_size(json);
        if (size < 2)
        {
            fprintf(stderr, "Can not construct equation, not enough fields in array\n");
            return nullptr;
        }
        eqOp op = INIT;
        vector<eqNode*> children;
        size_t index;
        json_t *value;
        const char *str;
        json_array_foreach(json, index, value)
        {
            if (index == 0)
            {
                
                if (!json_is_string(value))
                {
                    fprintf(stderr,"Equation array should start with a string.\n");
                    return nullptr;
                }
                str = json_string_value(value);
                if (string(str) == "add" || string(str) == "sum")   op = ADD;    
                if (string(str) == "sub")   op = SUB;    
                if (string(str) == "mul")   op = MUL;    
                if (string(str) == "div")   op = DIV;    
                if (string(str) == "if")    op = IF;    
                if (string(str) == "min")   op = MIN;    
                if (string(str) == "max")   op = MAX;    
                if (string(str) == "avg")   op = AVG;
                if (string(str) == "and")   op = AND;
                if (string(str) == "or")   op = OR;
                if (string(str) == "xor")   op = XOR;
                if (string(str) == "not"|| string(str) == "!")   op = NOT;
                if (string(str) == "me"|| string(str) == ">=")   op = ME;
                if (string(str) == "eq"|| string(str) == "==")   op = EQ;
                if (string(str) == "le"|| string(str) == "<=")   op = LE;
                if (string(str) == "ne"|| string(str) == "!="|| string(str) == "<>")   op = NE;
                if (string(str) == "more"|| string(str) == ">")   op = MORE;
                if (string(str) == "less"|| string(str) == "<")   op = LESS;
                if (op == INIT)
                {
                    fprintf(stderr, "Equation: could not determine function: %s\n", str);
                    return nullptr;
                }
                if (op == NOT && size != 2)
                {
                    fprintf(stderr, "not operator can only have one operand\n");
                    return nullptr;                   
                }
                if (((op == ME) || (op == EQ) || (op == LE) || (op == NE) || (op == MORE) || (op == LESS)) && size != 3)
                {
                    fprintf(stderr, ">=, ==, <=, !=, >, < operators require exactly two operands\n");
                    return nullptr;                   
                }                
            }
            else
            {
                eqNode *c = eqFromJson(value, vardes);    
                if (c == nullptr)
                {
                    for (auto c: children)
                        delete c;
                    return nullptr;
                }
                children.push_back(c);                    
            }
        }
        node = new eqNode();
        node->op = op;
        node->children = children;
        return node;
    }
    fprintf(stderr, "Failed to construct equation: unhandled json type.\n");
    return nullptr;
}
























