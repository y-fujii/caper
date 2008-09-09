// 2008/08/13 Naoyuki Hirayama

/*!
    @file     leaf_compile.hpp
    @brief    <概要>

    <説明>
*/

#ifndef LEAF_COMPILE_HPP_
#define LEAF_COMPILE_HPP_

#include <fstream>
#include <limits>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include "leaf_scanner.hpp"
#include "leaf_error.hpp"

namespace leaf {

class Compiler : public boost::noncopyable {
public:
	Compiler(){ env_.idseed = 1; }
	~Compiler(){}

	// filenameはソースのidのようなもの、オープンはしない
	template < class IIt >
	Node* read( const std::string& filename, IIt b, IIt e )
	{
		return read_internal( filename, b, e );
	}

	void compile( Node* n, std::ostream& os )
	{
		compile_internal( n, os );
	}

public:
	////////////////////////////////////////////////////////////////
	// as semantic action
    template < class T >
    T* h( T* p )
    {
        p->h.id = env_.idseed++;
        return p;
    }

    template < class T >
    T* h( const Header& header, T* p )
    {
        p->h = header;
        p->h.id = env_.idseed++;
        return p;
    }

	heap_cage& c() { return env_.cage; }

    template < class T, class U >
    T* makeSeq1( U* x )
    {
        std::vector< U* > v;
        v.push_back( x );
        return h( x->h, c().allocate<T>( v ) );
    }


    template < class T, class U >
    T* append( T* x, U* y )
    {
        x->h += y->h;
        x->v.push_back( y );
        return x;
    }

    void syntax_error() {}
    void stack_overflow(){}

    template < class T0, class T1 >
    void downcast( T0& x, T1 y ) { x = dynamic_cast<T0>(y); }

    template < class T0, class T1 >
    void upcast( T0& x, T1 y ) { x = y; }

    template < class T >
    T* identity( T* x ) { return x; }

    Module* makeModule( TopElems* s )
    {
        return h( s->h, c().allocate<Module>( s ) );
    }

    TopElems* makeTopElems0( TopElem* e )
    {
        return makeSeq1<TopElems>( e );
    }

    TopElems* makeTopElems1( TopElems* s, TopElem* e )
    {
        return append( s, e );
    }

    Statements* makeStatements0()
    {
        return h( c().allocate<Statements>() );
    }

    Statements* makeStatements1( Statements* ss,
								 Statement* s )
    {
        return append( ss, s );
    }

    Require* makeRequire( Identifier* i )
    {
        std::string filename = "rtl/" + i->s->s + ".lh";

        std::ifstream ifs( filename.c_str() );
        if( !ifs ) {
            throw require_fail( i->h.beg, filename );
        }
    
        // スキャナ
        typedef std::istreambuf_iterator<char> is_iterator;
        is_iterator b( ifs );    // 即値にするとVC++が頓珍漢なことを言う
        is_iterator e;

        Node* v = read( filename, b, e ); 
        Module* m = dynamic_cast<Module*>(v);
        assert( m );

        return h( i->h, c().allocate<Require>( i, m ) );
    }
	
    TopLevelFunDecl* makeTopLevelFunDecl( FunDecl* f )
    {
        return c().allocate<TopLevelFunDecl>( f );
    }

    TopLevelFunDef* makeTopLevelFunDef( FunDef* f )
    {
        return c().allocate<TopLevelFunDef>( f );
    }

    FunDecl* makeFunDecl( FunSig* s )
    {
        return h( s->h, c().allocate<FunDecl>( s ) );
    }

    FunDef* makeFunDef( FunSig* s, Block* b )
    {
        symmap_t sm;
        return h( s->h + b->h, c().allocate<FunDef>( s, b, sm ) );
    }

    FunSig* makeFunSig0( Identifier* i,
                               FormalArgs* fa )
    {
        return h( i->h + fa->h, c().allocate<FunSig>(
                      i, fa, (Types*)NULL ) );
    }

    FunSig* makeFunSig1( Identifier* i,
                               FormalArgs* fa,
                               Types* t )
    {
        return h( i->h + fa->h + t->h,
                  c().allocate<FunSig>( i, fa, t ) );
    }

    FormalArgs* makeFormalArgs0()
    {
        return h( c().allocate<FormalArgs>() );
    }

    FormalArgs* makeFormalArgs1( FormalArg* fa )
    {
        return makeSeq1<FormalArgs>( fa );
    }

    FormalArgs* makeFormalArgs2( FormalArgs* fargs,
                                       FormalArg* fa )
    {
        return append( fargs, fa );
    }

    FormalArg* makeFormalArg0( Identifier* i )
    {
#if 0
        return h( i->h, c().allocate<FormalArg>(
                      i, (TypeRef*)NULL ) );
#else
        throw formalarg_must_be_typed( i->h.beg, i->s->s );
#endif
    }

    FormalArg* makeFormalArg1( Identifier* i, TypeRef* t )
    {
        return h( i->h + t->h,
                  c().allocate<FormalArg>( i, t ) );
    }

