/// @file       color.c
/// @brief      См. colors.h
/// @author     Тузиков Г.А. janisrus35@gmail.com

#include "color.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
    Внутренние структуры
*/

#pragma pack (push, 1)

/// @brief      Структура цвета
typedef struct colorStruct
{
    char key[COLOR_KEY_MAX_LENGTH];   ///< Ключ
    char ansi[COLOR_ANSI_MAX_LENGTH]; ///< ANSI последовательность
}colorStruct;

#pragma pack (pop)

/*
    Прототипы внутренних функций
*/

/// @brief      Функция получения ключа, соответствующего информации о файле
/// @param[in]  fileNamePtr    Указатель на имя файла
/// @param[in]  type           Тип файла
/// @param[in]  accessPtr      Указатель на права доступа файла
/// @param[in]  isTargetExists Флаг существования цели символической ссылки
/// @return     Возвращает ключ, соответствующий информации о файле.
///                 Если при вычислении ключа произошла ошибка, возвращает 0
static const char *colorGetKey(const char *fileNamePtr, fileInfoTypesEnum type, const fileInfoAccessStruct *accessPtr, bool isTargetExists);

/// @brief      Функция получения Ansi кода, соответствующего key из colorList
/// @param[in]  key Ключ из colorList
/// @note       Если key отсутсвует в colorList, будет произведен поиск в colorListDefault
/// @return     Возвращает Ansi код, соответствующий key из colorList.
///                 Если при поиске Ansi кода произошла ошибка, возвращает 0
static const char *colorGetAnsi(const char *key);

/*
    Внутренние переменные
*/

/// @brief      Список цветов по умолчанию
const colorStruct colorListDefault[] = 
{
    { "fi", "0"     }, // Обычный файл
    { "no", "0"     }, // Обычный файл
    { "rs", "0"     }, // Сброс цвета
    { "di", "01;34" }, // Директория
    { "ln", "01;36" }, // Симлинк
    { "mh", "0"     }, // Файл с несколькими жесткими ссылками
    { "pi", "33"    }, // FIFO
    { "so", "01;35" }, // Сокет
    { "do", "01;35" }, // Особый IPC файл
    { "bd", "01;33" }, // Блочное устройство
    { "cd", "01;33" }, // Символьное устройство
    { "or", "0"     }, // Цель ссылки отсутствует
    { "mi", "0"     }, // Файл не обнаружен
    { "su", "37;41" }, // Файл с правами владельца s/S
    { "sg", "30;43" }, // Файл с правами группы s/S
    { "ca", "30;41" }, // Файл совместимости
    { "tw", "30;42" }, // Директория с правами прочих t/T и w
    { "ow", "34;42" }, // Директория с правами прочих w
    { "st", "37;44" }, // Директория с правами прочих t/T
    { "ex", "01;32" }, // Исполняемый файл
};

/// @brief      Количество элементов в списке цветов по умолчанию
const size_t colorListDefaultCount = sizeof(colorListDefault) / sizeof(colorListDefault[0]);

/// @brief      Список цветов
colorStruct colorList[COLOR_KEY_MAX_COUNT] = {0};

/// @brief      Количество элементов в списке цветов
size_t colorListCount = 0;

/*
    Функции
*/

bool colorUpdateColorsList(void)
{
    colorStruct newColorsList[COLOR_KEY_MAX_COUNT] = {0};
    
    char *env = getenv("LS_COLORS");
    if (!env)
    {
        return false;
    }
    
    char *keyANSIPtr  = 0;
    char *colonSave   = 0;
    
    size_t currentColor = 0;

    keyANSIPtr = strtok_r(env, ":", &colonSave);
    while (keyANSIPtr)
    {
        const char *key  = 0;
        const char *ansi = 0;
        
        char *equealsSave = 0;

        key = strtok_r(keyANSIPtr, "=", &equealsSave);
        if (key)
        {
            ansi = strtok_r(NULL, "=", &equealsSave);
            if (!ansi)
            {
                for (int i = 0; i < colorListDefaultCount; ++i)
                {
                    if (strcmp(&colorListDefault[i].key[0], key) == 0)
                    {
                        ansi = &colorListDefault[i].ansi[0];
                        break;
                    }
                }
            }
        }
        
        if (key && ansi)
        {
            strcpy(&newColorsList[currentColor].key[0],  key);
            strcpy(&newColorsList[currentColor].ansi[0], ansi);
            ++currentColor;
        }
        
        if (currentColor == COLOR_KEY_MAX_COUNT)
        {
            break;
        }
        
        keyANSIPtr = strtok_r(NULL, ":", &colonSave);
    }

    memcpy(&colorList[0], &newColorsList[0], currentColor * sizeof(colorStruct));
    colorListCount = currentColor;

    return true;
}

