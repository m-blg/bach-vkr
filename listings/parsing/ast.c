truct_decl(C_Ast_Ident)
struct_decl(C_Ast_Literal)
struct_decl(C_Ast_Expr)
struct_decl(C_Ast_Stmt)
struct_decl(C_Ast_Type)
struct_decl(C_Ast_Decl)
struct_decl(C_Ast_Node)

enum_def(C_Ast_NodeKind,
    C_AST_NODE_KIND_INVALID,
    // TERMINALS
    C_AST_NODE_KIND_IDENT,
    C_AST_NODE_KIND_LITERAL,

    // NON-TERMINALS

    C_AST_NODE_KIND_EXPR,
    C_AST_NODE_KIND_STMT,
    C_AST_NODE_KIND_TYPE_NAME,
    C_AST_NODE_KIND_DECL,
    C_AST_NODE_KIND_TR_UNIT,

#ifdef EXTENDED_C
    C_AST_NODE_KIND_AT_DIRECTIVE,
#endif // EXTENDED_C
    
)


#define C_AST_NODE_BASE \
    C_Ast_NodeKind kind;\
    C_ParserSpan span;\

struct_def(C_Ast_Ident, {
    C_AST_NODE_BASE

    str_t name;
})
#define c_ast_ident_new(name, span) \
    ((C_Ast_Ident) {\
        .kind = C_AST_NODE_KIND_IDENT, \
        .name = name, \
        .span = span\
        })\

enum_def(C_Ast_LiteralKind, 
    C_AST_LITERAL_KIND_INVALID,
    C_AST_LITERAL_KIND_STRING,
    C_AST_LITERAL_KIND_CHAR,
    C_AST_LITERAL_KIND_NUMBER,
    C_AST_LITERAL_KIND_COMPOUND,
) 

// // TODO
// enum_def(C_NumberType,
//     C_NUMBER_TYPE_UINT,
//     C_NUMBER_TYPE_INT,
// )

#define C_AST_NODE_LITERAL_BASE \
    C_AST_NODE_BASE \
    C_Ast_LiteralKind lit_kind;

struct_def(C_Ast_LiteralNumber, {
    C_AST_NODE_LITERAL_BASE
    C_TokenNumLiteral *t_num_lit;
})
struct_def(C_Ast_LiteralString, {
    C_AST_NODE_LITERAL_BASE
    C_TokenStringLiteral *t_str_lit;
})
struct_def(C_Ast_LiteralChar, {
    C_AST_NODE_LITERAL_BASE
    C_TokenCharLiteral *t_char_lit;
})


struct_def(C_Ast_LiteralCompoundEntry, {
    C_Ast_Ident NLB(*)name;
    C_Ast_Expr *value;
})

struct_def(C_Ast_LiteralCompound, {
    C_AST_NODE_LITERAL_BASE
    C_Ast_Type *ty;
    darr_T(C_Ast_LiteralCompoundEntry) entries;
})

struct_def(C_Ast_Literal, {
    union {
    struct{
        C_AST_NODE_LITERAL_BASE
    };
        C_Ast_LiteralNumber l_num;
        C_Ast_LiteralString l_str;
        C_Ast_LiteralChar l_char;
        C_Ast_LiteralCompound l_compound;
    };
})

// if I'm gonna do expr interpretation, I need NodeValue
// how do I store data associated with a type, like vtable, which can be diffrent per type

enum_def(C_Ast_ExprKind, 
    C_AST_EXPR_KIND_INVALID,
    C_AST_EXPR_KIND_IDENT,
    C_AST_EXPR_KIND_LITERAL,
    C_AST_EXPR_KIND_UNOP,
    C_AST_EXPR_KIND_BINOP,
    C_AST_EXPR_KIND_CONDOP, // ?:
    C_AST_EXPR_KIND_CAST,
    C_AST_EXPR_KIND_FN_CALL,
    C_AST_EXPR_KIND_ARRAY_SUB,
    C_AST_EXPR_KIND_COMPOUND, // expr, expr, ... expr
)


typedef u8_t C_ExprFlags; 

enum_def(C_ExprFlag,
    C_EXPR_FLAG_EMPTY = (u8_t)0,
    C_EXPR_FLAG_IN_PARENS = (u8_t)1 << 0,
    C_EXPR_FLAG_STRICT_PRECEDENCE = (u8_t)1 << 1, // used in parsing only
    C_EXPR_FLAG_IS_PREFIX = (u8_t)1 << 2, // is unary operator prefix or postfix
);