    Block* makeBlock( Statements* s )
    {
        return h( s->h, c().allocate<Block>( s ) );
    }

    VarDecl* makeVarDecl( VarDeclElems* v, MultiExpr* e )
    {
        return h( v->h + e->h, c().allocate<VarDecl>( v, e ) );
    }

    VarDeclElems* makeVarDeclElems0( VarDeclElem* y )
    {
        return makeSeq1<VarDeclElems>( y );
    }

    VarDeclElems* makeVarDeclElems1(
        VarDeclElems* x, VarDeclElem* y )
    {
        return append( x, y );
    }

    VarDeclElem* makeVarDeclIdentifier0( Identifier* i )
    {
        return h( i->h, c().allocate<VarDeclIdentifier>(
                      i, (TypeRef*)NULL ) );
    }

    VarDeclElem* makeVarDeclIdentifier1( Identifier* i,
                                               TypeRef* t )
    {
        return h( i->h + t->h, c().allocate<VarDeclIdentifier>(
                      i, t ) );
    }

    IfThenElse* makeIfThenElse0( Expr* cond,
                                       Block* t,
                                       Block* f )
    {
        return h( cond->h + t->h + f->h,
                  c().allocate<IfThenElse>( cond, t, f ) );
    }

    IfThenElse* makeIfThenElse1( Expr* cond,
								 Block* t,
								 Statement* ite )
    {
        Block* else_clause =
            h( ite->h, c().allocate<Block>(
                   makeSeq1<Statements>( ite ) ) );
        
        return h( cond->h + t->h + ite->h,
                  c().allocate<IfThenElse>( cond, t, else_clause ) );
    }

    TypeRef* makeTypeVoid()
    {
        return h( c().allocate<TypeRef>( Type::getVoidType() ) );
    }

    TypeRef* makeTypeLong()
    {
        return h( c().allocate<TypeRef>( Type::getLongType() ) );
    }

    TypeRef* makeTypeInt()
    {
        return h( c().allocate<TypeRef>( Type::getIntType() ) );
    }

    TypeRef* makeTypeShort()
    {
        return h( c().allocate<TypeRef>( Type::getShortType() ) );
    }

    TypeRef* makeTypeChar()
    {
        return h( c().allocate<TypeRef>( Type::getCharType() ) );
    }

    TypeRef* makeFunctionType(
        Types* atype, Types* rtype )
    {
        typevec_t rtypes;
        for( size_t i = 0 ; i < rtype->v.size() ; i++ ) {
            rtypes.push_back( rtype->v[i]->t );
        }

        typevec_t atypes;
        for( size_t i = 0 ; i < atype->v.size() ; i++ ) {
            atypes.push_back( atype->v[i]->t );
        }

        return h( atype->h + rtype->h,
                  c().allocate<TypeRef>(
                      Type::getFunctionType(
                          Type::getTupleType( rtypes ),
                          Type::getTupleType( atypes ) ) ) );
    }

    Types* makeTypes0()
    {
        return h( c().allocate<Types>() );
    }
    
    Types* makeTypes1( TypeRef* t )
    {
        return makeSeq1<Types>( t );
    }
    
    Types* makeTypes2( Types* x, TypeRef* y )
    {
        return append( x, y );
    }
    
    MultiExpr* makeMultiExpr0( Expr* y )
    {
        return makeSeq1<MultiExpr>( y );
    }

    MultiExpr* makeMultiExpr1( MultiExpr* x, Expr* y )
    {
        return append( x, y );
    }

    LogicalOr* makeLogicalOr0( LogicalAnd* y )
    {
        return makeSeq1<LogicalOr>( y );
    }

    LogicalOr* makeLogicalOr1( LogicalOr* x, LogicalAnd* y )
    {
        return append( x, y );
    }

    LogicalAnd* makeLogicalAnd0( Equality* y )
    {
        return makeSeq1<LogicalAnd>( y );
    }

    LogicalAnd* makeLogicalAnd1( LogicalAnd* x, Equality* y )
    {
        return append( x, y );
    }

    Equality* makeEq( Equality* x, Relational* y )
    {
        return h( x->h + y->h, c().allocate<EqualityEq>( x, y ) );
    }

    Equality* makeNe( Equality* x, Relational* y )
    {
        return h( x->h + y->h, c().allocate<EqualityNe>( x, y ) );
    }

    Relational* makeLt( Relational* x, Additive* y )
    {
        return h( x->h + y->h, c().allocate<RelationalLt>( x, y ) );
    }

    Relational* makeGt( Relational* x, Additive* y )
    {
        return h( x->h + y->h, c().allocate<RelationalGt>( x, y ) );
    }

    Relational* makeLe( Relational* x, Additive* y )
    {
        return h( x->h + y->h, c().allocate<RelationalLe>( x, y ) );
    }

    Relational* makeGe( Relational* x, Additive* y )
    {
        return h( x->h + y->h, c().allocate<RelationalGe>( x, y ) );
    }

