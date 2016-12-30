#include <iostream>

#include "book.h"
#include "bookutil.h"
#include "bookext.h"
#include "system/files.h"

using namespace std;

int main()
{
    cout << "STep 1\n" << endl;
    char path[] = "пример.txt";
    cout << "STep 2\n" << endl;
    book_info_t book_info;
    int res = book_open(path, &book_info, BOOK_READ);

    std::string s;
    if (book_info.text != NULL) {
        s = std::string(book_info.text, book_info.text + 15);
    }

    cout << res << " --> " << book_info.status << " : " << s << endl;

    char dir[1024];
    char dir2[1024];
    int r = get_user_directory(dir, 1024);
    cout << r << " : " << dir << endl;
    r = get_app_directory(dir2, 1024);
    cout << r << " : " << dir2 << endl;

    FILE *f = fopen("cnv.txt", "wb");
    fwrite(book_info.text, sizeof(char), book_info.text_sz, f);
    fclose(f);

//    FILE *f = fopen("example.txt", "r");
//    char fb[20];
//    fgets(fb, 20, f);
//    fclose(f);
//    char *pos;
//    if ((pos=strchr(fb, '\n')) != NULL)
//        *pos = '\0';
//    printf("--: %s :--\n", fb);

    //if (f) {
    //    fclose(f);
    //}

    return 0;
}
