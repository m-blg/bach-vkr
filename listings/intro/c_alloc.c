AllocatorError
c_alloc(void *self, usize_t size, usize_t alignment, void **out_ptr) {
    // malloc return pointers with alignof(max_align_t), so alignment adjustments are not required
    // for alignment <= MAX_ALIGNMENT
    if (alignment > MAX_ALIGNMENT) {
        // TODO
        unimplemented();
    }
    void *p = malloc(size);
    if (p == nullptr) {
        RAISE(ALLOCATOR_ERROR(MEM_ALLOC));
    }

    *out_ptr = p;
    return ALLOCATOR_ERROR(OK);
}

AllocatorError
c_resize(void *self, usize_t size, usize_t alignment, void **in_out_ptr) {
    if (alignment > MAX_ALIGNMENT) {
        // TODO
        unimplemented();
    }
    void *p = realloc(*in_out_ptr, size);
    if (p == nullptr) {
        RAISE(ALLOCATOR_ERROR(MEM_ALLOC));
    }

    *in_out_ptr = p;
    return ALLOCATOR_ERROR(OK);
}
void
c_free(void *self, void **in_out_ptr) {
    free(*in_out_ptr);
    *in_out_ptr = nullptr;
}

Allocator
c_allocator() {
    return (Allocator) {
        ._vtable = (Allocator_Vtable) {
            .alloc = c_alloc,
            .resize = c_resize,
            .free = c_free,
        },
        .data = nullptr,
    };
}

INLINE
Allocator
arena_allocator(Arena self[non_null]) {
    return (Allocator) {
        ._vtable = (Allocator_Vtable) {
            // .alloc = (AllocatorAllocFn *)arena_alloc,
            .alloc = _arena_allocator_alloc,
            .resize = (AllocatorResizeFn *)arena_resize,
            .free = (AllocatorFreeFn *)arena_free,
        },
        .data = self,
    };
}