#include "Semantics.hpp"

#include <algorithm>
#include <memory>
#include <vector>
#include <cassert>

#include "Sort.hpp"
#include "Term.hpp"
#include "Formula.hpp"
#include "Theory.hpp"

#include "SemanticsHelper.hpp"
#include "SymbolDeclarations.hpp"

namespace analysis {
    
    std::vector<std::shared_ptr<const program::Variable>> intersection(std::vector<std::shared_ptr<const program::Variable>> v1,
                                                                       std::vector<std::shared_ptr<const program::Variable>> v2)
    {
        std::vector<std::shared_ptr<const program::Variable>> v3;
        
        std::sort(v1.begin(), v1.end());
        std::sort(v2.begin(), v2.end());
        
        std::set_intersection(v1.begin(),v1.end(),
                              v2.begin(),v2.end(),
                              back_inserter(v3));
        return v3;
    }

    std::vector<std::shared_ptr<const logic::Formula>> Semantics::generateSemantics()
    {
        // generate semantics compositionally
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;
        for(const auto& function : program.functions)
        {
            std::vector<std::shared_ptr<const logic::Formula>> conjunctsFunction;

            for (const auto& statement : function->statements)
            {
                auto semantics = generateSemantics(statement.get());
                if (twoTraces)
                {
                    auto tr = logic::Signature::varSymbol("tr", logic::Sorts::traceSort());
                    conjunctsFunction.push_back(logic::Formulas::universal({tr}, semantics));
                }
                else
                {
                    conjunctsFunction.push_back(semantics);
                }
            }
            conjuncts.push_back(logic::Formulas::conjunction(conjunctsFunction, "Semantics of function " + function->name));
        }
        
        return conjuncts;
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const program::Statement* statement)
    {
        if (statement->type() == program::Statement::Type::IntAssignment)
        {
            auto castedStatement = static_cast<const program::IntAssignment*>(statement);
            return generateSemantics(castedStatement);
        }
        else if (statement->type() == program::Statement::Type::IfElse)
        {
            auto castedStatement = static_cast<const program::IfElse*>(statement);
            return generateSemantics(castedStatement);
        }
        else if (statement->type() == program::Statement::Type::WhileStatement)
        {
            auto castedStatement = static_cast<const program::WhileStatement*>(statement);
            return generateSemantics(castedStatement);
        }
        else
        {
            assert(statement->type() == program::Statement::Type::SkipStatement);
            auto castedStatement = static_cast<const program::SkipStatement*>(statement);
            return generateSemantics(castedStatement);
        }
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const program::IntAssignment* intAssignment)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;
        
        auto l1 = startTimePointMap.at(intAssignment);
        auto l2 = endTimePointMap.at(intAssignment);
        auto l1Name = l1->symbol->name;
        auto l2Name = l2->symbol->name;
        auto activeVars = intersection(locationToActiveVars.at(l1Name), locationToActiveVars.at(l2Name));

