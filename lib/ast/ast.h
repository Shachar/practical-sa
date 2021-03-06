/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef AST_AST_H
#define AST_AST_H

#include <practical/practical.h>

#include "parser.h"
#include "ast/lookup_context.h"
#include "ast/module.h"

namespace AST {

extern JumpPointId::Allocator<> jumpPointAllocator;

class AST {
    static LookupContext builtinCtx;
    static bool _prepared;
    Module::Ptr module;

public:
    static void prepare( BuiltinContextGen *ctxGen );
    static bool prepared();

    static const LookupContext &getBuiltinCtx() {
        return builtinCtx;
    }

    void codeGen(const NonTerminals::Module &module, PracticalSemanticAnalyzer::ModuleGen *codeGen);

private:
    static void registerBuiltinTypes( BuiltinContextGen *ctxGen );
};

} // End namespace AST

#endif // AST_AST_H
