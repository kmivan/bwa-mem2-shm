#pragma once

#include <string>

void shm(int argc, char **argv);
void *get_file_from_shm(const std::string &path, long &size);
