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

void *get_file_from_shm(const string &path, long &size)
{
    key_t shm_key = ftok(path.c_str(), SHM_PROJ_ID);
    int shm_id = shmget(shm_key, 0, 0);
    if (shm_id < 0)
    {
        cerr << "File not on shared memory: " << path << endl;
        exit(1);
    }

    long *addr = (long*) shmat(shm_id, NULL, 0);
    size = *addr;
    return addr + 1;
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
    int shm_id = shmget(shm_key, size + sizeof(long), IPC_CREAT | IPC_EXCL);
    if (shm_id < 0)
    {
        cerr << "File already on shared memory: " << path << endl;
        return;
    }

    long *addr = (long*) shmat(shm_id, NULL, 0);
    while ((unsigned long) addr % 64 != 0)
    {
        shmdt(addr);
        addr = (long*) shmat(shm_id, (void*)(((unsigned long) addr / 64 + 1) * 64), 0);
    }

    *addr = size;
    addr++;
    fs.read((char*) addr, size);
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
        exit(1);
    }

    shmctl(shm_id, IPC_RMID, NULL);
}

void shm(int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: bwa-mem3 shm (add|remove|check) <prefix>" << endl << endl;
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

}
