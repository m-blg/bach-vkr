
enum_def(C_Ast_NodeKind,
    C_AST_NODE_KIND_INVALID,
    C_AST_NODE_KIND_IDENT,
    C_AST_NODE_KIND_LITERAL,
    C_AST_NODE_KIND_EXPR,
    C_AST_NODE_KIND_STMT,
    C_AST_NODE_KIND_TYPE_NAME,
    C_AST_NODE_KIND_DECL,
    C_AST_NODE_KIND_TR_UNIT,
    )


#define C_AST_NODE_BASE \
    C_Ast_NodeKind kind;\
    C_ParserSpan span;\

struct_def(C_Ast_Ident, {
    C_AST_NODE_BASE

    str_t name;
})

enum_def(C_Ast_LiteralKind, 
    C_AST_LITERAL_KIND_STRING,
    C_AST_LITERAL_KIND_CHAR,
    C_AST_LITERAL_KIND_NUMBER
) 

struct_def(C_Ast_Literal, {
    C_AST_NODE_BASE

    union {
        C_Ast_LiteralKind lit_kind;

        C_Ast_NumberLiteral num;
        C_Ast_StringLiteral str;
        C_Ast_CharLiteral ch;
    };
})

enum_def(C_Ast_ExprKind, 
    C_AST_EXPR_KIND_IDENT,
    C_AST_EXPR_KIND_BINOP,
    C_AST_EXPR_KIND_UNOP,
    C_AST_EXPR_KIND_LITERAL,
    C_AST_EXPR_KIND_COMPOUND,
)

struct_def(C_Ast_Expr, {
    union {
        C_Ast_ExprKind expr_kind;

        C_Ast_Literal lit;
        //...
    };
})

enum_def(C_Ast_StmtKind, 
    //...
    C_AST_STMT_KIND_COMPOUND
)
struct_def(C_Ast_Stmt, {
    C_AST_NODE_BASE // TODO: put in leafs

    union {
        C_Ast_StmtKind stmt_kind;

        // ...
    };
})

enum_def(C_Ast_TypeKind, 
    C_AST_TYPE_KIND_IDENT,
    C_AST_TYPE_KIND_POINTER,
    C_AST_TYPE_KIND_ARRAY,
    C_AST_TYPE_KIND_FUNCTION,
    C_AST_TYPE_KIND_STRUCT,
    C_AST_TYPE_KIND_UNION,
    C_AST_TYPE_KIND_ENUM
)

struct_decl(C_Ast_Type)

struct_def(C_Ast_TypeIdent, {
    C_AST_NODE_BASE

    C_Ast_TypeKind ty_kind;
    str_t name;
})
struct_def(C_Ast_TypePointer, {
    C_AST_NODE_BASE

    C_Ast_TypeKind ty_kind;
    C_Ast_Type *pointee;
})
struct_def(C_Ast_TypeArray, {
    C_AST_NODE_BASE

    C_Ast_TypeKind ty_kind;
    C_Ast_Type *item;
    C_Ast_Expr *count;
})
struct_def(C_Ast_TypeFn, {
    C_AST_NODE_BASE

    C_Ast_TypeKind ty_kind;
    C_Ast_Type *ret;
    darr_T(C_Ast_Decl) *args;

})


struct_def(C_Ast_TypeRecord, {
    C_AST_NODE_BASE

    C_Ast_TypeKind ty_kind;
    C_Ast_Ident *name;
    darr_T(C_Ast_Decl) fields;
})
typedef C_Ast_TypeRecord C_Ast_TypeStruct;
typedef C_Ast_TypeRecord C_Ast_TypeUnion;

struct_def(C_Ast_TypeEnum, {
    C_AST_NODE_BASE

})

struct_def(C_Ast_Type, {
    union {
    struct {
        C_AST_NODE_BASE
        C_Ast_TypeKind ty_kind;
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


struct_def(C_Ast_InitDeclarator, {
    C_Ast_Type *ty;
    C_Ast_Ident *name;
    C_Ast_Expr *initializer;
})

struct_def(C_Ast_Decl, {
    C_Ast_NodeKind kind;
    C_ParserSpan span;

    C_Ast_Type *ty;
    C_Ast_Ident *name;
    C_Ast_Expr *initializer;
    darr_T(C_Ast_InitDeclarator) others;
})
struct_def(C_Ast_TranslationUnit, {
    C_Ast_NodeKind kind;
    C_ParserSpan span;
    darr_T(C_Ast_Decl) decls;
})


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
    };
})