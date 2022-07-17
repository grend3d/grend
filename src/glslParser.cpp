#include <exception>
#include <memory>

#include <grend/glslParser.hpp>

using namespace grendx;
using namespace p_comb;

static std::string parserSpec = R"PARSER(
# direct translation of glsl es 3.0 spec

# lexer rules
ADD_ASSIGN       *:= "+=" ;
AMPERSAND        *:= "&" ;
AND_ASSIGN       *:= "&=" ;
AND_OP           *:= "&&" ;
BANG             *:= "!" ;
BOOL             *:= "bool" ;
BOOLCONSTANT     *:= ("true" | "false");
BREAK            *:= "break" ;
BVEC2            *:= "TODO: bvec2" ;
BVEC3            *:= "TODO: bvec3" ;
BVEC4            *:= "TODO: bvec4" ;
CARET            *:= "^" ;
CASE             *:= "case" ;
CENTROID         *:= "TODO: centroid" ;
COLON            *:= ":" ;
COMMA            *:= "," ;
CONST            *:= "const" ;
CONTINUE         *:= "continue" ;
DASH             *:= "-" ;
DEC_OP           *:= "--" ;
DEFAULT          *:= "default" ;
DISCARD          *:= "discard" ;
DIV_ASSIGN       *:= "/=" ;
DO               *:= "do" ;
DOT              *:= "." ;
ELSE             *:= "else" ;
EQ_OP            *:= "==" ;
EQUAL            *:= "=" ;
# TODO: check this
FIELD_SELECTION *:= IDENTIFIER ;
FLAT            *:= "TODO: flat";
FLOAT           *:= "float" ;
# TODO: check this, exponent specifier (think I can reuse the json float parser here)
FLOATCONSTANT   *:= [(PLUS | DASH)] (({digit} DOT {digit})
                                     | (DOT {digit})
                                     | ({digit} DOT)) ["f"];
FOR             *:= "for" ;
GE_OP           *:= ">=" ;
HIGH_PRECISION  *:= "highp" ;
IDENTIFIER      *:= identifier ;
IF              *:= "if";
IN              *:= "in" whitespace;
INC_OP          *:= "++";
INOUT           *:= "inout";
INT             *:= "int" ;
# TODO: check this
INTCONSTANT     *:= [(PLUS | DASH)] {digit} ;
INVARIANT       *:= "TODO: invariant" ;
ISAMPLER2D      *:= "TODO: ISAMPLER2D" ;
ISAMPLER2DARRAY *:= "TODO: ISAMPLER2DARRAY" ;
ISAMPLER3D      *:= "TODO: ISAMPLER3D" ;
ISAMPLERCUBE    *:= "TODO: ISAMPLERCUBE" ;
IVEC2           *:= "ivec2" ;
IVEC3           *:= "ivec3" ;
IVEC4           *:= "ivec4" ;
LAYOUT          *:= "layout" ;
LEFT_ANGLE      *:= "<" ;
# TODO: check this
LEFT_ASSIGN      *:= "=" ;
LEFT_BRACE       *:= "{" ;
LEFT_BRACKET     *:= "[" ;
LEFT_OP          *:= "<<" ;
LEFT_PAREN       *:= "(" ;
LE_OP            *:= "<=" ;
LOW_PRECISION    *:= "lowp" ;
MAT2             *:= "mat2" ;
MAT2X2           *:= "mat2x2" ;
MAT2X3           *:= "mat2x3" ;
MAT2X4           *:= "mat2x4" ;
MAT3             *:= "mat3" ;
MAT3X2           *:= "mat3x2" ;
MAT3X3           *:= "mat3x3" ;
MAT3X4           *:= "mat3x4" ;
MAT4             *:= "mat4" ;
MAT4X2           *:= "mat4x2" ;
MAT4X3           *:= "mat4x3" ;
MAT4X4           *:= "mat4x4" ;
MEDIUM_PRECISION *:= "mediump" ;
MOD_ASSIGN       *:= "%=" ;
MUL_ASSIGN       *:= "*=" ;
NE_OP            *:= "!=" ;
OR_ASSIGN        *:= "|=" ;
OR_OP            *:= "||" ;
OUT              *:= "out" ;
PERCENT          *:= "%" ;
PLUS             *:= "+" ;
PRECISION        *:= "precision" ;
QUESTION         *:= "?" ;
RETURN           *:= "return" ;
RIGHT_ANGLE      *:= ">" ;
# TODO: check
RIGHT_ASSIGN          *:= "=" ;
RIGHT_BRACE           *:= "}" ;
RIGHT_BRACKET         *:= "]" ;
RIGHT_OP              *:= ">>" ;
RIGHT_PAREN           *:= ")";
SAMPLER2D             *:= "sampler2D" ;
# TODO: pretty sure this is a core opengl feature, not part of gles3,
#       maybe implement full 3.3/4.3/maybe 4.5-ish core parsing?
SAMPLER2DMS           *:= "sampler2DMS" ;
# TODO: check
SAMPLER2DARRAY        *:= "sampler2DArray" ;
SAMPLER2DARRAYSHADOW  *:= "sampler2DArrayShadow" ;
SAMPLER2DSHADOW       *:= "sampler2DShadow" ;
SAMPLER3D             *:= "sampler3D" ;
SAMPLERCUBE           *:= "samplerCube" ;
SAMPLERCUBESHADOW     *:= "samplerCubeShadow" ;
SEMICOLON             *:= ";" ;
SLASH                 *:= "/" ;
# TODO: check
SMOOTH                *:= "smooth" ;
STAR                  *:= "*" ;
STRUCT                *:= "struct" ;
SUB_ASSIGN            *:= "-=" ;
SWITCH                *:= "switch" ;
TILDE                 *:= "~" ;
TODO                  *:= "TODO: not todo" ;
# TODO: check
TYPE_NAME             *:= identifier ;            
UINT                  *:= "uint" ;
# TODO: check
UINTCONSTANT          *:= {digit} "u";
UNIFORM               *:= "uniform";
USAMPLER2D            *:= "TODO: USAMPLER2D" ;
USAMPLER2DARRAY       *:= "TODO: USAMPLER2DARRAY" ;
USAMPLER3D            *:= "TODO: USAMPLER3D" ;
USAMPLERCUBE          *:= "TODO: USAMPLERCUBE" ;
UVEC2                 *:= "uvec2" ;
UVEC3                 *:= "uvec3" ;
UVEC4                 *:= "uvec4" ;
VEC2                  *:= "vec2" ;
VEC3                  *:= "vec3" ;
VEC4                  *:= "vec4" ;
VERTICAL_BAR          *:= "|" ;
VOID                  *:= "void" ;
WHILE                 *:= "while" ;
XOR_ASSIGN            *:= "^=" ;
XOR_OP                *:= "^" ;

