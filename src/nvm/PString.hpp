/*
Taken from: https://pmem.io/2017/01/23/cpp-strings.html
*/

#ifndef PString_hpp_
#define PString_hpp_

#include "nvml/include/libpmemobj/tx_base.h"
#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/make_persistent_array.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"

using nvml::obj::delete_persistent;
using nvml::obj::make_persistent;
using nvml::obj::persistent_ptr;

#define SSO_CHARS 15
#define SSO_SIZE (SSO_CHARS + 1)

class PString {
public:
  char* data() const { return str ? str.get() : const_cast<char*>(sso); }
  void reset();
  void set(std::string* value);
private:
  char sso[SSO_SIZE];
  persistent_ptr<char[]> str;
};

void PString::reset() {
  pmemobj_tx_add_range_direct(sso, 1);
  sso[0] = 0;
  if (str) delete_persistent<char[]>(str, strlen(str.get()) + 1);
}

void PString::set(std::string* value) {
  unsigned long length = value->length();
  if (length <= SSO_CHARS) {
    if (str) {
      delete_persistent<char[]>(str, strlen(str.get()) + 1);
      str = nullptr;
    }
    pmemobj_tx_add_range_direct(sso, SSO_SIZE);
    strcpy(sso, value->c_str());
  } else {
    if (str) delete_persistent<char[]>(str, strlen(str.get()) + 1);
    str = make_persistent<char[]>(length + 1);
    strcpy(str.get(), value->c_str());
  }
}

#endif
