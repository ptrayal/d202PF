/* shopconv.c - Converts 2.20 shop files to 3.0 shop files */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "shop.h"

void basic_mud_log(const char *message, ...)
{
    puts(message);
}

char *fread_string(FILE *file, const char *error)
{
    char buffer[MAX_STRING_LENGTH] = {'\0'}, temp[512] = {'\0'}, *result, *pointer;
    int flag;

    *buffer = '\0';

    do
    {
        if (!fgets(temp, sizeof(temp), file))
        {
            printf("fread_string: format error at or near %s\n", error);
            exit(1);
        }

        if (strlen(temp) + strlen(buffer) > MAX_STRING_LENGTH)
        {
            printf("SYSERR: fread_string: string too large (shopconv.c)\n");
            exit(1);
        }
        else
        {
            strcat(buffer, temp);
        }

        for (pointer = buffer + strlen(buffer) - 2; pointer >= buffer && isspace(*pointer); pointer--);
        if ((flag = (*pointer == '~')))
        {
            if (*(buffer + strlen(buffer) - 3) == '\n')
            {
                *(buffer + strlen(buffer) - 2) = '\0';
            }
            else
            {
                *(buffer + strlen(buffer) - 2) = '\0';
            }
        }
    }
    while (!flag);

    if (strlen(buffer) > 0)
    {
        CREATE(result, char, strlen(buffer) + 1);
        strcpy(result, buffer);
    }
    else
    {
        result = NULL;
    }

    return result;
}

void do_list(FILE *shopFile, FILE *newShopFile, int max)
{
    int count, temp;
    char buffer[MAX_STRING_LENGTH] = {'\0'};

    for (count = 0; count < max; count++)
    {
        if (fscanf(shopFile, "%d", &temp) != 1)
        {
            printf("Error reading integer in do_list.\n");
            exit(1);
        }

        if (fgets(buffer, MAX_STRING_LENGTH - 1, shopFile) == NULL)
        {
            printf("Error reading string in do_list.\n");
            exit(1);
        }

        if (temp > 0)
        {
            fprintf(newShopFile, "%d%s", temp, buffer);
        }
    }

    fprintf(newShopFile, "-1\n");
}


void do_float(FILE *shopFile, FILE *newShopFile)
{
    float f;
    char str[20];

    if (fscanf(shopFile, "%f \n", &f) != 1)
    {
        printf("Error reading float in do_float.\n");
        exit(1);
    }

    sprintf(str, "%f", f);

    while ((str[strlen(str) - 1] == '0') && (str[strlen(str) - 2] != '.'))
    {
        str[strlen(str) - 1] = 0;
    }

    fprintf(newShopFile, "%s \n", str);
}

void do_int(FILE *shopFile, FILE *newShopFile)
{
    int i;

    if (fscanf(shopFile, "%d \n", &i) != 1)
    {
        printf("Error reading integer in do_int.\n");
        exit(1);
    }

    fprintf(newShopFile, "%d \n", i);
}

void do_string(FILE *shopFile, FILE *newShopFile, char *msg)
{
    char *ptr;

    ptr = fread_string(shopFile, msg);
    fprintf(newShopFile, "%s~\n", ptr);
    free(ptr);
}

int boot_the_shops(FILE *shopFile, FILE *newShopFile, char *filename)
{
    char *buffer, buffer2[150];
    int temp, count;

    sprintf(buffer2, "beginning of shop file %s", filename);
    fprintf(newShopFile, "CircleMUD %s Shop File~\n", VERSION3_TAG);

    for (;;)
    {
        buffer = fread_string(shopFile, buffer2);

        if (*buffer == '#')   /* New shop */
        {
            sscanf(buffer, "#%d\n", &temp);
            sprintf(buffer2, "shop #%d in shop file %s", temp, filename);
            fprintf(newShopFile, "#%d~\n", temp);
            free(buffer);
            printf("   #%d\n", temp);

            do_list(shopFile, newShopFile, MAX_PROD); /* Produced Items */
            do_float(shopFile, newShopFile);           /* Ratios */
            do_float(shopFile, newShopFile);

            do_list(shopFile, newShopFile, MAX_TRADE); /* Bought Items */

            for (count = 0; count < 7; count++)       /* Keeper msgs */
                do_string(shopFile, newShopFile, buffer2);

            for (count = 0; count < 5; count++)       /* Misc */
                do_int(shopFile, newShopFile);

            fprintf(newShopFile, "-1\n");

            for (count = 0; count < 4; count++)       /* Open/Close */
                do_int(shopFile, newShopFile);
        }
        else
        {
            if (*buffer == '$')   /* EOF */
            {
                free(buffer);
                fprintf(newShopFile, "$~\n");
                break;
            }
            else if (strstr(buffer, VERSION3_TAG))
            {
                printf("%s: New format detected, conversion aborted!\n", filename);
                free(buffer);
                return 1;
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    FILE *shopFile, *newShopFile;
    char filename[256], tempFilename[1024];  // Increased buffer size for tempFilename
    int result, index;

    if (argc < 2)
    {
        printf("Usage: shopconv <file1> [file2] [file3] ...\n");
        exit(1);
    }

    for (index = 1; index < argc; index++)
    {
        snprintf(filename, sizeof(filename), "%s", argv[index]);
        snprintf(tempFilename, sizeof(tempFilename), "mv %s %s.tmp", filename, filename);

        if (system(tempFilename) == -1)
        {
            printf("Error executing system command: %s\n", tempFilename);
            exit(1);
        }

        snprintf(tempFilename, sizeof(tempFilename), "%s.tmp", filename);
        shopFile = fopen(tempFilename, "r");

        if (shopFile == NULL)
        {
            perror(filename);
        }
        else
        {
            if ((newShopFile = fopen(filename, "w")) == NULL)
            {
                printf("Error writing to %s.\n", filename);
                fclose(shopFile);
                continue;
            }

            printf("%s:\n", filename);
            result = boot_the_shops(shopFile, newShopFile, filename);
            fclose(newShopFile);
            fclose(shopFile);

            if (result)
            {
                snprintf(tempFilename, sizeof(tempFilename), "mv %s.tmp %s", filename, filename);
                if (system(tempFilename) == -1)
                {
                    printf("Error executing system command: %s\n", tempFilename);
                    exit(1);
                }
            }
            else
            {
                snprintf(tempFilename, sizeof(tempFilename), "mv %s.tmp %s.bak", filename, filename);
                if (system(tempFilename) == -1)
                {
                    printf("Error executing system command: %s\n", tempFilename);
                    exit(1);
                }
                printf("Done!\n");
            }
        }
    }

    return 0;
}