#define C_AST_NODE_EXPR_BASE \
    C_AST_NODE_BASE \
    C_Ast_ExprKind expr_kind; \
    C_ExprFlags flags; // right now only IN_PARENS is used

// though better to put pointer to C_Ast_Ident, but it's simple so it's a small optimization
struct_def(C_Ast_ExprIdent, {
    C_AST_NODE_EXPR_BASE

    C_Ast_Ident *ident; 
})
struct_def(C_Ast_ExprLit, {
    C_AST_NODE_EXPR_BASE

    C_Ast_Literal *lit; 
})
struct_def(C_Ast_ExprUnOp, {
    C_AST_NODE_EXPR_BASE
    
    C_OperatorKind op;
    C_Ast_Expr *e_operand;
})
struct_def(C_Ast_ExprBinOp, {
    C_AST_NODE_EXPR_BASE
    
    C_OperatorKind op;
    C_Ast_Expr *e_lhs;   
    C_Ast_Expr *e_rhs;   
})
struct_def(C_Ast_ExprCondOp, {
    C_AST_NODE_EXPR_BASE
    
    C_Ast_Expr *e_cond;   
    C_Ast_Expr *e_then;   
    C_Ast_Expr *e_else;
})

struct_def(C_Ast_ExprCast, {
    C_AST_NODE_EXPR_BASE
    
    C_Ast_Type *ty;   
    C_Ast_Expr *e_rhs;   
})

struct_def(C_Ast_ExprArraySub, {
    C_AST_NODE_EXPR_BASE
    
    C_Ast_Expr *array;
    C_Ast_Expr NLB(*)arg;
})
struct_def(C_Ast_ExprFnCall, {
    C_AST_NODE_EXPR_BASE
    
    C_Ast_Expr *caller;
    NLB(darr_T(C_Ast_Expr *))args;
})

struct_def(C_Ast_ExprCompound, {
    C_AST_NODE_EXPR_BASE
    
    darr_T(C_Ast_Expr *) items;
})

struct_def(C_Ast_Expr, {
    union {
    struct{
        C_AST_NODE_EXPR_BASE
    };
        C_Ast_ExprIdent e_ident;
        C_Ast_ExprLit e_lit;
        
        C_Ast_ExprUnOp e_un_op;
        C_Ast_ExprBinOp e_bin_op;
        C_Ast_ExprCondOp e_cond_op;
        C_Ast_ExprCast e_cast;
        C_Ast_ExprFnCall e_fn_call;
        C_Ast_ExprArraySub e_array_sub;

        C_Ast_ExprCompound e_compound;
    };
})

// struct_def(C_Ast_IfStmt, {
//     C_Ast_NodeKind kind;
//     C_Ast_Expr *condition;
//     C_Ast_Block *body;

//     C_ParserSpan span;
// })

enum_def(C_Ast_TypeKind, 
    C_AST_TYPE_KIND_INVALID,
    C_AST_TYPE_KIND_IDENT,
    C_AST_TYPE_KIND_POINTER,
    C_AST_TYPE_KIND_ARRAY,
    C_AST_TYPE_KIND_FUNCTION,
    C_AST_TYPE_KIND_STRUCT,
    C_AST_TYPE_KIND_UNION,
    C_AST_TYPE_KIND_ENUM
)

#define C_AST_NODE_TYPE_BASE \
    C_AST_NODE_BASE \
    C_Ast_TypeKind ty_kind; \

struct_decl(C_Ast_Type)

struct_def(C_Ast_TypeIdent, {
    C_AST_NODE_TYPE_BASE

    C_Ast_Ident *ident;
})
struct_def(C_Ast_TypePointer, {
    C_AST_NODE_TYPE_BASE

    C_Ast_Type *pointee;
})
struct_def(C_Ast_TypeArray, {
    C_AST_NODE_TYPE_BASE

    C_Ast_Type *item;
    C_Ast_Expr *count;
})
struct_def(C_Ast_TypeFn, {
    C_AST_NODE_TYPE_BASE

    C_Ast_Type *ret;
    darr_T(C_Ast_FnParam) args;

    // C_ParserSpan span;
})

// struct_def(C_Ast_RecordField, {
//     C_Ast_Type ty_name;
//     C_Ast_Ident *ident;
// })

struct_def(C_Ast_TypeRecord, {
    C_AST_NODE_TYPE_BASE

    C_Ast_Ident NLB(*)name;
    darr_T(C_Ast_Decl) fields;
})
typedef C_Ast_TypeRecord C_Ast_TypeStruct;
typedef C_Ast_TypeRecord C_Ast_TypeUnion;

