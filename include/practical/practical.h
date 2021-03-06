/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef PRACTICAL_PRACTICAL_H
#define PRACTICAL_PRACTICAL_H

#include "nocopy.h"
#include "typed.h"
#include "slice.h"

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <variant>

// An unsigned int type long enough to be castable to any int type without losing precision
using LongEnoughInt = std::uintmax_t;
using LongEnoughIntSigned = std::intmax_t;

// Hopefully unique module ID for our typed ids
static constexpr size_t PracticalSAModuleId = 0xc5489da402dc5a84;
DECL_TYPED_NS( PracticalSemanticAnalyzer, ModuleId, unsigned long, 0, PracticalSAModuleId );
DECL_TYPED_NS( PracticalSemanticAnalyzer, ExpressionId, unsigned long, 0, PracticalSAModuleId );
DECL_TYPED_NS( PracticalSemanticAnalyzer, JumpPointId, unsigned long, 0, PracticalSAModuleId );

namespace PracticalSemanticAnalyzer {
    class CompilerArguments {
    public:
    };

    struct SourceLocation {
        unsigned line=0, col=0;

        constexpr bool operator==(const SourceLocation &that) const {
            return line==that.line && col==that.col;
        }
        constexpr bool operator!=(const SourceLocation &that) const {
            return !( (*this)==that );
        }

        SourceLocation &operator++() {
            col++;
            return *this;
        }

        friend std::ostream &operator<<(std::ostream &out, const SourceLocation &location) {
            return out<<location.line<<":"<<location.col;
        }
    };

    // Cookie type used by the backend to identify types. Backend can choose whether to use an integer or a pointer
    union TypeId {
        uintptr_t n;
        void *p;
    };

    class StaticType : private NoCopy, public boost::intrusive_ref_counter<StaticType, boost::thread_unsafe_counter> {
    public:
        using CPtr = boost::intrusive_ptr<const StaticType>;

        struct Flags {
            using Type = uint8_t;
            static constexpr Type
                    Reference = 1<<0,
                    Mutable = 1<<1;
        };

        class Scalar {
        public:
            enum class Type {
                Void, Bool, SignedInt, UnsignedInt, Char
            };

            friend std::ostream &operator<<(std::ostream &out, Type type);

        private:
            size_t size=0, alignment=1;
            TypeId typeId;
            Type type;
            unsigned literalWeight;

        public:
            Scalar( size_t size, size_t alignment, Type type, TypeId typeId, unsigned literalWeight ) :
                size(size), alignment(alignment), typeId(typeId), type(type), literalWeight(literalWeight)
            {}

            virtual String getName() const = 0;

            size_t getSize() const {
                return size;
            }

            size_t getAlignment() const {
                return alignment;
            }

            Type getType() const {
                return type;
            }

            TypeId getTypeId()  const {
                return typeId;
            }

            unsigned getLiteralWeight() const {
                return literalWeight;
            }

            bool operator==( const Scalar &rhs ) const;
        };

        class Function {
        public:
            virtual ~Function() {}

            virtual CPtr getReturnType() const = 0;
            virtual size_t getNumArguments() const = 0;
            virtual CPtr getArgumentType( unsigned index ) const = 0;

            bool operator==( const Function &rhs ) const;
        };

        class Pointer {
        public:
            virtual ~Pointer() {}

            virtual CPtr getPointedType() const = 0;

            bool operator==( const Pointer &rhs ) const;
        };

        class Array {
        public:
            virtual ~Array() {}

            virtual CPtr getElementType() const = 0;
            virtual size_t getNumElements() const = 0;

            size_t getSize() const;
            size_t getAlignment() const;

            bool operator==( const Array &rhs ) const;
        };

        class Struct : public boost::intrusive_ref_counter<Struct, boost::thread_unsafe_counter> {
        public:
            using CPtr = boost::intrusive_ptr<const Struct>;

            struct MemberDescriptor {
                StaticType::CPtr type;
                String name;

