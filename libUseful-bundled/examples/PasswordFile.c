#include "../libUseful.h"

void main()
{
PasswordFileAdd("/tmp/test.pw", "sha1", "test user", "testing 123", NULL);
PasswordFileAppend("/tmp/test.pw", "sha1", "another", "another password", NULL);
//PasswordFileAdd("/tmp/test.pw", "sha1", "test user", "testing 456", NULL);

printf("CHECK1: %d\n", PasswordFileCheck("/tmp/test.pw", "test user", "testing 123", NULL));
printf("CHECK2: %d\n", PasswordFileCheck("/tmp/test.pw", "test user", "testing 456", NULL));
printf("CHECK2: %d\n", PasswordFileCheck("/tmp/test.pw", "another", "another password", NULL));
}