# TODO: redundant
variable_identifier *= identifier;

primary_expression :=
      variable_identifier
    | FLOATCONSTANT
    | UINTCONSTANT
    | INTCONSTANT
    | BOOLCONSTANT
    | (LEFT_PAREN expression RIGHT_PAREN)
    ;

# avoid left recursion
base_postfix :=
      function_call
    | primary_expression
    ;

postfix_op :=
      (LEFT_BRACKET integer_expression RIGHT_BRACKET)
    | (DOT FIELD_SELECTION)
    | INC_OP
    | DEC_OP
    ;

postfix_expression :=
    base_postfix [{postfix_op}] [DOT function_call_generic];

integer_expression :=
    expression
    ;

function_call :=
    function_call_or_method
    ;

#function_call_or_method :=
#      function_call_generic
#    | (postfix_expression DOT function_call_generic)
#    ;

function_call_or_method :=
      function_call_generic
    ;

function_call_generic :=
    (function_call_header_with_parameters
     | function_call_header_no_parameters) RIGHT_PAREN
     ;

function_call_header_no_parameters :=
    function_call_header [VOID]
    ;

#function_call_header_with_parameters :=
#      (function_call_header assignment_expression)
#    | (function_call_header_with_parameters COMMA assignment_expression)
#    ;

function_call_header_with_parameters :=
      (function_call_header function_call_argument_list)
    ;

function_call_argument_list :=
	assignment_expression [COMMA function_call_argument_list]
	;

function_call_header :=
    function_identifier LEFT_PAREN
    ;

function_identifier :=
      type_specifier
    | IDENTIFIER
    | FIELD_SELECTION
    ;

unary_expression :=
      (INC_OP unary_expression)
    | (DEC_OP unary_expression)
    | postfix_expression
    | (unary_operator unary_expression)
    ;

unary_operator :=
      PLUS
    | DASH
    | BANG
    | TILDE
    ;

multiplicative_expression :=
    unary_expression [(STAR | SLASH | PERCENT) multiplicative_expression]
    ;

additive_expression :=
    multiplicative_expression [(PLUS | DASH) additive_expression]
    ;

shift_expression :=
    additive_expression [(LEFT_OP | RIGHT_OP) shift_expression]
    ;

relational_expression :=
    shift_expression [(LE_OP | GE_OP | LEFT_ANGLE | RIGHT_ANGLE)
                      relational_expression]
    ;

equality_expression :=
    relational_expression [(EQ_OP | NE_OP) equality_expression]
    ;

and_expression :=
    equality_expression [AMPERSAND and_expression]
    ;

exclusive_or_expression :=
    and_expression [CARET exclusive_or_expression]
    ;

inclusive_or_expression :=
    exclusive_or_expression [VERTICAL_BAR inclusive_or_expression]
    ;

