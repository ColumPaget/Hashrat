#include "../libUseful.h"

void main()
{
PasswordFileAdd("/tmp/test.pw", "sha1", "test user", "testing 123");
PasswordFileAppend("/tmp/test.pw", "sha1", "another", "another password");
//PasswordFileAdd("/tmp/test.pw", "sha1", "test user", "testing 456");

printf("CHECK1: %d\n", PasswordFileCheck("/tmp/test.pw", "test user", "testing 123"));
printf("CHECK2: %d\n", PasswordFileCheck("/tmp/test.pw", "test user", "testing 456"));
printf("CHECK2: %d\n", PasswordFileCheck("/tmp/test.pw", "another", "another password"));
}
