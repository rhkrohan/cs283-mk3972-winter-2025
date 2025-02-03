#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>     
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

// database include files
#include "db.h"
#include "sdbsc.h"

/*
 *  open_db
 *      dbFile:  name of the database file
 *      should_truncate:  indicates if opening the file also empties it
 *
 *  returns:  File descriptor on success, or ERR_DB_FILE on failure
 *
 *  console:  Does not produce any console I/O on success
 *            M_ERR_DB_OPEN on error
 *
 */
int open_db(char *dbFile, bool should_truncate)
{
    // Set permissions: rw-rw----
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

    // open the file if it exists for Read and Write,
    // create it if it does not exist
    int flags = O_RDWR | O_CREAT;
    if (should_truncate)
        flags |= O_TRUNC;

    int fd = open(dbFile, flags, mode);
    if (fd == -1)
    {
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }
    return fd;
}

/*
 *  get_student
 *      fd:  linux file descriptor
 *      id:  the student id we are looking for
 *      *s:  pointer where the located student data will be copied
 *
 *  returns:  NO_ERROR       student located and copied into *s
 *            ERR_DB_FILE    database file I/O issue
 *            SRCH_NOT_FOUND student was not located in the database
 *
 *  console:  Does not produce any console I/O
 */
int get_student(int fd, int id, student_t *s)
{
    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    ssize_t n = read(fd, s, STUDENT_RECORD_SIZE);
    if (n < STUDENT_RECORD_SIZE)
    {
        // If no record is read, treat as not found.
        return SRCH_NOT_FOUND;
    }
    // If the record is all zeros, it is empty (or deleted).
    if (memcmp(s, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) == 0)
    {
        return SRCH_NOT_FOUND;
    }
    return NO_ERROR;
}

/*
 *  add_student
 *      fd:     linux file descriptor
 *      id:     student id (range is defined in db.h )
 *      fname:  student first name
 *      lname:  student last name
 *      gpa:    GPA as an integer (range defined in db.h)
 *
 *  returns:  NO_ERROR       student added to database
 *            ERR_DB_FILE    database file I/O issue
 *            ERR_DB_OP      student already exists
 *
 *  console:  M_STD_ADDED       on success
 *            M_ERR_DB_ADD_DUP  if student already exists
 *            M_ERR_DB_READ     error reading the database file
 *            M_ERR_DB_WRITE    error writing to the database file
 */
