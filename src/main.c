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
    
    bool testMode = false;

    jlsIsSafeModeEnabled = false;

    if (isatty(STDOUT_FILENO))
    {
        jlsIsSafeModeEnabled = true;
    }

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

    if (filesIndex == -1)
    {
        if (jls(".") != 0)
        {
            isOk = false;
        }
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
        }
    }

    for (int i = filesIndex; i < argc; ++i)
    {
        char *filePtr = argv[i];

        if (!fileInfoSetActiveFile(filePtr))
        {
            continue;
        }

        if (argc - filesIndex > 1 && fileInfoGetType(0) == fileInfoTypeDirectory)
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
