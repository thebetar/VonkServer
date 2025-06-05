#include <stdio.h>
#include <string.h>

FILE *open_file(const char *collection_name, const char *mode)
{
    char filename[256];

    if (collection_name == NULL || strlen(collection_name) == 0)
    {
        strcpy(filename, "data/data.txt");
    }
    else
    {
        sprintf(filename, "data/%s.txt", collection_name);
    }

    FILE *file = fopen(filename, mode);
    if (file == NULL)
    {
        perror("Could not open file");
        return NULL;
    }

    return file;
}

char *get_data(char *collection_name)
{
    // Read data from data.txt
    static char data[4096]; // Buffer to hold the data

    FILE *file = open_file(collection_name, "r");

    if (file == NULL)
    {
        perror("Could not open data.txt");
        return "STATUS: Collection not found";
    }

    data[0] = '\0';

    // Initialise a buffer to read lines
    char line[256];

    // Read each line from the file and append it to data
    while (fgets(line, sizeof(line), file))
    {
        // Check if this line will fit in the data buffer
        if (strlen(data) + strlen(line) < sizeof(data) - 1)
        {
            // Concatenate the line to data
            strcat(data, line);
        }
        else
        {
            fprintf(stderr, "Data buffer overflow prevented\n");
            break; // Prevent buffer overflow
        }
    }

    fclose(file);

    return data;
}

char *post_data(char *collection_name, char *data)
{
    // Check if newline character is in the data (not allowed)
    if (strchr(data, '\n') != NULL)
    {
        return "STATUS: Newline character is not allowed in data";
    }

    // Write data to data.txt
    FILE *file = open_file(collection_name, "a");

    if (file == NULL)
    {
        perror("Could not open data.txt for writing");
        return "STATUS: Collection not found";
    }

    // Push newline character to data
    if (data[strlen(data) - 1] != '\n')
    {
        strcat(data, "\n");
    }

    // Get current data from file
    char *curRows = get_data(collection_name);

    // Check if data already exists
    if (strstr(curRows, data) != NULL)
    {
        fclose(file);
        return "STATUS: Data already exists in collection";
    }

    // Write data to file
    if (fputs(data, file) == EOF && fputs("\n", file) == EOF)
    {
        perror("Could not write data to data.txt");
        fclose(file);
        return "STATUS: Error writing data";
    }

    fclose(file);
    return "STATUS: Data written successfully";
}

char *delete_data(char *collection_name, char *data)
{
    // Read current data from file
    char *curRows = get_data(collection_name);

    // Check if data exists in current data
    if (strstr(curRows, data) == NULL)
    {
        return "STATUS: Data not found";
    }

    // Open file for writing
    FILE *file = fopen("data.txt", "w");
    if (file == NULL)
    {
        perror("Could not open data.txt for writing");
        return "STATUS: Error writing data";
    }

    // Write all lines except the one to delete
    char *line = strtok(curRows, "\n");

    // Loop through line until the end of the string
    while (line != NULL)
    {
        if (strcmp(line, data) != 0)
        {
            fputs(line, file);
            fputs("\n", file);
        }

        // Pass NULL to get the next token in the string
        line = strtok(NULL, "\n");
    }

    fclose(file);
    return "STATUS: Data deleted successfully";
}