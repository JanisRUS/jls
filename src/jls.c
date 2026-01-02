/// @file       jls.c
/// @brief      См. jls.h
/// @author     Тузиков Г.А.

#include "jls.h"
#include "fileInfo.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>

/*
    Прототипы внутренних функций
*/

/// @brief      Функция установки пути, где находятся файлы
/// @details    Данная функция выполняет запись pathPtr в bufferPtr размером bufferSize
/// @param[in]  pathPtr    Указатель на путь к файлам
/// @param[in]  bufferPtr  Указатель на буфер, куда будет записан pathPtr. Включая \0
/// @param[in]  bufferSize Размер bufferPtr
/// @return     Возвращает количество записаных в pathPtr байт
static size_t jlsPathSet(const char *pathPtr, char *bufferPtr, size_t bufferSize);

/// @brief      Функция добавления имени файла к пути
/// @details    Данная функция выполняет запись filePtr в bufferPtr размером bufferSize в позицию bufferLength
/// @param[in]  filePtr      Указатель на файл
/// @param[in]  bufferPtr    Указатель на буфер, куда будет записан filePtr. Включая \0
/// @param[in]  bufferLength Позиция, с которой нужно записать filePtr
/// @param[in]  bufferSize   Размер bufferPtr
/// @return     Возвращает длину bufferPtr. Включая \0
static size_t jlsPathAppend(const char *filePtr, char *bufferPtr, size_t bufferLength, size_t bufferSize);

/// @brief      Функция сортировки по возрастанию
/// @param[in]  a Первый элемент
/// @param[in]  b Второй элемент
/// @return     Возвращает результат выполнения strcmp(a, b)
static int jlsFilesListCompareAscend(const void *a, const void *b);

/// @brief      Функция сортировки по убыванию
/// @param[in]  a Первый элемент
/// @param[in]  b Второй элемент
/// @return     Возвращает результат выполнения jlsFilesListCompareAscend(b, a)
static int jlsFilesListCompareDescend(const void *a, const void *b);

/*
    Функции
*/

int jls(const char *filePtr)
{
    static char    fileInfoString[JLS_FILE_INFO_MAX_LENGTH] = {0};
    static size_t  fileInfoStringLength = 0;

    bool isOk = true;
    
    fileInfoStruct     fileInfo  = {0};
    jlsFilesListStruct filesList = {0};

    if (!filePtr)
    {
        isOk = false;
        goto cleanup;
    }
    
    if (!fileInfoGet(filePtr, &fileInfo))
    {
        isOk = false;
        goto cleanup;
    }

    if (fileInfo.type != fileInfoTypeDirectory)
    {
        fileInfoStringLength = fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH);
        if (!fileInfoStringLength)
        {
            isOk = false;
        }
        else
        {
            jlsPrintFileInfo(&fileInfoString[0], 0);
        }
        goto cleanup;
    }
    
    filesList = jlsGetFilesList(filePtr);
    if (!filesList.count)
    {
        isOk = false;
        goto cleanup;
    }
    
    jlsSortFilesList(&filesList, jlsSortAscend);
    
    jlsAlignmentStruct alignment = {0};

    alignment = jlsCalculateAlignment(filePtr, &filesList);

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(filePtr, &fullPath[0], PATH_MAX);
    if (!pathLength)
    {
        isOk = false;
        goto cleanup;
    }

    for (int i = 0; i < filesList.count; ++i)
    {
        char *fileName = 0;

        fileName = filesList.list[i];

        if (fileInfo.fileNamePtr)
        {
            free(fileInfo.fileNamePtr);
            fileInfo.fileNamePtr = 0;
        }
        if (fileInfo.targetPtr)
        {
            free(fileInfo.targetPtr);
            fileInfo.targetPtr = 0;
        }
    
        if (!jlsPathAppend(fileName, &fullPath[0], pathLength, PATH_MAX))
        {
            isOk = false;
            goto cleanup;
        }

        if (!fileInfoGet(&fullPath[0], &fileInfo))
        {
            isOk = false;
            goto cleanup;
        }

        fileInfoStringLength = fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH);
        if (!fileInfoStringLength)
        {
            isOk = false;
            goto cleanup;
        }
        jlsPrintFileInfo(&fileInfoString[0], &alignment);
    }

