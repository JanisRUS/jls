/// @file       jls.c
/// @brief      См. jls.h
/// @author     Тузиков Г.А. janisrus35@gmail.com

#include "jls.h"
#include "fileInfo.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <inttypes.h>
#include <stddef.h>
#include <wchar.h>
#include <wctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*
    Прототипы внутренних функций
*/

/// @brief      Функция обновления значения jlsMaxVisibleChars
/// @details    Данная функция выполняет считывание максимальной ширины окна у одного из следующих источников: <br>
///                 -) ioctl <br>
///                 -) Переменная окружения COLUMNS <br>
///                 -) jlsMaxVisibleCharsDefault <br>
///                 Результат записывается в jlsMaxVisibleChars
static void jlsUpdateMaxVisibleChars(void);

/// @brief      Функция установки пути, где находятся файлы
/// @details    Данная функция выполняет запись pathPtr в bufferPtr размером bufferSize
/// @param[in]  pathPtr    Указатель на путь к файлам
/// @param[in]  bufferPtr  Указатель на буфер, куда будет записан pathPtr. Включая \0
/// @param[in]  bufferSize Размер bufferPtr
/// @param[out] isOkPtr    Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает количество записаных в pathPtr байт
static size_t jlsPathSet(const char *pathPtr, char *bufferPtr, size_t bufferSize, bool *isOkPtr);

/// @brief      Функция добавления имени файла к пути
/// @details    Данная функция выполняет запись filePtr в bufferPtr размером bufferSize в позицию bufferLength
/// @param[in]  filePtr      Указатель на файл
/// @param[in]  bufferPtr    Указатель на буфер, куда будет записан filePtr. Включая \0
/// @param[in]  bufferLength Позиция, с которой нужно записать filePtr
/// @param[in]  bufferSize   Размер bufferPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длину bufferPtr. Включая \0
static size_t jlsPathAppend(const char *filePtr, char *bufferPtr, size_t bufferLength, size_t bufferSize, bool *isOkPtr);

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

/// @brief      Функция проверки строки на небезопасные символы
/// @details    Выполняет посимвольную проверку stringPtr на наличие небезопасных символов
/// @param[in]  stringPtr Указатель на строку
/// @param[out] isOkPtr   Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает true если в строке есть небезопасные символы.
///                 В противном случае, возвращает false
static bool jlsCheckIsUnsafe(const char *stringPtr, bool *isOkPtr);

/*
    Внутренние переменные
*/

/// @brief      Escape-последовательность для сброса цветов
const char *jlsResetColorESC = "";

/// @brief      Отпуступы по умолчанию
const jlsAlignmentStruct jlsAlignmentDefault = 
{
    .linksCount = 2,
    .owner      = 0,
    .group      = 0,
    .size       = 4
};

/// @brief      Максимальное количество выводимых видимых символов по умолчанию
const uint64_t jlsMaxVisibleCharsDefault = 80;

/// @brief      Максимальное количество выводимых видимых символов
uint64_t jlsMaxVisibleChars = jlsMaxVisibleCharsDefault;

/*
    Переменные
*/

bool jlsIsSafeModeEnabled = false;

bool jlsIsColorModeEnabled = false;

/*
    Функции
*/

