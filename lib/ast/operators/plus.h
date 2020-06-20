/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef AST_OPERATORS_PLUS_H
#define AST_OPERATORS_PLUS_H

#include "ast/expression.h"

#include <practical-sa.h>

namespace AST::Operators {

using PracticalSemanticAnalyzer::ExpressionId;

ExpressionId bPlusCodegen(
        Slice<const Expression>, const LookupContext::Function::Definition *, PracticalSemanticAnalyzer::FunctionGen *);

} // namespace AST::Operators

#endif // AST_OPERATORS_PLUS_H
