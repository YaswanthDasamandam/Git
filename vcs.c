#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define TRACKING_FILE_PATH ".vcs/tracking"
#define MAX_FILE_PATH_LENGTH 256
#define MAX_LINE_LENGTH 256 + 256 + 2
#define VCS_PATH ".vcs"
#define MAX_TRACKING_FILES 100
#define MAX_COMMIT_MESSAGE_LENGTH 1000
#define BUFFER_SIZE 1024
#define COMMIT_FILE_PATH ".vcs/commitfile"
#define LOG_PATH ".vcs/log"
#define BLOB_STORE ".vcs/blobs"
#define COMMIT_STORE ".vcs/commits"

#ifdef _WIN32
#include <io.h>
#define access _access
#endif

#ifdef _WIN32 /* For Windows systems (adjust error handling as needed)*/
#define mkdir(name, mode) _mkdir(name)
#endif

char ARR_Tracking_files[MAX_TRACKING_FILES][MAX_FILE_PATH_LENGTH];
int num_files = 0;

void saveArrayToFile(const char *filePath)
{
    FILE *fp = fopen(filePath, "w");
    if (fp == NULL)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_files; i++)
    {
        if (fprintf(fp, "%s\n", ARR_Tracking_files[i]) < 0)
        {
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);
}

char *read_file_to_string(const char *filename)
{
    FILE *file = fopen(filename, "rb"); /* Open in binary mode for any kind of file*/
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return NULL;
    }

    /* Get file size (optional, can be omitted if buffer size is large enough)*/
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Allocate memory for the string (plus null terminator)*/
    char *buffer;
    if (size != -1)
    {
        buffer = malloc(size + 1);
    }
    else
    {
        buffer = malloc(BUFFER_SIZE + 1);
    }
    if (buffer == NULL)
    {
        fclose(file);
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    /* Read file contents into the buffer*/
    size_t bytes_read = fread(buffer, 1, (size != -1) ? size : BUFFER_SIZE, file);
    buffer[bytes_read] = '\0'; /* Ensure null termination*/

    fclose(file);
    return buffer;
}

char *DJB2_hash_string(const char *str, size_t hash_size)
{
    if (hash_size == 0 || hash_size > 32)
    {
        return NULL; /* Handle invalid hash size*/
    }

    uint32_t mask = (1u << hash_size) - 1; /* Create a mask for desired hash size*/
    uint32_t hash = 5381;
    unsigned char c; /* Use unsigned char to avoid sign extension issues*/
    while ((c = (unsigned char)*str++))
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    /* Allocate memory for the hash string (adjusted for size)*/
    size_t string_size = (hash_size + 3) / 4 + 1; /* Account for potential extra characters*/
    char *hash_string = (char *)malloc(string_size * sizeof(char));
    if (hash_string == NULL)
    {
        return NULL; /* Handle memory allocation failure*/
    }

    /* Convert a portion of the hash to a hexadecimal string based on size*/
    snprintf(hash_string, string_size, "%0*x", (int)(hash_size / 4), hash & mask);

    return hash_string;
}

int save_string_to_file(const char *file_string, const char *to_folder, const char *file_path)
{
    /* Ensure folder path ends with a slash*/
    size_t folder_len = strlen(to_folder);
    char folder_with_slash[folder_len + 2]; /* +2 for slash and null terminator*/

    strcpy(folder_with_slash, to_folder);
    if (to_folder[folder_len - 1] != '/')
    {
        strcat(folder_with_slash, "/");
    }

    /* Concatenate folder and file path*/
    size_t full_path_len = strlen(folder_with_slash) + strlen(file_path) + 1;
    char full_path[full_path_len];

    snprintf(full_path, full_path_len, "%s%s", folder_with_slash, file_path);

    /* Open the file for writing*/
    FILE *file = fopen(full_path, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file save: %s\n", full_path);
        return -1;
    }

    /* Write the string to the file*/
    size_t string_len = strlen(file_string);
    size_t bytes_written = fwrite(file_string, 1, string_len, file);
    if (bytes_written < string_len)
    {
        fclose(file);
        fprintf(stderr, "Error writing to file: %s\n", full_path);
        return -1;
    }

    fclose(file);
    return 0;
}


/* Function to check if a file exists*/
int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

/* Function to create a directory if it doesn't already exist*/
int create_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("Directory '%s' already exists.\n", path);
        return 0;
    }

    if (mkdir(path, 0755) != 0) {
        perror("Error creating directory");
        return 1;
    }
    return 0;
}