int jls(const char *filePtr)
{
    static char    fileInfoString[JLS_FILE_INFO_MAX_LENGTH] = {0};
    static size_t  fileInfoStringLength = 0;

    bool isOk = true;
    
    // Объявление переменных, используемых в cleanup
    fileInfoStruct      fileInfo   = {0};
    jlsCommonInfoStruct commonInfo = {0};

    if (!filePtr)
    {
        isOk = false;
        goto cleanup;
    }

    if (jlsIsColorModeEnabled)
    {
        colorUpdateColorsList();
        jlsResetColorESC = colorGetReset();
        if (!jlsResetColorESC)
        {
            isOk = false;
            goto cleanup;
        }
        jlsUpdateMaxVisibleChars();
    }

    fileInfoGet(filePtr, &fileInfo, true, &isOk);
    if (!isOk)
    {
        goto cleanup;
    }

    if (fileInfo.type != fileInfoTypeDirectory)
    {
        if (fileInfo.fileNamePtr)
        {
            free(fileInfo.fileNamePtr);
            fileInfo.fileNamePtr = 0;
        }
        
        fileInfo.fileNamePtr = malloc(strlen(filePtr) + 1);
        if (!fileInfo.fileNamePtr)
        {
            isOk = false;
            goto cleanup;
        }
        
        strcpy(fileInfo.fileNamePtr, filePtr);

        fileInfoStringLength = fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH, &isOk);
        if (isOk)
        {
            bool             isFileUnsafe   = false;
            bool             isTargetUnsafe = false;
            jlsSafeTypesEnum safeType       = jlsSafeTypeNone;

            isFileUnsafe = jlsCheckIsUnsafe(fileInfo.fileNamePtr, &isOk);
            if (!isOk)
            {
                goto cleanup;
            }

            if (fileInfo.type == fileInfoTypeLink)
            {
                isTargetUnsafe = jlsCheckIsUnsafe(fileInfo.targetInfo.fileNamePtr, &isOk);
                if (!isOk)
                {
                    goto cleanup;
                }
            }

            if (isFileUnsafe)
            {
                safeType += jlsSafeTypeName;
            }
            if (isTargetUnsafe)
            {
                safeType += jlsSafeTypeTarget;
            }

            colorFileTargetStruct colors = {0};

            if (jlsIsColorModeEnabled)
            {
                colors = colorFileToESC(&fileInfo, &isOk);
                if (!isOk)
                {
                    goto cleanup;
                }
            }

            jlsPrintFileInfo(&fileInfoString[0], 0, safeType, &colors, &isOk);
            goto cleanup;
        }
        goto cleanup;
    }
    
    commonInfo = jlsGetCommonInfo(filePtr, &isOk);
    if (!isOk || !commonInfo.files.count)
    {
        printf("total 0\n");
        goto cleanup;
    }

    if (commonInfo.files.count >= 2)
    {
        jlsSortFilesList(&commonInfo.files, jlsSortAscend, &isOk);
        if (!isOk)
        {
            goto cleanup;
        }
    }

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(filePtr, &fullPath[0], PATH_MAX, &isOk);
    if (!isOk)
    {
        goto cleanup;
    }

    printf("total %" PRIu64 "\n", commonInfo.total);

    for (int i = 0; i < commonInfo.files.count; ++i)
    {
        char *fileName = 0;

        fileName = commonInfo.files.list[i];

        if (fileInfo.fileNamePtr)
        {
            free(fileInfo.fileNamePtr);
            fileInfo.fileNamePtr = 0;
        }
        if (fileInfo.targetInfo.filePathPtr)
        {
            free(fileInfo.targetInfo.filePathPtr);
            fileInfo.targetInfo.filePathPtr = 0;
        }
    
        jlsPathAppend(fileName, &fullPath[0], pathLength, PATH_MAX, &isOk);
        if (!isOk)
        {
            goto cleanup;
        }

        fileInfoGet(&fullPath[0], &fileInfo, true, &isOk);
        if (!isOk)
        {
            goto cleanup;
        }

        fileInfoStringLength = fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH, &isOk);
        if (!isOk)
        {
            goto cleanup;
        }

        colorFileTargetStruct colors = {0};

        if (jlsIsColorModeEnabled)
        {
            colors = colorFileToESC(&fileInfo, &isOk);
            if (!isOk)
            {
                goto cleanup;
            }
        }

        jlsPrintFileInfo(&fileInfoString[0], &commonInfo.alignment, commonInfo.safeType, &colors, &isOk);
        if (!isOk)
        {
            goto cleanup;
        }
    }

