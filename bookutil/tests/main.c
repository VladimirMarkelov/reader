#include <string.h>
#include <stdio.h>

int skip_failed_pkg = 1;
#include "unittest.h"

#include "bookutil.h"
//#include "utf8proc.h"

int tests_run = 0;
int tests_fail = 0;

#define BSIZE 256

#ifdef _WIN32
    wchar_t *ascii_file= L"ascii.txt";
    wchar_t *koi8_file = L"koi8.txt";
    wchar_t *utf16_file = L"utf16bom.txt";
    wchar_t *utf8_file = L"utf8.txt";
    wchar_t *utf8bom_file = L"utf8bom.txt";
    wchar_t *win1251_file = L"win1251.txt";
    wchar_t *win866 = L"win866.txt";
#else
    char *ascii_file= "ascii.txt";
    char *koi8_file = "koi8.txt";
    char *utf16_file = "utf16bom.txt";
    char *utf8_file = "utf8.txt";
    char *utf8bom_file = "utf8bom.txt";
    char *win1251_file = "win1251.txt";
    char *win866 = "win866.txt";
#endif

#ifdef _WIN32
size_t read_file(wchar_t *file_name, char *buf, size_t buf_size) {
    FILE *f = _wfopen(file_name, L"rb");
#else
int read_file(char *file_name, char *buf, size_t buf_size) {
    FILE *f = fopen(file_name, "rb");
#endif
    if (f == NULL) {
        return -1;
    }

    size_t bytesRead = fread(buf, sizeof(char), buf_size, f);
    fclose(f);

    if (bytesRead < buf_size) {
        buf[bytesRead] = '\0';
    } else {
        buf[buf_size - 1] = '\0';
    }

    return 0;
}

const char* test_get_user_directory() {
    int res = get_user_directory(NULL, 0);
    ut_assert("NULL destination", res == BOOK_INVALID_ARG);
    char udir[1024] = {0};
    res = get_user_directory(udir, 10);
    ut_assert("Buffer small", res == BOOK_BUFFER_SMALL);
    res = get_user_directory(udir, 1024);
    ut_assert("User directory got OK", res == BOOK_SUCCESS);
    ut_assert("User directory not empty", *udir != '\0');
    return 0;
}

const char* test_get_temp_directory() {
    int res = get_temp_directory(NULL, 0);
    ut_assert("NULL destination", res == BOOK_INVALID_ARG);
    char udir[1024] = {0};
    res = get_temp_directory(udir, 10);
    ut_assert("Buffer small", res == BOOK_BUFFER_SMALL);
    res = get_temp_directory(udir, 1024);
    ut_assert("TEMP directory got OK", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("TEMP directory not empty", *udir >= 'A' && *udir <= 'Z');
#else
    ut_assert("TEMP directory not empty", *udir >= '/');
#endif
    return 0;
}

const char* test_append_path() {
    char dir[128];
#ifdef _WIN32
    strcpy(dir, "c:\\root\\");
#else
    strcpy(dir, "/root/");
#endif
    int res = append_path(NULL, 0, NULL);
    ut_assert("NULL destination", res == BOOK_INVALID_ARG);
    res = append_path(dir, 8, "first");
    ut_assert("Small buffer", res == BOOK_BUFFER_SMALL);

    res = append_path(dir, 10, NULL);
    ut_assert("NULL source", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("NULL source is OK", strcmp(dir, "c:\\root\\") == 0);
#else
    ut_assert("NULL source is OK", strcmp(dir, "/root/") == 0);
#endif

    res = append_path(dir, 128, "first");
    ut_assert("first added", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("first added to the end", strcmp(dir, "c:\\root\\first") == 0);
#else
    ut_assert("first added to the end", strcmp(dir, "/root/first") == 0);
#endif

    res = append_path(dir, 128, "second");
    ut_assert("second added", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("second added to the end", strcmp(dir, "c:\\root\\first\\second") == 0);
#else
    ut_assert("second added to the end", strcmp(dir, "/root/first/second") == 0);
#endif

#ifdef _WIN32
    strcpy(dir, "c:");
    res = append_path(dir, 128, "first");
    ut_assert("Windows added to drive name", res == BOOK_SUCCESS);
    ut_assert("Windows added to drive name correctly", strcmp(dir, "c:\\first") == 0);
    strcat(dir, "/");
    res = append_path(dir, 128, "second");
    ut_assert("Windows added to slash", res == BOOK_SUCCESS);
    ut_assert("Windows added to slash correctly", strcmp(dir, "c:\\first/second") == 0);
#endif

    dir[0] = '\0';
    res = append_path(dir, 128, "first");
    ut_assert("Empty root", res == BOOK_NO_PATH);

    return 0;
}

const char* test_path_up() {
    char dir[128];
    dir[0] = '\0';

    int res = path_up(NULL);
    ut_assert("NULL directory", res == BOOK_INVALID_ARG);
    res = path_up(dir);
    ut_assert("Empty directory", res == BOOK_NO_PATH);

#ifdef _WIN32
    strcpy(dir, "c:\\root\\subdir\\");
#else
    strcpy(dir, "/root/subdir/");
#endif

    res = path_up(dir);
    ut_assert("First up dir", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("First up dir correct", strcmp(dir, "c:\\root") == 0);
#else
    ut_assert("First up dir correct", strcmp(dir, "/root") == 0);
#endif

    res = path_up(dir);
    ut_assert("Second up dir", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("Second up dir correct", strcmp(dir, "c:") == 0);
#else
    ut_assert("Second up dir correct", strcmp(dir, "/") == 0);
#endif

    res = path_up(dir);
    ut_assert("Third up dir", res == BOOK_NO_PATH);
#ifdef _WIN32
    ut_assert("Third up dir correct", strcmp(dir, "c:") == 0);
#else
    ut_assert("Third up dir correct", strcmp(dir, "/") == 0);
#endif

#ifdef _WIN32
    strcpy(dir, "c:/root/subdir");
    res = path_up(dir);
    ut_assert("Windows up dir", res == BOOK_SUCCESS);
    ut_assert("Windows slashes dir up", strcmp(dir, "c:/"));
#endif

    return 0;
}

const char* test_lower() {
    char str[1024] = {0};

    int res = to_lower(NULL, 10);
    ut_assert("NULL string", res == BOOK_INVALID_ARG);
    res = to_lower(str, 10);
    ut_assert("Empty string OK", res == BOOK_SUCCESS);
    ut_assert("Empty string", *str == '\0');

    strcpy(str, "The word, Another worD");
    res = to_lower(str, 10);
    ut_assert("Small buffer", res == BOOK_BUFFER_SMALL);
    res = to_lower(str, 1024);
    ut_assert("ASCII OK", res == BOOK_SUCCESS);
    ut_assert("ASCII", strcmp(str, "the word, another word") == 0);

    strcpy(str, "The слово, Another Слово");
    res = to_lower(str, 1024);
    ut_assert("NON-ASCII OK", res == BOOK_SUCCESS);
    ut_assert("NON-ASCII", strcmp(str, "the слово, another слово") == 0);

    return 0;
}

const char* test_upper() {
    char str[1024] = {0};

    int res = to_upper(NULL, 10);
    ut_assert("NULL string", res == BOOK_INVALID_ARG);
    res = to_upper(str, 10);
    ut_assert("Empty string OK", res == BOOK_SUCCESS);
    ut_assert("Empty string", *str == '\0');

    strcpy(str, "The word, Another worD");
    res = to_upper(str, 10);
    ut_assert("Small buffer", res == BOOK_BUFFER_SMALL);
    res = to_upper(str, 1024);
    ut_assert("ASCII OK", res == BOOK_SUCCESS);
    ut_assert("ASCII", strcmp(str, "THE WORD, ANOTHER WORD") == 0);

    strcpy(str, "The слово, Another Слово");
    res = to_upper(str, 1024);
    ut_assert("NON-ASCII OK", res == BOOK_SUCCESS);
    ut_assert("NON-ASCII", strcmp(str, "THE СЛОВО, ANOTHER СЛОВО") == 0);

    return 0;
}

const char* test_rel_dirs() {
    char dir[128] = {0},
         main_dir[128] = {0};

    int res = make_relative(NULL, NULL);
    ut_assert("NULL args", res == BOOK_INVALID_ARG);
    res = make_relative(dir, NULL);
    ut_assert("NULL main dir", res == BOOK_INVALID_ARG);

    strcpy(dir, "no_root");
    res = make_relative(dir, main_dir);
    ut_assert("Directory is absolute", res == BOOK_NO_PATH);
#ifdef _WIN32
    strcpy(dir, "c:/dir");
#else
    strcpy(dir, "/dir");
#endif
    strcpy(main_dir, "no_root");
    res = make_relative(dir, main_dir);
    ut_assert("Main directory is absolute", res == BOOK_NO_PATH);

#ifdef _WIN32
    strcpy(dir, "d:/dir/");
    strcpy(main_dir, "c:/dir");
#else
    strcpy(dir, "/dir/");
    strcpy(main_dir, "/base");
#endif
    res = make_relative(dir, main_dir);
    ut_assert("Roots differ", res == BOOK_NO_PATH);

#ifdef _WIN32
    strcpy(dir, "c:/dir/");
    strcpy(main_dir, "c:/dir");
#else
    strcpy(dir, "/dir/");
    strcpy(main_dir, "/dir");
#endif
    res = make_relative(dir, main_dir);
    ut_assert("The same dir - OK", res == BOOK_SUCCESS);
    ut_assert("The same dir", strlen(dir) == 1 && *dir == '.');

#ifdef _WIN32
    strcpy(dir, "c:/dir");
    strcpy(main_dir, "c:/dir");
#else
    strcpy(dir, "/dir");
    strcpy(main_dir, "/dir");
#endif
    res = make_relative(dir, main_dir);
    ut_assert("The totally same dir - OK", res == BOOK_SUCCESS);
    ut_assert("The totally same dir", strlen(dir) == 1 && *dir == '.');

#ifdef _WIN32
    strcpy(dir, "c:/dir\\first/second\\third");
    strcpy(main_dir, "c:/dir");
#else
    strcpy(dir, "/dir/first/second/third");
    strcpy(main_dir, "/dir");
#endif
    res = make_relative(dir, main_dir);
    ut_assert("The inside dir - OK", res == BOOK_SUCCESS);
#ifdef _WIN32
    ut_assert("The inside dir", strcmp(dir, "first/second\\third") == 0);
#else
    ut_assert("The inside dir", strcmp(dir, "first/second/third") == 0);
#endif

    return 0;
}

const char* test_BOM() {
    char buf[BSIZE];
    int res = read_file(ascii_file, buf, BSIZE);
    ut_assert("Read ASCII file", res == 0);
    res = detect_bom(buf, BSIZE);
    ut_assert("ASCII BOM", res == 0);

    res = read_file(utf8_file, buf, BSIZE);
    ut_assert("Read UTF8 no BOM file", res == 0);
    res = detect_bom(buf, BSIZE);
    ut_assert("UTF8 no BOM", res == 0);

    res = read_file(utf8bom_file, buf, BSIZE);
    ut_assert("Read UTF8 with BOM file", res == 0);
    res = detect_bom(buf, BSIZE);
    ut_assert("UTF8 with BOM", res == 3);

    res = read_file(utf16_file, buf, BSIZE);
    ut_assert("Read UTF16 file", res == 0);
    res = detect_bom(buf, BSIZE);
    ut_assert("UTF16", res == 2);

    return 0;
}

const char* test_encoding() {
    char buf[BSIZE];
    int res;

    res = read_file(ascii_file, buf, BSIZE);
    ut_assert("Read ASCII file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("ASCII detected", res == ENC_ASCII);

    res = read_file(win1251_file, buf, BSIZE);
    ut_assert("Read WIN1251 file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("WIN1251 detected", res == ENC_WIN1251);

    res = read_file(koi8_file, buf, BSIZE);
    ut_assert("Read KOI8 file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("KOI8 detected", res == ENC_KOI8R);

    res = read_file(win866, buf, BSIZE);
    ut_assert("Read DOS866 file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("DOS866 detected", res == ENC_DOS866);

    res = read_file(utf8_file, buf, BSIZE);
    ut_assert("Read UTF8 file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("UTF8 detected", res == ENC_UTF8);

    res = read_file(utf8bom_file, buf, BSIZE);
    ut_assert("Read UTF8-BOM file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("UTF8-BOM detected", res == ENC_UTF8);

    res = read_file(utf16_file, buf, BSIZE);
    ut_assert("Read UTF16-LE file", res == 0);
    res = detect_encoding(buf, BSIZE);
    ut_assert("UTF16-LE detected", res == ENC_UTF16_LE);

    return 0;
}

const char* test_str_funcs() {
    size_t res;

    res = utf_len(NULL);
    ut_assert("NULL string", res == 0);
    res = utf_len("");
    ut_assert("Empty string", res == 0);
    res = utf_len("test");
    ut_assert("ASCII length", res == strlen("test"));
    res = utf_len("ZпримерXдва");
    ut_assert("UTF length", res == 11);

    char *test_ascii = "example";
    char *test_utf = "ZпримерXдва";
    char *empty = "";
    char *adv = utf_advance(NULL, 10);
    ut_assert("NULL string advance", adv == NULL);
    adv = utf_advance(empty, 2);
    ut_assert("Empty string advance", adv != NULL && *adv == '\0');

    adv = utf_advance(test_ascii, 2);
    ut_assert("ASCII string advance - 2 chars", adv != NULL && strcmp(adv, "ample") == 0);
    adv = utf_advance(adv, 3);
    ut_assert("ASCII string advance - 3 chars", adv != NULL && strcmp(adv, "le") == 0);
    adv = utf_advance(adv, 5);
    ut_assert("ASCII string advance - to end", adv != NULL && *adv == '\0');

    adv = utf_advance(test_utf, 2);
    ut_assert("UTF string advance - 2 chars", adv != NULL && strcmp(adv, "римерXдва") == 0);
    adv = utf_advance(adv, 3);
    ut_assert("UTF string advance - 3 chars", adv != NULL && strcmp(adv, "ерXдва") == 0);
    adv = utf_advance(adv, 10);
    ut_assert("UTF string advance - to end", adv != NULL && *adv == '\0');

    char buf[32];
    int r = utf_substr(NULL, 1, 3, buf, 32);
    ut_assert("substr: NULL string", r == BOOK_INVALID_ARG);
    r = utf_substr(test_ascii, 1, 3, NULL, 32);
    ut_assert("substr: NULL buffer", r == BOOK_INVALID_ARG);
    r = utf_substr(test_ascii, 4, 3, NULL, 32);
    ut_assert("substr: Invalid from", r == BOOK_INVALID_ARG);
    r = utf_substr(test_ascii, 1, 3, buf, 32);
    ut_assert("substr: 2 to 3", r == BOOK_SUCCESS && strcmp(buf, "xa") == 0);
    r = utf_substr(test_ascii, 1, 12, buf, 32);
    ut_assert("substr: too long - 2 to 12", r == BOOK_SUCCESS && strcmp(buf, "xample") == 0);
    r = utf_substr(test_ascii, 1, 8, buf, 3);
    ut_assert("substr: buffer small", r == BOOK_BUFFER_SMALL);

    r = utf_substr(test_utf, 0, 3, buf, 32);
    ut_assert("substr utf: 0 to 3", r == BOOK_SUCCESS && strcmp(buf, "Zпр") == 0);
    r = utf_substr(test_utf, 2, 8, buf, 32);
    ut_assert("substr utf: 2 to 8", r == BOOK_SUCCESS && strcmp(buf, "римерX") == 0);
    r = utf_substr(test_utf, 4, 28, buf, 32);
    ut_assert("substr utf: to long - 4 to 28", r == BOOK_SUCCESS && strcmp(buf, "мерXдва") == 0);
    r = utf_substr(test_utf, 4, 28, buf, 10);
    ut_assert("substr utf: buffer small - 4 to 28", r == BOOK_BUFFER_SMALL);

    return 0;
}

const char* test_hypen() {
    int r;
    char ascii[] = "example";
    char utf[] = "примерный";
    size_t hyph[10];

    r = hyphenation(NULL, hyph, 0);
    ut_assert("NULL string", r == BOOK_INVALID_ARG);
    r = hyphenation(ascii, hyph, 1);
    ut_assert("Buffer too small", r == BOOK_BUFFER_SMALL);
    r = hyphenation(ascii, hyph, 10);
    ut_assert("ASCII hyph - OK", r == BOOK_SUCCESS);
    ut_assert("ASCII hyph - correct", hyph[0] == 2 && hyph[1] == 4 && hyph[2] == 0);
    r = hyphenation(utf, hyph, 10);
    ut_assert("UTF hyph - OK", r == BOOK_SUCCESS);
    ut_assert("UTF hyph - correct", hyph[0] == 3 && hyph[1] == 6 && hyph[2] == 0);

    return 0;
}

const char* test_buffer_convert() {
    size_t r;
    char ascii[] = "example";
    char out[32];
    char utf[] = "примерный";
    char win1251[] = {0xF2, 0xE5, 0xEC, 0xE0, 0x20, 0};

    r = convert_to_utf8_buffer(NULL, 10, NULL, 10, "CP1251");
    ut_assert("Convert NULL", r == (size_t)-1);
    r = convert_to_utf8_buffer(ascii, 10, NULL, 10, "CP1251");
    ut_assert("Convert NULL #2", r == 40);
    r = convert_to_utf8_buffer(ascii, 0, NULL, 10, "CP1251");
    ut_assert("Convert NULL #3", r == (strlen(ascii) + 1) * 4);

    r = convert_to_utf8_buffer(ascii, 0, out, 2, "CP1251");
    ut_assert("Buffer small", r == (size_t)-2);

    r = convert_to_utf8_buffer(ascii, 0, out, 2, "WIN1251");
    ut_assert("Unsupported encoding", r == (size_t)-3);

    r = convert_to_utf8_buffer(ascii, 0, out, 32, "CP1251");
    ut_assert("From ASCII convert", r == strlen(ascii)+1);
    r = convert_to_utf8_buffer(win1251, 0, out, 32, "CP1251");
    ut_assert("From CP1251 convert", r == 10);

    r = convert_from_utf8_buffer(ascii, 0, out, 32, "CP1251");
    ut_assert("To ASCII convert", r == strlen(ascii)+1);
    r = convert_from_utf8_buffer(utf, 0, out, 32, "CP1251");
    ut_assert("To CP1251 convert", r == 10);

    return 0;
}

static size_t number_of_ext_blocks(struct ext_buffer_t* bf) {
    if (bf == NULL || bf->head == NULL) {
        return 0;
    }

    struct ext_buffer_node_t *node = bf->head;
    size_t l = 0;
    while (node != NULL) {
        l++;
        node = node->next;
    }

    return l;
}

const char* test_ext_buffer() {
    struct ext_buffer_t* ebf = NULL;
    int res;
    size_t bsz = 0;

    ebf = ext_buffer_init();
    ut_assert("Ext Buffer initialized OK", ebf != NULL && ebf->head != NULL && ebf->current != NULL);

    res = ext_buffer_put_char(ebf, 'z');
    ut_assert("Ext Buffer - add char - OK", res == BOOK_SUCCESS);
    ut_assert("Ext Buffer - add char - used", ebf->current->used == 1);
    res = ext_buffer_put_char(ebf, 'x');
    ut_assert("Ext Buffer - add char - used", ebf->current->used == 2);
    ut_assert("Ext Buffer - add 2 chars - block count", number_of_ext_blocks(ebf) == 1);
    bsz = ext_buffer_size(ebf);
    ut_assert("Ext Buffer - add 2 chars - size", bsz == 2);

    char tbuf[8];
    tbuf[7] = '\0';
    res = ext_buffer_copy_data(ebf, tbuf, 1);
    ut_assert("Ext Buffer - copy small buffer", res == BOOK_BUFFER_SMALL);
    res = ext_buffer_copy_data(ebf, tbuf, 8);
    ut_assert("Ext Buffer - copy", res == BOOK_SUCCESS);
    ut_assert("Ext Buffer - copy size", strlen(tbuf) == 2);
    ut_assert("Ext Buffer - copy value", strcmp(tbuf, "zx") == 0);

    char *val = "example пример testing";
    size_t valsz = strlen(val);
    size_t cnt = (EXT_BUF_NODE_SIZE / strlen(val)) + 1;
    for (size_t i = 0; i < cnt; i++) {
        res = ext_buffer_put_string(ebf, val, 0);
        if (res != BOOK_SUCCESS) {
            printf("Iteration %d failed\n", (int)i);
            ut_assert("Ext Buffer - add string failed", res == BOOK_SUCCESS);
        }
    }
    ut_assert("Ext Buffer - many chars - block count", number_of_ext_blocks(ebf) == 2);
    ut_assert("Ext Buffer - many chars - new block started", ebf->head != ebf->current && ebf->head->next == ebf->current);
    size_t msize = 2 + cnt * valsz;
    size_t fsize = ((EXT_BUF_NODE_SIZE - 2) / valsz) * valsz + 2;
    ut_assert("Ext Buffer - many chars - first block free space", ebf->head->used == fsize);
    bsz = ext_buffer_size(ebf);
    ut_assert("Ext Buffer 2 sections size", bsz == msize);

    res = ext_buffer_destroy(ebf);
    ut_assert("Free Ext Buffer", res == BOOK_SUCCESS);

    return 0;
}

const char* test_utf_funcs() {
    char str[] = "    \x09 example \nsecond line\nthird line\n\n    forth";
    size_t cnt;

    char *nosp = utf_skip_spaces(str, &cnt);
    ut_assert("Leading spaces", cnt == 6);
    ut_assert("Leading spaces - correct", nosp != NULL && *nosp == 'e');
    nosp = utf_skip_spaces(str, NULL);
    ut_assert("Leading spaces - do not count", nosp != NULL && *nosp == 'e');

    char *nxt = utf_next_line(str);
    ut_assert("Second line", nxt != NULL && *nxt == 's');
    size_t spaces;
    size_t len = utf_line_length(nxt, &spaces);
    ut_assert("Second line lenght", spaces == 0 && len == strlen("second line"));
    nxt = utf_next_line(nxt);
    ut_assert("Third line", nxt != NULL && *nxt == 't');
    len = utf_line_length(nxt, &spaces);
    ut_assert("Third line lenght", spaces == 0 && len == strlen("third line"));
    nxt = utf_next_line(nxt);
    ut_assert("Fourth line", nxt != NULL && *nxt == '\n');
    len = utf_line_length(nxt, &spaces);
    ut_assert("Fourth line lenght", spaces == 0 && len == 0);
    nxt = utf_next_line(nxt);
    ut_assert("Fifth line", nxt != NULL && *nxt == ' ');
    len = utf_line_length(nxt, &spaces);
    ut_assert("Fifth line lenght", spaces == 4 && len == strlen("    forth"));
    nxt = utf_next_line(nxt);
    ut_assert("No more lines", nxt != NULL && *nxt == '\0');
    nxt = utf_next_line(nxt);
    ut_assert("No more lines - 2", nxt != NULL && *nxt == '\0');

    char *wrd_start;
    char *fwd = utf_word_forward(str, &wrd_start);
    ut_assert("Word forward 1", fwd != NULL && *wrd_start == 'e' && *fwd == ' ');
    char *save = fwd;
    fwd = utf_word_forward(fwd, &wrd_start);
    ut_assert("Word forward 2", fwd != NULL && *wrd_start == '\n' && *fwd == '\n' && save != wrd_start);
    fwd = utf_skip_newline(fwd);
    fwd = utf_word_forward(fwd, &wrd_start);
    ut_assert("Word forward 3", fwd != NULL && *wrd_start == 's' && *fwd == ' ');
    fwd = utf_word_forward(fwd, &wrd_start);
    ut_assert("Word forward 4", fwd != NULL && *wrd_start == 'l' && *fwd == '\n');
    fwd = utf_skip_newline(fwd);
    fwd = utf_word_forward(fwd, &wrd_start);
    ut_assert("Word forward 6", fwd != NULL && *wrd_start == 't' && *fwd == ' ');
    fwd = utf_word_forward(fwd, &wrd_start);
    fwd = utf_skip_newline(fwd);
    fwd = utf_skip_newline(fwd);
    save = fwd;
    fwd = utf_word_forward(fwd, &wrd_start);
    ut_assert("Word forward last", fwd != NULL && *wrd_start == 'f' && *fwd == '\0' && wrd_start - save == 4);
    fwd = utf_word_forward(fwd, &wrd_start);
    ut_assert("Word nowhere", fwd != NULL && *wrd_start == '\0' && *fwd == '\0' &&  wrd_start == fwd);

    int wcnt = utf_word_count("", 0);
    ut_assert("Empty string", wcnt == 0);
    wcnt = utf_word_count("   \x09  \x09  ", 0);
    ut_assert("Whitespaces only", wcnt == 0);
    wcnt = utf_word_count("wordпример", 0);
    ut_assert("One word", wcnt == 1);
    wcnt = utf_word_count("word  пример", 0);
    ut_assert("Two words", wcnt == 2);
    wcnt = utf_word_count("   word пример   ", 0);
    ut_assert("Two word with whitespaces", wcnt == 2);
    wcnt = utf_word_count(" áŋ \x09 word пример   ", 0);
    ut_assert("Three words with whitespaces", wcnt == 3);
    wcnt = utf_word_count("Two word with whitespaces", 0);
    ut_assert("Four words", wcnt == 4);
    wcnt = utf_word_count("Two word with whitespaces", 8);
    ut_assert("Four words/Two words", wcnt == 2);

    wcnt = utf_starts_with("test", "test2");
    ut_assert("Pattern long", wcnt == BOOK_NOT_EQUAL);
    wcnt = utf_starts_with("test", "");
    ut_assert("Empty pattern", wcnt == BOOK_EQUAL);
    wcnt = utf_starts_with("пример", "пример");
    ut_assert("Pattern the same", wcnt == BOOK_EQUAL);
    wcnt = utf_starts_with("пример", "прим");
    ut_assert("Pattern shorter", wcnt == BOOK_EQUAL);
    wcnt = utf_starts_with("пример", "примес");
    ut_assert("Pattern differs by letter", wcnt == BOOK_NOT_EQUAL);

    int em = utf_line_is_empty(NULL);
    ut_assert("NULL is empty line", em == 1);
    em = utf_line_is_empty("");
    ut_assert("Empty is empty line", em == 1);
    em = utf_line_is_empty("    \x09  ");
    ut_assert("Whitespaces is empty line", em == 1);
    em = utf_line_is_empty("    \x09 some text ");
    ut_assert("Not empty line", em == 0);

    char orig1[64] = "",
         orig2[64] = "abcd efg",
         orig3[64] = "abcdefgijk",
         orig4[64] = "abcde efghi",
         orig5[64] = "abcde efgh 01234 9810 23456",
         orig6[64] = "abcde efgh 01234 9810 23456"
         ;
    int res = utf_make_wide(NULL, 64, 20);
    ut_assert("Widen NULL string", res == BOOK_INVALID_ARG);
    res = utf_make_wide(orig1, 64, 20);
    ut_assert("Widen Empty string", res == BOOK_SUCCESS);
    res = utf_make_wide(orig2, 64, 20);
    ut_assert("Widen Too short string", res == BOOK_SUCCESS && strcmp(orig2, "abcd efg") == 0);
    res = utf_make_wide(orig3, 64, 12);
    ut_assert("Widen One long word", res == BOOK_SUCCESS && strcmp(orig3, "abcdefgijk") == 0);
    res = utf_make_wide(orig3, 64, 8);
    ut_assert("Widen Too long string", res == BOOK_CONVERT_FAIL && strcmp(orig3, "abcdefgijk") == 0);
    res = utf_make_wide(orig4, 64, 14);
    ut_assert("Widen Two words", res == BOOK_SUCCESS && strlen(orig4) == 14 && strcmp(orig4, "abcde    efghi") == 0);
    res = utf_make_wide(orig5, 64, 31);
    ut_assert("Widen Many words - 1 space after each word", res == BOOK_SUCCESS && strlen(orig5) == 31 && strcmp(orig5, "abcde  efgh  01234  9810  23456") == 0);
    res = utf_make_wide(orig6, 64, 33);
    ut_assert("Widen Many words - many spaces", res == BOOK_SUCCESS && strlen(orig6) == 33 && strcmp(orig6, "abcde  efgh   01234  9810   23456") == 0);

    res = utf_equal_no_case(NULL, NULL);
    ut_assert("Both NULL equal", res == 1);
    res = utf_equal_no_case(NULL, "");
    ut_assert("One NULL not equal - 1", res == 0);
    res = utf_equal_no_case("", NULL);
    ut_assert("One NULL not equal - 2", res == 0);
    res = utf_equal_no_case("", "");
    ut_assert("Empty equal", res == 1);
    res = utf_equal_no_case("AbcD", "abcd");
    ut_assert("Ascii equal", res == 1);
    res = utf_equal_no_case("AbcD", "abcde");
    ut_assert("Ascii not equal", res == 0);
    res = utf_equal_no_case("пример", "приМер");
    ut_assert("Non-ascii equal", res == 1);

    return 0;
}

const char * run_all_test() {
    ut_run_test("User Directory", test_get_user_directory);
    ut_run_test("Temp Directory", test_get_temp_directory);
    ut_run_test("Adding directory", test_append_path);
    ut_run_test("Up directory", test_path_up);
    ut_run_test("Upcase", test_upper);
    ut_run_test("Lower", test_lower);
    ut_run_test("Relative directories", test_rel_dirs);
    ut_run_test("Detect BOM", test_BOM);
    ut_run_test("Detect encoding", test_encoding);
    ut_run_test("String functions", test_str_funcs);
    ut_run_test("Hyphenation", test_hypen);
    ut_run_test("Buffer convert", test_buffer_convert);
    ut_run_test("Ext Buffer", test_ext_buffer);
    ut_run_test("UTF utils", test_utf_funcs);
    return 0;
}

int main() {
    const char* res = run_all_test();
    if (res && tests_fail == 0) {
        printf("%s\n", res);
    } else {
        printf("Tests run: %d\nSuccess: %d.\nFail: %d.", tests_run, tests_run - tests_fail, tests_fail);
    }
    return 0;
}
