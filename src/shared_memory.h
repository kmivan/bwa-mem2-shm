#pragma once

#include <string>
#include "FMI_search.h"

using std::string;

struct IndexShmInfo
{
    int64_t reference_seq_len;
    int64_t count[5];
    CP_OCC *occ;
    int8_t *sa_ms_byte;
    uint32_t *sa_ls_word;
    int64_t sentinel_index;
};

string shm_file_path(const string &id, const string &filename);
void shm(int argc, char **argv);
uint8_t *get_bin_seq(const string &id);
void get_index(const string &id, IndexShmInfo *info);
string get_others_prefix(const string &id);
