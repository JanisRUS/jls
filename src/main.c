/// @file       main.c
/// @brief      Файл с точкой входа в программу
/// @author     Тузиков Г.А. janisrus35@gmail.com

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "fileInfo.h"
#include "jls.h"

int main(int argc, char *argv[])
{
    bool isOk = true;

    setlocale(LC_ALL, "");

    char    *filePtr    = 0;
    size_t   fileLength = 0;

    filePtr = malloc(2);
    if (!filePtr)
    {
        isOk = false;
        goto cleanup;
    }

    filePtr[fileLength++] = '.';
    filePtr[fileLength++] = '\0';

    if (argc > 2)
    {
        printf("Unsupported argument(s) found. Utility supports only 0 or 1 argument\n");
        isOk = false;
        goto cleanup;
    }
    if (argc > 1)
    {
        char  *lastArgumentPtr = 0;
        
        lastArgumentPtr = argv[argc - 1];
        
        free(filePtr);
        
        // Для \0
        filePtr = malloc(strlen(lastArgumentPtr) + 1);
        if (!filePtr)
        {
            isOk = false;
            goto cleanup;
        }

        strcpy(filePtr, lastArgumentPtr);
    }

    if (!fileInfoIsExists(filePtr, &isOk))
    {
        if (isOk)
        {
            printf("ls: cannot access '%s': No such file or directory\n", filePtr);
        }
        goto cleanup;
    }

    if (jls(filePtr) != 0)
    {
        isOk = false;
        goto cleanup;
    }

cleanup:
    if (filePtr)
    {
        free(filePtr);
        filePtr = 0;
    }

    fileInfoClearActiveFile();

    if (isOk)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
