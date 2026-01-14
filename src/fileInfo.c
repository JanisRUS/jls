/// @file       fileInfo.c
/// @brief      См. fileInfo.h
/// @author     Тузиков Г.А. janisrus35@gmail.com

#include "fileInfo.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <sys/sysmacros.h>

/*
    Константы
*/

/// @brief      Путь до активного файла
char *fileInfoPath = 0;

/// @brief      Результат вызова lstat активного файла
struct stat fileInfoStat = {0};

/*
    Функции
*/

bool fileInfoIsExists(const char *filePtr, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    struct stat fileInfo = {0};

    if (!filePtr)
    {
        *isOkPtr = false;
        return false;
    }

    if (lstat(filePtr, &fileInfo) && errno == ENOENT)
    {
        return false;
    }

    return true;
}

void fileInfoGet(const char *filePtr, fileInfoStruct *fileInfoPtr, bool isFollowLink, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    // Объявление переменных, используемых в cleanup
    char *filePtrCopy1    = 0;
    char *filePtrCopy2    = 0;
    char *linkTargetPtr   = 0;

    if (!filePtr || !fileInfoPtr || !fileInfoSetActiveFile(filePtr))
    {
        *isOkPtr = false;
        goto cleanup;
    }
    
    memset(fileInfoPtr, 0, sizeof(fileInfoStruct));

    filePtrCopy1 = strdup(filePtr);
    if (!filePtrCopy1) 
    {
        *isOkPtr = false;
        goto cleanup;
    }

    filePtrCopy2 = strdup(filePtr);
    if (!filePtrCopy2) 
    {
        *isOkPtr = false;
        goto cleanup;
    }

    char *fileNamePtr = 0;
    char *filePathPtr = 0;

    fileNamePtr = basename(filePtrCopy1);
    filePathPtr = dirname(filePtrCopy2);

    fileInfoPtr->fileNamePtr = strdup(fileNamePtr);
    if (!fileInfoPtr->fileNamePtr)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    fileInfoPtr->type = fileInfoGetType(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    fileInfoPtr->access = fileInfoGetAccess(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    fileInfoPtr->linksCount = fileInfoGetLinksCount(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    fileInfoPtr->ownerId = fileInfoGetOwnerId(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    fileInfoPtr->groupId = fileInfoGetGroupId(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }
    
    fileInfoPtr->size = fileInfoGetSize(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }
    
    fileInfoPtr->deviceNumber = fileInfoGetDeviceNumber(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    fileInfoPtr->timeEdit = fileInfoGetTimeEdit(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }
    
    fileInfoPtr->blocks = fileInfoGet512BytesBlocks(isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    if (fileInfoPtr->type == fileInfoTypeLink)
    {
        linkTargetPtr = malloc(FILE_INFO_TARGET_LENGTH_MAX);
        if (!linkTargetPtr)
        {
            *isOkPtr = false;
            goto cleanup;
        }

        fileInfoGetLinkTarget(linkTargetPtr, FILE_INFO_TARGET_LENGTH_MAX, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        if (linkTargetPtr[0] == '/')
        {
            fileInfoPtr->targetInfo.filePathPtr = strdup(linkTargetPtr);
            if (!fileInfoPtr->targetInfo.filePathPtr)
            {
                *isOkPtr = false;
                goto cleanup;
            }
        }
        else
        {
            size_t filePathLength = 0;

            // \0 и /
            filePathLength = strlen(filePathPtr) + strlen(linkTargetPtr) + 2;
    
            fileInfoPtr->targetInfo.filePathPtr = malloc(filePathLength);
            if (!fileInfoPtr->targetInfo.filePathPtr)
            {
                *isOkPtr = false;
                goto cleanup;
            }
            if (snprintf(fileInfoPtr->targetInfo.filePathPtr,
                         filePathLength,
                         "%s/%s", 
                         filePathPtr, 
                         linkTargetPtr) < 0)
            {
                *isOkPtr = false;
                goto cleanup;
            }
        }
        fileInfoPtr->targetInfo.fileNamePtr = &fileInfoPtr->targetInfo.filePathPtr[strlen(fileInfoPtr->targetInfo.filePathPtr) - strlen(linkTargetPtr)];

        fileInfoPtr->targetInfo.isTargetExists = fileInfoIsExists(fileInfoPtr->targetInfo.filePathPtr, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        if (fileInfoPtr->targetInfo.isTargetExists)
        {
            if (!fileInfoSetActiveFile(fileInfoPtr->targetInfo.filePathPtr))
            {
                *isOkPtr = false;
                goto cleanup;
            }

            fileInfoPtr->targetInfo.access = fileInfoGetAccess(isOkPtr);
            if (!*isOkPtr)
            {
                goto cleanup;
            }

            fileInfoPtr->targetInfo.type = fileInfoGetType(isOkPtr);
            if (!*isOkPtr)
            {
                goto cleanup;
            }

            char *filePathOrig = 0;
            char *fileNameOrig = 0;
            
            filePathOrig = fileInfoPtr->targetInfo.filePathPtr;
            fileNameOrig = fileInfoPtr->targetInfo.fileNamePtr;

            char *fileNameBufferPtr = 0;
            char *linkPathBufferPtr = 0;

            while (fileInfoPtr->targetInfo.type == fileInfoTypeLink && isFollowLink)
            {
                fileInfoStruct linkInfo = {0};

                fileInfoGet(fileInfoPtr->targetInfo.filePathPtr, &linkInfo, false, isOkPtr);
                if (!*isOkPtr)
                {
                    break;
                }
                fileInfoPtr->targetInfo = linkInfo.targetInfo;

                if (fileNameBufferPtr)
                {
                    free(fileNameBufferPtr);
                }
                fileNameBufferPtr = linkInfo.fileNamePtr;

                if (linkPathBufferPtr)
                {
                    free(linkPathBufferPtr);
                }
                linkPathBufferPtr = linkInfo.targetInfo.filePathPtr;
            }

            if (fileNameBufferPtr)
            {
                free(fileNameBufferPtr);
                fileNameBufferPtr = 0;
            }
            if (linkPathBufferPtr)
            {
                free(linkPathBufferPtr);
                linkPathBufferPtr = 0;
            }

            fileInfoPtr->targetInfo.filePathPtr = filePathOrig;
            fileInfoPtr->targetInfo.fileNamePtr = fileNameOrig;
        }
    }

cleanup:

    if (filePtrCopy1)
    {
        free(filePtrCopy1);
        filePtrCopy1 = 0;
    }

    if (filePtrCopy2)
    {
        free(filePtrCopy2);
        filePtrCopy2 = 0;
    }

    if (linkTargetPtr)
    {
        free(linkTargetPtr);
        linkTargetPtr = 0;
    }

    if (!*isOkPtr)
    {
        if (fileInfoPtr->fileNamePtr)
        {
            free(fileInfoPtr->fileNamePtr);
            fileInfoPtr->fileNamePtr = 0;
        }
        if (fileInfoPtr->targetInfo.filePathPtr)
        {
            free(fileInfoPtr->targetInfo.filePathPtr);
            fileInfoPtr->targetInfo.filePathPtr = 0;
        }
    }
}

bool fileInfoSetActiveFile(const char *filePtr)
{
    struct stat fileInfo = {0};

    if (!filePtr || lstat(filePtr, &fileInfo))
    {
        return false;
    }

    if (fileInfoPath)
    {
        free(fileInfoPath);
        fileInfoPath = 0;
    }
    
    fileInfoPath = malloc(strlen(filePtr) + 1);
    if (!fileInfoPath)
    {
        return false;
    }
    strcpy(fileInfoPath, filePtr);

    fileInfoStat = fileInfo;

    return true;
}

void fileInfoClearActiveFile(void)
{
    if (fileInfoPath)
    {
        free(fileInfoPath);
        fileInfoPath = 0;
    }
}

fileInfoTypesEnum fileInfoGetType(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return fileInfoTypeUnknown;
    }

    if (S_ISDIR(fileInfoStat.st_mode))
    {
        return fileInfoTypeDirectory;
    }
    if (S_ISCHR(fileInfoStat.st_mode))
    {
        return fileInfoTypeChar;
    }
    if (S_ISBLK(fileInfoStat.st_mode))
    {
        return fileInfoTypeBlock;
    }
    if (S_ISREG(fileInfoStat.st_mode))
    {
        return fileInfoTypeFile;
    }
    if (S_ISFIFO(fileInfoStat.st_mode))
    {
        return fileInfoTypeFIFO;
    }
    if (S_ISLNK(fileInfoStat.st_mode))
    {
        return fileInfoTypeLink;
    }
    if (S_ISSOCK(fileInfoStat.st_mode))
    {
        return fileInfoTypeSock;
    }

    *isOkPtr = false;
    return fileInfoTypeUnknown;
}

fileInfoAccessStruct fileInfoGetAccess(bool *isOkPtr)
{
    bool isOk = true;
    
    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    fileInfoAccessStruct answer = {0};
    
    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return answer;
    }

    answer.owner.value = ((bool)(fileInfoStat.st_mode & S_IRUSR) << 3) +
                         ((bool)(fileInfoStat.st_mode & S_IWUSR) << 2) +
                         ((bool)(fileInfoStat.st_mode & S_IXUSR) << 1) +
                         ((bool)(fileInfoStat.st_mode & S_ISUID) << 0);

    answer.group.value = ((bool)(fileInfoStat.st_mode & S_IRGRP) << 3) +
                         ((bool)(fileInfoStat.st_mode & S_IWGRP) << 2) +
                         ((bool)(fileInfoStat.st_mode & S_IXGRP) << 1) +
                         ((bool)(fileInfoStat.st_mode & S_ISGID) << 0);

    answer.other.value = ((bool)(fileInfoStat.st_mode & S_IROTH) << 3) +
                         ((bool)(fileInfoStat.st_mode & S_IWOTH) << 2) +
                         ((bool)(fileInfoStat.st_mode & S_IXOTH) << 1) +
                         ((bool)(fileInfoStat.st_mode & S_ISVTX) << 0);

    return answer;
}

uint32_t fileInfoGetLinksCount(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_nlink;
}


uint32_t fileInfoGetOwnerId(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_uid;
}

uint32_t fileInfoGetGroupId(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_gid;
}

uint32_t fileInfoGetSize(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_size;
}

__uint64_t fileInfoGetDeviceNumber(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_rdev;
}

time_t fileInfoGetTimeEdit(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_mtime;
}

size_t fileInfoGetLinkTarget(char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    ssize_t answer = 0;

    if (!fileInfoPath || !stringPtr || stringLength <= 1)
    {
        *isOkPtr = false;
        return 0;
    }

    memset(stringPtr, 0, stringLength);

    // Длина -1 потому что readlink не создает \0 в конце
    answer = readlink(fileInfoPath, stringPtr, stringLength - 1);
    if (answer <= 0)
    {
        *isOkPtr = false;
        return 0;
    }
    stringPtr[answer++] = '\0';

    return (size_t)answer;
}

uint32_t fileInfoGet512BytesBlocks(bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPath)
    {
        *isOkPtr = false;
        return 0;
    }

    return fileInfoStat.st_blocks;
}

/*
    Внешние получения строкового представления информации о файле
*/

size_t fileInfoToString(const fileInfoStruct *fileInfoPtr, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    size_t answer = 0;
    size_t length = 0;
    
    if (!fileInfoPtr || !stringPtr || stringLength < 1)
    {
        *isOkPtr = false;
        return 0;
    }
    
    --stringLength;

    #define APPEND(FUNC)  if (stringLength - answer < 0)                    \
                          {                                                 \
                              *isOkPtr = false;                             \
                              return 0;                                     \
                          }                                                 \
                          length = FUNC;                                    \
                          if (!*isOkPtr)                                    \
                          {                                                 \
                              return 0;                                     \
                          }                                                 \
                          answer += length;                                 \
                          stringPtr[answer++] = FILE_INFO_TO_STRING_DELIMER;

    APPEND(fileInfoToStringType(fileInfoPtr->type, &stringPtr[answer], stringLength - answer, isOkPtr));

    APPEND(fileInfoToStringAccess(&fileInfoPtr->access, fileInfoPtr->type, &stringPtr[answer], stringLength - answer, isOkPtr));

    APPEND(fileInfoToStringLinksCount(fileInfoPtr->linksCount, &stringPtr[answer], stringLength - answer, isOkPtr));

    APPEND(fileInfoToStringOwnerId(fileInfoPtr->ownerId, &stringPtr[answer], stringLength - answer, isOkPtr));

    APPEND(fileInfoToStringGroupId(fileInfoPtr->groupId, &stringPtr[answer], stringLength - answer, isOkPtr));
    
    if (fileInfoPtr->type != fileInfoTypeBlock && fileInfoPtr->type != fileInfoTypeChar)
    {
        APPEND(fileInfoToStringSize(fileInfoPtr->size, &stringPtr[answer], stringLength - answer, isOkPtr));
    }
    else
    {
        APPEND(fileInfoToStringDeviceNumber(fileInfoPtr->deviceNumber, &stringPtr[answer], stringLength - answer, isOkPtr));
    }

    APPEND(fileInfoToStringTimeEdit(fileInfoPtr->timeEdit, &stringPtr[answer], stringLength - answer, isOkPtr));

    APPEND(snprintf(&stringPtr[answer], stringLength - answer, "%s", fileInfoPtr->fileNamePtr));

    if (fileInfoPtr->type == fileInfoTypeLink)
    {
        APPEND(snprintf(&stringPtr[answer], stringLength - answer, "%s", fileInfoPtr->targetInfo.fileNamePtr));
    }

    // Перезаписываем последний разделитель на \0
    stringPtr[--answer] = '\0';

    return answer;
}

size_t fileInfoToStringType(fileInfoTypesEnum type, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    static const char typeToStringList[fileInfoTypeCount] =
    {
        '?',
        'd',
        'c',
        'b',
        '-',
        'p',
        'l',
        's'
    };

    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    size_t answer = 0;

    if (type < fileInfoTypeUnknown || type > fileInfoTypeCount || !stringPtr || stringLength < 2)
    {
        *isOkPtr = false;
        return 0;
    }

    stringPtr[answer++] = typeToStringList[type];
    stringPtr[answer]   = '\0';

    return answer;
}

size_t fileInfoToStringAccess(const fileInfoAccessStruct *accessPtr, fileInfoTypesEnum type, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    size_t answer = 0;
    size_t length = 0;

    if (!accessPtr || !stringPtr || stringLength < 10)
    {
        *isOkPtr = false;
        return 0;
    }

    stringPtr[answer++] = accessPtr->owner.bits.read  ? 'r' : '-';
    stringPtr[answer++] = accessPtr->owner.bits.write ? 'w' : '-';
    if (accessPtr->owner.bits.special)
    {
        stringPtr[answer++] = accessPtr->owner.bits.execute ? 's' : 'S';
    }
    else
    {
        stringPtr[answer++] = accessPtr->owner.bits.execute ? 'x' : '-';
    }

    stringPtr[answer++] = accessPtr->group.bits.read  ? 'r' : '-';
    stringPtr[answer++] = accessPtr->group.bits.write ? 'w' : '-';
    if (accessPtr->group.bits.special)
    {
        stringPtr[answer++] = accessPtr->group.bits.execute ? 's' : 'S';
    }
    else
    {
        stringPtr[answer++] = accessPtr->group.bits.execute ? 'x' : '-';
    }

    stringPtr[answer++] = accessPtr->other.bits.read  ? 'r' : '-';
    stringPtr[answer++] = accessPtr->other.bits.write ? 'w' : '-';
    if (accessPtr->other.bits.special)
    {
        stringPtr[answer++] = accessPtr->other.bits.execute ? 't' : 'T';
    }
    else
    {
        stringPtr[answer++] = accessPtr->other.bits.execute ? 'x' : '-';
    }

    stringPtr[answer] = '\0';

    return answer;
}

size_t fileInfoToStringLinksCount(uint32_t linksCount, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    int answer = 0;
    
    if (!stringPtr)
    {
        *isOkPtr = false;
        return 0;
    }
    
    answer = snprintf(stringPtr, stringLength, "%" PRIu32, linksCount);
    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }

    return (size_t)answer;
}


size_t fileInfoToStringOwnerId(uint32_t ownerId, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    int answer = 0;

    if (!stringPtr)
    {
        *isOkPtr = false;
        return 0;
    }

    struct passwd *user = 0;

    user = getpwuid(ownerId);
    if (user)
    {
        answer = snprintf(stringPtr, stringLength, "%s", user->pw_name);
    }
    else
    {
        answer = snprintf(stringPtr, stringLength, "%" PRIu32, ownerId);
    }

    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }

    return (size_t)answer;
}

size_t fileInfoToStringGroupId(uint32_t groupId, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    int answer = 0;
    
    if (!stringPtr)
    {
        *isOkPtr = false;
        return 0;
    }

    struct group *group = 0;

    group = getgrgid(groupId);
    if (group)
    {
        answer = snprintf(stringPtr, stringLength, "%s", group->gr_name);
    }
    else
    {
        answer = snprintf(stringPtr, stringLength, "%" PRIu32, groupId);
    }

    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }

    return (size_t)answer;
}

size_t fileInfoToStringSize(uint32_t size, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    int answer = 0;
    
    if (!stringPtr)
    {
        *isOkPtr = false;
        return 0;
    }
    
    answer = snprintf(stringPtr, stringLength, "%" PRIu32, size);
    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }

    return (size_t)answer;
}

size_t fileInfoToStringDeviceNumber(__uint64_t deviceNumber, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    int answer = 0;
    
    if (!stringPtr)
    {
        *isOkPtr = false;
        return 0;
    }
    
    answer = snprintf(stringPtr, stringLength, "%d, %d", major(deviceNumber), minor(deviceNumber));
    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }

    return (size_t)answer;
}

size_t fileInfoToStringTimeEdit(time_t timeEdit, char *stringPtr, size_t stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    int answer = 0;
    
    if (!stringPtr)
    {
        *isOkPtr = false;
        return 0;
    }

    time_t currentTime = time(NULL);
 
    // Максимальная допустимая разница между текущим временем и временем модификации файла
    static const time_t maxTimeDifference = (365.2425 * 24 * 60 * 60) / 2;

    // Если разница превышает максимально допустимое значение или файл модифицирован в будущем
    if (currentTime - timeEdit > maxTimeDifference ||
        currentTime - timeEdit < 0)
    {
        answer = strftime(stringPtr, stringLength, "%b %e  %Y", localtime(&timeEdit));
    }
    else
    {
        answer = strftime(stringPtr, stringLength, "%b %e %H:%M", localtime(&timeEdit));
    }

    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }

    // answer может быть равен 0, если время не поместилось в stringLength, хотя stringPtr будет заполнен данными
    if (!answer)
    {
        answer = stringLength;
        stringPtr[answer - 1] = '\0';
    }

    return answer;
}
