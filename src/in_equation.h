#ifndef HAVE_IN_EQUATION_H
#define HAVE_IN_EQUATION_H

#include "equation.h"
#include "in.h"
#include "eq.h"

#include <list>

// This is an calculated in. Typically a top level object. It is constructed after interface ins.
// JCE, 6-3-2022


class in_equation: public in{
	public:
		in_equation(const std::string, const std::string name = "", const std::string units = "", const unsigned int decimals = 0); // descr, name, units, decimals in floating point representation.
		~in_equation();		

		in_equation(const in_equation& obj_temp) = delete;
		in_equation& operator=(const in_equation& temp_obj) = delete;

        bool getDataOrNaN(std::deque<record>&, double from, double to, unsigned num);
		void getDataSummary(vector<flStat>&, unsigned int size, double from, double to);
        void getData(map<double, float>& data, double from, double to);
    
		//void call_on_equation_result_update(); 	CC(in_equation, call_on_equation_result_update); 
		//void call_on_equation_result_change(); 	CC(in_equation, call_on_equation_result_change); 
		//void call_on_turn_valid();			 	CC(in_equation, call_on_turn_valid); 
		//void call_on_turn_invalid();			CC(in_equation, call_on_turn_invalid); 
        void updateValue();                     CC(in_equation, updateValue);

        const std::vector<eqVarDescr> getVardes();
        const eqNode* getEq();
    private:
        std::vector<eqVarDescr> vardes;
        eqNode *eq;  
        void registerCallbacks();
    friend void buildInEquations(json_t *json);
	};

// Feed this the part of the config.json that contains the array with in_equation descriptions.
void buildInEquations(json_t *json);

// List of in_equations in the order of constructing. Usefull for cleanup.
extern mutex ineqmap_mutex;
extern std::map<std::string, in_equation*> in_eq_map;
in_equation* getInEq(std::string);

#endif // HAVE_IN_EQUATION_H
