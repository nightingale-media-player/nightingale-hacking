/* vim: set sw=2 :miv */

#include "regparse.h"
#include <cstdio>

#define LEN(x) (sizeof(x)/sizeof(x[0]))

/**
 * Test a REG_SZ value
 * i is the number of bytes, including one null terminator
 * n is the test number
 * NOTE that the string must be dobule-terminated
 */
template<int i>
int test_sz(int n, TCHAR value[i]) {
  printf("=== test %i ===\n", n);
  int ret = 0;
  TCHAR empty[4096];
  memset(empty, 0xCC, sizeof(empty));
  buffer_t buffer((LPBYTE)empty, sizeof(empty));
  valueList_t v = ParseValues(buffer_t((LPBYTE)value, sizeof(TCHAR)*i), REG_SZ);
  if (memcmp(v.begin()->p, value, (i + 1) * sizeof(TCHAR))) {
    printf("list: got ?? expected %S [%i]\n", value, (i + 1) * sizeof(TCHAR));
    ++ret;
  }
  MakeMultiSzRegString(buffer, v);
  if (buffer.cbBytes != sizeof(TCHAR) * (i + 1)) {
    printf("got %i bytes, expected %i\n", buffer.cbBytes, sizeof(TCHAR) * (i + 1));
    ++ret;
  }
  if (memcmp(empty, value, (i + 1) * sizeof(TCHAR))) {
    printf("got ?? expected %S [%i]\n", value, (i + 1) * sizeof(TCHAR));
    ++ret;
  }
  if (ret) {
    for (int i = 0; i < 30; ++i) {
      printf("%02x ", ((char*)empty)[i]& 0xFF);
    }
    printf("\n");
    for (int i = 0; i < 30; ++i) {
      printf("%2c ", (((char*)empty)[i] ? ((char*)empty)[i] : ' '));
    }
    printf("\n");
  }
  return ret;
}

int test_multi(int n, TCHAR *data, DWORD dataLen, TCHAR** ref, int refLen) {
  printf("=== test %i ===\n", n);
  int ret = 0;
  TCHAR empty[4096];
  memset(empty, 0xCC, sizeof(empty));
  buffer_t buffer((LPBYTE)empty, sizeof(empty));
  valueList_t v = ParseValues(buffer_t((LPBYTE)data, dataLen), REG_MULTI_SZ);
  MakeMultiSzRegString(buffer, v);
  if (v.size() != refLen) {
    printf("got %i items, expected %i\n", v.size(), refLen);
    ++ret;
  }
  int i = 0;
  for (valueList_t::const_iterator it = v.begin();
       i < refLen && it != v.end();
       ++it, ++i)
  {
    if (_tcscmp(it->p, ref[i])) {
      printf("got mismatch at position %i\n", i);
      ++ret;
    }
  }
  if (memcmp(empty, data, dataLen)) {
    printf("mismatch in raw buffer\n");
    ++ret;
  }
  if (ret) {
    for (int i = 0; i < dataLen + 2; ++i) {
      printf("%02x ", ((char*)empty)[i]& 0xFF);
    }
    printf("\n");
    for (int i = 0; i < dataLen + 2; ++i) {
      printf("%2c ", (((char*)empty)[i] ? ((char*)empty)[i] : ' '));
    }
    printf("\n");
  }
  return ret;
}

int test_multi_0() {
  TCHAR data[] = _T("hello\0world\0");
  TCHAR *ref[] = {_T("hello"), _T("world")};
  DWORD count = sizeof(data);
  
  return test_multi(101, data, sizeof(data), ref, LEN(ref));
}

int test_multi_1() {
  TCHAR data[] = _T("");
  DWORD count = sizeof(data);
  
  return test_multi(102, data, sizeof(data), NULL, 0);
}

int main() {
  int ret = 0;
  ret += test_sz<12>(1, L"hello world\0");
  ret += test_sz<10>(2, L"null\0byte\0");
  ret += test_sz<1>(3, L"\0"); // empty
  ret += test_multi_0();
  ret += test_multi_1();
  printf("failed %i tests\n", ret);
  return ret;
}