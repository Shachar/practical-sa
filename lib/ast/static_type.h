/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * To the extent header files enjoy copyright protection, this file is file is copyright (C) 2018-2020 by its authors
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#ifndef AST_STATIC_TYPE_H
#define AST_STATIC_TYPE_H

#include "ast/value_range_base.h"
#include "asserts.h"

#include <practical-sa.h>

#include <memory>

namespace AST {

class StaticTypeImpl;

class ScalarTypeImpl final : public PracticalSemanticAnalyzer::StaticType::Scalar, private NoCopy {
    std::string name;
    std::string mangledName;

public:
    explicit ScalarTypeImpl(
            String name,
            String mangledName,
            size_t size,
            size_t alignment,
            Scalar::Type type,
            PracticalSemanticAnalyzer::TypeId backendType,
            unsigned literalWeight );
    ScalarTypeImpl( ScalarTypeImpl &&that ) :
        PracticalSemanticAnalyzer::StaticType::Scalar( that ),
        name( std::move(that.name) ),
        mangledName( std::move(that.mangledName) )
    {}

    virtual String getName() const override {
        return name.c_str();
    }

    virtual String getMangledName() const override {
        return mangledName.c_str();
    }
};

class FunctionTypeImpl final : public PracticalSemanticAnalyzer::StaticType::Function, private NoCopy {
    boost::intrusive_ptr<const StaticTypeImpl> returnType;
    std::vector< boost::intrusive_ptr<const StaticTypeImpl> > argumentTypes;
    mutable std::string mangledNameCache;

public:
    explicit FunctionTypeImpl(
            boost::intrusive_ptr<const StaticTypeImpl> &&returnType,
            std::vector< boost::intrusive_ptr<const StaticTypeImpl> > &&argumentTypes);

    FunctionTypeImpl( FunctionTypeImpl &&that ) :
        returnType( std::move(that.returnType) ),
        argumentTypes( std::move(that.argumentTypes) )
    {}

    PracticalSemanticAnalyzer::StaticType::CPtr getReturnType() const override;

    size_t getNumArguments() const override {
        return argumentTypes.size();
    }

    PracticalSemanticAnalyzer::StaticType::CPtr getArgumentType( unsigned index ) const override;

    String getMangledName() const override;
};

class PointerTypeImpl : public PracticalSemanticAnalyzer::StaticType::Pointer, private NoCopy {
    boost::intrusive_ptr<const StaticTypeImpl> pointed;
    mutable std::string mangledName;

public:
    explicit PointerTypeImpl( boost::intrusive_ptr<const StaticTypeImpl> pointed ) : pointed(pointed) {}

    virtual String getMangledName() const override;
    virtual PracticalSemanticAnalyzer::StaticType::CPtr getPointedType() const override;
};

class StaticTypeImpl final : public PracticalSemanticAnalyzer::StaticType {
private:
    std::variant<
            std::unique_ptr<ScalarTypeImpl>,
            std::unique_ptr<FunctionTypeImpl>,
            PointerTypeImpl
    > content;
    ValueRangeBase::CPtr valueRange;

public:
    using CPtr = boost::intrusive_ptr<const StaticTypeImpl>;
    using Ptr = boost::intrusive_ptr<StaticTypeImpl>;

    template<typename... Args>
    static Ptr allocate(Args&&... args) {
        return new StaticTypeImpl( std::forward<Args>(args)... );
    }

    virtual Types getType() const override final;
    ValueRangeBase::CPtr defaultRange() const {
        return valueRange;
    }

    friend std::ostream &operator<<( std::ostream &out, const AST::StaticTypeImpl::CPtr &type );

private:
    explicit StaticTypeImpl( ScalarTypeImpl &&scalar, ValueRangeBase::CPtr valueRange );
    explicit StaticTypeImpl( FunctionTypeImpl &&function );
    explicit StaticTypeImpl( PointerTypeImpl &&function );
};

} // End namespace AST

inline bool operator==( const AST::StaticTypeImpl::CPtr &lhs, const AST::StaticTypeImpl::CPtr &rhs ) {
    if( !lhs && !rhs )
        return true;

    if( !lhs || !rhs )
        return false;

    return *lhs == *rhs;
}

inline bool operator!=( const AST::StaticTypeImpl::CPtr &lhs, const AST::StaticTypeImpl::CPtr &rhs ) {
    return !( lhs == rhs );
}

inline std::ostream &operator<<( std::ostream &out, const AST::StaticTypeImpl &type ) {
    return out<<AST::StaticTypeImpl::CPtr(&type);
}

namespace std {
    template<>
    class hash< AST::StaticTypeImpl::CPtr > {
    public:
        size_t operator()( const AST::StaticTypeImpl::CPtr &ptr ) const {
            return hash< PracticalSemanticAnalyzer::StaticType::CPtr >()( ptr.get() );
        }
    };
}

#endif // AST_STATIC_TYPE_H
