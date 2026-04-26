#ifdef PS3_BUILD

#include <cerrno>
#include <cstddef>
#include <unistd.h>

extern "C" {

int symlink(const char* /*target*/, const char* /*linkpath*/) {
    errno = ENOSYS;
    return -1;
}

ssize_t readlink(const char* /*path*/, char* /*buf*/, size_t /*bufsiz*/) {
    errno = ENOSYS;
    return -1;
}

long pathconf(const char* /*path*/, int /*name*/) {
    errno = ENOSYS;
    return -1;
}

int fchmod(int /*fd*/, mode_t /*mode*/) {
    errno = ENOSYS;
    return -1;
}

}

#endif