                bool operator==(const MemberDescriptor &rhs) const;
                bool operator!=(const MemberDescriptor &rhs) const {
                    return ! operator==(rhs);
                }
            };

            virtual ~Struct() {}

            virtual String getName() const = 0;
            virtual size_t getNumMembers() const = 0;
            virtual MemberDescriptor getMember( size_t index ) const = 0;

            virtual size_t getSize() const = 0;
            virtual size_t getAlignment() const = 0;

            bool operator==( const Struct &rhs ) const;
        };

        using Types = std::variant<
                const Scalar *, const Function *, const Pointer *, const Array *, const Struct *>;

        virtual ~StaticType() {}

        virtual Types getType() const = 0;

        virtual String getMangledName() const = 0;
        virtual size_t getSize() const = 0;
        virtual size_t getAlignment() const = 0;

        static size_t ptrSize();
        static size_t ptrAlignment();

        bool operator==( const StaticType &rhs ) const;
        bool operator!=( const StaticType &rhs ) const {
            return ! (*this==rhs);
        }

        virtual Flags::Type getFlags() const = 0;
        virtual CPtr setFlags( Flags::Type flags ) const = 0;

        CPtr addFlags( Flags::Type moreFlags ) const {
            return setFlags( getFlags() | moreFlags );
        }
        CPtr removeFlags( Flags::Type lessFlags ) const {
            return setFlags( getFlags() & ~lessFlags );
        }
    };
    std::ostream &operator<<(std::ostream &out, StaticType::CPtr type);

    class BuiltinContextGen {
    public:
        virtual TypeId registerVoidType() = 0;
        virtual TypeId registerBoolType() = 0;
        virtual TypeId registerIntegerType( size_t bitSize, size_t alignment, bool _signed ) = 0;
        virtual TypeId registerCharType( size_t bitSize, size_t alignment, bool _signed ) = 0;
    };

    struct ArgumentDeclaration {
        StaticType::CPtr type;
        String name;
        ExpressionId lvalueId;

        ArgumentDeclaration(StaticType::CPtr _type, String _name, ExpressionId _lvalueId)
                : type(_type), name(_name), lvalueId(_lvalueId)
        {}
    };

    /// Callbacks used by the semantic analyzer to allow the SA user to actually generate code
    class FunctionGen {
    public:
        // Function handling
        virtual void functionEnter(
                String name, StaticType::CPtr returnType, Slice<const ArgumentDeclaration> arguments,
                String file, const SourceLocation &location) = 0;
        virtual void functionLeave() = 0;

        virtual void returnValue(ExpressionId id) = 0;
        virtual void returnValue() = 0; // For Void functions

        // Flow control

        // If id==ExpressionId(), then the code gen SHOULD assume that the result of the expression is never used.
        // If elsePoint==JumpPointId(), there is no else clause.
        // practical-sa will generate the jump points, but will not generate the jumps. It is up to the code generation
        // to use the jump points to identify the code flow.
        virtual void conditionalBranch(
                ExpressionId id, StaticType::CPtr type, ExpressionId conditionExpression, JumpPointId elsePoint,
                JumpPointId continuationPoint
            ) = 0;
        // Called twice, once for "then" and once for "else", to signify which expression is that clause's return
        virtual void setConditionClauseResult( ExpressionId id ) = 0;
        virtual void setJumpPoint(JumpPointId id, String name = String()) = 0;
        virtual void jump(JumpPointId destination) = 0;

        // Litarals
        virtual void setLiteral(ExpressionId id, LongEnoughInt value, StaticType::CPtr type) = 0;
        virtual void setLiteral(ExpressionId id, bool value) = 0;
        virtual void setLiteral(ExpressionId id, String value) = 0;
        virtual void setLiteralNull(ExpressionId id, StaticType::CPtr type) = 0;