struct_def(C_Ast_TypeEnum, {
    C_AST_NODE_TYPE_BASE

    C_Ast_Ident NLB(*)name;
    darr_T(C_Ast_Ident *) items;
})

struct_def(C_Ast_Type, {
    union {
    struct {
        C_AST_NODE_TYPE_BASE
    };
        C_Ast_TypeIdent ty_ident;
        C_Ast_TypePointer ty_pointer;
        C_Ast_TypeArray ty_array;
        C_Ast_TypeFn ty_fn;
        C_Ast_TypeStruct ty_struct;
        C_Ast_TypeUnion ty_union;
        C_Ast_TypeEnum ty_enum;
    };
})




// struct_def(C_Ast_Decl, {
//     C_Ast_NodeKind kind;
//     C_Ast_Type *ty;
//     C_Ast_Ident *name;
//     C_Ast_Expr *initializer;
// })
struct_def(C_Ast_Declarator, {
    C_Ast_Type *ty;
    C_Ast_Ident *name;
})
struct_def(C_Ast_InitDeclarator, {
    C_Ast_Type *ty;
    C_Ast_Ident *name;
    C_Ast_Expr NLB(*)initializer;
})

struct_def(C_Ast_FnParam, {
    C_ParserSpan span;

    C_Ast_Type *ty;
    C_Ast_Ident *name;
})

struct_decl(C_Ast_StmtCompound)


enum_def(C_Ast_DeclKind,
    C_AST_DECL_KIND_INVALID,
    C_AST_DECL_KIND_EMPTY, // ;;
    C_AST_DECL_KIND_TYPE_DECL, // `struct(enum, union) A {};`
    C_AST_DECL_KIND_VARIABLE, //  `struct A {} a;`, `int (*foo_p)(void), foo(void);`
    // C_AST_DECL_KIND_FN_DECL, // `int foo(void);`
    C_AST_DECL_KIND_FN_DEF, // `int foo(void) {}`
    C_AST_DECL_KIND_TYPEDEF, // typedef int Foo(void), Arr[3];
)

#define C_AST_NODE_DECL_BASE \
    C_AST_NODE_BASE \
    C_Ast_DeclKind decl_kind; \
    darr_T(C_Ast_AtDirective *) at_directives;

struct_def(C_Ast_DeclType, {
    C_AST_NODE_DECL_BASE
    C_Ast_Type *ty;
})
struct_def(C_Ast_DeclVar, {
    C_AST_NODE_DECL_BASE
    C_Ast_Type *ty;
    C_Ast_Ident *name;
    C_Ast_Expr NLB(*)initializer;
    NLB(darr_T(C_Ast_InitDeclarator)) others;
})
struct_def(C_Ast_DeclFnDef, {
    C_AST_NODE_DECL_BASE
    C_Ast_Type *ty;
    C_Ast_Ident *name;
    C_Ast_StmtCompound NLB(*)body;
})
struct_def(C_Ast_DeclTypedef, {
    C_AST_NODE_DECL_BASE
    C_Ast_Type *ty;
    C_Ast_Ident *name;
    NLB(darr_T(C_Ast_Declarator)) others;
})


/// @note for all init-declarators the leaf type should be the same
/// @example 
///    int field1 = 3, (*(*field1_2)[3])(int), *field1_3[] = {0};
///    the leaf type - 'int' is the same for three declarators with name
struct_def(C_Ast_Decl, {

    union {
    struct {
        C_AST_NODE_DECL_BASE
    };
        C_Ast_DeclType d_type; 
        C_Ast_DeclVar d_var; 
        // C_Ast_DeclFn d_fn; 
        C_Ast_DeclFnDef d_fn_def; 
        C_Ast_DeclTypedef d_typedef; 
    };

    // C_Ast_Type *ty;
    // C_Ast_Ident *name;
    // union {
    // C_Ast_Expr *initializer;
    // C_Ast_StmtCompound *body; // if ty->kind == .FUNCTION
    // };
    // darr_T(C_Ast_InitDeclarator) others;
})

enum_def(C_Ast_StmtKind, 
    C_AST_STMT_KIND_INVALID,
    C_AST_STMT_KIND_EXPR,

    C_AST_STMT_KIND_IF,
    C_AST_STMT_KIND_SWITCH,

    C_AST_STMT_KIND_LABEL,
    C_AST_STMT_KIND_CASE,
    C_AST_STMT_KIND_DEFAULT,

    C_AST_STMT_KIND_FOR,
    C_AST_STMT_KIND_WHILE,
    C_AST_STMT_KIND_DO_WHILE,

    C_AST_STMT_KIND_GOTO,
    C_AST_STMT_KIND_CONTINUE,
    C_AST_STMT_KIND_BREAK,
    C_AST_STMT_KIND_RETURN,

    C_AST_STMT_KIND_COMPOUND
)

