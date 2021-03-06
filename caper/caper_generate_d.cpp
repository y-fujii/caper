#include "caper_ast.hpp"
#include "caper_generate_d.hpp"
#include <algorithm>

using std::endl;
std::string const indent = "\t";

void generate_d(
        const std::string&                      filename,
        std::ostream&                           os,
        const GenerateOptions&                  options,
        const symbol_map_type&                  terminal_types,
        const symbol_map_type&                  nonterminal_types,
        const std::map< size_t, std::string >&  token_id_map,
        const action_map_type&                  actions,
        const tgt::parsing_table&               table )
{
        std::string modulename = filename;
        if( modulename [modulename.size() - 2] == '.' &&
            modulename [modulename.size() - 1] == 'd')
        {
                modulename.erase (modulename.end() - 2, modulename.end());
        }
        for( std::string::iterator i = modulename.begin() ; i != modulename.end() ; ++i ) {
                *i = ( *i == '/' ) ? '.' : tolower( *i );
        }

        // module declaration
        os << "module " << modulename << ";" << endl << endl ;

        if( !options.external_token ) {
                // token enumeration
                os << "enum Token {" << endl;
                for( size_t i = 0 ; i < token_id_map.size() ; i++ ) {
                        os << indent << options.token_prefix << (*token_id_map.find( i )).second;
                        if(i == token_id_map.size() - 1){
                               os << endl << "}" << endl << endl;
                               break;
                        }else{
                                os << "," << endl;
                        }
                }
        }

        // stack class header

        if( !options.dont_use_stl ) {
                // dynamic array version
                os << "class Stack(T, int stackSize)" << endl
                   << "{" << endl
                   << indent << "this(){ gap_ = 0; }" << endl
                   << indent << "~this(){}" << endl
                   << endl
                   << indent << "void reset_tmp()" << endl
                   << indent << "{" << endl
                   << indent << indent << "gap_ = stack_.length;" << endl
                   << indent << indent << "tmp_ = null;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "void commit_tmp()" << endl
                   << indent << "{" << endl
                   << indent << indent << "stack_ = stack_[0 .. gap_] ~ tmp_;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "bool push(T f)" << endl
                   << indent << "{" << endl
                   << indent << indent << "if(stackSize != 0 && stackSize <= stack_.length + tmp_.length){" << endl
                   << indent << indent << indent << "return false;" << endl
                   << indent << indent << "}" << endl
                   << indent << indent << "tmp_ ~= f;" << endl
                   << indent << indent << "return true;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "void pop(uint n)" << endl
                   << indent << "{" << endl
                   << indent << indent << "if(tmp_.length < n){" << endl
                   << indent << indent << indent << "n -= tmp_.length;" << endl
                   << indent << indent << indent << "tmp_ = null;" << endl
                   << indent << indent << indent << "gap_ -= n;" << endl
                   << indent << indent << "}else{" << endl
                   << indent << indent << indent << "tmp_ = tmp_[0 .. $ - n];" << endl
                   << indent << indent << "}" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "T* top()" << endl
                   << indent << "{" << endl
                   << indent << indent << "if(tmp_.length > 0){" << endl
                   << indent << indent << indent << "return &tmp_[$ - 1];" << endl
                   << indent << indent << "}else{" << endl
                   << indent << indent << indent << "return &stack_[gap_ - 1];" << endl
                   << indent << indent << "}" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "T* get_arg(uint base, uint index)" << endl
                   << indent << "{" << endl
                   << indent << indent << "uint n = tmp_.length;" << endl
                   << indent << indent << "if(base - index <= n){" << endl
                   << indent << indent << indent << "return &tmp_[n - (base - index)];" << endl
                   << indent << indent << "}else{" << endl
                   << indent << indent << indent << "return &stack_[gap_ - (base - n) + index];" << endl
                   << indent << indent << "}" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "void clear()" << endl
                   << indent << "{" << endl
                   << indent << indent << "stack_ = null;" << endl
                   << indent << "}" << endl
                   << endl
                   << "private:" << endl
                   << indent << "T[] stack_;" << endl
                   << indent << "T[] tmp_;" << endl
                   << indent << "uint gap_;" << endl
                   << "}" << endl << endl;
        }else {
                // static array "bulkmemory" version
                os << "class Stack(T, int stackSize)" << endl
                   << "{" << endl
                   << indent << "this(){ top_ = 0; gap_ = 0; tmp_ = 0; }" << endl
                   << indent << "~this(){}" << endl
                   << endl
                   << indent << "void reset_tmp()" << endl
                   << indent << "{" << endl
                   << indent << indent << "tmp_ = 0;" << endl
                   << indent << indent << "gap_ = top_;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "void commit_tmp()" << endl
                   << indent << "{" << endl
                   << indent << indent << "for(uint i = 0; i < tmp_; i++){" << endl
                   << indent << indent << indent << "stack_[gap_ + i] = stack_[stackSize - 1 - i];" << endl
                   << indent << indent << "}" << endl
                   << indent << indent << "top_ = gap_ = gap_ + tmp_;" << endl
                   << indent << indent << "tmp_ = 0;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "bool push(T f)" << endl
                   << indent << "{" << endl
                   << indent << indent << "if(stackSize <= top_ + tmp_) { return false; }" << endl
                   << indent << indent << "stack_[stackSize - 1 - tmp_] = f;" << endl
                   << indent << indent << "++ tmp_;" << endl
                   << indent << indent << "return true;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "void pop(uint n)" << endl
                   << indent << "{" << endl
                   << indent << indent << "uint m = n;" << endl
                   << indent << indent << "if(m > tmp_) m = tmp_;" << endl
                   << indent << indent << "tmp_ -= m;" << endl
                   << indent << indent << "gap_ -= n - m;" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "T* top()" << endl
                   << indent << "{" << endl
                   << indent << indent << "if(0 < tmp_){" << endl
                   << indent << indent << indent << "return &stack_[stackSize - tmp_];" << endl
                   << indent << indent << "}else{" << endl
                   << indent << indent << indent << "return &stack_[gap_ - 1];" << endl
                   << indent << indent << "}" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "T* get_arg(uint base, uint index)" << endl
                   << indent << "{" << endl
                   << indent << indent << "if(base - index <= tmp_){" << endl
                   << indent << indent << indent << "return &stack_[stackSize - 1 - (tmp_ - (base - index))];" << endl
                   << indent << indent << "}else{" << endl
                   << indent << indent << indent << "return &stack_[gap_ - (base - tmp_) + index];" << endl
                   << indent << indent << "}" << endl
                   << indent << "}" << endl
                   << endl
                   << indent << "void clear()" << endl
                   << indent << "{" << endl
                   << indent << indent << "top_ = 0;" << endl
                   << indent << indent << "gap_ = 0;" << endl
                   << indent << indent << "tmp_ = 0;" << endl
                   << indent << "}" << endl
                   << endl
                   << "private:" << endl
                   << indent << "T[stackSize] stack_;" << endl
                   << indent << "uint top_;" << endl
                   << indent << "uint gap_;" << endl
                   << indent << "uint tmp_;" << endl
                   << "}" << endl << endl;
        }

        // parser class header
        std::string default_stacksize = "0";
        if( options.dont_use_stl ) {
                default_stacksize = "1024";
        }

        std::string template_parameters = "Value, SemanticAction, int stackSize = " + default_stacksize;
        if( options.external_token ) {
                template_parameters = "Token, " + template_parameters;
        }
        
        os << "class Parser(" << template_parameters << ")" << endl
           << "{" << endl;

        // public interface
        os << indent << "alias Token TokenType;" << endl
           << indent << "alias Value ValueType;" << endl
           << endl
           << indent << "this(SemanticAction sa){ sa_ = sa; reset(); }" <<endl
           << endl
           << indent << "void reset()" << endl
           << indent << "{" << endl
           << indent << indent << "error_ = false;" << endl
           << indent << indent << "accepted_ = false;" << endl
           << indent << indent << "stack_ = new typeof(stack_);" << endl
           << indent << indent << "clear_stack();" << endl
           << indent << indent << "reset_tmp_stack();" << endl
           << indent << indent << "ValueType defaultValue;" << endl
           << indent << indent << "if(push_stack("
               << "&state_" << table.first_state() << ", "
               << "&gotof_" << table.first_state() << ", "
               << "defaultValue)){" << endl
           << indent << indent << indent << "commit_tmp_stack();" << endl
           << indent << indent << "}else{" << endl
           << indent << indent << indent << "sa_.stack_overflow();" << endl
           << indent << indent << indent << "error_ = true;" << endl
           << indent << indent << "}" << endl
           << indent << "}" << endl
           << endl
           << indent << "bool post(TokenType token, ValueType value)" << endl
           << indent << "{" << endl
           << indent << indent << "assert(!error_);" << endl
           << indent << indent << "reset_tmp_stack();" << endl
           << indent << indent << "while((stack_top().state)(token, value)){ }" << endl
           << indent << indent << "if(!error_){" << endl
           << indent << indent << indent << "commit_tmp_stack();" << endl
           << indent << indent << "}" << endl
           << indent << indent << "return accepted_;" << endl
           << indent << "}" << endl
           << endl
           << indent << "bool accept(out ValueType v)" << endl
           << indent << "{" << endl
           << indent << indent << "assert(accepted_);" << endl
           << indent << indent << "if(error_){ return false; }" << endl
           << indent << indent << "v = accepted_value_;" << endl
           << indent << indent << "return true;" << endl
           << indent << "}" << endl
           << endl
           << indent << "bool error(){ return error_; }" << endl
           << endl;

        // implementation
        os << "private:" << endl
           << indent << "alias typeof(this) self_type;" << endl
           << indent << "alias bool delegate(TokenType, ValueType) state_type;" << endl
           << indent << "alias bool delegate(int, ValueType) gotof_type;" << endl
           << endl
           << indent << "bool accepted_;" << endl
           << indent << "bool error_;" << endl
           << indent << "ValueType accepted_value_;" << endl
           << endl
           << indent << "SemanticAction sa_;" << endl
           << endl
           << indent << "struct stack_frame" << endl
           << indent << "{" << endl
           << indent << indent << "state_type state;" << endl
           << indent << indent << "gotof_type gotof;" << endl
           << indent << indent << "ValueType value;" << endl
           << endl
           << indent << indent << "static stack_frame opCall(state_type s, gotof_type g, ValueType v)" << endl
           << indent << indent << "{" << endl
           << indent << indent << indent << "stack_frame result;" << endl
           << indent << indent << indent << "result.state = s;" << endl
           << indent << indent << indent << "result.gotof = g;" << endl
           << indent << indent << indent << "result.value = v;" << endl
           << indent << indent << indent << "return result;" << endl
           << indent << indent << "}" << endl
           << indent << "}" << endl
           << endl;

        // stack operation
        os << indent << "Stack!(stack_frame, stackSize) stack_;" << endl
           << endl
           << indent << "bool push_stack(state_type s, gotof_type g, ValueType v)" << endl
           << indent << "{" << endl
           << indent << indent << "bool f = stack_.push(stack_frame(s, g, v));" << endl
           << indent << indent << "assert(!error_);" << endl
           << indent << indent << "if(!f){" << endl
           << indent << indent << indent << "error_ = true;" << endl
           << indent << indent << indent << "sa_.stack_overflow();" << endl
           << indent << indent << "}" << endl
           << indent << indent << "return f;" << endl
           << indent << "}" << endl
           << endl
           << indent << "void pop_stack(uint n)" << endl
           << indent << "{" << endl
           << indent << indent << "stack_.pop(n);" << endl
           << indent << "}" << endl
           << endl
           << indent << "stack_frame* stack_top()" << endl
           << indent << "{" << endl
           << indent << indent << "return stack_.top();" << endl
           << indent << "}" << endl
           << endl
           << indent << "ValueType* get_arg(uint base, uint index)" << endl
           << indent << "{" << endl
           << indent << indent << "return &stack_.get_arg(base, index).value;" << endl
           << indent << "}" << endl
           << endl
           << indent << "void clear_stack()" << endl
           << indent << "{" << endl
           << indent << indent << "stack_.clear();" << endl
           << indent << "}" << endl
           << endl
           << indent << "void reset_tmp_stack()" << endl
           << indent << "{" << endl
           << indent << indent << "stack_.reset_tmp();" << endl
           << indent << "}" << endl
           << endl
           << indent << "void commit_tmp_stack()" << endl
           << indent << "{" << endl
           << indent << indent << "stack_.commit_tmp();" << endl
           << indent << "}" << endl
           << endl;

        // states handler
        for( tgt::parsing_table::states_type::const_iterator i = table.states().begin();
             i != table.states().end() ;
             ++i ) {
                const tgt::parsing_table::state& s = *i;

                // gotof header
                os << indent << "bool gotof_" << s.no << "(int nonterminal_index, ValueType v)" << endl
                   << indent << "{" << endl;

                // gotof dispatcher
                std::stringstream ss;
                ss << indent << indent << "switch(nonterminal_index){" << endl;
                bool output_switch = false;
                std::set<size_t> generated;
                for( tgt::parsing_table::rules_type::const_iterator j = table.rules().begin() ;
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


                                ss << indent << indent << "case " << nonterminal_index
                                   << ": return push_stack(&state_" << (*k).second
                                   << ", &gotof_" << (*k).second
                                   << ", v);" << endl;
                                output_switch = true;
                                generated.insert( nonterminal_index );
                        }
                }
                // D's default "default:" is same action.
                // ss << indent << indent << "default: assert(false);" << endl;
                ss << indent << indent << "}" << endl;
                if( output_switch ) {
                        os << ss.str();
                } else {
                        os << indent << indent << "assert(false);" << endl;
                }

                // gotof footer
                os << indent << "}" << endl
                   << endl;

                // state header
                os << indent << "bool state_" << s.no << "(TokenType token, ValueType value)" << endl;
                os << indent << "{" << endl;

                // dispatcher header
                os << indent << indent << "switch(token) {" << endl;

                // action table
                for( tgt::parsing_table::state::action_table_type::const_iterator j = s.action_table.begin();
                     j != s.action_table.end() ;
                     ++j ) {
                        // action header 
                        os << indent << indent << "case Token." << options.token_prefix
                           << (*token_id_map.find( (*j).first )).second << ":" << endl;

                        // action
                        const tgt::parsing_table::action* a = &(*j).second;
                        switch( a->type ) {
                        case zw::gr::action_shift:
                                os << indent << indent << indent << "// shift" << endl
                                   << indent << indent << indent << "push_stack("
                                       << "&state_" << a->dest_index << ", "
                                       << "&gotof_" << a->dest_index << ", "
                                       << "value);" << endl
                                   << indent << indent << indent << "return false;" << endl;
                                break;
                        case zw::gr::action_reduce:
                                os << indent << indent << indent << "// reduce" << endl;
                                {
                                        size_t base = table.rules()[ a->rule_index ].right().size();
                                        
                                        const tgt::parsing_table::rule_type& rule = table.rules()[a->rule_index];
                                        action_map_type::const_iterator k = actions.find( rule );

                                        size_t nonterminal_index = std::distance(
                                                nonterminal_types.begin(),
                                                nonterminal_types.find( rule.left().name() ) );

                                        if( k != actions.end() ) {
                                                const semantic_action& sa = (*k).second;

                                                os << indent << indent << indent << "{" << endl;
                                                // automatic argument conversion
                                                for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                                                        const semantic_action_argument& arg =
                                                                (*sa.args.find( l )).second;
                                                        os << indent << indent << indent << indent << arg.type << " arg" << l << ";" << endl
                                                           << indent << indent << indent << indent << "sa_.downcast(arg" << l << ", *get_arg(" << base
                                                           << ", " << arg.src_index << "));" << endl;
                                                }

                                                // semantic action
                                                os << indent << indent << indent << indent
                                                   << (*nonterminal_types.find( rule.left().name() )).second
                                                   << " r = sa_." << sa.name << "(";
                                                bool first = true;
                                                for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                                                        if( first ) { first = false; } else { os << ", "; }
                                                        os << "arg" << l;
                                                }
                                                os << ");" << endl;

                                                // automatic return value conversion
                                                os << indent << indent << indent << indent << "ValueType v;" << endl
                                                   << indent << indent << indent << indent << "sa_.upcast(v, r);" << endl;
                                                os << indent << indent << indent << indent << "pop_stack("
                                                   << base << ");" << endl;
                                                os << indent << indent << indent << indent << "return (stack_top().gotof)("
                                                   << nonterminal_index << ", v);" << endl;
                                                os << indent << indent << indent << "}" << endl;
                                        } else {
                                                os << indent << indent << indent << "// run_semantic_action();" << endl;
                                                os << indent << indent << indent << "pop_stack( "
                                                   << base
                                                   << " );" << endl;
                                                os << indent << indent << indent << "return (stack_top().gotof)("
                                                   << nonterminal_index << ", ValueType());" << endl;
                                        }
                                }
                                break;
                        case zw::gr::action_accept:
                                os << indent << indent << indent << "// accept" << endl
                                   << indent << indent << indent << "// run_semantic_action();" << endl
                                   << indent << indent << indent << "accepted_ = true;" << endl
                                   << indent << indent << indent << "accepted_value_ = *get_arg(1, 0);" << endl // implicit root
                                   << indent << indent << indent << "return false;" << endl;
                                break;
                        case zw::gr::action_error:
                                os << indent << indent << indent << "sa_.syntax_error();" << endl;
                                os << indent << indent << indent << "error_ = true;" << endl; 
                                os << indent << indent << indent << "return false;" << endl;
                                break;
                        }

                        // action footer
                }

                // dispatcher footer
                os << indent << indent << "default:" << endl
                   << indent << indent << indent << "sa_.syntax_error();" << endl
                   << indent << indent << indent << "error_ = true;" << endl
                   << indent << indent << indent << "return false;" << endl
                   << indent << indent << "}" << endl;

                // state footer
                os << indent << "}" << endl << endl;
        }

        // parser class footer
        os << "}" << endl << endl;

}

