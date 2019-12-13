/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2019 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef PARSER_H
#define PARSER_H

#include "asserts.h"
#include "defines.h"
#include "operators.h"
#include "practical-sa.h"
#include "slice.h"
#include "tokenizer.h"

#include <memory>
#include <variant>
#include <vector>

using namespace PracticalSemanticAnalyzer;

namespace NonTerminals {
    // Base class for all non-terminals.
    struct NonTerminal {
    protected:
        Slice<const Tokenizer::Token> parsedSlice;

    public:
        // This function is not really virtual. It's used this way to force all children to have the same signature
        // Returns how many tokens were consumed
        // Throws parser_error if fails to parse
        virtual size_t parse(Slice<const Tokenizer::Token> source) = 0;

        virtual ~NonTerminal() {}

        Slice<const Tokenizer::Token> getNTTokens() const {
            return parsedSlice;
        }
    };

    struct Identifier : public NonTerminal {
        const Tokenizer::Token *identifier = nullptr;

        size_t parse(Slice<const Tokenizer::Token> source) override final;

        String getName() const {
            return identifier->text;
        }

        size_t getLine() const {
            ASSERT(identifier != nullptr) << "Dereferencing an unparsed identifier";
            return identifier->line;
        }

        size_t getCol() const {
            ASSERT(identifier != nullptr) << "Dereferencing an unparsed identifier";
            return identifier->col;
        }
    };

    struct Type : public NonTerminal {
        Identifier type;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
        size_t getLine() const {
            return type.getLine();
        }
        size_t getCol() const {
            return type.getCol();
        }
    };

    struct Literal : public NonTerminal {
        Tokenizer::Token token;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct Expression;
    struct FunctionArguments : public NonTerminal {
        std::vector<Expression> arguments;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct CompoundExpression;

    struct Expression : public NonTerminal {
        struct FunctionCall {
            std::unique_ptr<Expression> expression;
            FunctionArguments arguments;
        };

        struct UnaryOperator {
            const Tokenizer::Token *op;
            std::unique_ptr<Expression> operand;
        };

        struct BinaryOperator {
            const Tokenizer::Token *op;
            std::unique_ptr<Expression> operand1, operand2;
        };

        std::variant<
                std::unique_ptr<::NonTerminals::CompoundExpression>,
                ::NonTerminals::Literal,
                Identifier,
                UnaryOperator,
                BinaryOperator,
                FunctionCall,
                Type
            > value;
    private:
        mutable std::unique_ptr<Type> altTypeParse;

    public:
        Expression() {}
        Expression( Expression &&that ) : value( std::move(that.value) ), altTypeParse( std::move(that.altTypeParse) )
        {}
        Expression &operator=( Expression &&that ) {
            value = std::move( that.value );
            altTypeParse = std::move( that.altTypeParse );

            return *this;
        }

        explicit Expression( std::unique_ptr<CompoundExpression> &&compoundExpression ) :
            value( std::move(compoundExpression) )
        {}

        size_t parse(Slice<const Tokenizer::Token> source) override final;
        const Type *reparseAsType() const;

    private:
        size_t actualParse(Slice<const Tokenizer::Token> source, size_t level);
        size_t basicParse(Slice<const Tokenizer::Token> source);

        size_t parsePrefixOp(
                Slice<const Tokenizer::Token> source, size_t level, const Operators::OperatorPriority::OperatorsMap &operators);
        size_t parseInfixOp(
                Slice<const Tokenizer::Token> source, size_t level, const Operators::OperatorPriority::OperatorsMap &operators);
        size_t parseInfixR2LOp(
                Slice<const Tokenizer::Token> source, size_t level, const Operators::OperatorPriority::OperatorsMap &operators);
        size_t parsePostfixOp(
                Slice<const Tokenizer::Token> source, size_t level, const Operators::OperatorPriority::OperatorsMap &operators);
    };

    struct VariableDeclBody : public NonTerminal {
        Identifier name;
        Type type;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct VariableDefinition : public NonTerminal {
        VariableDeclBody body;
        std::unique_ptr<Expression> initValue;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct CompoundStatement;

    struct Statement : public NonTerminal {
        struct ConditionalStatement {
            Expression condition;
            std::unique_ptr<Statement> ifClause, elseClause;
        };

        Statement() {}
        explicit Statement( std::unique_ptr<CompoundStatement> &&compoundStatement ) :
            content( std::move(compoundStatement) )
        {}
        explicit Statement( Expression &&expression ) : content( std::move(expression) ) {}

        std::variant<
                std::monostate,
                Expression,
                VariableDefinition,
                ConditionalStatement,
                std::unique_ptr<CompoundStatement>
            > content;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct StatementList : public NonTerminal {
        std::vector<Statement> statements;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct CompoundExpression : public NonTerminal {
        StatementList statementList;
        Expression expression;

        CompoundExpression() {
        }
        CompoundExpression( StatementList &&statements, Expression &&expression ) :
            statementList( std::move(statements) ),
            expression( std::move(expression) )
        {}
        CompoundExpression( CompoundExpression &&that ) :
            statementList( std::move(that.statementList) ),
            expression( std::move(that.expression) )
        {}

        CompoundExpression &operator=( CompoundExpression &&that ) {
            statementList = std::move( that.statementList );
            expression = std::move( that.expression );

            return *this;
        }

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct CompoundStatement {
        StatementList statements;

        CompoundStatement() {}
        CompoundStatement( StatementList &&statements ) : statements( std::move(statements) ) {}
    };

    struct FuncDeclRet : public NonTerminal {
        Type type;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct FuncDeclArg : public NonTerminal {
        Identifier name;
        Type type;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct FuncDeclArgsNonEmpty : public NonTerminal {
        std::vector<FuncDeclArg> arguments;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct FuncDeclArgs : public NonTerminal {
        std::vector<FuncDeclArg> arguments;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct FuncDeclBody : public NonTerminal {
        Identifier name;
        FuncDeclArgs arguments;
        FuncDeclRet returnType;

        size_t parse(Slice<const Tokenizer::Token> source) override final;
    };

    struct FuncDef : public NonTerminal {
        FuncDeclBody decl;
        CompoundExpression body;

        FuncDef() : body{} {
        }
        FuncDef( FuncDef &&that ) : decl( std::move(that.decl) ), body( std::move(that.body) ) {}

        size_t parse(Slice<const Tokenizer::Token> source) override final;

        String getName() const {
            return decl.name.getName();
        }
    };

    struct Module : public NonTerminal {
        std::vector< FuncDef > functionDefinitions;
        std::vector< Tokenizer::Token > tokens;

        void parse(String source);
        size_t parse(Slice<const Tokenizer::Token> source) override final;
        String getName() const {
            return toSlice("__main");
        }

    };
} // NonTerminals namespace

#endif // PARSER_H