logical_and_expression :=
    inclusive_or_expression [AND_OP logical_and_expression]
    ;

logical_xor_expression :=
    logical_and_expression [XOR_OP logical_xor_expression]
    ;

logical_or_expression :=
    logical_xor_expression [OR_OP logical_or_expression]
    ;

conditional_expression :=
    logical_or_expression [QUESTION expression COLON assignment_expression]
    ;

assignment_expression :=
      (unary_expression assignment_operator assignment_expression)
    | conditional_expression
    ;

assignment_operator :=
      EQUAL
    | MUL_ASSIGN
    | DIV_ASSIGN
    | MOD_ASSIGN
    | ADD_ASSIGN
    | SUB_ASSIGN
    | LEFT_ASSIGN
    | RIGHT_ASSIGN
    | AND_ASSIGN
    | XOR_ASSIGN
    | OR_ASSIGN
    ;

expression :=
    assignment_expression [COMMA expression]
    ;

constant_expression :=
    conditional_expression
    ;

declaration :=
      (function_prototype SEMICOLON)
    | (init_declarator_list SEMICOLON)
    | (PRECISION precision_qualifier type_specifier_no_prec SEMICOLON)
    | (type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE
          [IDENTIFIER [LEFT_BRACKET constant_expression RIGHT_BRACKET]]
       SEMICOLON)
    | (type_qualifier SEMICOLON)
    ;

function_prototype :=
    function_declarator RIGHT_PAREN
    ;

# TODO: double check this
function_declarator :=
      function_header_with_parameters
    | (function_header [VOID])
    ;

#function_header_with_parameters :=
#    function_header [COMMA function_header_with_parameters] parameter_declaration
#    ;

function_header_with_parameters :=
    function_header function_parameter_list
    ;

function_parameter_list :=
    parameter_declaration [COMMA function_parameter_list]
    ;

function_header :=
    fully_specified_type IDENTIFIER LEFT_PAREN
    ;

parameter_declarator :=
    type_specifier IDENTIFIER [LEFT_BRACKET constant_expression RIGHT_BRACKET]
    ;

#parameter_declaration :=
#      (parameter_type_qualifier parameter_qualifier parameter_declarator)
#    | (parameter_type_qualifier parameter_qualifier parameter_type_specifier)
#    | (parameter_qualifier parameter_declarator)
#    | (parameter_qualifier parameter_type_specifier)
#    ;

parameter_declaration :=
    [parameter_type_qualifier] parameter_qualifier
        (parameter_declarator | parameter_type_qualifier)
    ;

# optional
parameter_qualifier :=
    [(IN | OUT | INOUT)]
    ;

parameter_type_specifier :=
    type_specifier ;

#init_declarator_list :=
#    single_declaration [
#          [COMMA IDENTIFIER ((EQUAL initializer) | init_declarator_list)]
#        | [LEFT_BRACKET INTCONSTANT RIGHT_BRACKET [EQUAL initializer]]
#    ]
#    ;

init_declarator_list :=
    single_declaration [{init_declarator_asdf}]
    ;

init_declarator_asdf :=
	COMMA IDENTIFIER [LEFT_BRACKET [constant_expression] RIGHT_BRACKET] [EQUAL initializer]
	;

single_declaration :=
    fully_specified_type
        [IDENTIFIER [LEFT_BRACKET [constant_expression] RIGHT_BRACKET]]
		            [EQUAL initializer]
    ;

fully_specified_type :=
    [type_qualifier] type_specifier
    ;

invariant_qualifier :=
    INVARIANT
    ;

interpolation_qualifier :=
      SMOOTH
    | FLAT
    ;

layout_qualifier :=
    LAYOUT LEFT_PAREN layout_qualifier_id_list RIGHT_PAREN
    ;

layout_qualifier_id_list :=
    layout_qualifier_id [COMMA layout_qualifier_id_list]
    ;

layout_qualifier_id :=
    IDENTIFIER [EQUAL (INTCONSTANT | UINTCONSTANT)]
    ;

parameter_type_qualifier :=
    CONST
    ;

type_qualifier :=
      (layout_qualifier storage_qualifier)
    | (interpolation_qualifier [storage_qualifier])
    | (invariant_qualifier interpolation_qualifier storage_qualifier)
    | (invariant_qualifier storage_qualifier)
    | layout_qualifier
    | storage_qualifier
    ;

storage_qualifier :=
      CONST
    | IN
    | OUT
    | (CENTROID IN)
    | (CENTROID OUT)
    | UNIFORM
    ;

type_specifier :=
      type_specifier_no_prec
    | (precision_qualifier type_specifier_no_prec)
    ;

type_specifier_no_prec :=
    type_specifier_nonarray [LEFT_BRACKET [constant_expression] RIGHT_BRACKET]
    ;

