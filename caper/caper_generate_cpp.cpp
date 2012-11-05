// Copyright (C) 2008 Naoyuki Hirayama.
// All Rights Reserved.

// $Id$

#include "caper_ast.hpp"
#include "caper_generate_cpp.hpp"
#include <algorithm>
#include <cstdio>
#include <tuple>

namespace {

struct indent {
    indent( int n ) : n_(n) {}
    int n_;
};

std::ostream& operator<<( std::ostream& os, const indent& x )
{
    for( int i = 0 ; i < x.n_; i++ ) { os << "\t"; }
    return os;
}

void make_signature(
    const symbol_map_type&                  nonterminal_types,
    const tgt::parsing_table::rule_type&    rule,
    const semantic_action&                  sa,
    std::vector< std::string >&             signature )
{
    // function name
    signature.push_back( sa.name );

    // return value
    signature.push_back(
        (*nonterminal_types.find( rule.left().name() )).second );

    // arguments
    for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
        signature.push_back( (*sa.args.find( l )).second.type );
    }
}

} // unnamed namespace

void generate_cpp(
    const std::string&                      src_filename,
    std::ostream&                           os,
    const GenerateOptions&                  options,
    const symbol_map_type&                  terminal_types,
    const symbol_map_type&                  nonterminal_types,
    const std::map< size_t, std::string >&  token_id_map,
    const action_map_type&                  actions,
    const tgt::parsing_table&               table )
{
#ifdef _WINDOWS
    char basename[_MAX_PATH];
    char extension[_MAX_PATH];
    _splitpath( src_filename.c_str(), NULL, NULL, basename, extension );
    std::string filename = std::string(basename) + extension;
#else
    std::string filename = src_filename;
#endif
        
    std::string headername = filename;
    for( std::string::iterator i = headername.begin() ;
         i != headername.end() ;
         ++i ) {
        if( !isalpha( *i ) && !isdigit( *i ) ) {
            *i = '_';
        } else {
            *i = toupper( *i );
        }
    }

    indent ind1(1);

    // once header
    os << "#ifndef " << headername << "_\n"
       << "#define " << headername << "_\n\n"
        ;
    // include
    os << "#include <cstdlib>\n";
    os << "#include <cassert>\n";
    if( !options.dont_use_stl ) {
        os << "#include <vector>\n";
    }
    os << "\n";

    // namespace header
    os << "namespace " << options.namespace_name << " {\n\n";

    if( !options.external_token ) {
        // token enumeration
        os << "enum Token {\n";
        for( size_t i = 0 ; i < token_id_map.size() ; i++ ) {
            os << "    " << options.token_prefix
               << (*token_id_map.find( i )).second << ",\n";
        }
        os << "};\n\n";
    }

    // stack class header

    if( !options.dont_use_stl ) {
        // STL version
        os << "template < class T, int StackSize >\n"
           << "class Stack {\n"
		   << "public:\n"
		   << ind1 << "typedef Stack< T, StackSize > self_type;\n\n"
		   << "public:\n"
           << ind1 << "Stack(){ gap_ = 0; }\n"
           << ind1 << "~Stack(){}\n"
           << ind1 << "\n"
           << ind1 << "void reset_tmp()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "gap_ = stack_.size();\n"
           << ind1 << ind1 << "tmp_.clear();\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "void commit_tmp()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "// may throw\n"
           << ind1 << ind1 << "stack_.reserve( gap_ + tmp_.size() );\n"
           << "\n"
           << ind1 << ind1 << "// expect not to throw\n"
           << ind1 << ind1
           << "stack_.erase( stack_.begin() + gap_, stack_.end() );\n"
           << ind1 << ind1
           << "stack_.insert( stack_.end(), tmp_.begin(), tmp_.end() );\n"
           << ind1 << "}\n"
           << ind1 << "bool push( const T& f )\n"
           << ind1 << "{\n"
           << ind1 << ind1
           << "if( StackSize != 0 && StackSize <= "
           << "stack_.size() + tmp_.size() ) {\n"
           << ind1 << ind1 << ind1 << "return false;\n"
           << ind1 << ind1 << "}\n"
           << ind1 << ind1 << "tmp_.push_back( f );\n"
           << ind1 << ind1 << "return true;\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "void pop( size_t n )\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "if( tmp_.size() < n ) {\n"
           << ind1 << ind1 << ind1 << "n -= tmp_.size();\n"
           << ind1 << ind1 << ind1 << "tmp_.clear();\n"
           << ind1 << ind1 << ind1 << "gap_ -= n;\n"
           << ind1 << ind1 << "} else {\n"
           << ind1 << ind1 << ind1
           << "tmp_.erase( tmp_.end() - n, tmp_.end() );\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "const T& top()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "if( !tmp_.empty() ) {\n"
           << ind1 << ind1 << ind1 << "return tmp_.back();\n"
           << ind1 << ind1 << "} else {\n"
           << ind1 << ind1 << ind1 << "return stack_[ gap_ - 1 ];\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "const T& get_arg( size_t base, size_t index )\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "size_t n = tmp_.size();\n"
           << ind1 << ind1 << "if( base - index <= n ) {\n"
           << ind1 << ind1 << ind1 << "return tmp_[ n - ( base - index ) ];\n"
           << ind1 << ind1 << "} else {\n"
           << ind1 << ind1 << ind1
           << "return stack_[ gap_ - ( base - n ) + index ];\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "void clear()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "stack_.clear();\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "self_type& operator=(const self_type& other)"
           << ind1 << "{\n"
           << ind1 << ind1 << "if( this != &other ) {\n"
           << ind1 << ind1 << ind1 << "stack_ = other.stack_;\n"
           << ind1 << ind1 << ind1 << "tmp_ = other.tmp_;\n"
           << ind1 << ind1 << ind1 << "gap_ = other.gap_;\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << "private:\n"
           << ind1 << "std::vector< T > stack_;\n"
           << ind1 << "std::vector< T > tmp_;\n"
           << ind1 << "size_t gap_;\n"
           << "\n"
           << "};\n\n"
            ;
    }else {
        // bulkmemory version
        os << "template < class T, int StackSize >\n"
           << "class Stack {\n"
		   << "public:\n"
		   << ind1 << "typedef Stack< T, StackSize > self_type;\n\n"
           << "public:\n"
           << ind1 << "Stack(){ top_ = 0; gap_ = 0; tmp_ = 0; }\n"
           << ind1 << "~Stack(){}\n"
           << ind1 << "\n"
           << ind1 << "void reset_tmp()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "for( size_t i = 0 ; i < tmp_ ; i++ ) {\n"
           << ind1 << ind1 << ind1
           << "at( StackSize - 1 - i ).~T(); // explicit destructor\n"
           << ind1 << ind1 << "}\n"
           << ind1 << ind1 << "tmp_ = 0;\n"
           << ind1 << ind1 << "gap_ = top_;\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "void commit_tmp()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "for( size_t i = 0 ; i < tmp_ ; i++ ) {\n"
           << ind1 << ind1 << ind1 << "if( gap_ + i < top_ ) {\n"
           << ind1 << ind1 << ind1 << ind1
           << "at( gap_ + i ) = at( StackSize - 1 - i );\n"
           << ind1 << ind1 << ind1 << "} else {\n"
           << ind1 << ind1 << ind1 << ind1
           << "// placement new copy constructor\n"
           << ind1 << ind1 << ind1 << ind1 << "new ( &at( gap_ + i ) ) \n"
           << ind1 << ind1 << ind1 << ind1
           << "    T( at( StackSize - 1 - i ) );\n"
           << ind1 << ind1 << ind1 << "}\n"
           << ind1 << ind1 << ind1
           << "at( StackSize - 1 - i).~T(); // explicit destructor\n"
           << ind1 << ind1 << "}\n"
           << ind1 << ind1 << "if( gap_ + tmp_ < top_ ) {\n"
           << ind1 << ind1 << ind1
           << "for( int i = 0 ; i < int( top_ - gap_ - tmp_ ) ; i++ ) {\n"
           << ind1 << ind1 << ind1 << ind1
           << "at( top_ - 1 - i ).~T(); // explicit destructor\n"
           << ind1 << ind1 << ind1 << "}\n"
           << ind1 << ind1 << "}\n"
           << "\n"
           << ind1 << ind1 << "top_ = gap_ = gap_ + tmp_;\n"
           << ind1 << ind1 << "tmp_ = 0;\n"
           << ind1 << "}\n"
           << ind1 << "\n"
           << ind1 << "bool push( const T& f )\n"
           << ind1 << "{\n"
           << ind1 << ind1
           << "if( StackSize <= top_ + tmp_ ) { return false; }\n"
           << ind1 << ind1 << "// placement new copy constructor\n"
           << ind1 << ind1 << "new( &at( StackSize - 1 - tmp_++ ) ) T( f );\n"
           << ind1 << ind1 << "return true;\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "void pop( size_t n )\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "size_t m = n; if( m > tmp_ ) m = tmp_;\n"
           << "\n"
           << ind1 << ind1 << "for( size_t i = 0 ; i < m ; i++ ) {\n"
           << ind1 << ind1 << ind1
           << "at( StackSize - tmp_ + i ).~T(); // explicit destructor\n"
           << ind1 << ind1 << "}\n"
           << "\n"
           << ind1 << ind1 << "tmp_ -= m;\n"
           << ind1 << ind1 << "gap_ -= n - m;\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "const T& top()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "if( 0 < tmp_ ) {\n"
           << ind1 << ind1 << ind1
           << "return at( StackSize - 1 - (tmp_-1) );\n"
           << ind1 << ind1 << "} else {\n"
           << ind1 << ind1 << ind1 << "return at( gap_ - 1 );\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "const T& get_arg( size_t base, size_t index )\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "if( base - index <= tmp_ ) {\n"
           << ind1 << ind1 << ind1
           << "return at( StackSize-1-( tmp_ - ( base - index ) ) );\n"
           << ind1 << ind1 << "} else {\n"
           << ind1 << ind1 << ind1
           << "return at( gap_ - ( base - tmp_ ) + index );\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "void clear()\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "reset_tmp();\n"
           << ind1 << ind1 << "for( size_t i = 0 ; i < top_ ; i++ ) {\n"
           << ind1 << ind1 << ind1 << "at( i ).~T(); // explicit destructor\n"
           << ind1 << ind1 << "}\n"
           << ind1 << ind1 << "top_ = gap_ = tmp_ = 0;\n"
           << ind1 << "}\n"
           << "\n"
           << ind1 << "self_type& operator=(const self_type& other)"
           << ind1 << "{\n"
           << ind1 << ind1 << "if( this != &other ) {\n"
           << ind1 << ind1 << ind1 << "for( size_t i = 0 ; i < other.top_ ; i++ ) {\n"
           << ind1 << ind1 << ind1 << ind1 << "new (&at(i)) T(other.at(i));\n"
           << ind1 << ind1 << ind1 << "}\n"
           << ind1 << ind1 << ind1 << "top_ = other.top_;\n"
           << ind1 << ind1 << ind1 << "gap_ = other.gap_;\n"
           << ind1 << ind1 << ind1 << "tmp_ = other.tmp_;\n"
           << ind1 << ind1 << "}\n"
           << ind1 << "}\n"
           << "\n"
           << "private:\n"
           << ind1 << "T& at( size_t n )\n"
           << ind1 << "{\n"
           << ind1 << ind1 << "return *(T*)(stack_ + (n * sizeof(T) ));\n"
           << ind1 << "}\n"
           << "\n"
           << "private:\n"
           << ind1 << "char stack_[ StackSize * sizeof(T) ];\n"
           << ind1 << "size_t top_;\n"
           << ind1 << "size_t gap_;\n"
           << ind1 << "size_t tmp_;\n"
           << "\n"
           << "};\n\n"
            ;
    }

    // parser class header
    std::string default_stacksize = "0";
    if( options.dont_use_stl ) {
        default_stacksize = "1024";
    }

    std::string template_parameters =
        "class Value, class SemanticAction, int StackSize = " +
        default_stacksize;
    if( options.external_token ) {
        template_parameters = "class Token, " + template_parameters;
    }
        
    os << "template < " << template_parameters << " >\n";
    os <<  "class Parser {\n";

    // public interface
    os << "public:\n";
    if( options.external_token ) {
        os << ind1 << "typedef "
           << "Parser< Token, Value, SemanticAction, StackSize > self_type;\n";
    } else {
        os << ind1 << "typedef "
           << "Parser< Value, SemanticAction, StackSize > self_type;\n";
    }
	os << ind1 << "typedef Token token_type;\n"
       << ind1 << "typedef Value value_type;\n\n"
       << "public:\n"
       << ind1 << "Parser( SemanticAction& sa ) : sa_( sa ) { reset(); }\n\n"
       << ind1 << "void reset()\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "error_ = false;\n"
       << ind1 << ind1 << "accepted_ = false;\n"
       << ind1 << ind1 << "clear_stack();\n"
       << ind1 << ind1 << "reset_tmp_stack();\n"
       << ind1 << ind1 << "if( push_stack( "
       << "&Parser::state_" << table.first_state() << ", "
       << "&Parser::gotof_" << table.first_state() << ", "
       << "value_type() ) ) {\n"
       << ind1 << ind1 << ind1 << "commit_tmp_stack();\n"
       << ind1 << ind1 << "} else {\n"
       << ind1 << ind1 << ind1 << "sa_.stack_overflow();\n"
       << ind1 << ind1 << ind1 << "error_ = true;\n"
       << ind1 << ind1 << "}\n"
       << ind1 << "}\n\n"
       << ind1 << "bool post( token_type token, const value_type& value )\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "assert( !error_ );\n"
       << ind1 << ind1 << "reset_tmp_stack();\n"
       << ind1 << ind1 << "while( (this->*(stack_top()->state) )"
       << "( token, value ) ); "
       << "// may throw\n"
       << ind1 << ind1 << "if( !error_ ) {\n"
       << ind1 << ind1 << ind1 << "commit_tmp_stack();\n"
       << ind1 << ind1 << "}\n"
       << ind1 << ind1 << "return accepted_ || error_;\n"
       << ind1 << "}\n\n"
       << ind1 << "bool accept( value_type& v )\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "assert( accepted_ );\n"
       << ind1 << ind1 << "if( error_ ) { return false; }\n"
       << ind1 << ind1 << "v = accepted_value_;\n"
       << ind1 << ind1 << "return true;\n"
       << ind1 << "}\n\n"
       << ind1 << "bool error() { return error_; }\n\n"
       << ind1 << "self_type& operator=(const self_type& other)"
       << ind1 << "{\n"
       << ind1 << ind1 << "if (this != &other) {\n"
       << ind1 << ind1 << ind1 << "accepted_ = other.accepted_;\n"
       << ind1 << ind1 << ind1 << "error_ = other.error_;\n"
       << ind1 << ind1 << ind1 << "accepted_value_ = other.accepted_value_;\n"
       << ind1 << ind1 << ind1 << "stack_ = other.stack_;\n"
       << ind1 << ind1 << "}\n"
       << ind1 << "}\n"
        ;

    // implementation
    os << "private:\n";
    os << ind1 << "typedef bool ( self_type::*state_type )"
       << "( token_type, const value_type& );\n"
       << ind1 << "typedef bool ( self_type::*gotof_type )"
       << "( int, const value_type& );\n\n"
       << ind1 << "bool            accepted_;\n"
       << ind1 << "bool            error_;\n"
       << ind1 << "value_type      accepted_value_;\n\n"
       << ind1 << "SemanticAction& sa_;\n\n"
       << ind1 << "struct stack_frame {\n"
       << ind1<< ind1 << "state_type state;\n"
       << ind1<< ind1 << "gotof_type gotof;\n"
       << ind1<< ind1 << "value_type value;\n\n"
       << ind1<< ind1
       << "stack_frame( state_type s, gotof_type g, const value_type& v )\n"
       << ind1<< ind1 << "    : state( s ), gotof( g ), value( v ) {}\n"
       << ind1 << "};\n\n"
        ;

    // stack operation
    os << ind1 << "Stack< stack_frame, StackSize > stack_;\n"
       << ind1 << "bool push_stack( state_type s, gotof_type g, "
       << "const value_type& v )\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "bool f = stack_.push( stack_frame( s, g, v ) );\n"
       << ind1 << ind1 << "assert( !error_ );\n"
       << ind1 << ind1 << "if( !f ) {\n"
       << ind1 << ind1 << ind1 << "error_ = true;\n"
       << ind1 << ind1 << ind1 << "sa_.stack_overflow();\n"
       << ind1 << ind1 << "}\n"
       << ind1<< ind1 << "return f;\n"
       << ind1 << "}\n\n"
       << ind1 << "void pop_stack( size_t n )\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "stack_.pop( n );\n"
       << ind1 << "}\n\n"
       << ind1 << "const stack_frame* stack_top()\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "return &stack_.top();\n"
       << ind1 << "}\n\n"
       << ind1 << "const value_type& get_arg( size_t base, size_t index )\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "return stack_.get_arg( base, index ).value;\n"
       << ind1 << "}\n\n"
       << ind1 << "void clear_stack()\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "stack_.clear();\n"
       << ind1 << "}\n\n"
       << ind1 << "void reset_tmp_stack()\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "stack_.reset_tmp();\n"
       << ind1 << "}\n\n"
       << ind1 << "void commit_tmp_stack()\n"
       << ind1 << "{\n"
       << ind1 << ind1 << "stack_.commit_tmp();\n"
       << ind1 << "}\n\n"
        ;

    // member function signature -> index
    std::map< std::vector< std::string >, int > stub_index;
    {
        // member function name -> count
        std::map< std::string, int > stub_count; 

        // action handler stub
        for( action_map_type::const_iterator k = actions.begin() ;
             k != actions.end() ;
             ++k ) {

            const tgt::parsing_table::rule_type& rule = (*k).first;

            const semantic_action& sa = (*k).second;

            // make signature
            std::vector< std::string > signature;

            // ... function name
            signature.push_back( sa.name );

            // ... return value
            signature.push_back(
                (*nonterminal_types.find( rule.left().name() )).second );

            // ... arguments
            for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                signature.push_back( (*sa.args.find( l )).second.type );
            }

            // skip duplicated
            if( stub_index.find( signature ) != stub_index.end() ) {
                continue;
            }

            // make function name
            if( stub_count.find( sa.name ) == stub_count.end()) {
                stub_count[sa.name] = 0;
            }
            int index = stub_count[sa.name];
            stub_index[signature] = index;
            stub_count[sa.name] = index+1;

            char function_name_header[256];
            sprintf( function_name_header, "call_%d_", index );
            std::string function_name = function_name_header + sa.name;

            // generate
            os << ind1 << "bool " << function_name
			   << "( int nonterminal_index, int base";
            for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                os << ", int arg_index" << l;
            }
			os << " )\n";

            os << ind1 << "{\n";
            // automatic argument conversion
            for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                const semantic_action_argument& arg =
                    (*sa.args.find( l )).second;
                os << ind1 << ind1 << arg.type
                   << " arg" << l << "; "
                   << "sa_.downcast( arg" << l
                   << ", get_arg( base, arg_index" << l << " ) );\n";
            }

            // semantic action
            os << ind1 << ind1
               << (*nonterminal_types.find(
                       rule.left().name() )).second
               << " r = sa_." << sa.name << "( ";
            bool first = true;
            for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                if( first ) { first = false; } else { os << ", "; }
                os << "arg" << l;
            }
            os << " );\n";

            // automatic return value conversion
            os << ind1 << ind1
               << "value_type v; sa_.upcast( v, r );\n";
            os << ind1 << ind1 << "pop_stack( base );\n";
            os << ind1 << ind1
               << "return (this->*(stack_top()->gotof))( nonterminal_index, v );\n";
            os << ind1 << "}\n\n";
        }
    }

    // states handler
    for( tgt::parsing_table::states_type::const_iterator i =
             table.states().begin();
         i != table.states().end() ;
         ++i ) {
        const tgt::parsing_table::state& s = *i;

        // gotof header
        os << ind1 << "bool gotof_" << s.no
           << "( int nonterminal_index, const value_type& v )\n"
           << ind1 << "{\n";

        // gotof dispatcher
        std::stringstream ss;
        ss << ind1 << ind1 << "switch( nonterminal_index ) {\n";
        bool output_switch = false;
        std::set<size_t> generated;
        for( tgt::parsing_table::rules_type::const_iterator j =
                 table.rules().begin() ;
             j != table.rules().end() ;
             ++j ) {

            size_t nonterminal_index = std::distance(
                nonterminal_types.begin(),
                nonterminal_types.find( (*j).left().name() ) );
            if( generated.find( nonterminal_index ) != generated.end() ) {
                continue;
            }

            tgt::parsing_table::state::goto_table_type::const_iterator k =
                (*i).goto_table.find( (*j).left() );

            if( k != (*i).goto_table.end() ) {

                
                ss << ind1 << ind1 << "case " << nonterminal_index
                   << ": return push_stack( &Parser::state_" << (*k).second
                   << ", &Parser::gotof_" << (*k).second
                   << ", v );\n";
                output_switch = true;
                generated.insert( nonterminal_index );
            }
        }
        ss << ind1 << ind1 << "default: assert(0); return false;\n"; 
        ss << ind1 << ind1 << "}\n";
        if( output_switch ) {
            os << ss.str();
        } else {
            os << ind1<< ind1 << "assert(0);\n"
               << ind1<< ind1 << "return true;\n";
        }

        // gotof footer
        os << ind1 << "}\n\n";
        
        // state header
        os << ind1 << "bool state_" << s.no
           << "( token_type token, const value_type& value )\n";
        os << ind1 << "{\n";

        // dispatcher header
        os << ind1 << ind1 << "switch( token ) {\n";

        // reduce action cache
		typedef std::tuple<
			std::vector< std::string >,
			size_t,
			size_t,
			std::vector< int > >
			reduce_action_cache_key_type;
		typedef 
			std::map< reduce_action_cache_key_type, 
					  std::vector< std::string > >
			reduce_action_cache_type;
		reduce_action_cache_type reduce_action_cache;

        // action table
        for( tgt::parsing_table::state::action_table_type::const_iterator j =
                 s.action_table.begin();
             j != s.action_table.end() ;
             ++j ) {
            // action header 
            std::string case_tag =
                options.token_prefix +
                (*token_id_map.find( (*j).first )).second;

            // action
            const tgt::parsing_table::action* a = &(*j).second;
            switch( a->type ) {
            case zw::gr::action_shift:
                os << ind1 << ind1 << "case " << case_tag << ":\n";

                os << ind1 << ind1 << ind1 << "// shift\n"
                   << ind1 << ind1 << ind1 << "push_stack( "
                   << "&Parser::state_" << a->dest_index << ", "
                   << "&Parser::gotof_" << a->dest_index << ", "
                   << "value );\n"
                   << ind1 << ind1 << ind1 << "return false;\n";
                break;
            case zw::gr::action_reduce:
                {
                    size_t base =
                        table.rules()[ a->rule_index ].right().size();
                                        
                    const tgt::parsing_table::rule_type& rule =
                        table.rules()[a->rule_index];
                    action_map_type::const_iterator k =
                        actions.find( rule );

                    size_t nonterminal_index = std::distance(
                        nonterminal_types.begin(),
                        nonterminal_types.find( rule.left().name() ) );

                    if( k != actions.end() ) {
                        const semantic_action& sa = (*k).second;

                        std::vector< std::string > signature;
                        make_signature(
                            nonterminal_types,
                            rule,
                            sa,
                            signature );

						std::vector< int > arg_indices;
						for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
							const semantic_action_argument& arg =
								(*sa.args.find( l )).second;
							arg_indices.push_back( arg.src_index );
						}

						reduce_action_cache_key_type key =
							std::make_tuple(
								signature,
								nonterminal_index,
								base,
								arg_indices );

                        reduce_action_cache[key].push_back( case_tag );
                    } else {
                        os << ind1 << ind1 << "case " << case_tag << ":\n";
                        os << ind1 << ind1 << ind1 << "// reduce\n";

                        os << ind1 << ind1 << ind1 << ind1
                           << "// run_semantic_action();\n";
                        os << ind1 << ind1 << ind1 << ind1 << "pop_stack( "
                           << base
                           << " );\n";
                        os << ind1 << ind1 << ind1 << ind1
                           << "return (this->*(stack_top()->gotof))( "
                           << nonterminal_index << ", value_type() );\n";
                    }
                }
                break;
            case zw::gr::action_accept:
                os << ind1 << ind1 << "case " << case_tag << ":\n";

                os << ind1 << ind1 << ind1 << "// accept\n"
                   << ind1 << ind1 << ind1 << "// run_semantic_action();\n"
                   << ind1 << ind1 << ind1 << "accepted_ = true;\n"
                   << ind1 << ind1 << ind1
                   << "accepted_value_  = get_arg( 1, 0 );\n" // implicit root
                   << ind1 << ind1 << ind1 << "return false;\n";
                break;
            case zw::gr::action_error:
                os << ind1 << ind1 << "case " << case_tag << ":\n";

                os << ind1 << ind1 << ind1 << "sa_.syntax_error();\n";
                os << ind1 << ind1 << ind1 << "error_ = true;\n"; 
                os << ind1 << ind1 << ind1 << "return false;\n";
                break;
            }

            // action footer
        }

        // flush reduce action cache
        for( reduce_action_cache_type::const_iterator i =
				 reduce_action_cache.begin() ;
             i != reduce_action_cache.end();
             ++i ) {
            const reduce_action_cache_key_type& key = (*i).first;
            const std::vector< std::string >& cases = (*i).second;

			const std::vector< std::string >& signature = std::get<0>( key );
			size_t nonterminal_index = std::get<1>( key );
			size_t base = std::get<2>( key );
			const std::vector< int >& arg_indices = std::get<3>( key );

            for( size_t j = 0 ; j < cases.size() ; j++ ) {
                os << ind1 << ind1 << "case " << cases[j] << ":\n";
            }

            int index = stub_index[signature];

            char function_name_header[256];
            sprintf( function_name_header, "call_%d_", index );
            std::string function_name =
                function_name_header + signature[0];


            os << ind1 << ind1 << ind1 << "return "
               << function_name << "( " << nonterminal_index << ", " << base;
			for( std::vector< int >::const_iterator j = arg_indices.begin() ;
				 j != arg_indices.end() ;
				 ++j ) {
				os  << ", " << (*j);
			}
			os << " );\n";
        }

        // dispatcher footer
        os << ind1 << ind1 << "default:\n"
           << ind1 << ind1 << ind1 << "sa_.syntax_error();\n"
           << ind1 << ind1 << ind1 << "error_ = true;\n"
           << ind1 << ind1 << ind1 << "return false;\n";
        os << ind1 << ind1 << "}\n";

        // state footer
        os << ind1 << "}\n\n";
    }

    // parser class footer
    os << "};\n\n";

    // namespace footer
    os << "} // namespace " << options.namespace_name << "\n\n";

    // once footer
    os << "#endif // #ifndef " << headername << "_\n";
}
