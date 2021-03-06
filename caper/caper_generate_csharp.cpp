#include "caper_ast.hpp"
#include "caper_generate_csharp.hpp"
#include <algorithm>

struct semantic_action_entry {
        std::string                     name;
        std::vector< std::string >      args;

        bool operator<( const semantic_action_entry& x ) const
        { 
                if( name < x.name ) { return true; }
                if( x.name < name ) { return false; }
                return args < x.args;
        }
};

void generate_csharp(
        const std::string&                      filename,
        std::ostream&                           os,
        const GenerateOptions&                  options,
        const symbol_map_type&                  terminal_types,
        const symbol_map_type&                  nonterminal_types,
        const std::map< size_t, std::string >&  token_id_map,
        const action_map_type&                  actions,
        const tgt::parsing_table&               table )
{
        // using header
        os << "using System.Collections.Generic;\n";
        os << "\n";

        // namespace header
        os << "namespace " << options.namespace_name << "\n"
           << "{\n";

        // enum Token
        if( !options.external_token ) {
                // token enumeration
                os << "	" << options.access_modifier << "enum Token\n"
                   << "	{\n";
                for( size_t i = 0 ; i < token_id_map.size() ; i++ ) {
                        os << "		" << options.token_prefix << (*token_id_map.find( i )).second << ",\n";
                }
                os << "	}\n\n";
        }

        // ISemanticAction interface
        std::set< semantic_action_entry > ss;

        for( action_map_type::const_iterator it = actions.begin();
             it != actions.end();
             ++it ) {
                const tgt::parsing_table::rule_type& rule = it->first;
                const semantic_action& sa = it->second;
                
                semantic_action_entry sae;
                sae.name = sa.name;

                 // 1st argument = out parameter
                sae.args.push_back( (*nonterminal_types.find( rule.left().name() )).second );
                
                for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                        sae.args.push_back( (*sa.args.find( l )).second.type );
                }

                ss.insert( sae );
        }

        std::set< std::string > types;
        for( symbol_map_type::const_iterator i = terminal_types.begin() ;
             i != terminal_types.end() ;
             ++i ) {
                if( (*i).second != "" ) { types.insert( (*i).second ); }
        }

        for( symbol_map_type::const_iterator i = nonterminal_types.begin() ;
             i != nonterminal_types.end() ;
             ++i ) {
                if( (*i).second != "" ) { types.insert( (*i).second ); }
        }

        os << "	" << options.access_modifier << "interface ISemanticAction\n"
           << "	{\n"
           << "		void syntax_error();\n"
           << "		void stack_overflow();\n\n";

		/*
        for( std::set< std::string >::const_iterator i = types.begin();
             i != types.end();
             ++i ) {
                os << "		" << "void upcast( out object x, " << (*i) << " y );\n";
        }
        
        for( std::set< std::string >::const_iterator i = types.begin();
             i != types.end();
             ++i ) {
                os << "		" << "void downcast( out " << (*i) << " x, object y );\n";
        }*/
        os << "\n";
        
		std::set< std::string > methods;

        for( std::set< semantic_action_entry >::const_iterator it = ss.begin();
             it != ss.end();
             ++it ) {
				std::stringstream methodstream;
				methodstream << "		void " << (*it).name << "( out ";
				bool first = true;
				for( size_t l = 0 ; l < (*it).args.size() ; l++ ) {
					if( first ) { first = false; } else { methodstream << ", "; }
					methodstream << ((*it).args[l]) << " " << "arg" << l;
				}
				methodstream << ");\n";
				methods.insert(methodstream.str());
        }

		for(std::set<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) os << *it;

        os << "\t}\n\n";

        // parser class
        os << "	" << options.access_modifier << "class Parser\n"
           << "	{\n"

        // stack_frame clas
           << "		private class stack_frame\n"
           << "		{\n"
           << "			public state_type state;\n"
           << "			public gotof_type gotof;\n"
           << "			public object value;\n\n"
           << "			public stack_frame(state_type s, gotof_type g, object v)\n"
           << "			{\n"
           << "				state = s; gotof = g; value = v;\n"
           << "			}\n"
           << "		}\n\n"
        // Stack class
           << "		private class Stack\n"
           << "		{\n"
                
           << "			private List<stack_frame> stack = new List<stack_frame>();\n"
           << "			private List<stack_frame> tmp = new List<stack_frame>();\n"
           << "			private int gap;\n"
           << "\n"
                
           << "			public Stack(){ this.gap = 0; }\n"
                
           << "			public void reset_tmp()\n"
           << "			{\n"
           << "				this.gap = this.stack.Count;\n"
           << "				this.tmp.Clear();\n"
           << "			}\n"
           << "\n"
                
           << "			public void commit_tmp()\n"
           << "			{\n"
           << "				int size = this.gap + this.tmp.Count;\n"
           << "				if(size > this.stack.Capacity) this.stack.Capacity = size;\n"
           << "				this.stack.RemoveRange(this.gap, this.stack.Count - this.gap);\n"
           << "				this.stack.AddRange(this.tmp);\n"
           << "			}\n"
                
           << "			public bool push(stack_frame f)\n"
           << "			{\n"
           << "				this.tmp.Add(f);\n"
           << "				return true;\n"
           << "			}\n"
           << "\n"
                
           << "			public void pop(int n)\n"
           << "			{\n"
           << "				if(this.tmp.Count < n)\n"
           << "				{\n"
           << "				n -= this.tmp.Count;\n"
           << "				this.tmp.Clear();\n"
           << "				this.gap -= n;\n"
           << "				}else\n"
           << "				{\n"
           << "					this.tmp.RemoveRange(this.tmp.Count - n, n);\n"
           << "				}\n"
           << "			}\n"
           << "\n"
                
           << "			public stack_frame top()\n"
           << "			{\n"
           << "				if( this.tmp.Count != 0 )\n"
           << "				{\n"
           << "					return this.tmp[this.tmp.Count - 1];\n"
           << "				}else\n"
           << "				{\n"
           << "					return this.stack[this.gap - 1];\n"
           << "				}\n"
           << "			}\n"
           << "\n"
                
           << "			public stack_frame get_arg(int b, int i)\n"
           << "			{\n"
           << "				int n = this.tmp.Count;\n"
           << "				if(b - i <= n)\n"
           << "				{\n"
           << "					return this.tmp[n - (b - i)];\n"
           << "				}else\n"
           << "				{\n"
           << "					return this.stack[this.gap - (b - n) + i];\n"
           << "				}\n"
           << "			}\n"
           << "\n"
                
           << "			public void clear()\n"
           << "			{\n"
           << "				this.stack.Clear();\n"
           << "			}\n"
           << "\n"
                
           << "		} // class Stack\n\n"
                
       // delegate
           << "		private delegate bool state_type(Token token, object value);\n"
           << "		private delegate bool gotof_type(int i, object value);\n"
           << "\n"
                
       // constructor
           << "		public Parser(ISemanticAction sa)\n"
           << "		{\n"
           << "			this.stack = new Stack();\n"
           << "			this.sa = sa;\n"
           << "			this.reset();\n"
           << "		}\n"
           << "\n\n"
                
       // public member
           << "		public void reset()\n"
           << "		{\n"
           << "			this.error = false;\n"
           << "			this.accepted = false;\n"
           << "			this.clear_stack();\n"
           << "			this.reset_tmp_stack();\n"
           << "			if( this.push_stack( "
           << "this.state_" << table.first_state() << ", "
           << "this.gotof_" << table.first_state()
           << ", " << "new object()) )\n"
           << "			{\n"
           << "				this.commit_tmp_stack();\n"
           << "			}else\n"
           << "			{\n"
           << "				this.sa.stack_overflow();\n"
           << "				this.error = true;\n"
           << "			}\n"
           << "		}\n"
                
           << "		public bool post(Token token,object value)\n"
           << "		{\n"
           << "			System.Diagnostics.Debug.Assert(!this.error);\n"
           << "			this.reset_tmp_stack();\n"
           << "			while(stack_top().state(token, value));\n"
           << "			if( !this.error )\n"
           << "			{\n"
           << "				this.commit_tmp_stack();\n"
           << "			}\n"
           << "			return this.accepted;\n"
           << "		}\n\n"
                
           << "		public bool accept(out object v)\n"
           << "		{\n"
           << "			System.Diagnostics.Debug.Assert(this.accepted);\n"
           << "			if(this.error) { v = new object(); return false; }\n"
           << "			v = this.accepted_value;\n"
           << "			return true;\n"
           << "		}\n\n"

           << "		public bool Error() { return this.error; }\n\n"
                
       // private member
           << "		private ISemanticAction sa;\n"
           << "		private Stack stack;\n"
           << "		private bool accepted;\n"
           << "		private bool error;\n"
           << "		private object accepted_value;\n"
           << "\n"
                
           << "		private bool push_stack(state_type s, gotof_type g, object v)\n"
           << "		{\n"
           << "			bool f = this.stack.push(new stack_frame(s, g, v));\n"
           << "			System.Diagnostics.Debug.Assert(!this.error);\n"
           << "			if(!f)\n"
           << "			{\n"
           << "				this.error = true;\n"
           << "				this.sa.stack_overflow();\n"
           << "			}\n"
           << "			return f;\n"
           << "		}\n\n"
                
           << "		private void pop_stack(int n)\n"
           << "		{\n"
           << "			this.stack.pop(n);\n"
           << "		}\n\n"
                
           << "		private stack_frame stack_top()\n"
           << "		{\n"
           << "			return this.stack.top();\n"
           << "		}\n\n"
                
           << "		private object get_arg(int b, int i)\n"
           << "		{\n"
           << "			return stack.get_arg(b, i).value;\n"
           << "		}\n\n"
                
           << "		private void clear_stack()\n"
           << "		{\n"
           << "			this.stack.clear();\n"
           << "		}\n\n"
                
           << "		private void reset_tmp_stack()\n"
           << "		{\n"
           << "			this.stack.reset_tmp();\n"
           << "		}\n\n"
                
           << "		private void commit_tmp_stack()\n"
           << "		{\n"
           << "			this.stack.commit_tmp();\n"
           << "		}\n\n"
                ;
        
        // states handler
        for( tgt::parsing_table::states_type::const_iterator i = table.states().begin();
             i != table.states().end() ;
             ++i) {
                const tgt::parsing_table::state& s = *i;

                // gotof header
                os << "		bool gotof_" << s.no << "(int nonterminal_index, object v)\n"
                   << "		{\n";
                
                // gotof dispatcher
                std::stringstream ss;
                ss << "			switch(nonterminal_index)\n"
                   << "			{\n";

                bool output_switch = false;
                std::set<size_t> generated;
                for( tgt::parsing_table::rules_type::const_iterator j = table.rules().begin();
                     j != table.rules().end();
                     ++j ) {
                        size_t nonterminal_index = std::distance(
                                nonterminal_types.begin(),
                                nonterminal_types.find( (*j).left().name() ) );
                        
                        if( generated.find( nonterminal_index ) != generated.end() ) {
                                continue;
                        }
                        
                        tgt::parsing_table::state::goto_table_type::const_iterator k =
                                (*i).goto_table.find((*j).left());
                        
                        if( k != (*i).goto_table.end() ) {
                                ss << "				case " << nonterminal_index
                                   << ": return push_stack( this.state_" << (*k).second
                                   << ", this.gotof_" << (*k).second
                                   << ", v );\n";
                                output_switch = true;
                                generated.insert( nonterminal_index );
                        }
                }
                
                ss << "				default: System.Diagnostics.Debug.Assert(false); return false;\n"; 
                ss << "			}\n";
                if( output_switch ) {
                        os << ss.str();
                } else {
                        os << "			System.Diagnostics.Debug.Assert(false);\n"
                           << "			return true;\n";
                }
                
                // gotof footer
                os << "		}\n\n";
                
                // state header
                os << "		bool state_" << s.no << "(Token token, object value)\n";
                os << "		{\n";
                
                // dispatcher header
                os << "			switch(token)\n"
                   << "			{\n";
                
                // action table
                for( tgt::parsing_table::state::action_table_type::const_iterator j = s.action_table.begin();
                     j != s.action_table.end();
                     ++j) {
                        // action header 
                        os << "				case Token." << options.token_prefix
                           << (*token_id_map.find( (*j).first )).second << ":\n";
                        
                        // action
                        const tgt::parsing_table::action* a = &(*j).second;
                        switch( a->type ) {
                        case zw::gr::action_shift:
                                os << "				// shift\n"
                                   << "				push_stack( "
                                   << "this.state_" << a->dest_index << ", "
                                   << "this.gotof_" << a->dest_index << ", "
                                   << "value);\n"
                                   << "				return false;\n";
                                break;
                        case zw::gr::action_reduce:
                                os << "				// reduce\n";
                                {
                                        size_t base = table.rules()[ a->rule_index ].right().size();
				        
                                        const tgt::parsing_table::rule_type& rule = table.rules()[a->rule_index];
                                        action_map_type::const_iterator k = actions.find( rule );
                                        
                                        size_t nonterminal_index = std::distance(
                                                nonterminal_types.begin(),
                                                nonterminal_types.find( rule.left().name() ) );
                                        
                                        if( k != actions.end() ) {
                                                const semantic_action& sa = (*k).second;
                                                
                                                os << "				{\n";
                                                // automatic argument conversion
                                                for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                                                        const semantic_action_argument& arg =
                                                                (*sa.args.find( l )).second;
                                                        os << "                " << arg.type << " arg" << l
                                                           << " = (" << arg.type << ")get_arg(" << base
                                                           << ", " << arg.src_index << ");\n";
                                                }
                                                
                                                // semantic action
                                                os << "					"
                                                   << (*nonterminal_types.find( rule.left().name() )).second
                                                   << " r; " << "this.sa." << sa.name << "( out r ";
                                                for( size_t l = 0 ; l < sa.args.size() ; l++ ) {
                                                        os << ", arg" << l;
                                                }
                                                os << ");\n";
                                                
                                                // automatic return value conversion
                                                os << "					"
                                                   << "object v = (object)r;\n";
                                                os << "					pop_stack( "
                                                   << base
                                                   << ");\n";
                                                os << "					return stack_top().gotof("
                                                   << nonterminal_index << ", v);\n";
                                                os << "				}\n";
                                        } else {
                                                os << "				// run_semantic_action();\n";
                                                os << "				pop_stack( "
                                                   << base
                                                   << ");\n";
                                                os << "				return stack_top().gotof("
                                                   << nonterminal_index << ", new object());\n";
                                        }
                                }
                                break;
                        case zw::gr::action_accept:
                                os << "				// accept\n"
                                   << "				// run_semantic_action();\n"
                                   << "				this.accepted = true;\n"
                                   << "				this.accepted_value  = get_arg( 1, 0 );\n" // implicit root
                                   << "				return false;\n";
                                break;
                        case zw::gr::action_error:
                                os << "				this.sa.syntax_error();\n";
                                os << "				this.error = true;\n"; 
                                os << "				return false;\n";
                                break;
                        }
                        
                        // action footer
                }
                
                // dispatcher footer
                os << "			default:\n"
                   << "				this.sa.syntax_error();\n"
                   << "				this.error = true;\n"
                   << "				return false;\n"
                   << "			}\n"
                        
                // state footer
                   << "		}\n\n";
        }

        os << "	} // class Parser\n\n"
                ;
        
        // namespace footer
        os << "} // namespace " << options.namespace_name;

        // once footer
}