cleanup:
    if (fileInfo.fileNamePtr)
    {
        free(fileInfo.fileNamePtr);
        fileInfo.fileNamePtr = 0;
    }
    if (fileInfo.targetPtr)
    {
        free(fileInfo.targetPtr);
        fileInfo.targetPtr = 0;
    }

    if (filesList.list)
    {
        for (int i = 0; i < filesList.count; ++i)
        {
            if (filesList.list[i])
            {
                free(filesList.list[i]);
                filesList.list[i] = 0;
            }
        }
    }
    if (filesList.list)
    {
        free(filesList.list);
        filesList.list = 0;
    }

    if (isOk)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void jlsPrintFileInfo(const char *fileInfoStringPtr, const jlsAlignmentStruct *alignmentPtr)
{
    static const char delimer[] = {FILE_INFO_TO_STRING_DELIMER, '\0'};
    
    char buffer[JLS_FILE_INFO_MAX_LENGTH] = {0};

    if (!fileInfoStringPtr)
    {
        return;
    }

    jlsAlignmentStruct alignment = {0};

    if (!alignmentPtr)
    {
        alignmentPtr = &alignment;
    }

    memcpy(&buffer[0], &fileInfoStringPtr[0], strlen(fileInfoStringPtr) < JLS_FILE_INFO_MAX_LENGTH - 1 ? 
                                              strlen(fileInfoStringPtr) : JLS_FILE_INFO_MAX_LENGTH - 1);

    char *fileInfoStringTypePtr       = 0;
    char *fileInfoStringAccessPtr     = 0;
    char *fileInfoStringLinksCountPtr = 0;
    char *fileInfoStringOwnerPtr      = 0;
    char *fileInfoStringGroupPtr      = 0;
    char *fileInfoStringSizePtr       = 0;
    char *fileInfoStringTimeEditPtr   = 0;
    char *fileInfoStringFilePtr       = 0;
    char *fileInfoStringTargetPtr     = 0;

    fileInfoStringTypePtr       = strtok(&buffer[0], delimer);
    fileInfoStringAccessPtr     = strtok(NULL,       delimer);
    fileInfoStringLinksCountPtr = strtok(NULL,       delimer);
    fileInfoStringOwnerPtr      = strtok(NULL,       delimer);
    fileInfoStringGroupPtr      = strtok(NULL,       delimer);
    fileInfoStringSizePtr       = strtok(NULL,       delimer);
    fileInfoStringTimeEditPtr   = strtok(NULL,       delimer);
    fileInfoStringFilePtr       = strtok(NULL,       delimer);
    fileInfoStringTargetPtr     = strtok(NULL,       delimer);

    printf("%s%s %*s %-*s %-*s %*s %s %s", fileInfoStringTypePtr,
                                           fileInfoStringAccessPtr,
            (int)alignmentPtr->linksCount, fileInfoStringLinksCountPtr,
            (int)alignmentPtr->owner,      fileInfoStringOwnerPtr,
            (int)alignmentPtr->group,      fileInfoStringGroupPtr,
            (int)alignmentPtr->size,       fileInfoStringSizePtr,
                                           fileInfoStringTimeEditPtr,
                                           fileInfoStringFilePtr);

    if (fileInfoStringTargetPtr)
    {
        printf(" -> %s", fileInfoStringTargetPtr);
    }

    printf("\n");
}

void jlsSortFilesList(const jlsFilesListStruct *filesListPtr, jlsSortEnum sort)
{
    if (!filesListPtr || !filesListPtr->list || filesListPtr->count < 2)
    {
        return;
    }

    switch (sort)
    {
        case jlsSortNone:
        default:
            break;

        case jlsSortAscend:
        {
            qsort(filesListPtr->list, filesListPtr->count, sizeof(char *), jlsFilesListCompareAscend);
            break;
        }

        case jlsSortDescend:
        {
            qsort(filesListPtr->list, filesListPtr->count, sizeof(char *), jlsFilesListCompareDescend);
            break;
        }
    }
}

jlsFilesListStruct jlsGetFilesList(const char *dirPtr)
{
    jlsFilesListStruct answer          = {0};
    size_t             filesCount      = 0;
    DIR               *directory       = 0;
    struct dirent     *directoryEntity = {0};

    bool isOk = true;

    filesCount = jlsCountFilesInDirectory(dirPtr);
    if (!filesCount)
    {
        return answer;
    }

    answer.list = calloc(filesCount, sizeof(char *));
    if (!answer.list)
    {
        isOk = false;
        goto cleanup;
    }

    directory = opendir(dirPtr);
    if (!directory)
    {
        isOk = false;
        goto cleanup;
    }

    while ((directoryEntity = readdir(directory)) != NULL) 
    {
        if (strcmp(directoryEntity->d_name, ".")  == 0 ||
            strcmp(directoryEntity->d_name, "..") == 0)
        {
            continue;
        }

        answer.list[answer.count] = malloc(strlen(directoryEntity->d_name) + 1);
        if (!answer.list[answer.count])
        {
            isOk = false;
            goto cleanup;
        }

        strcpy(answer.list[answer.count], directoryEntity->d_name);

        ++answer.count;
        if (answer.count == filesCount)
        {
            // Если количество файлов в директории изменилось за время выполнения функции, обрабатываем не все файлы
            break;
        }
    }

cleanup:
    if (directory)
    {
        closedir(directory);
    }

    if (!isOk)
    {
        if (answer.list)
        {
            for (int i = 0; i < answer.count; ++i)
            {
                if (answer.list[i])
                {
                    free(answer.list[i]);
                    answer.list[i] = 0;
                }
            }
        }
        if (answer.list)
        {
            free(answer.list);
            answer.list = 0;
        }
    }

    if (isOk)
    {
        return answer;
    }
    else
    {
        return (jlsFilesListStruct){0};
    }
}

