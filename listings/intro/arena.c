AllocatorError
arena_alloc(Arena self[non_null], usize_t block_size, usize_t block_align, void **out_ptr) 
{
    ArenaChunk NLB(*)chunk;
    #define pred(item) (arena_chunk_rest_cap_aligned(item, block_align) >= block_size)
    iter_pref_find(list, list_iter(self->chunks), pred, &chunk);
    #undef pred

    if (chunk == nullptr) {
        // allocate new chunk to fit the data
        usize_t data_section_size = MAX(block_size + alignment_pad(block_align), self->default_chunk_data_size);
        List_dynT_Node *node;
        list_node_alloc_size_align(self->chunks, sizeof(ArenaChunk) + data_section_size, alignof(ArenaChunk), &node);

        chunk = list_node_data(node, alignof(ArenaChunk));
        *chunk = (ArenaChunk) {
            .data_size = data_section_size,
            .cursor = chunk->data,
        };
        list_push_node(self->chunks, node);
    }
    chunk->cursor = align_forward(chunk->cursor, block_align);

    // NOTE: at this point the chunk is valid, chunk->cursor is aligned
    // insert data
    *out_ptr = chunk->cursor;
    chunk->cursor += block_size;

    return ALLOCATOR_ERROR(OK);
}

/// doesn't deallocate memory
void
arena_free(Arena self[non_null], void **in_out_ptr) {
    *in_out_ptr = nullptr;
}