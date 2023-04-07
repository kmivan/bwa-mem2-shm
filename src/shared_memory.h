#pragma once

#include <string>

using std::string;

struct IndexShmInfo
{
    long size;
    int pad_before_cp_occ;
    int pad_before_ls_word;
    int pad_before_ms_byte;
} __attribute__((aligned(64)));

string shm_file_path(const string &id, const string &filename);
void shm(int argc, char **argv);
void *get_index_from_shm(const string &id, IndexShmInfo &info);
void *get_file(const string &id, const string &filename);
