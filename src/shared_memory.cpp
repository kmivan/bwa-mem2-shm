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

string get_others_prefix(const string &id)
{
    return SHM_PREFIX + id + '/' + "ref";
}

void *get_file(const string &id, const string &filename)
{
    string path = shm_file_path(id, filename);
    if (access(path.c_str(), F_OK) != 0)
    {
        cerr << "Not on shared memory: " << id << endl;
        exit(1);
    }

    ifstream fs(path, ios::binary);
    fs.seekg(0, ios::end);
    long len = fs.tellg();
    fs.seekg(0, ios::beg);
    fs.close();

    int fd = open(path.c_str(), O_RDONLY);
    void *addr = nullptr;
    do
    {
        addr = (void*) (((unsigned long) addr + 63L) / 64 * 64);
        addr = mmap(addr, len, PROT_READ, MAP_SHARED, fd, 0);
    } while ((unsigned long)addr % 64 != 0);

    return addr;
}

uint8_t *get_bin_seq(const string &id)
{
    return (uint8_t*) get_file(id, "ref.0123");
}

void get_index(const string &id, IndexShmInfo *info)
{
    char *head = (char*) get_file(id, "index.bwt");
    info->reference_seq_len = *(int64_t*) head;

    head += 8;
    memcpy(&info->count, head, sizeof(int64_t) * 5);
    head += 56;

    info->occ = (CP_OCC*) head;
    long cp_occ_size = (info->reference_seq_len >> CP_SHIFT) + 1;
    head += cp_occ_size * sizeof(CP_OCC);
    long reference_seq_len_ = SA_COMPRESSION ? (info->reference_seq_len >> SA_COMPX) + 1 : info->reference_seq_len;
    info->sa_ms_byte = (int8_t*) head;
    head += reference_seq_len_;
    head += 64 - reference_seq_len_ % 64L;
    info->sa_ls_word = (uint32_t*) head;
    head += reference_seq_len_ * 4;

    info->sentinel_index = *(int64_t*) head;
}

void write_index(ifstream &fs, ofstream &os)
{
    fs.seekg(0, ios::end);
    long size = fs.tellg();
    fs.seekg(0, ios::beg);

    int64_t meta_info[8];
    fs.read((char*) &meta_info, sizeof(int64_t) * 6);
    os.write((char*) &meta_info, sizeof(meta_info));
    long reference_seq_len = meta_info[0];
    long cp_occ_size = (reference_seq_len >> CP_SHIFT) + 1;

    char *buffer = new char[size];

    fs.read(buffer, cp_occ_size * sizeof(CP_OCC));
    os.write(buffer, cp_occ_size * sizeof(CP_OCC));

    if (SA_COMPRESSION)
        reference_seq_len = (reference_seq_len >> SA_COMPX) + 1;

    fs.read(buffer, reference_seq_len * sizeof(int8_t));
    os.write(buffer, reference_seq_len * sizeof(int8_t));

    fs.read(buffer, reference_seq_len * sizeof(uint32_t));
    os.write(buffer, 64 - reference_seq_len % 64L);
    os.write(buffer, reference_seq_len * sizeof(uint32_t));

    int64_t sentinel_index = -1;
    fs.read((char*) &sentinel_index, sizeof(sentinel_index));
    os.write((char*) &sentinel_index, sizeof(sentinel_index));

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

    if (*path.rbegin() == '4')
    {
        ofstream os(shm_file_path(id, "index.bwt"), ios::binary);
        write_index(fs, os);
        os.close();
    }
    else {
        string shm_path = shm_file_path(id, "ref"s + file_suffix(path));
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
    string usage = "Usage: bwa-mem2-shm shm (add | remove | check) <id> ...";

    if (argc < 3)
    {
        cerr << usage << endl << endl;
        return;
    }

    string id = argv[2];

    if (!strcmp(argv[1], "add"))
    {
        if (argc < 4)
        {
            cerr << "Usage: bwa-mem2-shm shm add <id> <prefix>" << endl << endl;
            return;
        }

        cerr << "Loading index to shared memory..." << endl;

        if (access((SHM_PREFIX + id).c_str(), F_OK) == 0)
        {
            system(("rm -rf " + SHM_PREFIX + id).c_str());
        }

        string path = argv[3];
        mkdir((SHM_PREFIX).c_str(), 0777);
        mkdir((SHM_PREFIX + id).c_str(), 0777);
        add_file(id, path + ".0123");
        add_file(id, path + ".pac");
        add_file(id, path + ".ann");
        add_file(id, path + ".amb");
        add_file(id, path + CP_FILENAME_SUFFIX);
    }
    else if (!strcmp(argv[1], "remove"))
    {
        cerr << "Removing file from shared memory..." << endl;
        remove(id);
    }
    else if (!strcmp(argv[1], "check"))
    {
        string shm_path = SHM_PREFIX + id + '/';
        static const char *files[] {"index.bwt", "ref.0123", "ref.pac", "ref.ann", "ref.amb"};
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
