#pragma once

#include <string>

struct IndexShmInfo
{
    long size;
    long pad_before_cp_occ;
    long pad_before_ms_byte;
    long pad_before_ls_word;
} __attribute__((aligned(64)));

void shm(int argc, char **argv);
void *get_file_from_shm(const std::string &path, IndexShmInfo &info);
