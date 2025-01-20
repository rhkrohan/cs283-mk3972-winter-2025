//Name: Muhammad Rohan Khan
//CS283: Assignment 1 - C Refresher 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SZ 50

void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);
int count_words(char *, int, int);
int reverse_string(char *, int);
int print_words(char *, int);
int search_replace(char *, int, char *, char *);

int setup_buff(char *buff, char *user_str, int len) {
    char *src = user_str;
    char *dst = buff;
    int count = 0;
    int seenSpace = 0;
    while (*src == ' ' || *src == '\t') {
        src++;
    }
    while (*src != '\0') {
        if (*src == ' ' || *src == '\t') {
            if (!seenSpace) {
                if (count >= len) return -1;
                *dst++ = ' ';
                count++;
                seenSpace = 1;
            }
            src++;
        } else {
            if (count >= len) return -1;
            *dst++ = *src++;
            count++;
            seenSpace = 0;
        }
    }
    if (count > 0 && *(dst - 1) == ' ') {
        dst--;
        count--;
    }
    while (count < len) {
        *dst++ = '.';
        count++;
    }
    {
        int userCount = -1;
        char *p = buff;
        for (int i = 0; i < len; i++) {
            if (*p == '.') {
                userCount = i;
                break;
            }
            p++;
        }
        if (userCount < 0) {
            userCount = len;
        }
        return userCount;
    }
}

void print_buff(char *buff, int len) {
    printf("Buffer:  [");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i));
    }
    printf("]\n");
}

void usage(char *exename) {
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int buflen, int str_len) {
    if (str_len <= 0) return 0;
    int count = 0;
    int inWord = 0;
    char *p = buff;
    for (int i = 0; i < str_len; i++) {
        char c = *(p + i);
        if (c == ' ') {
            inWord = 0;
        } else {
            if (!inWord) {
                count++;
                inWord = 1;
            }
        }
    }
    return count;
}

int reverse_string(char *buff, int str_len) {
    char *left = buff;
    char *right = buff + (str_len - 1);
    while (left < right) {
        char tmp = *left;
        *left = *right;
        *right = tmp;
        left++;
        right--;
    }
    return 0;
}

int print_words(char *buff, int str_len) {
    printf("Word Print\n");
    printf("----------\n");
    int wordCount = 0;
    int inWord = 0;
    int startIdx = 0;
    char *p = buff;
    for (int i = 0; i < str_len; i++) {
        char c = *(p + i);
        if (c == ' ') {
            if (inWord) {
                wordCount++;
                printf("%d. ", wordCount);
                int wlen = i - startIdx;
                for (int j = startIdx; j < i; j++) {
                    putchar(*(p + j));
                }
                printf("(%d)\n", wlen);
                inWord = 0;
            }
        } else {
            if (!inWord) {
                inWord = 1;
                startIdx = i;
            }
        }
    }
    if (inWord) {
        wordCount++;
        printf("%d. ", wordCount);
        int wlen = str_len - startIdx;
        for (int j = startIdx; j < str_len; j++) {
            putchar(*(p + j));
        }
        printf("(%d)\n", wlen);
    }
    return wordCount;
}

int search_replace(char *buff, int str_len, char *find, char *replace) {
    int findLen = 0;
    int replaceLen = 0;
    {
        char *f = find;
        while (*f != '\0') {
            findLen++;
            f++;
        }
    }
    {
        char *r = replace;
        while (*r != '\0') {
            replaceLen++;
            r++;
        }
    }
    if (findLen == 0) {
        return -1;
    }
    int foundIdx = -1;
    for (int i = 0; i <= str_len - findLen; i++) {
        int match = 1;
        for (int j = 0; j < findLen; j++) {
            if (*(buff + i + j) != *(find + j)) {
                match = 0;
                break;
            }
        }
        if (match) {
            foundIdx = i;
            break;
        }
    }
    if (foundIdx < 0) {
        return -1;
    }
    int newLen = str_len - findLen + replaceLen;
    if (newLen > BUFFER_SZ) {
        newLen = BUFFER_SZ;
    }
    int remainderCount = str_len - (foundIdx + findLen);
    int newRemainderStart = foundIdx + replaceLen;
    char remainder[BUFFER_SZ];
    int i;
    for (i = 0; i < remainderCount; i++) {
        remainder[i] = *(buff + foundIdx + findLen + i);
    }
    for (i = 0; i < replaceLen; i++) {
        if ((foundIdx + i) >= BUFFER_SZ) {
            break;
        }
        *(buff + foundIdx + i) = *(replace + i);
    }
    int toCopy = remainderCount;
    if (newRemainderStart + toCopy > BUFFER_SZ) {
        toCopy = BUFFER_SZ - newRemainderStart;
    }
    for (i = 0; i < toCopy; i++) {
        *(buff + newRemainderStart + i) = remainder[i];
    }
    for (int fill = newRemainderStart + toCopy; fill < BUFFER_SZ; fill++) {
        *(buff + fill) = '.';
    }
    return newLen;
}

int main(int argc, char *argv[]) {
    char *buff;
    char *input_string;
    char opt;
    int rc;
    int user_str_len;
    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }
    opt = (char)*(argv[1] + 1);
    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }
    input_string = argv[2];
    buff = (char *)malloc(BUFFER_SZ * sizeof(char));
    if (!buff) {
        fprintf(stderr, "error: memory allocation failed\n");
        exit(2);
    }
    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);
    if (user_str_len < 0) {
        fprintf(stderr, "error: Provided input string is too long\n");
        free(buff);
        exit(3);
    }
    switch (opt) {
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error counting words, rc = %d\n", rc);
                free(buff);
                exit(3);
            }
            printf("Word Count: %d\n", rc);
            break;
        case 'r':
            rc = reverse_string(buff, user_str_len);
            if (rc < 0) {
                fprintf(stderr, "error reversing string\n");
                free(buff);
                exit(3);
            }
            break;
        case 'w': {
            int wordCount = print_words(buff, user_str_len);
            printf("\nNumber of words returned: %d\n", wordCount);
        }
            break;
        case 'x':
            if (argc < 5) {
                usage(argv[0]);
                free(buff);
                exit(1);
            } else {
                char *findStr = argv[3];
                char *replaceStr = argv[4];
                rc = search_replace(buff, user_str_len, findStr, replaceStr);
                if (rc < 0) {
                    fprintf(stderr, "error: substring not found or other replace error\n");
                    free(buff);
                    exit(3);
                } else {
                    user_str_len = rc;
                }
            }
            break;
        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}
