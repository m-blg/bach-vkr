StringFormatter
string_formatter_default(StreamWriter *sw) {
    return (StringFormatter) {
        .pad_level = 0,
        .pad_string = S("    "),
        .target = *sw,
        .is_line_padded = false,
    };
}

INLINE
void 
string_formatter_init_default(StringFormatter *self, StreamWriter *sw) {
    *self = string_formatter_default(sw);
}
INLINE
void 
string_formatter_init_string_default(StringFormatter *self, String *s) {
    StreamWriter sw = string_stream_writer(s);
    *self = string_formatter_default(&sw);
}
INLINE
void 
string_formatter_pad_inc(StringFormatter *self) {
    self->pad_level += 1;
}
INLINE
void 
string_formatter_pad_dec(StringFormatter *self) {
    ASSERT(self->pad_level > 0);
    self->pad_level -= 1;
}

FmtError
string_formatter_write(StringFormatter *fmt, const str_t s) {
    if (!fmt->is_line_padded) {
        for_in_range(_, 0, fmt->pad_level) {
            if (stream_writer_write(&fmt->target, 
                fmt->pad_string.byte_len, 
                fmt->pad_string.ptr) != IO_ERROR(OK) ) 
            {
                return FMT_ERROR(ERROR);
            }
        }
        fmt->is_line_padded = true;
    }
    if (stream_writer_write(&fmt->target, s.byte_len, s.ptr) != IO_ERROR(OK)) {
        return FMT_ERROR(ERROR);
    }
    return FMT_ERROR(OK);
}
FmtError
string_formatter_writeln(StringFormatter *fmt, const str_t s) {
    TRY(string_formatter_write(fmt, s));
    auto r = stream_writer_write(&fmt->target, 1, (uchar_t *)"\n");
    if (IS_ERR(r)) {
        return FMT_ERROR(ERROR);
    }
    fmt->is_line_padded = false;
    return FMT_ERROR(OK);
}

FmtError
string_formatter_write_no_pad(StringFormatter *fmt, const str_t s) {
    if (stream_writer_write(&fmt->target, s.byte_len, s.ptr) != IO_ERROR(OK)) {
        return FMT_ERROR(ERROR);
    }
    return FMT_ERROR(OK);
}
FmtError
string_formatter_done(StringFormatter *fmt) {
    if (stream_writer_flush(&fmt->target) != IO_ERROR(OK)) {
        return FMT_ERROR(ERROR);
    }
    return FMT_ERROR(OK);
}

#include <stdarg.h>


INLINE
FmtError
string_formatter_pad_line(StringFormatter *fmt) {
    for_in_range(_, 0, fmt->pad_level) {
        TRY(string_formatter_write_no_pad(fmt, fmt->pad_string));
    }
    fmt->is_line_padded = true;
    return FMT_ERROR(OK);
}

INLINE 
str_t
_str_from_begin_end_sub1(str_t b, str_t e) {
    str_t s =  str_from_begin_end(b, e);
    s.byte_len -= 1;
    return s;
}

// TODO: fix iteration byte counting
FmtError
string_formatter_write_fmt(StringFormatter *fmt, str_t fmt_str, ...) {
    va_list args;
    va_start(args, fmt_str);

    str_t s = (str_t) { };
    str_t e_fmt_str = fmt_str;
    

    rune_t r;
    while (str_len(e_fmt_str) > 0) {
        constexpr usize_t size = 64;
        uchar_t buffer[size];
        int len = 0;

        ASSERT_OK(str_next_rune(e_fmt_str, &r, &e_fmt_str));
        if (r == '%') {
            s = str_from_begin_end(fmt_str, e_fmt_str);
            s.byte_len -= 1;
            if (str_len(s) > 0) {
                if (!fmt->is_line_padded) {
                    TRY(string_formatter_pad_line(fmt));
                }
                TRY(string_formatter_write_no_pad(fmt, s));
                fmt_str = e_fmt_str;
            }

            ASSERT_OK(str_next_rune(e_fmt_str, &r, &e_fmt_str));
            switch (r)
            {
            case '%':
                s = (str_t) {
                    .ptr = fmt_str.ptr,
                    .byte_len = 1,
                };
                continue;
                break;
            case '+':
                string_formatter_pad_inc(fmt);
                break;
            case '-':
                string_formatter_pad_dec(fmt);
                break;
            case 's':
                s = va_arg(args, str_t);
                TRY(string_formatter_write(fmt, s));
                break;
            case 'v':
                auto fo = va_arg(args, Formattable);
                TRY(formattable_fmt(&fo, fmt));
                break;
            case 'd':
                auto val = va_arg(args, i32_t);
                len = snprintf((char *)buffer, size, "%d", val);
                goto num_write;
                break;
            case 'u':
                auto uval = va_arg(args, u32_t);
                len = snprintf((char *)buffer, size, "%u", uval);
                goto num_write;
                break;
            case 'l':
                ASSERT_OK(str_next_rune(fmt_str, &r, &fmt_str));
                switch (r) {
                case 'd':
                    auto val = va_arg(args, i64_t);
                    len = snprintf((char *)buffer, size, "%ld", val);
                    goto num_write;
                    break;
                case 'u':
                    auto uval = va_arg(args, u64_t);
                    len = snprintf((char *)buffer, size, "%lu", uval);
                    break;
                default:
                    print_error(S("Unkown formatting option"));                
                    return FMT_ERROR(ERROR);
                }
num_write:
                s = (str_t) {
                    .ptr = buffer,
                    .byte_len = (usize_t)len,
                };

                if (!fmt->is_line_padded) {
                    TRY(string_formatter_pad_line(fmt));
                }
                TRY(string_formatter_write_no_pad(fmt, s));
                break;
            
            default:
                print_error(S("Unkown formatting option"));                

                return FMT_ERROR(ERROR);
                unreacheble();
                break;
            }

            fmt_str = e_fmt_str;
            s = (str_t) {
                .ptr = fmt_str.ptr,
                .byte_len = 0,
            };
        } else {
            if (r == '\n') {
                s = str_from_begin_end(fmt_str, e_fmt_str);
                if (str_len(s) > 0) {
                    if (!fmt->is_line_padded) {
                        TRY(string_formatter_pad_line(fmt));
                    }
                    TRY(string_formatter_write_no_pad(fmt, s));
                }
                fmt_str = e_fmt_str;
                s = (str_t) {
                    .ptr = fmt_str.ptr,
                    .byte_len = 0,
                };
                fmt->is_line_padded = false;
            }
        }
    }
    s = str_from_begin_end(fmt_str, e_fmt_str);
    if (str_len(s) > 0) {
        TRY(string_formatter_write(fmt, s));
    }

    va_end(args);
    return FMT_ERROR(OK);
}