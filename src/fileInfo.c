/// @file       fileInfo.c
/// @brief      См. fileInfo.h
/// @author     Тузиков Г.А.

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

bool fileInfoIsExists(const char *filePtr)
{
    struct stat fileInfo = {0};

    if (!filePtr || (lstat(filePtr, &fileInfo) && errno == ENOENT))
    {
        return false;
    }

    return true;
}

bool fileInfoGet(const char *filePtr, fileInfoStruct *fileInfoPtr)
{
    bool isOk = true;

    if (!filePtr || !fileInfoPtr || !fileInfoSetActiveFile(filePtr))
    {
        isOk = false;
        goto cleanup;
    }
    
    memset(fileInfoPtr, 0, sizeof(fileInfoStruct));

    char *filePtrCopy = 0;

    filePtrCopy = strdup(filePtr);
    if (!filePtrCopy) 
    {
        isOk = false;
        goto cleanup;
    }

    char *fileNamePtr = 0;

    fileNamePtr = basename(filePtrCopy);
    fileInfoPtr->fileNamePtr = malloc(strlen(fileNamePtr) + 1);
    if (!fileInfoPtr->fileNamePtr)
    {
        isOk = false;
        goto cleanup;
    }

    strcpy(fileInfoPtr->fileNamePtr, fileNamePtr);

    fileInfoPtr->fileNameLength = strlen(fileInfoPtr->fileNamePtr) + 1;

    fileInfoPtr->targetPtr = malloc(FILE_INFO_TARGET_LENGTH_MAX);
    if (!fileInfoPtr->targetPtr)
    {
        isOk = false;
        goto cleanup;
    }

    fileInfoPtr->type       = fileInfoGetType();
    fileInfoPtr->access     = fileInfoGetAccess();
    fileInfoPtr->linksCount = fileInfoGetLinksCount();
    fileInfoPtr->ownerId    = fileInfoGetOwnerId();
    fileInfoPtr->groupId    = fileInfoGetGroupId();
    fileInfoPtr->size       = fileInfoGetSize();
    fileInfoPtr->timeEdit   = fileInfoGetTimeEdit();
    fileInfoPtr->blocks     = fileInfoGetBlocks();

    if (fileInfoPtr->type == fileInfoTypeLink)
    {
        fileInfoPtr->targetLength   = fileInfoGetLinkTarget(fileInfoPtr->targetPtr, FILE_INFO_TARGET_LENGTH_MAX);
        fileInfoPtr->isTargetExists = fileInfoIsExists(fileInfoPtr->targetPtr);
    }

cleanup:

    if (filePtrCopy)
    {
        free(filePtrCopy);
        filePtrCopy = 0;
    }

    if (!isOk)
    {
        if (fileInfoPtr->fileNamePtr)
        {
            free(fileInfoPtr->fileNamePtr);
            fileInfoPtr->fileNamePtr = 0;
        }
        if (fileInfoPtr->targetPtr)
        {
            free(fileInfoPtr->targetPtr);
            fileInfoPtr->targetPtr = 0;
        }
    }

    if (isOk)
    {
        return true;
    }
    else
    {
        return false;
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

void fileInfoClearActiveFile()
{
    if (fileInfoPath)
    {
        free(fileInfoPath);
        fileInfoPath = 0;
    }
}

fileInfoTypesEnum fileInfoGetType()
{
    if (!fileInfoPath)
    {
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

    return fileInfoTypeUnknown;
}

fileInfoAccessStruct fileInfoGetAccess()
{
    fileInfoAccessStruct answer = {0};
    
    if (!fileInfoPath)
    {
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

uint32_t fileInfoGetLinksCount()
{
    if (!fileInfoPath)
    {
        return 0;
    }

    return fileInfoStat.st_nlink;
}


uint32_t fileInfoGetOwnerId()
{
    if (!fileInfoPath)
    {
        return 0;
    }

    return fileInfoStat.st_uid;
}

uint32_t fileInfoGetGroupId()
{
    if (!fileInfoPath)
    {
        return 0;
    }

    return fileInfoStat.st_gid;
}

uint32_t fileInfoGetSize()
{
    if (!fileInfoPath)
    {
        return 0;
    }

    return fileInfoStat.st_size;
}

time_t fileInfoGetTimeEdit()
{
    if (!fileInfoPath)
    {
        return 0;
    }

    return fileInfoStat.st_mtime;
}

size_t fileInfoGetLinkTarget(char *stringPtr, size_t stringLength)
{
    ssize_t answer = 0;

    if (!fileInfoPath || !stringPtr || stringLength <= 1)
    {
        return 0;
    }

    memset(stringPtr, 0, stringLength);

    // Длина -1 потому что readlink не создает \0 в конце
    answer = readlink(fileInfoPath, stringPtr, stringLength - 1);
    if (answer <= 0)
    {
        return 0;
    }
    stringPtr[answer] = '\0';
    ++answer;

    return (size_t)answer;
}

uint32_t fileInfoGetBlocks(void)
{
    if (!fileInfoPath)
    {
        return 0;
    }

    return fileInfoStat.st_blocks;
}

size_t fileInfoToString(const fileInfoStruct *fileInfoPtr, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;
    size_t length = 0;
    
    if (!fileInfoPtr || !stringPtr || stringLength < 1)
    {
        return 0;
    }
    
    --stringLength;

    length = fileInfoToStringType(fileInfoPtr->type, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = fileInfoToStringAccess(&fileInfoPtr->access, fileInfoPtr->type, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = fileInfoToStringLinksCount(fileInfoPtr->linksCount, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = fileInfoToStringOwnerId(fileInfoPtr->ownerId, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = fileInfoToStringGroupId(fileInfoPtr->groupId, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = fileInfoToStringSize(fileInfoPtr->size, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = fileInfoToStringTimeEdit(fileInfoPtr->timeEdit, &stringPtr[answer], stringLength - answer);
    if (!length)
    {
        return 0;
    }
    answer += length;
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    length = snprintf(&stringPtr[answer], stringLength - answer, "%s", fileInfoPtr->fileNamePtr);
    answer += length;
    stringPtr[++answer]   = '\0';
    stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;

    if (fileInfoPtr->type == fileInfoTypeLink)
    {
        length = snprintf(&stringPtr[answer], stringLength - answer, "%s", fileInfoPtr->targetPtr);
        answer += length;
        stringPtr[++answer]   = '\0';
        stringPtr[answer - 1] = FILE_INFO_TO_STRING_DELIMER;
    }

    // Перезаписываем последний \t на \0
    stringPtr[answer - 1] = '\0';

    return answer;
}

size_t fileInfoToStringType(fileInfoTypesEnum type, char *stringPtr, size_t stringLength)
{
    static const char typeToStringList[fileInfoTypeCount] =
    {
        ' ',
        'd',
        'c',
        'b',
        '-',
        'p',
        'l',
        's'
    };

    size_t answer = 0;

    if (type < fileInfoTypeUnknown || type > fileInfoTypeCount || !stringPtr || stringLength < 2)
    {
        return 0;
    }

    stringPtr[answer++] = typeToStringList[type];
    stringPtr[answer++] = '\0';

    return answer;
}

size_t fileInfoToStringAccess(const fileInfoAccessStruct *accessPtr, fileInfoTypesEnum type, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;
    size_t length = 0;

    if (!accessPtr || !stringPtr || stringLength < 10)
    {
        return 0;
    }

    stringPtr[answer++] = accessPtr->owner.bits.read  ? 'r' : '-';
    stringPtr[answer++] = accessPtr->owner.bits.write ? 'w' : '-';
    if (type == fileInfoTypeFile && accessPtr->owner.bits.special)
    {
        stringPtr[answer++] = accessPtr->owner.bits.execute ? 's' : 'S';
    }
    else
    {
        stringPtr[answer++] = accessPtr->owner.bits.execute ? 'x' : '-';
    }

    stringPtr[answer++] = accessPtr->group.bits.read  ? 'r' : '-';
    stringPtr[answer++] = accessPtr->group.bits.write ? 'w' : '-';
    if (type == fileInfoTypeFile && accessPtr->group.bits.special)
    {
        stringPtr[answer++] = accessPtr->group.bits.execute ? 's' : 'S';
    }
    else
    {
        stringPtr[answer++] = accessPtr->group.bits.execute ? 'x' : '-';
    }

    stringPtr[answer++] = accessPtr->other.bits.read  ? 'r' : '-';
    stringPtr[answer++] = accessPtr->other.bits.write ? 'w' : '-';
    if (type == fileInfoTypeDirectory && accessPtr->other.bits.special)
    {
        stringPtr[answer++] = accessPtr->other.bits.execute ? 't' : 'T';
    }
    else
    {
        stringPtr[answer++] = accessPtr->other.bits.execute ? 'x' : '-';
    }

    stringPtr[answer++] = '\0';

    return answer;
}

size_t fileInfoToStringLinksCount(uint32_t linksCount, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;
    
    if (!stringPtr || stringLength < 1)
    {
        return 0;
    }
    
    answer = snprintf(stringPtr, stringLength, "%u", linksCount);
    stringPtr[answer++] = '\0';

    return answer;
}


size_t fileInfoToStringOwnerId(uint32_t ownerId, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;

    if (!stringPtr || stringLength < 1)
    {
        return 0;
    }

    --stringLength;

    struct passwd *user = 0;

    user = getpwuid(ownerId);
    if (user)
    {
        answer = stringLength > strlen(user->pw_name) ? strlen(user->pw_name) : stringLength;
        memcpy(stringPtr, user->pw_name, answer);
    }
    else
    {
        answer = snprintf(stringPtr, stringLength, "%u", ownerId);
    }
    stringPtr[answer++] = '\0';

    return answer;
}

size_t fileInfoToStringGroupId(uint32_t groupId, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;
    
    if (!stringPtr || stringLength < 1)
    {
        return 0;
    }
    
    --stringLength;

    struct group *group = 0;

    group = getgrgid(groupId);
    if (group)
    {
        answer = stringLength > strlen(group->gr_name) ? strlen(group->gr_name) : stringLength;
        memcpy(stringPtr, group->gr_name, answer);
    }
    else
    {
        answer = snprintf(stringPtr, stringLength, "%u", groupId);
    }
    stringPtr[answer++] = '\0';

    return answer;
}

size_t fileInfoToStringSize(uint32_t size, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;
    
    if (!stringPtr || stringLength < 1)
    {
        return 0;
    }
    
    answer = snprintf(stringPtr, stringLength, "%u", size);
    stringPtr[answer++] = '\0';

    return answer;
}

size_t fileInfoToStringTimeEdit(time_t timeEdit, char *stringPtr, size_t stringLength)
{
    size_t answer = 0;
    
    if (!stringPtr || stringLength < 1)
    {
        return 0;
    }

    --stringLength;

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

    // answer может быть равен 0, если время не поместилось в stringLength, хотя stringPtr будет заполнен данными
    if (!answer)
    {
        answer = stringLength;
    }
    stringPtr[answer++] = '\0';

    return answer;
}
