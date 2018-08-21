#include "Theory.hpp"

#include <cassert>

namespace logic {
    
    std::shared_ptr<const FuncTerm> Theory::intConstant(int i)
    {
        return Terms::funcTerm(Sorts::intSort(), std::to_string(i), {}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::intAddition(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Terms::funcTerm(Sorts::intSort(), "int_plus", {t1,t2}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::intSubtraction(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Terms::funcTerm(Sorts::intSort(), "int_minus", {t1,t2}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::intMultiplication(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Terms::funcTerm(Sorts::intSort(), "int_multiply", {t1,t2}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::intUnaryMinus(std::shared_ptr<const Term> t)
    {
        return Terms::funcTerm(Sorts::intSort(), "int_unary_minus", {t}, true);
    }
    
    std::shared_ptr<const Formula> Theory::intLess(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Formulas::predicateFormula("int_less", {t1,t2}, true);
    }
    
    std::shared_ptr<const Formula> Theory::intLessEqual(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Formulas::predicateFormula("int_less_eq", {t1,t2}, true);
    }

    std::shared_ptr<const Formula> Theory::intGreater(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Formulas::predicateFormula("int_greater", {t1,t2}, true);
    }
    
    std::shared_ptr<const Formula> Theory::intGreaterEqual(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Formulas::predicateFormula("int_greater_eq", {t1,t2}, true);
    }
    
    std::shared_ptr<const Formula> Theory::boolTrue()
    {
        return Formulas::predicateFormula("bool_true", {}, true);
    }
    
    std::shared_ptr<const Formula> Theory::boolFalse()
    {
        return Formulas::predicateFormula("bool_false", {}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::timeZero()
    {
        return Terms::funcTerm(Sorts::timeSort(), "time_zero", {}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::timeSucc(std::shared_ptr<const Term> term)
    {
        return Terms::funcTerm(Sorts::timeSort(), "time_succ", {term}, true);
    }
    
    std::shared_ptr<const FuncTerm> Theory::timePre(std::shared_ptr<const Term> term)
    {
        return Terms::funcTerm(Sorts::timeSort(), "time_pre", {term}, true);
    }
    
    std::shared_ptr<const Formula> Theory::timeSub(std::shared_ptr<const Term> t1, std::shared_ptr<const Term> t2)
    {
        return Formulas::predicateFormula("time_sub", {t1,t2}, true);
    }
    
}