        // case 1: assignment to int var
        if (intAssignment->lhs->type() == program::IntExpression::Type::IntVariableAccess)
        {
            auto castedLhs = std::static_pointer_cast<const program::IntVariableAccess>(intAssignment->lhs);
            
            // lhs(l2) = rhs(l1)
            auto eq = logic::Formulas::equality(toTerm(castedLhs,l2), toTerm(intAssignment->rhs,l1));
            conjuncts.push_back(eq);
            
            for (const auto& var : activeVars)
            {
                if(!var->isConstant)
                {
                    if (!var->isArray)
                    {
                        // forall other active non-const int-variables: v(l2) = v(l1)
                        if (*var != *castedLhs->var)
                        {
                            auto eq = logic::Formulas::equality(toTerm(var,l2), toTerm(var,l1));
                            conjuncts.push_back(eq);
                        }
                    }
                    else
                    {
                        // forall active non-const int-array-variables: forall p. v(l2,p) = v(l1,p)
                        auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                        auto p = logic::Terms::var(pSymbol);
                        auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var,l2,p), toTerm(var,l1,p)));
                        conjuncts.push_back(conjunct);
                    }
                }
            }

            return logic::Formulas::conjunction(conjuncts, "Update variable " + castedLhs->var->name + " at location " + intAssignment->location);
        }
        // case 2: assignment to int-array var
        else
        {
            assert(intAssignment->lhs->type() == program::IntExpression::Type::IntArrayApplication);
            auto application = std::static_pointer_cast<const program::IntArrayApplication>(intAssignment->lhs);
            
            // a(l2, e(l1)) = rhs(l1)
            auto eq1Lhs = toTerm(application->array, l2, toTerm(application->index,l1));
            auto eq1Rhs = toTerm(intAssignment->rhs,l1);
            auto eq1 = logic::Formulas::equality(eq1Lhs, eq1Rhs);
            conjuncts.push_back(eq1);

            // forall positions p. (p!=e(l1) => a(l2,p) = a(l1,p))
            auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
            auto p = logic::Terms::var(pSymbol);
            
            auto premise = logic::Formulas::disequality(p, toTerm(application->index,l1));
            auto eq2 = logic::Formulas::equality(toTerm(application->array, l2, p), toTerm(application->array, l1, p));
            auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::implication(premise, eq2));
            conjuncts.push_back(conjunct);
            
            for (const auto& var : activeVars)
            {
                if(!var->isConstant)
                {
                    if (!var->isArray)
                    {
                        // forall active non-const int-variables: v(l2) = v(l1)
                        auto eq = logic::Formulas::equality(toTerm(var,l2), toTerm(var,l1));
                        conjuncts.push_back(eq);
                    }
                    else
                    {
                        // forall other active non-const int-array-variables: forall p. v(l2,p) = v(l1,p)
                        if (*var != *application->array)
                        {
                            auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                            auto p = logic::Terms::var(pSymbol);
                            auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, l2, p), toTerm(var, l1, p)));
                            conjuncts.push_back(conjunct);
                        }
                    }
                }
            }
            return logic::Formulas::conjunction(conjuncts, "Update array variable " + application->array->name + " at location " + intAssignment->location);
        }
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const program::IfElse* ifElse)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;

        auto lStart = startTimePointMap.at(ifElse);
        auto lLeftStart = startTimePointMap.at(ifElse->ifStatements.front().get());
        auto lRightStart = startTimePointMap.at(ifElse->elseStatements.front().get());

        auto lEnd = endTimePointMap.at(ifElse);
        auto lLeftEnd = endTimePointMap.at(ifElse->ifStatements.back().get());
        auto lRightEnd = endTimePointMap.at(ifElse->elseStatements.back().get());
        
        auto lStartName = lStart->symbol->name;
        auto lLeftEndName = lLeftEnd->symbol->name;
        auto lRightEndName = lRightEnd->symbol->name;
        auto lEndName = lEnd->symbol->name;

        // Part 1: values at the beginning of any branch are the same as at the beginning of the ifElse-statement
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts1;
        
         // don't need to take the intersection with active vars at lLeftStart/lRightStart, since the active vars at lStart are always a subset of those at lLeftStart/lRightStart
        auto activeVars1 = locationToActiveVars.at(lStartName);
        
        for (const auto& var : activeVars1)
        {
            if (!var->isConstant)
            {
                if (!var->isArray)
                {
                    // v(lLeftStart) = v(lStart)
                    auto eq = logic::Formulas::equality(toTerm(var,lLeftStart), toTerm(var,lStart));
                    conjuncts1.push_back(eq);
                }
                else
                {
                    // forall p. v(lLeftStart,p) = v(lStart,p)
                    auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                    auto p = logic::Terms::var(pSymbol);
                    auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, lLeftStart, p), toTerm(var, lStart, p)));
                    conjuncts1.push_back(conjunct);
                }
            }
        }
        for (const auto& var : activeVars1)
        {
            if (!var->isConstant)
            {
                if (!var->isArray)
                {
                    // v(lRightStart) = v(lStart)
                    auto eq = logic::Formulas::equality(toTerm(var,lRightStart), toTerm(var,lStart));
                    conjuncts1.push_back(eq);
                }
                else
                {
                    // forall p. v(lRightStart,p) = v(lStart,p)
                    auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                    auto p = logic::Terms::var(pSymbol);
                    auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, lRightStart, p), toTerm(var, lStart, p)));
                    conjuncts1.push_back(conjunct);
                }
            }
        }
        
        conjuncts.push_back(logic::Formulas::conjunction(conjuncts1, "Jumping into any branch doesn't change the variable values"));
        
        // Part 2: values at the end of the ifElse-statement are either values at the end of the left branch or the right branch,
        // depending on the branching condition
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts2;
        
        auto c = toFormula(ifElse->condition, lStart);
        
        // TODO: activeVars2 and activeVars3 should be the same (not completely sure) and therefore only computed once.
        auto activeVars2 = intersection(locationToActiveVars.at(lLeftEndName), locationToActiveVars.at(lEndName));
        auto activeVars3 = intersection(locationToActiveVars.at(lRightEndName), locationToActiveVars.at(lEndName));
        
        for (const auto& var : activeVars2)
        {
            if(!var->isConstant)
            {
                if (!var->isArray)
                {
                    // condition(lStart) => v(lEnd)=v(lLeftEnd)
                    auto eq1 = logic::Formulas::equality(toTerm(var,lEnd), toTerm(var,lLeftEnd));
                    conjuncts2.push_back(logic::Formulas::implication(c, eq1));
                }
                else
                {
                    auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                    auto p = logic::Terms::var(pSymbol);
                    
                    // condition(lStart) => forall p. v(lEnd,p) = v(lLeftEnd,p)
                    auto conclusion1 = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, lEnd, p), toTerm(var, lLeftEnd, p)));
                    conjuncts2.push_back(logic::Formulas::implication(c, conclusion1));
                }
            }
        }
        for (const auto& var : activeVars3)
        {
            if(!var->isConstant)
            {
                if (!var->isArray)
                {
                    // not condition(lStart) => v(lEnd)=v(lRightEnd)
                    auto eq2 = logic::Formulas::equality(toTerm(var,lEnd), toTerm(var,lRightEnd));
                    conjuncts2.push_back(logic::Formulas::implication(logic::Formulas::negation(c), eq2));
                }
                else
                {
                    auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                    auto p = logic::Terms::var(pSymbol);
                    
                    // not condition(lStart) => forall p. v(lEnd,p) = v(lLeftEnd,p)
                    auto conclusion2 = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, lEnd, p), toTerm(var, lRightEnd, p)));
                    conjuncts2.push_back(logic::Formulas::implication(logic::Formulas::negation(c), conclusion2));
                }
            }
        }
        conjuncts.push_back(logic::Formulas::conjunction(conjuncts2, "The branching-condition determines which values to use after the if-statement"));
        
        // Part 3: collect all formulas describing semantics of branches
        for (const auto& statement : ifElse->ifStatements)
        {
            auto conjunct = generateSemantics(statement.get());
            conjuncts.push_back(conjunct);
        }
        for (const auto& statement : ifElse->elseStatements)
        {
            auto conjunct = generateSemantics(statement.get());
            conjuncts.push_back(conjunct);
        }
        
        return logic::Formulas::conjunction(conjuncts, "Semantics of IfElse at location " + ifElse->location);
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const program::WhileStatement* whileStatement)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;

        auto iSymbol = iteratorSymbol(whileStatement);
        auto i = logic::Terms::var(iSymbol);
        auto n = lastIterationTermForLoop(whileStatement, twoTraces);

        auto lStart0 = startTimePointMap.at(whileStatement);
        
        auto iteratorsItTerms = std::vector<std::shared_ptr<const logic::Term>>();
        auto iteratorsNTerms = std::vector<std::shared_ptr<const logic::Term>>();
        for (const auto& enclosingLoop : *whileStatement->enclosingLoops)
        {
            auto enclosingIterator = iteratorTermForLoop(enclosingLoop);
            iteratorsItTerms.push_back(enclosingIterator);
            iteratorsNTerms.push_back(enclosingIterator);
        }
        iteratorsItTerms.push_back(i);
        iteratorsNTerms.push_back(n);

        auto lStartIt = logic::Terms::func(locationSymbolForStatement(whileStatement), iteratorsItTerms);
        auto lStartN = logic::Terms::func(locationSymbolForStatement(whileStatement), iteratorsNTerms);
        
        auto lBodyStartIt = startTimePointMap.at(whileStatement->bodyStatements.front().get());
        
        auto lEnd = endTimePointMap.at(whileStatement);
        
        auto lStartName = lStart0->symbol->name;
        auto activeVars1 = locationToActiveVars.at(lStartName);
        
        // Part 1: values at the beginning of body are the same as at the beginning of the while-statement
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts1;
        for (const auto& var : locationToActiveVars.at(lStartName))
        {
            if(!var->isConstant)
            {
                if (!var->isArray)
                {
                    // v(lBodyStartIt) = v(lStartIt)
                    auto eq = logic::Formulas::equality(toTerm(var,lBodyStartIt), toTerm(var,lStartIt));
                    conjuncts1.push_back(eq);
                }
                else
                {
                    // forall p. v(lBodyStartIt,p) = v(lStartIt,p)
                    auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                    auto p = logic::Terms::var(pSymbol);
                    auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, lBodyStartIt, p), toTerm(var, lStartIt, p)));
                    conjuncts1.push_back(conjunct);
                }
            }
        }
        conjuncts.push_back(logic::Formulas::universal({iSymbol}, logic::Formulas::conjunction(conjuncts1), "Jumping into the body doesn't change the variable values"));
        
        // Part 2: collect all formulas describing semantics of body
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts2;
        for (const auto& statement : whileStatement->bodyStatements)
        {
            auto conjunct = generateSemantics(statement.get());
            conjuncts2.push_back(conjunct);
        }
        conjuncts.push_back(logic::Formulas::universal({iSymbol}, logic::Formulas::conjunction(conjuncts2), "Semantics of the body"));
        
        // Part 3: Define last iteration
        // Loop condition holds at main-loop-location for all iterations before n
        auto iLessN = logic::Theory::natSub(i, n);
        auto conditionAtLStartIt = toFormula(whileStatement->condition, lStartIt);
        auto imp = logic::Formulas::implication(iLessN, conditionAtLStartIt);
        conjuncts.push_back(logic::Formulas::universal({iSymbol}, imp, "The loop-condition holds always before the last iteration"));
        
        // loop condition doesn't hold at n
        auto negConditionAtN = logic::Formulas::negation(toFormula(whileStatement->condition, lStartN), "The loop-condition doesn't hold in the last iteration");
        conjuncts.push_back(negConditionAtN);
        
        // Part 4: The values after the while-loop are the values from the timepoint with location lStart and iteration n
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts3;
        for (const auto& var : locationToActiveVars.at(lStartName))
        {
            if(!var->isConstant)
            {
                if (!var->isArray)
                {
                    // v(lEnd) = v(lStartN)
                    auto eq = logic::Formulas::equality(toTerm(var,lEnd), toTerm(var,lStartN));
                    conjuncts3.push_back(eq);
                }
                else
                {
                    // forall p. v(lEnd,p) = v(lStartN,p)
                    auto pSymbol = logic::Signature::varSymbol("pos", logic::Sorts::intSort());
                    auto p = logic::Terms::var(pSymbol);
                    auto conjunct = logic::Formulas::universal({pSymbol}, logic::Formulas::equality(toTerm(var, lEnd, p), toTerm(var, lStartN, p)));
                    conjuncts3.push_back(conjunct);
                }
            }
        }
        conjuncts.push_back(logic::Formulas::conjunction(conjuncts3, "The values after the while-loop are the values from the last iteration"));
        
        return logic::Formulas::conjunction(conjuncts, "Loop at location " + whileStatement->location);
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const program::SkipStatement* skipStatement)
    {        
        auto l1 = startTimePointMap.at(skipStatement);
        auto l2 = endTimePointMap.at(skipStatement);

        // identify startTimePoint and endTimePoint
        auto eq = logic::Formulas::equality(l1, l2, "Ignore any skip statement");
        return eq;
    }
}