    Additive* makeAdd( Additive* x, Multiplicative* y )
    {
        return h( x->h + y->h, c().allocate<AddExpr>( x, y ) );
    }
    Additive* makeSub( Additive* x, Multiplicative* y )
    {
        return h( x->h + y->h, c().allocate<SubExpr>( x, y ) );
    }

    Multiplicative* makeMul( Multiplicative* x, CastExpr* y )
    {
        return h( x->h + y->h, c().allocate<MulExpr>( x, y ) );
    }
    Multiplicative* makeDiv( Multiplicative* x, CastExpr* y )
    {
        return h( x->h + y->h, c().allocate<DivExpr>( x, y ) );
    }

    Cast* makeCast( PrimExpr* e, TypeRef* t )
    {
        return h( e->h + t->h, c().allocate<Cast>( e, t ) );
    }

    PrimExpr* makeVarRef( Identifier* i )
    {
        return h( i->h, c().allocate<VarRef>( i ) );
    }
    PrimExpr* makeParenthized( MultiExpr* e )
    {
        return h( e->h, c().allocate<Parenthized>( e ) );
    }

    FunCall* makeFunCall0( Identifier* func )
    {
        return h( func->h,
                  c().allocate<FunCall>(
                      func, c().allocate<ActualArgs>() ) );
    }

    FunCall* makeFunCall1( Identifier* func,
                                 ActualArgs* aargs )
    {
        return h( func->h + aargs->h,
                  c().allocate<FunCall>( func, aargs ) );
    }

    Lambda* tt(){ return NULL; }


    Lambda* makeLambda( FormalArgs* a,
                              Types* r,
                              Block* b )
    {
        return h( a->h + r->h + b->h,
                  c().allocate<Lambda>(
                      (Symbol*)NULL, a, r, b, symmap_t() ) );
    }

    ActualArgs* makeActualArgs0( ActualArg* fa )
    {
        return makeSeq1<ActualArgs>( fa );
    }
    ActualArgs* makeActualArgs1( ActualArgs* aargs,
                                       ActualArg* aa )
    {
        return append( aargs, aa );
    }
    ActualArg* makeActualArg( Expr* e )
    {
        return h( e->h, c().allocate<ActualArg>( e ) );
    }

    PrimExpr* mismatchParen0()
    {
        throw mismatch_paren( Addr(), ';' );
    }

    PrimExpr* mismatchParen1()
    {
        throw mismatch_paren( Addr(), '}' );
    }

    PrimExpr* mismatchParen2()
    {
        throw primexpr_expected( Addr(), ')' );
    }

    PrimExpr* mismatchParen3()
    {
        throw primexpr_expected( Addr(), ';' );
    }

    PrimExpr* mismatchParen4()
    {
        throw primexpr_expected( Addr(), '}' );
    }

    FormalArgs* badFormalArgs0()
    {
        throw mismatch_paren( Addr(), ';' );
    }

    FormalArgs* badFormalArgs1()
    {
        throw primexpr_expected( Addr(), '}' );
    }

    FormalArgs* badFormalArgs2()
    {
        throw primexpr_expected( Addr(), ';' );
    }

    FormalArgs* badFormalArgs3()
    {
        throw primexpr_expected( Addr(), '}' );
    }

    FormalArg* badFormalArg0()
    {
        throw bad_formalarg( Addr() );
    }

    FunCall* badActualArgs0()
    {
        throw mismatch_paren( Addr(), ';' );
    }

    FunCall* badActualArgs1()
    {
        throw primexpr_expected( Addr(), '}' );
    }

    FunCall* badActualArgs2()
    {
        throw primexpr_expected( Addr(), ';' );
    }

    FunCall* badActualArgs3()
    {
        throw primexpr_expected( Addr(), '}' );
    }

    Statement* expectSemicolon()
    {
        throw semicolon_expected( Addr(), '}' );
    }

private:
	template < class IIt >
	Node* read_internal( const std::string& filename, IIt b, IIt e )
	{
		// ファイル名のintern
		Symbol* file = env_.intern( filename );

		// スキャナ
		Scanner< IIt > scanner( env_, file, b, e );

		// パーサ
		Parser< Node*, Compiler > parser( *this );

		// リードループ
		try {
			Token token = token_eof;
			Node* v = NULL;
			for(;;) {
				token = scanner.get( v );
				if( parser.post( token, v ) ) { break; }

				// TODO
				if( parser.error() ) {
					throw leaf::syntax_error(
						scanner.addr(),
						token_representation( token, v ) );
				}
			}

			if( parser.accept( v ) ) {
				return v;
			}
		}
		catch( error& e ) {
			if( e.addr.empty() ) { e.addr = scanner.addr(); }
			if( e.lineno < 0 ) { e.lineno = env_.sm.lineno( e.addr ); }
			if( e.column < 0 ) { e.column = env_.sm.column( e.addr ); }
			throw;
		}

		assert(0);
		return NULL;
	}

	void compile_internal( Node* n, std::ostream& os );
	std::string token_representation( Token token, Node* v );

private:
	CompileEnv	env_;

};

} // namespace leaf

#endif // LEAF_COMPILE_HPP_
