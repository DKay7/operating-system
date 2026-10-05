#include <inc/lib.h>
#include "common/check.h"

int
get_file_size(const char* filename);

char*
read_file(const char* filename, size_t* file_size) {
    int size = get_file_size(filename);

    int fd = open(filename, O_RDONLY);
    CHECK(fd >= 0, "can't open %s: %i\n", filename, fd);


    char* buf = balloc(size + 1, sizeof(char));
    CHECK(buf, "out of memory");

    int num_readed = 0;
    do {
        num_readed += read(fd, buf + num_readed, size - num_readed);
    } while (num_readed != size);

    buf[size] = 0;
    *file_size = size;

    return buf;
}

int
get_file_size(const char* filename) {

    struct Stat st;
    int ret = stat(filename, &st);
    CHECK(ret >= 0, "stat %s: %i", filename, ret);

    return st.st_size;
}