        // The ExpressionId refers to a pointer to the resulting allocated variable
        virtual void allocateStackVar(ExpressionId id, StaticType::CPtr type, String name) = 0;
        virtual void assign( ExpressionId lvalue, ExpressionId rvalue ) = 0;
        virtual void dereferencePointer( ExpressionId id, StaticType::CPtr type, ExpressionId addr ) = 0;

        // Casts
        virtual void truncateInteger(
                ExpressionId id, ExpressionId source, StaticType::CPtr sourceType, StaticType::CPtr destType ) = 0;
        virtual void changeIntegerSign(
                ExpressionId id, ExpressionId source, StaticType::CPtr sourceType, StaticType::CPtr destType ) = 0;
        virtual void expandIntegerSigned(
                ExpressionId id, ExpressionId source, StaticType::CPtr sourceType, StaticType::CPtr destType ) = 0;
        virtual void expandIntegerUnsigned(
                ExpressionId id, ExpressionId source, StaticType::CPtr sourceType, StaticType::CPtr destType ) = 0;

        // Function calls
        virtual void callFunctionDirect(
                ExpressionId id, String name, Slice<const ExpressionId> arguments, StaticType::CPtr returnType ) = 0;

        // Binary operators
        // Algebraic operators
        virtual void binaryOperatorPlusUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void binaryOperatorPlusSigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void binaryOperatorMinusUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void binaryOperatorMinusSigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void binaryOperatorMultiplyUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void binaryOperatorMultiplySigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void binaryOperatorDivideUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;

        // Comparison operators
        virtual void operatorEquals(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorNotEquals(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorLessThanUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorLessThanSigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorLessThanOrEqualsUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorLessThanOrEqualsSigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorGreaterThanUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorGreaterThanSigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorGreaterThanOrEqualsUnsigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;
        virtual void operatorGreaterThanOrEqualsSigned(
                ExpressionId id, ExpressionId left, ExpressionId right, StaticType::CPtr resultType ) = 0;


        // Unary operators
        virtual void operatorLogicalNot( ExpressionId id, ExpressionId argument ) = 0;
    };

    class ModuleGen {
    public:
        virtual void moduleEnter(ModuleId id, String name, String file, size_t line, size_t col) = 0;
        virtual void moduleLeave(ModuleId id) = 0;

        virtual void declareIdentifier(String name, String mangledName, StaticType::CPtr type) = 0;
        virtual void declareStruct(StaticType::CPtr structType) = 0;
        virtual void defineStruct(StaticType::CPtr structType) = 0;

        virtual std::shared_ptr<FunctionGen> handleFunction() = 0;
    };

    std::unique_ptr<CompilerArguments> allocateArguments();

    // Must be called exactly once, before starting actual compilation
    void prepare( BuiltinContextGen *ctxGen ); // This is the lookup context used for the builtin types
    // XXX Should path actually be a buffer?
    int compile(std::string path, const CompilerArguments *arguments, ModuleGen *codeGen);
} // End namespace PracticalSemanticAnalyzer

namespace std {
    template<>
    struct hash< PracticalSemanticAnalyzer::StaticType > {
        size_t operator()(const PracticalSemanticAnalyzer::StaticType &type) const;
    };

    template<>
    struct hash< PracticalSemanticAnalyzer::StaticType::CPtr > {
        size_t operator()(const PracticalSemanticAnalyzer::StaticType::CPtr &type) const {
            hash<PracticalSemanticAnalyzer::StaticType> realHash;
            return realHash( *type );
        }
    };
} // namespace std

inline bool operator==(
        const PracticalSemanticAnalyzer::StaticType::CPtr &lhs,
        const PracticalSemanticAnalyzer::StaticType::CPtr &rhs )
{
    if( !lhs && !rhs )
        return true;

    if( !lhs || !rhs )
        return false;

    return *lhs==*rhs;
}

inline bool operator!=(
        const PracticalSemanticAnalyzer::StaticType::CPtr &lhs,
        const PracticalSemanticAnalyzer::StaticType::CPtr &rhs )
{
    return !( lhs == rhs );
}

#endif // PRACTICAL_PRACTICAL_H
