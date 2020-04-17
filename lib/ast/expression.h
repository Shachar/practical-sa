/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef AST_EXPRESSION_H
#define AST_EXPRESSION_H

#include "ast/expected_result.h"
#include "parser.h"

#include "ast/lookup_context.h"

#include <practical-sa.h>

namespace AST {

class Expression {
    const NonTerminals::Expression &parserExpression;
    PracticalSemanticAnalyzer::ExpressionId id;

public:
    static PracticalSemanticAnalyzer::ExpressionId allocateId();

    explicit Expression( const NonTerminals::Expression &parserExpression );

    void buildAST( LookupContext &lookupContext, ExpectedResult expectedResult );
    void codeGen( PracticalSemanticAnalyzer::FunctionGen *functionGen );
    PracticalSemanticAnalyzer::ExpressionId getId() const {
        return id;
    }
};

} // namespace AST

#endif // AST_EXPRESSION_H
