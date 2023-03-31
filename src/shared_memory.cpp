#include "shared_memory.h"
#include "FMI_search.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>

using namespace std;

const string SHM_PREFIX = "/dev/shm/bwa-mem3-index";
constexpr int SHM_PROJ_ID = 42;

void *get_file_from_shm(const string &path, IndexShmInfo &info)
{
    key_t shm_key = ftok(path.c_str(), SHM_PROJ_ID);
    int shm_id = shmget(shm_key, 0, 0);
    if (shm_id < 0)
    {
        cerr << "File not on shared memory: " << path << endl;
        exit(1);
    }

    IndexShmInfo *addr = (IndexShmInfo*) shmat(shm_id, NULL, 0);
    while ((unsigned long) addr % 64 != 0)
    {
        shmdt(addr);
        addr = (IndexShmInfo*) shmat(shm_id, (void*)(((unsigned long) addr + 63) / 64 * 64), 0);
    }

    info = *addr;
    return addr + 1;
}

void write_index(ifstream &fs, int shm_id, long file_size)
{
    IndexShmInfo *addr = (IndexShmInfo*) shmat(shm_id, NULL, 0);
    while ((unsigned long) addr % 64 != 0)
    {
        shmdt(addr);
        addr = (IndexShmInfo*) shmat(shm_id, (void*)(((unsigned long) addr + 63) / 64 * 64), 0);
    }
    addr->size = file_size;
    addr->pad_before_cp_occ = 16;
    addr->pad_before_ms_byte = 0;

    char *data = (char*) (addr + 1);
    fs.read(data, sizeof(int64_t) * 6);
    int64_t reference_seq_len = *(int64_t*) data;
    int64_t cp_occ_size = (reference_seq_len >> CP_SHIFT) + 1;
    addr->pad_before_ls_word = reference_seq_len % 64;

    data += sizeof(int64_t) * 6 + addr->pad_before_cp_occ;
    fs.read(data, cp_occ_size * sizeof(CP_OCC));
    data += cp_occ_size * sizeof(CP_OCC);

    fs.read(data, reference_seq_len * sizeof(int8_t));
    data += reference_seq_len * sizeof(int8_t);

    data += addr->pad_before_ls_word;
    fs.read(data, reference_seq_len * sizeof(uint32_t));

    shmdt(addr);
}

void add_file(const string &path)
{
    if (access(path.c_str(), F_OK) != 0)
    {
        cerr << "File not exist: " << path << endl;
        exit(1);
    }

    ifstream fs(path, ios::binary);
    fs.seekg(0, ios::end);
    long size = fs.tellg();
    fs.seekg(0, ios::beg);

    key_t shm_key = ftok(path.c_str(), SHM_PROJ_ID);
    int shm_id = shmget(shm_key, size + sizeof(IndexShmInfo) + 64 * 3, IPC_CREAT | IPC_EXCL);
    if (shm_id < 0)
    {
        cerr << "File already on shared memory: " << path << endl;
        return;
    }

    if (path[path.size() - 1] == '3')
    {
        IndexShmInfo *addr = (IndexShmInfo*) shmat(shm_id, NULL, 0);
        addr->size = size;
        addr++;
        fs.read((char*) addr, size);
    }
    else {
        write_index(fs, shm_id, size);
    }
    fs.close();
}

void remove_file(const string &path)
{
    if (access(path.c_str(), F_OK) != 0)
    {
        cerr << "File not exist: " << path << endl;
        exit(1);
    }

    key_t shm_key = ftok(path.c_str(), SHM_PROJ_ID);
    int shm_id = shmget(shm_key, 0, 0);
    if (shm_id < 0)
    {
        cerr << "File not resided: " << path << endl;
    }

    shmctl(shm_id, IPC_RMID, NULL);
}

void shm(int argc, char **argv)
{
    string usage = "Usage: bwa-mem3 shm (add|remove|check) <prefix>";

    if (argc != 3)
    {
        cerr << usage << endl << endl;
    }
    else if (!strcmp(argv[1], "add"))
    {
        cerr << "Loading index to shared memory..." << endl;

        string &&path = argv[2];
        add_file(path + ".0123");
        add_file(path + CP_FILE_SUFFIX);
    }
    else if (!strcmp(argv[1], "remove"))
    {
        cerr << "Removing file from shared memory..." << endl;
        string &&path = argv[2];
        remove_file(path + ".0123");
        remove_file(path + CP_FILE_SUFFIX);
    }
    else if (!strcmp(argv[1], "check"))
    {
        string ref = string(argv[2]) + ".0123";
        string index = string(argv[2]) + CP_FILE_SUFFIX;

        if (shmget(ftok(ref.c_str(), SHM_PROJ_ID), 0, 0) > 0)
        {
            cout << "YES   " << ref << endl;
        }
        else{
            cout << "NO    " << ref << endl;
        }

        if (shmget(ftok(index.c_str(), SHM_PROJ_ID), 0, 0) > 0)
        {
            cout << "YES   " << index << endl;
        }
        else{
            cout << "NO    " << index << endl;
        }
    }
    else
    {
        cerr << usage << endl;
    }
}
