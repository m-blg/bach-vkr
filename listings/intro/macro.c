#define struct_decl(name) \
typedef struct name name; \
struct name; \

#define enum_decl(name) \
typedef enum name name; \
enum name; \

#define struct_def(name, fields) \
typedef struct name name; \
struct name fields; \

#define enum_def(name, ...) \
typedef enum name name; \
enum name {__VA_ARGS__}; \

#define IS_OK(e) ({ \
    auto _r = (e); \
    *(int *)&(_r) == 0; \
    })
#define IS_ERR(e) ({ \
    auto _r = (e); \
    *(int *)&(_r) != 0; \
    })

#define TRY(expr) { \
    auto _e_ = (expr); \
    if (IS_ERR(_e_)) { \
        return _e_; \
    } \
  } \

#define ASSERT(expr) \
    if (!(expr)) { \
        fprintf(stderr, "ASSERT at %s:%d:\n", __FILE__, __LINE__); \
        panic(); \
    } \

#define ASSERTM(expr, msg) { \
    if (!(expr)) { \
        fprintf(stderr, "ASSERTM: %*s\nat %s:%d:\n", (int)(sizeof(msg)-1), msg, __FILE__, __LINE__); \
        panic(); \
    } \
}

#define ASSERT_OK(expr) { \
    auto _e_ = (expr); \
    if (IS_ERR(_e_)) { \
        fprintf(stderr, "ASSERT_OK at %s:%d:\n", __FILE__, __LINE__); \
        panic(); \
    } \
}

#define S(c_str) ((str_t) { .ptr  = (uchar_t *)(c_str), .byte_len = sizeof(c_str)-1 })