const char *colorGetReset(void)
{
    static char reset[COLOR_ESC_MAX_LENGTH] = {0};

    const char *ansi = colorGetAnsi("rs");

    int answer = 0;

    answer = snprintf(&reset[0], COLOR_ESC_MAX_LENGTH, "\033[%sm", ansi);
    if (answer < 0)
    {
        return 0;
    }
    
    return &reset[0];
}

colorFileTargetStruct colorFileToESC(const fileInfoStruct *fileInfoPtr, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPtr)
    {
        *isOkPtr = false;
        return (colorFileTargetStruct){0};
    }

    colorFileTargetStruct answer = {0};

    const char *key  = 0;
    const char *ansi = 0;
    
    key  = colorGetKey(fileInfoPtr->fileNamePtr, fileInfoPtr->type, &fileInfoPtr->access, fileInfoPtr->targetInfo.isTargetExists);
    ansi = colorGetAnsi(key);

    if (!key || !ansi)
    {
        *isOkPtr = false;
        return (colorFileTargetStruct){0};
    }

    if (snprintf(answer.file, COLOR_ESC_MAX_LENGTH, "\033[%sm", ansi) < 0)
    {
        *isOkPtr = false;
        return (colorFileTargetStruct){0};
    }

    if (fileInfoPtr->type == fileInfoTypeLink)
    {
        key  = colorGetKey(fileInfoPtr->targetInfo.fileNamePtr, fileInfoPtr->targetInfo.type, &fileInfoPtr->targetInfo.access, fileInfoPtr->targetInfo.isTargetExists);
        ansi = colorGetAnsi(key);
        
        if (!key || !ansi)
        {
            *isOkPtr = false;
            return (colorFileTargetStruct){0};
        }

        if (snprintf(answer.target, COLOR_ESC_MAX_LENGTH, "\033[%sm", ansi) < 0)
        {
            *isOkPtr = false;
            return (colorFileTargetStruct){0};
        }
    }
    else
    {
        memcpy(&answer.target, &answer.file, COLOR_ESC_MAX_LENGTH);
    }
    
    return answer;
}

/*
    Внутренние функции
*/

static const char *colorGetKey(const char *fileNamePtr, fileInfoTypesEnum type, const fileInfoAccessStruct *accessPtr, bool isTargetExists)
{
    if (!fileNamePtr || !accessPtr)
    {
        return 0;
    }

    if (type == fileInfoTypeLink)
    {
        if (!isTargetExists)
        {
            return "or";
        }

        return "ln";
    }

    switch (type)
    {
        case fileInfoTypeSock:  
        {
            return "so";
        }
        case fileInfoTypeFIFO:  
        {
            return "pi";
        }
        case fileInfoTypeBlock: 
        {
            return "bd";
        }
        case fileInfoTypeChar:  
        {
            return "cd";
        }
        default: 
        {
            break;
        }
    }

    if (type == fileInfoTypeDirectory)
    {
        bool otherSpecial = accessPtr->other.bits.special;
        bool otherWrite   = accessPtr->other.bits.write;

        if (otherSpecial && otherWrite)
        {
            return "tw";
        }

        if (otherWrite)
        {
            return "ow";
        }

        if (otherSpecial)
        {
            return "st";
        }

        return "di";
    }

    if (accessPtr->owner.bits.special)
    {
        return "su";
    }

    if (accessPtr->group.bits.special)
    {
        return "sg";
    }

    size_t fileNameLength = strlen(fileNamePtr);

    for (size_t i = 0; i < colorListCount; ++i)
    {
        const char *pattern = colorList[i].key;

        if (strlen(pattern) < 2 || strcmp(pattern, "*.") != 0)
        {
            continue;
        }

        const char *suffix       = pattern + 1;
        size_t      suffixLength = strlen(suffix);

        if (fileNameLength < suffixLength)
        {
            continue;
        }

        if (strcmp(fileNamePtr + fileNameLength - suffixLength, suffix) == 0)
        {
            return colorList[i].key;
        }
    }

    if (accessPtr->owner.bits.execute ||
        accessPtr->group.bits.execute ||
        accessPtr->other.bits.execute)
    {
        return "ex";
    }

    return "fi";
}

static const char *colorGetAnsi(const char *key)
{
    const char *answer = 0;

    if (!key)
    {
        return answer;
    }

    for (int i = 0; i < colorListCount; ++i)
    {
        if (strcmp(&colorList[i].key[0], key) == 0)
        {
            answer = &colorList[i].ansi[0];
            break;
        }
    }

    if (!answer)
    {
        for (int i = 0; i < colorListDefaultCount; ++i)
        {
            if (strcmp(&colorListDefault[i].key[0], key) == 0)
            {
                answer = &colorListDefault[i].ansi[0];
                break;
            }
        }        
    }

    return answer;
}
