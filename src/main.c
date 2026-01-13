/// @file       main.c
/// @brief      Файл с точкой входа в программу
/// @author     Тузиков Г.А. janisrus35@gmail.com

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include "fileInfo.h"
#include "jls.h"

int main(int argc, char *argv[])
{
    bool isOk = true;

    setlocale(LC_ALL, "");
    
    char **filesList = 0;
    int   filesCount = 0;

    char **dirsList  = 0;
    int   dirsCount  = 0;

    /*
        Параметры ПО
    */

    bool testMode = false;

    jlsIsSafeModeEnabled = false;

    if (isatty(STDOUT_FILENO))
    {
        jlsIsSafeModeEnabled = true;
    }

    /*
        Парсинг аргументов
    */
    int filesIndex = -1;

    for (int i = 1; i < argc; ++i)
    {
        char *arg = argv[i];

        if (arg[0] == '-')
        {
            if (filesIndex != -1)
            {
                fprintf(stderr, "jls: Provide options before files");
                isOk = false;
                goto cleanup;
            }
            
            if (strcmp(arg, "-s")          == 0 ||
                strcmp(arg, "--safe-mode") == 0)
            {
                jlsIsSafeModeEnabled = true;
                continue;
            }
            
            if (strcmp(arg, "-test")       == 0 ||
                strcmp(arg, "--test-mode") == 0)
            {
                testMode = true;
                continue;
            }

            fprintf(stderr, "jls: Unknown option \"%s\"\n", arg);
            isOk = false;
            goto cleanup;
        }
        else
        {
            if (filesIndex == -1)
            {
                filesIndex = i;
            }
        }
    }

    /*
        Запуск без файлов в аргументах
    */

    if (filesIndex == -1)
    {
        if (jls(".") != 0)
        {
            isOk = false;
        }
        goto cleanup;
    }

    /*
        Запуск с файлами в аргументах
    */

    filesList = calloc(argc - filesIndex, sizeof(char *));
    if (!filesList)
    {
        isOk = false;
        goto cleanup;
    }

    dirsList = calloc(argc - filesIndex, sizeof(char *));
    if (!dirsList)
    {
        isOk = false;
        goto cleanup;
    }

    for (int i = filesIndex; i < argc; ++i)
    {
        char *filePtr = argv[i];

        if (!fileInfoIsExists(filePtr, &isOk))
        {
            if (isOk)
            {
                char nameTest[]  = "ls";
                char nameUsual[] = "jls";

                fprintf(stderr, "%s: cannot access '%s': No such file or directory\n", testMode ? nameTest : nameUsual, filePtr);
            }
            continue;
        }
        if (!fileInfoSetActiveFile(filePtr))
        {
            continue;
        }

        if (fileInfoGetType(0) == fileInfoTypeDirectory)
        {
            dirsList[dirsCount++] = filePtr;
        }
        else
        {
            filesList[filesCount++] = filePtr;
        }
    }

    for (int i = 0; i < filesCount; ++i)
    {
        if (jls(filesList[i]) != 0)
        {
            isOk = false;
            goto cleanup;
        }
    }

    for (int i = 0; i < dirsCount; ++i)
    {
        if (filesCount > 1)
        {
            printf("\n%s:\n", dirsList[i]);
        }

        if (jls(dirsList[i]) != 0)
        {
            isOk = false;
            goto cleanup;
        }
    }

cleanup:
    if (filesList)
    {
        free(filesList);
        filesList = 0;
    }

    if (dirsList)
    {
        free(dirsList);
        dirsList = 0;
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
