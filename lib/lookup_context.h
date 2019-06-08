/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2019 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef LOOKUP_CONTEXT_H
#define LOOKUP_CONTEXT_H

#include "asserts.h"
#include "expression.h"
#include "nocopy.h"
#include "tokenizer.h"

#include <practical-sa.h>

// A Context is anything that may contain further symbolic definitions. This may be a module, a struct, a function or even an
// anonymous block of code.
class LookupContext : private NoCopy {
public:
    class NamedType : public PracticalSemanticAnalyzer::NamedType, private NoCopy {
    private:
        PracticalSemanticAnalyzer::TypeId _id;
        size_t _size;
        const Tokenizer::Token *_name;
        Type _type;
        std::unique_ptr<ValueRange> _range;

    public:
        NamedType(const Tokenizer::Token *name, NamedType::Type type, size_t size);
        NamedType(NamedType &&other);
        NamedType &operator=(NamedType &&other);


        size_t size() const override {
            return _size;
        }

        String name() const override {
            return _name->text;
        }

        Type type() const override {
            return _type;
        }

        PracticalSemanticAnalyzer::TypeId id() const override {
            return _id;
        }

        ValueRange *range() const {
            return _range.get();
        }

        void setRange(FullRangeInt min, FullRangeInt max) {
            ASSERT( !_range )<<"Trying to set range when range is already set to "<<*_range;

            _range = safenew<ValueRange>( min, max );
        }
    };

    class LocalVariable {
    public:
        const Tokenizer::Token *name;
        Expression lvalueExpression; // Expression where variable was defined
        PracticalSemanticAnalyzer::IdentifierId id;

        explicit LocalVariable( const Tokenizer::Token *name );
        LocalVariable( const Tokenizer::Token *name, Expression &&lvalueExpression );
    };

    class Function {
    public:
        const Tokenizer::Token *name;
        PracticalSemanticAnalyzer::IdentifierId id;
        PracticalSemanticAnalyzer::StaticType::Ptr returnType;
        std::vector<PracticalSemanticAnalyzer::ArgumentDeclaration> arguments;

        Function( const Tokenizer::Token *name );
    };

    using NamedObject = std::variant< LocalVariable, Function >;
    
private:
    static std::unordered_map<PracticalSemanticAnalyzer::TypeId, const NamedType *> typeRepository;

    const LookupContext *parent;

    std::unordered_map< String, NamedType > types;
    std::unordered_map< String, NamedObject > symbols;

public:
    LookupContext( const LookupContext *parent );
    ~LookupContext();

    const LookupContext *getParent() const { return parent; }

    NamedType *registerType( const Tokenizer::Token *name, NamedType::Type type, size_t size );
    NamedType *registerType(
            const Tokenizer::Token *name, NamedType::Type type, size_t size, FullRangeInt min, FullRangeInt max );
    Function *registerFunctionPass1( const Tokenizer::Token *name );
    void registerFunctionPass2(
            String name,
            PracticalSemanticAnalyzer::StaticType::Ptr &&returnType,
            std::vector<PracticalSemanticAnalyzer::ArgumentDeclaration> &&arguments);
    const LocalVariable *registerVariable( LocalVariable &&variable );

    const NamedObject *lookupIdentifier(String name) const;
    const NamedType *lookupType(String name) const;

    static const PracticalSemanticAnalyzer::NamedType *lookupType(PracticalSemanticAnalyzer::TypeId id);
};

#endif // LOOKUP_CONTEXT_H
