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

    if (argc == 1)
    {
        if (jls(".") != 0)
        {
            isOk = false;
            goto cleanup;
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        char *filePtr = argv[i];

        if (!fileInfoIsExists(filePtr, &isOk))
        {
            if (isOk)
            {
                fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", filePtr);
            }
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        char *filePtr = argv[i];

        if (!fileInfoSetActiveFile(filePtr))
        {
            continue;
        }

        if (argc > 2 && fileInfoGetType(0) == fileInfoTypeDirectory)
        {
            printf("\n%s:\n", filePtr);
        }

        if (jls(argv[i]) != 0)
        {
            isOk = false;
        }
    }

cleanup:
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
