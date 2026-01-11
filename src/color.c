/// @file       color.c
/// @brief      См. colors.h
/// @author     Тузиков Г.А.

#include "color.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fileInfo.h"

/*
    Макроподстановки
*/

/*
    Внутренние труктуры
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

/// @brief      Функция получения ключа, соответствующего fileInfoPtr
/// @param[in]  fileInfoPtr Указатель на информацию о файле
/// @return     Возвращает ключ, соответствующий fileInfoPtr.
///                 Если при вычислении ключа произошла ошибка, 
///                 возвращает 0
static const char *colorGetKey(const fileInfoStruct *fileInfoPtr);

/// @brief      Функция получения Ansi кода, соответствующего key из colorsList
/// @param[in]  key Ключ из colorsList
/// @note       Если key отсутсвует в colorsList, будет произведен поиск в colorsListDefault
/// @return     Возвращает Ansi код, соответствующий key из colorsList.
///                 Если при поиске Ansi кода произошла ошибка, 
///                 возвращает 0
static const char *colorGetAnsi(const char *key);

/*
    Внутренние переменные
*/

/// @brief      Список цветов по умолчанию
const colorStruct colorsListDefault[] = 
{
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
const size_t colorsListDefaultCount = sizeof(colorsListDefault) / sizeof(colorsListDefault[0]);

/// @brief      Список цветов
colorStruct colorsList[COLOR_KEY_MAX_COUNT] = {0};

/// @brief      Количество элементов в списке цветов
size_t colorsListCount = 0;

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
                for (int i = 0; i < colorsListDefaultCount; ++i)
                {
                    if (strcmp(&colorsListDefault[i].key[0], key) == 0)
                    {
                        ansi = &colorsListDefault[i].ansi[0];
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

    memcpy(&colorsList[0], &newColorsList[0], currentColor * sizeof(colorStruct));
    colorsListCount = currentColor;

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

size_t colorFileToESC(const fileInfoStruct *fileInfoPtr, char *stringPtr, size_t *stringLength, bool *isOkPtr)
{
    bool isOk = true;

    if (!isOkPtr)
    {
        isOkPtr = &isOk;
    }

    *isOkPtr = true;

    if (!fileInfoPtr || !stringPtr || stringLength < 1)
    {
        *isOkPtr = false;
        return 0;
    }

    const char *key  = 0;
    const char *ansi = 0;
    
    key  = colorGetKey(fileInfoPtr);
    ansi = colorGetAnsi(key);

    if (!key || !ansi )
    {
        *isOkPtr = false;
        return 0;
    }
    
    int answer = 0;

    answer = snprintf(stringPtr, stringLength, "\033[%sm", ansi);
    if (answer < 0)
    {
        *isOkPtr = false;
        return 0;
    }
    
    return (size_t)answer;
}

/*
    Внутренние функции
*/

static const char *colorGetKey(const fileInfoStruct *fileInfoPtr)
{
    if (!fileInfoPtr)
    {
        return 0;
    }

    if (fileInfoPtr->type == fileInfoTypeUnknown)
    {
        return "no";
    }

    if (fileInfoPtr->type == fileInfoTypeLink)
    {
        if (!fileInfoPtr->isTargetExists)
        {
            return "or";
        }

        return "ln";
    }

    switch (fileInfoPtr->type)
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

    if (fileInfoPtr->type == fileInfoTypeDirectory)
    {
        bool otherSpecial = fileInfoPtr->access.other.bits.special;
        bool otherWrite   = fileInfoPtr->access.other.bits.write;

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

    if (fileInfoPtr->access.owner.bits.execute ||
        fileInfoPtr->access.group.bits.execute ||
        fileInfoPtr->access.other.bits.execute)
    {
        return "ex";
    }

    for (size_t i = 0; i < colorsListCount; ++i)
    {
        const char *pattern = colorsList[i].key;

        // Как минимум *.
        if (strlen(pattern) < 2)
        {
            continue;
        }

        const char *suffix       = pattern + 1;
        size_t      suffixLength = strlen(pattern) - 1;

        if (fileInfoPtr->fileNameLength < suffixLength)
        {
            continue;
        }

        if (strcmp(fileInfoPtr->fileNamePtr + fileInfoPtr->fileNameLength - suffixLength, suffix) == 0)
        {
            return colorsList[i].key;
        }
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

    for (int i = 0; i < colorsListCount; ++i)
    {
        if (strcmp(&colorsList[i].key[0], key) == 0)
        {
            answer = &colorsList[i].ansi[0];
            break;
        }
    }

    if (!answer)
    {
        for (int i = 0; i < colorsListDefaultCount; ++i)
        {
            if (strcmp(&colorsListDefault[i].key[0], key) == 0)
            {
                answer = &colorsListDefault[i].ansi[0];
                break;
            }
        }        
    }

    return answer;
}