cleanup:
    if (fileInfo.fileNamePtr)
    {
        free(fileInfo.fileNamePtr);
        fileInfo.fileNamePtr = 0;
    }
    if (fileInfo.targetInfo.filePathPtr)
    {
        free(fileInfo.targetInfo.filePathPtr);
        fileInfo.targetInfo.filePathPtr = 0;
    }

    if (commonInfo.files.list)
    {
        for (int i = 0; i < commonInfo.files.count; ++i)
        {
            if (commonInfo.files.list[i])
            {
                free(commonInfo.files.list[i]);
                commonInfo.files.list[i] = 0;
            }
        }
    }
    if (commonInfo.files.list)
    {
        free(commonInfo.files.list);
        commonInfo.files.list = 0;
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

void jlsPrintFileInfo(const char *fileInfoStringPtr, const jlsAlignmentStruct *alignmentPtr, jlsSafeTypesEnum safeType, const colorFileTargetStruct *colorsPtr, bool *isOkPtr)
{
    static const char delimer[] = {FILE_INFO_TO_STRING_DELIMER, '\0'};
    
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    char buffer[JLS_FILE_INFO_MAX_LENGTH] = {0};

    if (!fileInfoStringPtr)
    {
        *isOkPtr = false;
        return;
    }

    jlsAlignmentStruct alignment = jlsAlignmentDefault;

    if (!alignmentPtr)
    {
        alignmentPtr = &alignment;
    }

    colorFileTargetStruct colors = {0};
    static bool isResetPrinted = false;

    if (!jlsIsColorModeEnabled || !colorsPtr)
    {
        colorsPtr = &colors;
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

    // Информация для вывода \033[K
    size_t visibleCharsCount   = 0;
    size_t nameStartCharNumber = 0;
    bool   shouldTryK          = false;

    nameStartCharNumber = printf("%s%s %*s %-*s %-*s %*s %s ", fileInfoStringTypePtr,
                                                               fileInfoStringAccessPtr,
                                (int)alignmentPtr->linksCount, fileInfoStringLinksCountPtr,
                                (int)alignmentPtr->owner,      fileInfoStringOwnerPtr,
                                (int)alignmentPtr->group,      fileInfoStringGroupPtr,
                                (int)alignmentPtr->size,       fileInfoStringSizePtr,
                                                               fileInfoStringTimeEditPtr);

    // Пробел перед именем файла не учитывается
    --nameStartCharNumber;
    if (!jlsIsSafeModeEnabled)
    {
        safeType = jlsSafeTypeNone;
    }

    char safeStringFile[FILE_INFO_TARGET_LENGTH_MAX] = {0};

    if (safeType & jlsSafeTypeName)
    {
        size_t before = 0;
        size_t after  = 0;

        before = strlen(fileInfoStringFilePtr) + 1;

        after = jlsMakeStringSafe(fileInfoStringFilePtr, &safeStringFile[0], FILE_INFO_TARGET_LENGTH_MAX, isOkPtr);
        if (!*isOkPtr)
        {
            *isOkPtr = false;
            return;
        }

        if (before == after)
        {
            printf(" ");
        }

        fileInfoStringFilePtr = &safeStringFile[0];
    }

    visibleCharsCount = strlen(fileInfoStringFilePtr);

    if (!jlsIsColorModeEnabled)
    {
        printf("%s", fileInfoStringFilePtr);
    }
    else
    {
        bool isColored = false;

        if (strcmp(&colorsPtr->file[0], jlsResetColorESC) != 0)
        {
            if (!isResetPrinted)
            {
                printf("%s", jlsResetColorESC);
                isResetPrinted = true;
            }
            printf("%s", &colorsPtr->file[0]);
            isColored  = true;
            shouldTryK = true;
        }

        printf("%s", fileInfoStringFilePtr);
        if (isColored)
        {
            printf("%s", jlsResetColorESC);
        }
    }

    char safeStringTarget[FILE_INFO_TARGET_LENGTH_MAX] = {0};

    if (fileInfoStringTargetPtr)
    {
        if (safeType & jlsSafeTypeTarget)
        {
            jlsMakeStringSafe(fileInfoStringTargetPtr, &safeStringTarget[0], FILE_INFO_TARGET_LENGTH_MAX, isOkPtr);
            if (!*isOkPtr)
            {
                *isOkPtr = false;
                return;
            }

            fileInfoStringTargetPtr = &safeStringTarget[0];
        }

        visibleCharsCount = printf(" -> ");
        visibleCharsCount = strlen(fileInfoStringTargetPtr);

        if (!jlsIsColorModeEnabled)
        {
            printf("%s", fileInfoStringTargetPtr);
        }
        else
        {
            bool isColored = false;

            if (strcmp(&colorsPtr->target[0], jlsResetColorESC) != 0)
            {
                if (!isResetPrinted)
                {
                    printf("%s", jlsResetColorESC);
                    isResetPrinted = true;
                }
                printf("%s", &colorsPtr->target[0]);
                isColored  = true;
                shouldTryK = true;
            }

            printf("%s", fileInfoStringTargetPtr);

            if (isColored)
            {
                printf("%s", jlsResetColorESC);
            }
        }
    }

    if (shouldTryK && nameStartCharNumber / jlsMaxVisibleChars != (nameStartCharNumber + visibleCharsCount - 1) / jlsMaxVisibleChars)
    {
        printf("\033[K");
    }
    
    // TODO удалить
    /*if (fileInfoStringFilePtr[0] == 'd' && 
        fileInfoStringFilePtr[1] == 'b' && 
        fileInfoStringFilePtr[2] == 'u' && 
        fileInfoStringFilePtr[3] == 's' && 
        fileInfoStringFilePtr[4] == '-' && 
        fileInfoStringFilePtr[5] == 'u' && 
        fileInfoStringFilePtr[6] == 'p')
    {
        printf("FLAG\n");
        printf("nameStartCharNumber: %lu\n", nameStartCharNumber);
        printf("jlsMaxVisibleChars:  %lu\n", jlsMaxVisibleChars);
        printf("visibleCharsCount:   %lu\n", visibleCharsCount);
    }*/

    printf("\n");
}

jlsCommonInfoStruct jlsGetCommonInfo(const char *dirPtr, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    struct dirent *directoryEntity   = {0};
    size_t         currentFileNumber = 0;

    // Объявление переменных, используемых в cleanup
    DIR                 *directory = 0;
    jlsCommonInfoStruct  answer    = {0};
    fileInfoStruct       fileInfo  = {0};

    if (!dirPtr)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    answer.files.count = jlsCountFilesInDirectory(dirPtr, isOkPtr);
    if (!answer.files.count || !*isOkPtr)
    {
        goto cleanup;
    }

    answer.files.list = calloc(answer.files.count, sizeof(char *));
    if (!answer.files.list)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    directory = opendir(dirPtr);
    if (!directory)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(dirPtr, &fullPath[0], PATH_MAX, isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    while ((directoryEntity = readdir(directory)) != NULL) 
    {
        /*
            Общее. Начало цикла
        */

        jlsPathAppend(directoryEntity->d_name, &fullPath[0], pathLength, PATH_MAX, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        /*
            Расчет answer.files
        */

        if (strcmp(directoryEntity->d_name, ".")  == 0 ||
            strcmp(directoryEntity->d_name, "..") == 0)
        {
            continue;
        }

        answer.files.list[currentFileNumber] = malloc(strlen(directoryEntity->d_name) + 1);
        if (!answer.files.list[currentFileNumber])
        {
            *isOkPtr = false;
            goto cleanup;
        }

        strcpy(answer.files.list[currentFileNumber], directoryEntity->d_name);

        /*
            Общее. Получение информации о файле
        */

        static char fileInfoString[JLS_FILE_INFO_MAX_LENGTH] = {0};

        if (fileInfo.fileNamePtr)
        {
            free(fileInfo.fileNamePtr);
            fileInfo.fileNamePtr = 0;
        }
        if (fileInfo.targetInfo.filePathPtr)
        {
            free(fileInfo.targetInfo.filePathPtr);
            fileInfo.targetInfo.filePathPtr = 0;
        }

        fileInfoGet(&fullPath[0], &fileInfo, false, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        /*
            Рассчет answer.alignment
        */

        static const char  delimer[] = {FILE_INFO_TO_STRING_DELIMER, '\0'};
        static       char *field     = 0;

        // Пропускаем тип файла
        field = strtok(&fileInfoString[0], delimer);
        // Пропускаем права доступа файла
        field = strtok(NULL, delimer);

        field = strtok(NULL, delimer);
        if (answer.alignment.linksCount < strlen(field))
        {
            answer.alignment.linksCount = strlen(field);
        }

        field = strtok(NULL, delimer);
        if (answer.alignment.owner < strlen(field))
        {
            answer.alignment.owner = strlen(field);
        }

        field = strtok(NULL, delimer);
        if (answer.alignment.group < strlen(field))
        {
            answer.alignment.group = strlen(field);
        }

        field = strtok(NULL, delimer);
        if (answer.alignment.size < strlen(field))
        {
            answer.alignment.size = strlen(field);
        }

        /*
            Рассчет answer.safeType
        */

        if (jlsIsSafeModeEnabled)
        {
            static bool isFileUnsafe   = false;
            static bool isTargetUnsafe = false;

            if (!isFileUnsafe)
            {
                isFileUnsafe = jlsCheckIsUnsafe(fileInfo.fileNamePtr, isOkPtr);
                if (!*isOkPtr)
                {
                    goto cleanup;
                }
            }

            if (!isTargetUnsafe && fileInfo.type == fileInfoTypeLink)
            {
                isTargetUnsafe = jlsCheckIsUnsafe(fileInfo.targetInfo.fileNamePtr, isOkPtr);
                if (!*isOkPtr)
                {
                    goto cleanup;
                }
            }
        
            if (currentFileNumber + 1 == answer.files.count)
            {
                if (isFileUnsafe)
                {
                    answer.safeType += jlsSafeTypeName;
                }
                if (isTargetUnsafe)
                {
                    answer.safeType += jlsSafeTypeTarget;
                }
            }
        }

        /*
            Рассчет answer.blocks
        */

        answer.total += fileInfo.blocks;
        if (!*isOkPtr)
        {
            goto cleanup;
        }
    
        if (currentFileNumber + 1 == answer.files.count)
        {
            answer.total /= 2;
        }

        /*
            Общее. Завершение цикла
        */

        ++currentFileNumber;
        if (currentFileNumber == answer.files.count)
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

    if (fileInfo.fileNamePtr)
    {
        free(fileInfo.fileNamePtr);
        fileInfo.fileNamePtr = 0;
    }
    if (fileInfo.targetInfo.filePathPtr)
    {
        free(fileInfo.targetInfo.filePathPtr);
        fileInfo.targetInfo.filePathPtr = 0;
    }

    if (!*isOkPtr)
    {
        if (answer.files.list)
        {
            for (int i = 0; i < answer.files.count; ++i)
            {
                if (answer.files.list[i])
                {
                    free(answer.files.list[i]);
                    answer.files.list[i] = 0;
                }
            }
        }
        if (answer.files.list)
        {
            free(answer.files.list);
            answer.files.list = 0;
        }
    }
    
    if (*isOkPtr)
    {
        return answer;
    }
    else
    {
        return (jlsCommonInfoStruct){0};
    }
}

jlsFilesListStruct jlsGetFilesList(const char *dirPtr, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    size_t         filesCount      = 0;
    struct dirent *directoryEntity = {0};
    
    // Объявление переменных, используемых в cleanup
    jlsFilesListStruct answer          = {0};
    DIR               *directory       = 0;

    filesCount = jlsCountFilesInDirectory(dirPtr, isOkPtr);
    if (!filesCount || !*isOkPtr)
    {
        goto cleanup;
    }

    answer.list = calloc(filesCount, sizeof(char *));
    if (!answer.list)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    directory = opendir(dirPtr);
    if (!directory)
    {
        *isOkPtr = false;
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
            *isOkPtr = false;
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

    if (!*isOkPtr)
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

    if (*isOkPtr)
    {
        return answer;
    }
    else
    {
        return (jlsFilesListStruct){0};
    }
}

void jlsSortFilesList(jlsFilesListStruct *filesListPtr, jlsSortEnum sort, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    if (!filesListPtr || !filesListPtr->list || filesListPtr->count < 2)
    {
        *isOkPtr = false;
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

size_t jlsCountFilesInDirectory(const char *dirPtr, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;
    
    size_t         answer          = 0;
    DIR           *directory       = 0;
    struct dirent *directoryEntity = {0};

    directory = opendir(dirPtr);
    if (!directory)
    {
        *isOkPtr = false;
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

jlsAlignmentStruct jlsCalculateAlignment(const char *pathPtr, const jlsFilesListStruct *filesList, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    jlsAlignmentStruct answer = {0};

    // Объявление переменных, используемых в cleanup
    fileInfoStruct fileInfo = {0};

    if (!pathPtr || !filesList)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(pathPtr, &fullPath[0], PATH_MAX, isOkPtr);
    if (!*isOkPtr)
    {
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
        if (fileInfo.targetInfo.filePathPtr)
        {
            free(fileInfo.targetInfo.filePathPtr);
            fileInfo.targetInfo.filePathPtr = 0;
        }
    
        jlsPathAppend(filesList->list[i], &fullPath[0], pathLength, PATH_MAX, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        fileInfoGet(&fullPath[0], &fileInfo, false, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        fileInfoToString(&fileInfo, &fileInfoString[0], JLS_FILE_INFO_MAX_LENGTH, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
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
    if (fileInfo.targetInfo.filePathPtr)
    {
        free(fileInfo.targetInfo.filePathPtr);
        fileInfo.targetInfo.filePathPtr = 0;
    }
    if (*isOkPtr)
    {
        return answer;
    }
    else
    {
        return (jlsAlignmentStruct){0};
    }
}

jlsSafeTypesEnum jlsCalculateSafeType(const char *pathPtr, const jlsFilesListStruct *filesList, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    jlsSafeTypesEnum answer = jlsSafeTypeNone;

    if (!pathPtr || !filesList)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(pathPtr, &fullPath[0], PATH_MAX, isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    bool isFileUnsafe   = false;
    bool isTargetUnsafe = false;

    for (int i = 0; i < filesList->count; ++i)
    {
        jlsPathAppend(filesList->list[i], &fullPath[0], pathLength, PATH_MAX, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        if (!fileInfoSetActiveFile(&fullPath[0]))
        {
            goto cleanup;
        }

        fileInfoTypesEnum type = fileInfoTypeUnknown;

        type = fileInfoGetType(isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        if (!isFileUnsafe)
        {
            isFileUnsafe = jlsCheckIsUnsafe(filesList->list[i], isOkPtr);
            if (!*isOkPtr)
            {
                goto cleanup;
            }
        }

        if (!isTargetUnsafe && type == fileInfoTypeLink)
        {
            static char targetString[JLS_FILE_INFO_MAX_LENGTH] = {0};

            fileInfoGetLinkTarget(&targetString[0], JLS_FILE_INFO_MAX_LENGTH, isOkPtr);
            if (!*isOkPtr)
            {
                goto cleanup;
            }

            isTargetUnsafe = jlsCheckIsUnsafe(&targetString[0], isOkPtr);
            if (!*isOkPtr)
            {
                goto cleanup;
            }
        }
    }

    if (isFileUnsafe)
    {
        answer += jlsSafeTypeName;
    }
    if (isTargetUnsafe)
    {
        answer += jlsSafeTypeTarget;
    }

cleanup:
    fileInfoClearActiveFile();

    if (*isOkPtr)
    {
        return answer;
    }
    else
    {
        return jlsSafeTypeNone;
    }
}

uint64_t jlsCalculate1024ByteBlocks(const char *pathPtr, const jlsFilesListStruct *filesList, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    uint64_t answer = {0};

    if (!pathPtr || !filesList)
    {
        *isOkPtr = false;
        goto cleanup;
    }

    char   fullPath[PATH_MAX] = {0};
    size_t pathLength         = 0;

    pathLength = jlsPathSet(pathPtr, &fullPath[0], PATH_MAX, isOkPtr);
    if (!*isOkPtr)
    {
        goto cleanup;
    }

    for (int i = 0; i < filesList->count; ++i)
    {
        static char fileInfoString[JLS_FILE_INFO_MAX_LENGTH] = {0};
    
        jlsPathAppend(filesList->list[i], &fullPath[0], pathLength, PATH_MAX, isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }

        if (!fileInfoSetActiveFile(&fullPath[0]))
        {
            goto cleanup;
        }

        answer += fileInfoGet512BytesBlocks(isOkPtr);
        if (!*isOkPtr)
        {
            goto cleanup;
        }
    }

cleanup:

    fileInfoClearActiveFile();

    if (*isOkPtr)
    {
        answer /= 2;
        return answer;
    }
    else
    {
        return 0;
    }
}

size_t jlsMakeStringSafe(const char *stringPtr, char *safePtr, size_t safePtrLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!stringPtr || !safePtr || strlen(stringPtr) + 1 > safePtrLength)
    {
        *isOkPtr = false;
        return 0;
    }

    bool isUnsafe = true;

    isUnsafe = jlsCheckIsUnsafe(stringPtr, isOkPtr);
    if (!*isOkPtr)
    {
        return 0;
    }
    if (!isUnsafe)
    {
        strcpy(safePtr, stringPtr);
        return strlen(safePtr) + 1;
    }

    // \0 и 2 экранирующих символа 
    if (strlen(stringPtr) + 3 > safePtrLength)
    {
        *isOkPtr = false;
        return 0;
    }

    memset(safePtr, 0, safePtrLength);

    size_t answer = 0;

    if (!strchr(stringPtr, '\''))
    {
        safePtr[answer++] = '\'';
        memcpy(&safePtr[answer], stringPtr, strlen(stringPtr));
        answer += strlen(stringPtr);
        safePtr[answer++] = '\'';

        return answer;
    }

    if (!strchr(stringPtr, '"') && 
        !strchr(stringPtr, '$') && 
        !strchr(stringPtr, '`') && 
        !strchr(stringPtr, '\\'))
    {
        safePtr[answer++] = '"';
        memcpy(&safePtr[answer], stringPtr, strlen(stringPtr));
        answer += strlen(stringPtr);
        safePtr[answer++] = '"';

        return answer;
    }

    safePtr[answer++] = '\'';
    for (int i = 0; i < strlen(stringPtr); ++i)
    {
        static const char safeQuote[] = "\'\\\'\'";

        if (stringPtr[i] != '\'')
        {
            safePtr[answer++] = stringPtr[i];
            continue;
        }

        // \0 и закрывающий экранирующий символ
        if (answer + strlen(safeQuote) >= safePtrLength - 2)
        {
            *isOkPtr = false;
            break;
        }

        memcpy(&safePtr[answer], &safeQuote[0], strlen(safeQuote));

        answer += strlen(safeQuote);
    }
    safePtr[answer++] = '\'';
    safePtr[answer++] = '\0';

    return answer;
}

/*
    Внутренние функции
*/

static void jlsUpdateMaxVisibleChars(void)
{
    uint64_t newMaxVisibleChars = jlsMaxVisibleCharsDefault;

    if (!jlsIsColorModeEnabled)
    {
        // TODO удалить
        //printf("COLOR MODE FAILED\n");
        goto updateMaxVisibleChars;
    }
    
    struct winsize winSize = {0};

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize) != -1)
    {
        newMaxVisibleChars = winSize.ws_col;
        goto updateMaxVisibleChars;
    }
    // TODO удалить
    //printf("IOCTL FAILED\n");

    char *env = getenv("COLUMNS");
    if (!env)
    {
        // TODO удалить
        //printf("COLUMNS FAILED\n");
        goto updateMaxVisibleChars;
    }

    unsigned long long columns = 0;
    char               *envEnd = 0; 

    columns = strtoull(env, &envEnd, 10);
    if (env == envEnd || *envEnd != '\0' || errno != 0)
    {
        // TODO удалить
        //printf("STRTOULL FAILED\n");
        goto updateMaxVisibleChars;
    }

    newMaxVisibleChars = columns;

updateMaxVisibleChars:
    jlsMaxVisibleChars = newMaxVisibleChars;
    // TODO удалить
    //printf("jlsMaxVisibleChars %lu\n", jlsMaxVisibleChars);
}

static size_t jlsPathSet(const char *pathPtr, char *bufferPtr, size_t bufferSize, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    size_t answer = 0;

    if (!pathPtr || !bufferPtr || bufferSize < 2)
    {
        *isOkPtr = false;
        return 0;
    }

    memset(bufferPtr, 0, bufferSize);

    answer = snprintf(bufferPtr, bufferSize, "%s/", pathPtr);

    return answer;
}

static size_t jlsPathAppend(const char *filePtr, char *bufferPtr, size_t bufferLength, size_t bufferSize, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    size_t answer = 0;

    if (!filePtr || !bufferPtr || bufferSize < bufferLength)
    {
        *isOkPtr = false;
        return answer;
    }

    answer  = snprintf(&bufferPtr[bufferLength], bufferSize - bufferLength, "%s", filePtr);
    answer += bufferLength;
    bufferPtr[answer++] = '\0';

    return answer;
}

static int jlsFilesListCompareAscend(const void *a, const void *b)
{
    return strcoll(*(const char **)a, *(const char **)b);
}

static int jlsFilesListCompareDescend(const void *a, const void *b)
{
    return jlsFilesListCompareAscend(b, a);
}

static bool jlsCheckIsUnsafe(const char *stringPtr, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!stringPtr)
    {
        *isOkPtr = false;
        return false;
    }
    
    size_t stringLength = strlen(stringPtr);
    
    while (stringLength > 0)
    {
        bool      isPrintable    = true;
        wchar_t   wideChar       = L'\0';
        size_t    wideCharLength = 0;
        mbstate_t state          = {0};
        
        wideCharLength = mbrtowc(&wideChar, stringPtr, stringLength, &state);

        if (wideCharLength == (size_t) - 1 || wideCharLength == (size_t) - 2)
        {
            isPrintable    = false;
            wideCharLength = 1;
        }
        else
        {
            isPrintable = iswprint(wideChar);
        }

        if (!isPrintable)
        {
            return true;
        }

        switch (wideChar)
        {
            case L' ':  case L'\t': case L'\n': case L'\r': case L'\v':
            case L'\f': case L'\'': case L'"':  case L'\\': case L'|':
            case L'&':  case L';':  case L'<':  case L'>':  case L'(':
            case L')':  case L'$':  case L'!':  case L'*':  case L'?':
            case L'[':  case L']':  case L'{':  case L'}':  case L'`':
            {
                return true;
            }
        }

        stringPtr    += wideCharLength;
        stringLength -= wideCharLength;
    }

    return false;
}
