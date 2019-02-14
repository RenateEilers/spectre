#include "Problem.hpp"

#include <iostream>
#include <cassert>

#include "Output.hpp"

namespace logic {
    
    void Problem::outputSMTLIB(std::ostream& ostr)
    {       

        ostr << "(set-option :produce-unsat-cores true)\n";
        // output sort declarations
        for(const auto& pair : Sorts::nameToSort())
        {
            ostr << declareSortSMTLIB(*pair.second);
        }
        
        // output symbol definitions
        for (const auto& pairStringSymbol : Signature::signature())
        {
            ostr << pairStringSymbol.second->declareSymbolSMTLIB();
        }
        
        // output each axiom
        int ctr = 0;
        for (const auto& axiom : axioms)
        {
            ostr << "\n(assert\n (!\n" << axiom->toSMTLIB(3) + "\n :named aixiom" << ctr << "))\n";
        }

        // output each lemma
        ctr = 0;
        for (const auto& lemma : lemmas)
        {
            // TODO: improve handling for lemmas:
            // custom smtlib-extension
            ostr << "\n(assert\n (!\n" << lemma->toSMTLIB(3) + "\n :named lemma" << ctr << "))\n";
            ctr++;
        }
        
        // output conjecture
        assert(conjecture != nullptr);
        ostr << "\n(assert-not\n" << conjecture->toSMTLIB(3) + "\n)\n";

        ostr << "(check-sat)\n";
        ostr << "(get-unsat-core)\n";
    }
}