/*Function to create a file*/
int create_file(const char *path) {
    if (file_exists(path)) {
        printf("File '%s' already exists.\n", path);
        return 0;
    }
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror("Error creating file");
        return 1;
    }
    fclose(file);
    return 0;
}

int clear_contents(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("Error opening file");
        return 1;
    }
    fclose(fp);
    /*printf("File '%s' contents cleared.\n", filename);*/
    return 0;
}

int main(int argc, char *argv[])
{
    /*Some other handling */
    char filename[MAX_FILE_PATH_LENGTH];

    if (argc < 2)
    {
        printf("Usage: %s git <command>\n", argv[0]); /* argv[0] is the program name*/
        return 1;
    }

    if (strcmp(argv[1], "init") == 0)
    {
        /* For directories */
        if (create_directory(VCS_PATH) != 0)
            return 1;
        if (create_directory(BLOB_STORE) != 0)
            return 1;
        if (create_directory(COMMIT_STORE) != 0)
            return 1;

        /*For files */
        if (create_file(TRACKING_FILE_PATH) != 0)
            return 1;
        if (create_file(COMMIT_FILE_PATH) != 0)
            return 1;
        if (create_file(LOG_PATH) != 0)
            return 1;

        return 0;
    }
    else if (strcmp(argv[1], "add") == 0)
    {
        num_files = 0;
        char buffer[MAX_FILE_PATH_LENGTH];
        /* char ARR_Tracking_files[MAX_TRACKING_FILES][MAX_FILE_PATH_LENGTH];*/
        /* int row = 0;*/

        /* Check if all given files exist*/
        for (int i = 2; i < argc; i++)
        {
            struct stat statbuf;
            int result = stat(argv[i], &statbuf);

            if (result == -1)
            {
                /* Handle file open error (doesn't exist or other issues)*/
                printf("Not able to open the file %s", argv[i]);
                perror("stat Not able to open file");
                return 1;
            }

            if (S_ISREG(statbuf.st_mode))
            {
                /* printf("%s is a regular file.\n", argv[i]); */

                FILE *file = fopen(argv[i], "r");
                if (file == NULL)
                {
                    printf("File %s does not exist or cannot be opened.\n", argv[i]);
                    return 1;
                }
                fclose(file);
            }
            else if (S_ISDIR(statbuf.st_mode))
            {
                printf("%s is a directory.\n", argv[i]);
                return 1;
            }
            else
            {
                printf("%s is something else (not a regular file or directory).\n", argv[i]);
                return 1;
            }
        }

        /* Check if the tracking file exists*/
        FILE *tracking_file = fopen(TRACKING_FILE_PATH, "r");
        if (tracking_file == NULL)
        {
            printf("git is not initilized please run 'git init'  to start tracking "); /* Todo change message*/
            return 1;
        }
        else
        {
            while (fgets(buffer, MAX_FILE_PATH_LENGTH, tracking_file) != NULL && num_files < MAX_TRACKING_FILES)
            {
                /* Copy the contents of the buffer to the ARR_Tracking_files*/
                int col = 0;
                for (int i = 0; buffer[i] != '\0' && buffer[i] != '\n'; i++)
                {
                    ARR_Tracking_files[num_files][col++] = buffer[i];
                }
                ARR_Tracking_files[num_files][col] = '\0'; /* Null-terminate the string*/
                num_files++;
            }

            fclose(tracking_file);
        }

        /* add new files*/
        /* int file_no = 0;*/

        for (int i = 2; i < argc; i++)
        {
            /* Todo add if no of files is grater thant Max tracking files*/
            /*printf("Adding file %s\n", argv[i]);*/
            strcpy(ARR_Tracking_files[num_files], argv[i]);
            /* printf("Printing ARR_File_Buffer file %s\n", ARR_Tracking_files[num_files]);*/
            num_files++;
        }

        /* Remove duplicates using sorting and two-pointer approach*/
        if (num_files > 0)
        {
            qsort(ARR_Tracking_files, num_files, MAX_FILE_PATH_LENGTH, (int (*)(const void *, const void *))strcmp);
            int i, j;
            for (i = 0, j = 1; j < num_files; j++)
            {
                if (strcmp(ARR_Tracking_files[i], ARR_Tracking_files[j]) != 0)
                {
                    strcpy(ARR_Tracking_files[++i], ARR_Tracking_files[j]);
                }
            }
            num_files = i + 1;
        }

        /* Printing the contents of the 2D array*/
        for (int i = 0; i < num_files; i++)
        {
            printf("%s\n", ARR_Tracking_files[i]);
        }

        saveArrayToFile(TRACKING_FILE_PATH);
        return 0;
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        FILE *file = fopen(TRACKING_FILE_PATH, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error opening file.\n");
            return 1;
        }
        char buffer[MAX_FILE_PATH_LENGTH];
        char ARR_File_Buffer[MAX_TRACKING_FILES][MAX_FILE_PATH_LENGTH];
        int row = 0; /* change it to file*/

        while (fgets(buffer, MAX_FILE_PATH_LENGTH, file) != NULL && row < MAX_TRACKING_FILES)
        {
            /* Copy the contents of the buffer to the ARR_File_Buffer*/
            int col = 0;
            for (int i = 0; buffer[i] != '\0' && buffer[i] != '\n'; i++)
            {
                ARR_File_Buffer[row][col++] = buffer[i];
            }
            ARR_File_Buffer[row][col] = '\0'; /* Null-terminate the string*/
            row++;
        }
        fclose(file);
        /* Printing the contents of the 2D ARR_File_Buffer*/
        /* Add one more condition if there are no files to track*/
        printf("Files Tracked are \n");
        for (int i = 0; i < row; i++)
        {
            printf("%s\n", ARR_File_Buffer[i]);
        }
        return 0;
    }
    else if (strcmp(argv[1], "commit") == 0)
    {
        if (argc < 3 || strcmp(argv[2], "-m") != 0)
        {
            printf("Usage: git commit -m \"message\"\n");
            return 1;
        }

        char *commit_message = argv[3];

        FILE *file = fopen(TRACKING_FILE_PATH, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error opening file.\n");
            return 1;
        }

        char buffer[MAX_FILE_PATH_LENGTH];
        char ARR_File_Buffer[MAX_TRACKING_FILES][MAX_FILE_PATH_LENGTH];
        int row = 0;

        while (fgets(buffer, MAX_FILE_PATH_LENGTH, file) != NULL && row < MAX_TRACKING_FILES)
        {
            int col = 0;
            for (int i = 0; buffer[i] != '\0' && buffer[i] != '\n'; i++)
            {
                ARR_File_Buffer[row][col++] = buffer[i];
            }
            ARR_File_Buffer[row][col] = '\0';
            row++;
        }
        fclose(file);

        size_t initial_size = 1000;
        char *accumulated_data = malloc(initial_size * sizeof(char));
        if (accumulated_data == NULL)
        {
            fprintf(stderr, "Memory allocation error.\n");
            return 1;
        }
        accumulated_data[0] = '\0';
        size_t accumulated_size = initial_size;

        for (int i = 0; i < row; i++)
        {
            /*printf("Processing %s\n", ARR_File_Buffer[i]);*/
            char *file_string = read_file_to_string(ARR_File_Buffer[i]);

            int hash_size = 8;
            while (1)
            {
                char *hash = DJB2_hash_string(file_string, hash_size);
                if (hash == NULL)
                {
                    printf("Hash size not possible\n");
                    free(file_string);
                    free(accumulated_data);
                    return 1;
                }
                /*printf("Generated Hash: %s\n", hash);*/

                char check_file_path[MAX_FILE_PATH_LENGTH];
                strcpy(check_file_path, BLOB_STORE);
                strcat(check_file_path, "/");
                strcat(check_file_path, hash);

                if (access(check_file_path, F_OK) == 0)
                {
                    char *file_string2 = read_file_to_string(check_file_path);
                    if (strcmp(file_string, file_string2) == 0)
                    {
                        printf("Collision, but both are the same, no need to save again\n");
                        free(file_string2);

                        size_t new_size = accumulated_size + strlen(ARR_File_Buffer[i]) + strlen(hash) + 2;
                        char *new_data = realloc(accumulated_data, new_size * sizeof(char));
                        if (new_data == NULL)
                        {
                            fprintf(stderr, "Memory allocation error.\n");
                            free(file_string);
                            free(hash);
                            free(accumulated_data);
                            return 1;
                        }
                        accumulated_data = new_data;

                        strcat(accumulated_data, ARR_File_Buffer[i]);
                        strcat(accumulated_data, ":");
                        strcat(accumulated_data, hash);
                        strcat(accumulated_data, "\n");
                        accumulated_size = new_size;
                        free(hash);
                        break;
                    }
                    hash_size += 2;
                    free(file_string2);
                }
                else
                {
                    save_string_to_file(file_string, BLOB_STORE, hash);

                    size_t new_size = accumulated_size + strlen(ARR_File_Buffer[i]) + strlen(hash) + 2;
                    char *new_data = realloc(accumulated_data, new_size * sizeof(char));
                    if (new_data == NULL)
                    {
                        fprintf(stderr, "Memory allocation error.\n");
                        free(file_string);
                        free(hash);
                        free(accumulated_data);
                        return 1;
                    }
                    accumulated_data = new_data;

                    strcat(accumulated_data, ARR_File_Buffer[i]);
                    strcat(accumulated_data, ":");
                    strcat(accumulated_data, hash);
                    strcat(accumulated_data, "\n");
                    accumulated_size = new_size;
                    free(hash);
                    break;
                }
            }
            free(file_string);
            /*printf("Processing over %s\n", ARR_File_Buffer[i]);*/
        }

        /*printf("Accumulated data is %s\n", accumulated_data);*/
        int hash_size = 8;
        char *final_hash = DJB2_hash_string(accumulated_data, hash_size);

        char check_commit_path[MAX_FILE_PATH_LENGTH];
        strcpy(check_commit_path, COMMIT_STORE);
        strcat(check_commit_path, "/");
        strcat(check_commit_path, final_hash);
        while (file_exists(check_commit_path))
        {
            hash_size += 2;
            final_hash = DJB2_hash_string(accumulated_data, hash_size);
            strcpy(check_commit_path, COMMIT_STORE);
            strcat(check_commit_path, "/");
            strcat(check_commit_path, final_hash);
        }

        save_string_to_file(accumulated_data, COMMIT_STORE, final_hash);

        FILE *log_file = fopen(LOG_PATH, "a");
        if (log_file == NULL)
        {
            fprintf(stderr, "Error opening log file.\n");
            free(accumulated_data);
            free(final_hash);
            return 1;
        }
        fprintf(log_file, "Commit hash: %s\n", final_hash);
        fprintf(log_file, "Commit message: %s\n", commit_message);
        fclose(log_file);

        printf("Commit  is %s\n", final_hash);

        clear_contents(TRACKING_FILE_PATH);

        free(accumulated_data);
        free(final_hash);

        return 0;
    }
    else if (strcmp(argv[1], "log") == 0)
    {
        FILE *fp;
        char ch;
        /* Open the file in read mode*/
        fp = fopen(LOG_PATH, "r");
        /* Check if the file was opened successfully*/
        if (fp == NULL)
        {
            perror("Error opening file");
            return 1;
        }
        /* Read the file character by character and print it*/
        
        int cha;
        while ((cha = fgetc(fp)) != EOF ) 
        {
            printf("%c",(char)cha);
           
        }
        /* Close the file*/
        fclose(fp);
        return 0;
    }
    else if (strcmp(argv[1], "checkout") == 0)
    {
        /* git checkout <commit-hash>*/
        if (argc != 3)
        {
            fprintf(stderr, "Usage: %s checkout <commit hash>\n", argv[0]);
            return 1;
        }

        char filename[MAX_LINE_LENGTH];
        snprintf(filename, sizeof(filename), "%s/%s", COMMIT_STORE, argv[2]);

        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            perror("fopen");
            return 1;
        }

        char line[MAX_LINE_LENGTH];
        char *filepath, *hash;
        char From_file_Path [MAX_FILE_PATH_LENGTH];
        char To_file_Path [MAX_FILE_PATH_LENGTH];

        while (fgets(line, sizeof(line), fp))
        {
            /* Remove trailing newline character if present*/
            line[strcspn(line, "\n")] = '\0';

            /* Split the line using colon as delimiter*/
            filepath = strtok(line, ":");
            hash = strtok(NULL, ":");

            if (filepath == NULL || hash == NULL)
            {
                fprintf(stderr, "Invalid line format in file\n");
                continue;
            }
            /*From file */
            strcpy(From_file_Path,BLOB_STORE);
            strcat(From_file_Path,"/");
            strcat(From_file_Path,hash);
            /*To file */
            strcpy(To_file_Path,filepath);

            /*
            printf("To_file_Path: %s\n", To_file_Path);
            printf("From_file_Path: %s\n", From_file_Path); */

            char *file_string = read_file_to_string(From_file_Path);
            save_string_to_file(file_string, "./",To_file_Path);

            free(file_string);
        }

        fclose(fp);
        return 0;
    }
    return 0;
}