size_t jlsCountFilesInDirectory(const char *dirPtr)
{
    size_t         answer          = 0;
    DIR           *directory       = 0;
    struct dirent *directoryEntity = {0};

    directory = opendir(dirPtr);
    if (!directory)
    {
        return answer;
    }

    while ((directoryEntity = readdir(directory)) != NULL) 
    {
        if (strcmp(directoryEntity->d_name, ".")  == 0 ||
            strcmp(directoryEntity->d_name, "..") == 0)
        {
            continue;
        }
        ++answer;        
    }

    if (directory)
    {
        closedir(directory);
    }

    return answer;
}

jlsAlignmentStruct jlsCalculateAlignment(const char *pathPtr, const jlsFilesListStruct *filesList)
{
    jlsAlignmentStruct answer          = {0};
    fileInfoStruct     fileInfo        = {0};

    bool isOk = true;

    if (!pathPtr || !filesList)
    {
        isOk = false;
        goto cleanup;
    }

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(pathPtr, &fullPath[0], PATH_MAX);
    if (!pathLength)
    {
        isOk = false;
        goto cleanup;
    }

    for (int i = 0; i < filesList->count; ++i)
    {
        static char fileInfoString[JLS_FILE_INFO_MAX_LENGTH] = {0};

        if (fileInfo.fileNamePtr)
        {
            free(fileInfo.fileNamePtr);
            fileInfo.fileNamePtr = 0;
        }
        if (fileInfo.targetPtr)
        {
            free(fileInfo.targetPtr);
            fileInfo.targetPtr = 0;
        }
    
        if (!jlsPathAppend(filesList->list[i], &fullPath[0], pathLength, PATH_MAX))
        {
            isOk = false;
            goto cleanup;
        }

        if (!fileInfoGet(&fullPath[0], &fileInfo))
        {
            isOk = false;
            goto cleanup;
        }

        if (!fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH))
        {
            continue;
        }

        static const char delimer[] = {FILE_INFO_TO_STRING_DELIMER, '\0'};
        char             *field     = 0;

        // Пропускаем тип файла
        field = strtok(&fileInfoString[0], delimer);
        // Пропускаем права доступа файла
        field = strtok(NULL, delimer);

        field = strtok(NULL, delimer);
        if (answer.linksCount < strlen(field))
        {
            answer.linksCount = strlen(field);
        }

        field = strtok(NULL, delimer);
        if (answer.owner < strlen(field))
        {
            answer.owner = strlen(field);
        }

        field = strtok(NULL, delimer);
        if (answer.group < strlen(field))
        {
            answer.group = strlen(field);
        }

        field = strtok(NULL, delimer);
        if (answer.size < strlen(field))
        {
            answer.size = strlen(field);
        }
    }

cleanup:
    if (fileInfo.fileNamePtr)
    {
        free(fileInfo.fileNamePtr);
        fileInfo.fileNamePtr = 0;
    }
    if (fileInfo.targetPtr)
    {
        free(fileInfo.targetPtr);
        fileInfo.targetPtr = 0;
    }
    if (isOk)
    {
        return answer;
    }
    else
    {
        return (jlsAlignmentStruct){0};
    }
}

/*
    Внутренние функции
*/

static size_t jlsPathSet(const char *pathPtr, char *bufferPtr, size_t bufferSize)
{
    size_t answer = 0;

    if (!pathPtr || !bufferPtr || bufferSize < 2)
    {
        return 0;
    }

    memset(bufferPtr, 0, bufferSize);

    if (pathPtr[0] == '/')
    {
        answer = snprintf(bufferPtr, bufferSize, "%s", pathPtr);
    }
    else
    {
        answer = snprintf(bufferPtr, bufferSize, "%s/", pathPtr);
    }

    return answer;
}

static size_t jlsPathAppend(const char *filePtr, char *bufferPtr, size_t bufferLength, size_t bufferSize)
{
    size_t answer = 0;

    if (!filePtr || !bufferPtr || bufferSize < bufferLength)
    {
        return answer;
    }

    answer  = snprintf(&bufferPtr[bufferLength], bufferSize - bufferLength, "%s", filePtr);
    answer += bufferLength;
    bufferPtr[answer++] = '\0';

    return answer;
}

static int jlsFilesListCompareAscend(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

static int jlsFilesListCompareDescend(const void *a, const void *b)
{
    return jlsFilesListCompareAscend(b, a);
}
