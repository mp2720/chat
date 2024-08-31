#pragma once

#include <cstring>
#include <exception>
#include <mbedtls/error.h>
#include <string>

namespace chat {

class MbedtlsException : std::exception {
  public:
    MbedtlsException(int err) throw() : err(err) {}

    const char *what() const throw() {
        char *buf = (char *)malloc(128);
        mbedtls_strerror(err, buf, 128);
        return buf;
    }

    int Error() const {
        return err;
    }

    void ErrorText(std::string &s) const {
        s.resize(128);
        mbedtls_strerror(err, s.data(), s.size());
        s.resize(strlen(s.data()));
    }

    bool operator==(const MbedtlsException &rhs) const {
        return err == rhs.err;
    }

    bool operator!=(const MbedtlsException &rhs) const {
        return err != rhs.err;
    }

    bool operator==(const int &rhs) const {
        return err == rhs;
    }

    bool operator!=(const int &rhs) const {
        return err != rhs;
    }

  private:
    int err;
};

} // namespace chat