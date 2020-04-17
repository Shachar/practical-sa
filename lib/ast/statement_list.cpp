/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include "statement_list.h"

#include "statement.h"

namespace AST {

StatementList::StatementList( const NonTerminals::StatementList &statementList ) :
    statementList( statementList )
{
}

void StatementList::codeGen( LookupContext &lookupCtx, PracticalSemanticAnalyzer::FunctionGen *functionGen ) {
    for( const auto &parserStatement : statementList.statements ) {
        Statement statement( parserStatement );

        statement.codeGen( lookupCtx, functionGen );
    }
}

} // namespace AST
