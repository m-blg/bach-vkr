struct_def(String, {
    uchar_t *ptr;
    usize_t byte_cap;
    usize_t byte_len; // in bytes
    Allocator allocator;
})


struct_def(str_t, {
    uchar_t *ptr;
    usize_t byte_len; // in bytes
})