type_specifier_nonarray :=
      VOID
    | FLOAT
    | INT
    | UINT
    | BOOL
    | VEC2  | VEC3  | VEC4
    | BVEC2 | BVEC3 | BVEC4
    | IVEC2 | IVEC3 | IVEC4
    | UVEC2 | UVEC3 | UVEC4
    | MAT2  | MAT3  | MAT4
    | MAT2X2 | MAT2X3 | MAT2X4
    | MAT3X2 | MAT3X3 | MAT3X4
    | MAT4X2 | MAT4X3 | MAT4X4
    | SAMPLER2DMS
    | SAMPLER2D | SAMPLER3D | SAMPLERCUBE
    | SAMPLER2DSHADOW | SAMPLERCUBESHADOW | SAMPLER2DARRAY
    | SAMPLER2DARRAYSHADOW
    | ISAMPLER2D | ISAMPLER3D | ISAMPLERCUBE
    | ISAMPLER2DARRAY | USAMPLER2D | USAMPLER3D
    | USAMPLERCUBE | USAMPLER2DARRAY
    | struct_specifier
    | TYPE_NAME
    ;

precision_qualifier :=
      HIGH_PRECISION
    | MEDIUM_PRECISION
    | LOW_PRECISION
    ;

struct_specifier :=
    STRUCT [IDENTIFIER] LEFT_BRACE struct_declaration_list RIGHT_BRACE
    ;

struct_declaration_list :=
    struct_declaration [struct_declaration_list]
    ;

#struct_declaration :=
#    [type_qualifier] type_specifier struct_declaration_list SEMICOLON
#    ;
struct_declaration :=
    [type_qualifier] type_specifier struct_declarator_list SEMICOLON
    ;

struct_declarator_list :=
    struct_declarator [COMMA struct_declarator_list]
    ;

struct_declarator :=
    IDENTIFIER [LEFT_BRACKET [constant_expression] RIGHT_BRACKET]
    ;

initializer :=
    assignment_expression
    ;

declaration_statement :=
    declaration
    ;

statement :=
      compound_statement_with_scope
    | simple_statement
    ;

statement_no_new_scope :=
      compound_statement_no_new_scope
    | simple_statement
    ;

statement_with_scope :=
      compound_statement_no_new_scope
    | simple_statement
    ;

simple_statement :=
      declaration_statement
    | expression_statement
    | selection_statement
    | switch_statement
    | case_label
    | iteration_statement
    | jump_statement
    ;

compound_statement_with_scope :=
    LEFT_BRACE [statement_list] RIGHT_BRACE
    ;

compound_statement_no_new_scope :=
    LEFT_BRACE [statement_list] RIGHT_BRACE
    ;

statement_list :=
    statement [statement_list]
    ;

expression_statement :=
    [expression] SEMICOLON
    ;

selection_statement :=
    IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement
    ;

selection_rest_statement :=
    statement_with_scope [ELSE statement_with_scope]
    ;

condition :=
      expression
    | (fully_specified_type IDENTIFIER EQUAL initializer)
    ;

switch_statement :=
    SWITCH LEFT_PAREN expression RIGHT_PAREN
        LEFT_BRACE switch_statement_list RIGHT_BRACE
    ;

# switch_statement_list can be empty
##switch_statement_list :=
#    [statement_list]
#    ;

switch_statement_list :=
    statement [statement_list]
    ;

case_label :=
      (CASE expression COLON)
    | (DEFAULT COLON)
    ;

iteration_statement :=
      (WHILE LEFT_PAREN condition RIGHT_PAREN statement_no_new_scope)
    | (DO statement_with_scope WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON)
    | (FOR LEFT_PAREN for_init_statement for_rest_statement RIGHT_PAREN
       statement_no_new_scope)
    ;

for_init_statement :=
      expression_statement
    | declaration_statement
    ;

conditionopt :=
    [condition]
    ;

for_rest_statement :=
    conditionopt SEMICOLON [expression]
    ;

jump_statement :=
    (CONTINUE | BREAK | (RETURN [expression]) | DISCARD) SEMICOLON
    ;

translation_unit :=
	{external_declaration}
    ;

external_declaration :=
      function_definition
    | declaration
    ;

function_definition :=
    function_prototype compound_statement_no_new_scope
    ;

main := translation_unit EOF;
)PARSER";

cparser grendx::loadGlslParser(void) {
	auto meh = evaluate(ebnfish, parserSpec);

	if (!meh.matched) {
		throw std::logic_error("Couldn't parse parser spec...?");
	}

	return compile_parser(meh.tokens);
}

container grendx::parseGlsl(std::string_view source) {
	static auto glslParser = loadGlslParser();
	auto meh = evaluate(glslParser["main"], source);

	if (meh.matched) {
		return meh.tokens;
	} else {
		return {};
	}
}