int add_student(int fd, int id, char *fname, char *lname, int gpa)
{
    off_t offset = id * STUDENT_RECORD_SIZE;
    student_t existing;
    
    // Seek to the proper offset.
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    // Read the record to see if it is already occupied.
    ssize_t n = read(fd, &existing, STUDENT_RECORD_SIZE);
    if (n == STUDENT_RECORD_SIZE)
    {
        if (memcmp(&existing, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0)
        {
            printf(M_ERR_DB_ADD_DUP, id);
            return ERR_DB_OP;
        }
    }
    
    // Prepare new student record.
    student_t new_student;
    new_student.id = id;
    memset(new_student.fname, 0, sizeof(new_student.fname));
    memset(new_student.lname, 0, sizeof(new_student.lname));
    strncpy(new_student.fname, fname, sizeof(new_student.fname) - 1);
    strncpy(new_student.lname, lname, sizeof(new_student.lname) - 1);
    new_student.gpa = gpa;
    
    // Seek again to the correct offset.
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    
    n = write(fd, &new_student, STUDENT_RECORD_SIZE);
    if (n < STUDENT_RECORD_SIZE)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    
    printf(M_STD_ADDED, id);
    return NO_ERROR;
}

/*
 *  del_student
 *      fd:     linux file descriptor
 *      id:     student id to be deleted
 *
 *  returns:  NO_ERROR       student deleted from database
 *            ERR_DB_FILE    database file I/O issue
 *            ERR_DB_OP      student not in database
 *
 *  console:  M_STD_DEL_MSG      on success
 *            M_STD_NOT_FND_MSG  if student not found
 *            M_ERR_DB_READ      error reading the database file
 *            M_ERR_DB_WRITE     error writing to the database file
 */
int del_student(int fd, int id)
{
    student_t s;
    int rc = get_student(fd, id, &s);
    if (rc == SRCH_NOT_FOUND)
    {
        printf(M_STD_NOT_FND_MSG, id);
        return ERR_DB_OP;
    }
    else if (rc != NO_ERROR)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    
    ssize_t n = write(fd, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE);
    if (n < STUDENT_RECORD_SIZE)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    
    printf(M_STD_DEL_MSG, id);
    return NO_ERROR;
}

/*
 *  count_db_records
 *      fd:     linux file descriptor
 *
 *  returns:  <number>       number of valid student records in the db
 *            ERR_DB_FILE    database file I/O issue
 *
 *  console:  M_DB_RECORD_CNT  if there are records, or M_DB_EMPTY if none
 *            M_ERR_DB_READ    on error
 */
int count_db_records(int fd)
{
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    student_t s;
    int count = 0;
    ssize_t n;
    
    while ((n = read(fd, &s, STUDENT_RECORD_SIZE)) == STUDENT_RECORD_SIZE)
    {
        if (memcmp(&s, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0)
        {
            count++;
        }
    }
    
    if (count == 0)
        printf(M_DB_EMPTY);
    else
        printf(M_DB_RECORD_CNT, count);
    
    return count;
}

/*
 *  print_db
 *      fd:     linux file descriptor
 *
 *  returns:  NO_ERROR       on success
 *            ERR_DB_FILE    database file I/O issue
 *
 *  console:  If there are valid records, first prints a header then each record.
 *            Otherwise, prints M_DB_EMPTY.
 */
int print_db(int fd)
{
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    student_t s;
    bool printedHeader = false;
    bool foundAny = false;
    
    while (read(fd, &s, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE)
    {
        if (memcmp(&s, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0)
        {
            if (!printedHeader)
            {
                printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST_NAME", "LAST_NAME", "GPA");
                printedHeader = true;
            }
            float real_gpa = s.gpa / 100.0;
            printf(STUDENT_PRINT_FMT_STRING, s.id, s.fname, s.lname, real_gpa);
            foundAny = true;
        }
    }
    
    if (!foundAny)
        printf(M_DB_EMPTY);
    
    return NO_ERROR;
}

/*
 *  print_student
 *      *s:   pointer to a student_t structure to be printed
 *
 *  returns: nothing (void)
 *
 *  console:  If s is valid, prints a header and then the student record.
 *            If s is NULL or s->id is 0, prints M_ERR_STD_PRINT.
 */
void print_student(student_t *s)
{
    if (s == NULL || s->id == 0)
    {
        printf(M_ERR_STD_PRINT);
        return;
    }
    
    printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST NAME", "LAST_NAME", "GPA");
    float real_gpa = s->gpa / 100.0;
    printf(STUDENT_PRINT_FMT_STRING, s->id, s->fname, s->lname, real_gpa);
}

/*
 *  compress_db (Extra Credit)
 *      fd:     linux file descriptor of the active database file
 *
 *  returns:  file descriptor of the new (compressed) database file
 *            ERR_DB_FILE    on any file I/O error
 *
 *  console:  M_DB_COMPRESSED_OK on success, or appropriate error messages
 */
int compress_db(int fd)
{
    // Rewind the original database file.
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    
    // Open the temporary database file.
    int temp_fd = open(TMP_DB_FILE, O_RDWR | O_CREAT | O_TRUNC,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (temp_fd == -1)
    {
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }
    
    student_t s;
    // Copy each valid record to the temporary file at its proper offset.
    while (read(fd, &s, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE)
    {
        if (memcmp(&s, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0)
        {
            off_t offset = s.id * STUDENT_RECORD_SIZE;
            if (lseek(temp_fd, offset, SEEK_SET) == -1)
            {
                printf(M_ERR_DB_WRITE);
                close(temp_fd);
                return ERR_DB_FILE;
            }
            if (write(temp_fd, &s, STUDENT_RECORD_SIZE) < STUDENT_RECORD_SIZE)
            {
                printf(M_ERR_DB_WRITE);
                close(temp_fd);
                return ERR_DB_FILE;
            }
        }
    }
    
    // Close both file descriptors.
    close(fd);
    close(temp_fd);
    
    // Rename the temporary file to the actual database file.
    if (rename(TMP_DB_FILE, DB_FILE) == -1)
    {
        printf(M_ERR_DB_CREATE);
        return ERR_DB_FILE;
    }
    
    // Reopen the compressed database file.
    int new_fd = open_db(DB_FILE, false);
    if (new_fd == ERR_DB_FILE)
    {
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }
    
    printf(M_DB_COMPRESSED_OK);
    return new_fd;
}

/*
 *  validate_range
 *      id:  proposed student id
 *      gpa: proposed gpa
 *
 *  returns:    NO_ERROR       on success, both ID and GPA are in range
 *              EXIT_FAIL_ARGS if either ID or GPA is out of range
 *
 *  console:  Does not produce any output
 */
int validate_range(int id, int gpa)
{
    if ((id < MIN_STD_ID) || (id > MAX_STD_ID))
        return EXIT_FAIL_ARGS;
    if ((gpa < MIN_STD_GPA) || (gpa > MAX_STD_GPA))
        return EXIT_FAIL_ARGS;
    return NO_ERROR;
}

/*
 *  usage
 *      exename:  the name of the executable from argv[0]
 *
 *  Prints this program's expected usage
 *
 *  returns:    nothing, this is a void function
 *
 *  console:  This function prints the usage information
 */
void usage(char *exename)
{
    printf("usage: %s -[h|a|c|d|f|p|z] options.  Where:\n", exename);
    printf("\t-h:  prints help\n");
    printf("\t-a id first_name last_name gpa(as 3 digit int):  adds a student\n");
    printf("\t-c:  counts the records in the database\n");
    printf("\t-d id:  deletes a student\n");
    printf("\t-f id:  finds and prints a student in the database\n");
    printf("\t-p:  prints all records in the student database\n");
    printf("\t-x:  compress the database file [EXTRA CREDIT]\n");
    printf("\t-z:  zero db file (remove all records)\n");
}

// Welcome to main()
int main(int argc, char *argv[])
{
    char opt;      // user selected option
    int fd;        // file descriptor for database file
    int rc;        // return code from various operations
    int exit_code; // exit code to shell
    int id;        // student id
    int gpa;       // gpa

    // Space for a student structure to be used by various functions.
    student_t student = {0};

    // Check that there is at least one argument and that the first begins with '-'
    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    // Option is the character following '-'
    opt = *(argv[1] + 1);

    // If the help flag is given, show usage and exit.
    if (opt == 'h')
    {
        usage(argv[0]);
        exit(EXIT_OK);
    }

    // Open the database file (do not truncate).
    fd = open_db(DB_FILE, false);
    if (fd < 0)
    {
        exit(EXIT_FAIL_DB);
    }

    exit_code = EXIT_OK;
    switch (opt)
    {
    case 'a':
        // Expected arguments: -a id first_name last_name gpa
        if (argc != 6)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        gpa = atoi(argv[5]);
        exit_code = validate_range(id, gpa);
        if (exit_code == EXIT_FAIL_ARGS)
        {
            printf(M_ERR_STD_RNG);
            break;
        }
        rc = add_student(fd, id, argv[3], argv[4], gpa);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'c':
        rc = count_db_records(fd);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'd':
        if (argc != 3)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        rc = del_student(fd, id);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'f':
        if (argc != 3)
        {
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
            break;
        }
        id = atoi(argv[2]);
        rc = get_student(fd, id, &student);
        switch (rc)
        {
        case NO_ERROR:
            print_student(&student);
            break;
        case SRCH_NOT_FOUND:
            printf(M_STD_NOT_FND_MSG, id);
            exit_code = EXIT_FAIL_DB;
            break;
        default:
            printf(M_ERR_DB_READ);
            exit_code = EXIT_FAIL_DB;
            break;
        }
        break;

    case 'p':
        rc = print_db(fd);
        if (rc < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'x':
        // Compress the database file (extra credit).
        fd = compress_db(fd);
        if (fd < 0)
            exit_code = EXIT_FAIL_DB;
        break;

    case 'z':
        // Zero the database (remove all records).
        close(fd);
        fd = open_db(DB_FILE, true);
        if (fd < 0)
        {
            exit_code = EXIT_FAIL_DB;
            break;
        }
        printf(M_DB_ZERO_OK);
        exit_code = EXIT_OK;
        break;

    default:
        usage(argv[0]);
        exit_code = EXIT_FAIL_ARGS;
    }

    close(fd);
    exit(exit_code);
}