#define C_AST_NODE_STMT_BASE \
    C_AST_NODE_BASE \
    C_Ast_StmtKind stmt_kind;

struct_decl(C_Ast_Stmt)

struct_def(C_Ast_StmtExpr, {
    C_AST_NODE_STMT_BASE

    C_Ast_Expr *e_expr;
})

struct_def(C_Ast_StmtIf, {
    C_AST_NODE_STMT_BASE

    C_Ast_Expr *e_cond;
    C_Ast_Stmt *s_then;
    C_Ast_Stmt NLB(*)s_else;
})
struct_def(C_Ast_StmtSwitch, {
    C_AST_NODE_STMT_BASE

    C_Ast_Expr *e_item;
    C_Ast_Stmt *s_body;
})

struct_def(C_Ast_StmtCase, {
    C_AST_NODE_STMT_BASE

    C_Ast_Expr *e_item;
})
struct_def(C_Ast_StmtDefault, {
    C_AST_NODE_STMT_BASE
})
struct_def(C_Ast_StmtLabel, {
    C_AST_NODE_STMT_BASE

    C_Ast_Ident *label;
})

struct_def(C_Ast_StmtFor, {
    C_AST_NODE_STMT_BASE

    C_Ast_Decl NLB(*)d_vars;
    C_Ast_Expr NLB(*)e_cond;
    C_Ast_Expr NLB(*)e_step;

    C_Ast_Stmt *s_body;
})
struct_def(C_Ast_StmtWhile, {
    C_AST_NODE_STMT_BASE

    C_Ast_Expr *e_cond;
    C_Ast_Stmt *s_body;
})
struct_def(C_Ast_StmtDoWhile, {
    C_AST_NODE_STMT_BASE

    C_Ast_Stmt *s_body;
    C_Ast_Expr *e_cond;
})

struct_def(C_Ast_StmtGoto, {
    C_AST_NODE_STMT_BASE

    C_Ast_Ident *label;
})
struct_def(C_Ast_StmtBreak, {
    C_AST_NODE_STMT_BASE
})
struct_def(C_Ast_StmtContinue, {
    C_AST_NODE_STMT_BASE
})
struct_def(C_Ast_StmtReturn, {
    C_AST_NODE_STMT_BASE

    C_Ast_Expr *e_ret;
})

struct_def(C_Ast_StmtCompound, {
    C_AST_NODE_STMT_BASE

    C_SymbolTable scope;
    darr_T(C_Ast_BlockItem) items;
})


struct_def(C_Ast_Stmt, {
    union {
    struct {
        C_AST_NODE_STMT_BASE
    };
        C_Ast_StmtExpr s_expr;

        C_Ast_StmtIf s_if;
        C_Ast_StmtSwitch s_switch;

        C_Ast_StmtCase s_case;
        C_Ast_StmtDefault s_default;
        C_Ast_StmtLabel s_label;

        C_Ast_StmtFor s_for;
        C_Ast_StmtWhile s_while;
        C_Ast_StmtDoWhile s_do_while;

        C_Ast_StmtGoto s_goto;
        C_Ast_StmtBreak s_break;
        C_Ast_StmtContinue s_continue;
        C_Ast_StmtReturn s_return;

        C_Ast_StmtCompound s_compound;
    };
})

/// Decl or Stmt
struct_def(C_Ast_BlockItem, {
    union {
    struct {
        C_AST_NODE_BASE
    };
        C_Ast_Decl decl;
        C_Ast_Stmt stmt;
    };
    
})

struct_def(C_Ast_TranslationUnit, {
    C_AST_NODE_BASE

    darr_T(C_Ast_Decl *) decls;
})


#ifdef EXTENDED_C
struct_def(C_Ast_AtDirective, {
    C_AST_NODE_BASE

    C_Ast_Ident *name;
    darr_T(C_Ast_Expr *) params;
})
#endif // EXTENDED_C

struct_def(C_Ast_Node, {
    union {
    struct {
        C_AST_NODE_BASE
    };

        C_Ast_Ident ident;
        C_Ast_Literal lit;
        C_Ast_Expr expr;
        C_Ast_Stmt stmt;
        C_Ast_Type ty;
        C_Ast_Decl decl;
        C_Ast_TranslationUnit tr_unit;
    #ifdef EXTENDED_C
        C_Ast_AtDirective at_directive;
    #endif // EXTENDED_C
    };
})