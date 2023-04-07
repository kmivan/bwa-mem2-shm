#include "shared_memory.h"
#include "FMI_search.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>

using namespace std;

const string SHM_PREFIX = "/dev/shm/bwa-mem2-index/";

string shm_file_path(const string &id, const string &filename)
{
    return SHM_PREFIX + id + '/' + filename;
}

void *get_index_from_shm(const string &id, IndexShmInfo &info)
{
    IndexShmInfo *addr = (IndexShmInfo*) get_file(id, "index." + to_string(CP_BLOCK_SIZE));

    info = *addr;
    return addr + 1;
}

void *get_file(const string &id, const string &filename)
{
    string path = shm_file_path(id, filename);

    ifstream fs(path, ios::binary);
    if (!fs.is_open())
    {
        cerr << "Failed to load index from shared memory: " << id << endl;
        cerr << "Failed to open file: " << path << endl;
        exit(1);
    }
    fs.seekg(0, ios::end);
    long size = fs.tellg();
    fs.close();

    int fd = open(path.c_str(), O_RDONLY, 0);
    void *addr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    while ((unsigned long) addr % 64 != 0)
    {
        munmap(addr, size);
        addr = (void*) (((unsigned long) addr + 63) / 64 * 64);
        addr = mmap(addr, size, PROT_READ, MAP_SHARED, fd, 0);
    }
    return addr;
}

void write_index(ifstream &fs, ofstream &os)
{
    IndexShmInfo info{.pad_before_cp_occ=16, .pad_before_ms_byte=0};

    fs.seekg(0, ios::end);
    info.size = fs.tellg();
    fs.seekg(0, ios::beg);

    int64_t meta_info[6];
    fs.read((char*) &meta_info, sizeof(meta_info));
    long reference_seq_len = meta_info[0];
    long cp_occ_size = (reference_seq_len >> CP_SHIFT) + 1;
    info.pad_before_ls_word = reference_seq_len % 64L;

    os.write((char*) &info, sizeof(info));
    os.write((char*) &meta_info, sizeof(meta_info));

    char *buffer = new char[max(reference_seq_len * sizeof(int32_t), cp_occ_size * sizeof(CP_OCC))];

    os.seekp(info.pad_before_cp_occ, ios::cur);
    fs.read(buffer, cp_occ_size * sizeof(CP_OCC));
    os.write(buffer, cp_occ_size * sizeof(CP_OCC));

    os.seekp(info.pad_before_ms_byte, ios::cur);
    fs.read(buffer, reference_seq_len * sizeof(int8_t));
    os.write(buffer, reference_seq_len * sizeof(int8_t));

    os.seekp(info.pad_before_ls_word, ios::cur);
    fs.read(buffer, reference_seq_len * sizeof(uint32_t));
    os.write(buffer, reference_seq_len * sizeof(uint32_t));

    delete[] buffer;
}

const char *file_suffix(const string &filename)
{
    auto it = filename.rbegin();
    while (*it != '.')
    {
        it++;
    }
    return &*it;
}

void add_file(const string &id, const string &path)
{
    if (access(path.c_str(), F_OK) != 0)
    {
        cerr << "File not exist: " << path << endl;
        exit(1);
    }

    ifstream fs(path, ios::binary);

    if (*path.rbegin() == '2' || *path.rbegin() == '4')
    {
        ofstream os(shm_file_path(id, "index"s + file_suffix(path)), ios::binary);
        write_index(fs, os);
        os.close();
    }
    else {
        string &&shm_path = shm_file_path(id, "ref"s + file_suffix(path));
        system(("cp " + path + " " + shm_path).c_str());
    }
    fs.close();
}

void remove(const string &id)
{
    string shm_path = SHM_PREFIX + id;
    if (access(shm_path.c_str(), F_OK) != 0)
    {
        cerr << "Not on shared memory: " << id << endl;
        exit(1);
    }
    system(("rm -rf " + shm_path).c_str());
}

void shm(int argc, char **argv)
{
    string usage = "Usage: bwa-mem2-shm shm (add | remove | check) <prefix>";

    if (argc < 3)
    {
        cerr << usage << endl << endl;
        return;
    }

    string &&id = argv[2];

    if (!strcmp(argv[1], "add"))
    {
        cerr << "Loading index to shared memory..." << endl;

        if (access((SHM_PREFIX + id).c_str(), F_OK) == 0)
        {
            system(("rm -rf " + SHM_PREFIX + id).c_str());
        }

        string &&path = argv[3];
        mkdir((SHM_PREFIX).c_str(), 0777);
        mkdir((SHM_PREFIX + id).c_str(), 0777);
        add_file(id, path + ".0123");
        add_file(id, path + ".pac");
        add_file(id, path + ".ann");
        add_file(id, path + ".amb");
        add_file(id, path + CP_FILE_SUFFIX);
    }
    else if (!strcmp(argv[1], "remove"))
    {
        cerr << "Removing file from shared memory..." << endl;
        remove(id);
    }
    else if (!strcmp(argv[1], "check"))
    {
        string &&shm_path = SHM_PREFIX + id + '/';
        static const char *files[] {"index.32", "index.64", "ref.0123", "ref.pac", "ref.ann", "ref.amb"};
        for (auto f : files) {
            if (access((shm_path + f).c_str(), F_OK) == 0)
            {
                cerr << "YES   " << shm_path << f << endl;
            }
            else
            {
                cerr << "NO    " << shm_path << f << endl;
            }
        }
    }
    else
    {
        cerr << usage << endl;
    }
}
