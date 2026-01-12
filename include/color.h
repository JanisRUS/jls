/// @file       color.h
/// @brief      Файл с объявлениями модуля работы с цветами
/// @details    Порядок работы с модулем: <br>
///                 1) colorUpdateColorsList() для обновления списка цветов в соответствии с LS_COLORS <br>
///                 2) colorGetReset() для получения escape-последовательности сброса цвета <br>
///                 3) colorFileToESC() для получения escape-последовательности с цветом, соответствующим файлу <br>
/// @author     Тузиков Г.А. janisrus35@gmail.com

#ifndef _COLOR_H_
#define _COLOR_H_

#include <stddef.h>
#include <stdbool.h>
#include "fileInfo.h"

/*
    Макроподстановки
*/

/// @brief      Максимальное количество ключей, берущихся из LS_COLORS
#define COLOR_KEY_MAX_COUNT 256

/// @brief      Максимальная длина ключа, берущегося из LS_COLORS
#define COLOR_KEY_MAX_LENGTH 10

/// @brief      Максимальное количество ANSI команд, берущихся из LS_COLORS
#define COLOR_ANSI_MAX_COUNT 5

/// @brief      Максимальная длина ANSI команд в строковом виде
/// @note       Один SGR код занимает 2 символа + разделитель ;
#define COLOR_ANSI_MAX_LENGTH (COLOR_ANSI_MAX_COUNT * 3)

/// @brief      Максимальная escape-последовательности
/// @note       Длина \033[m = 6
#define COLOR_ESC_MAX_LENGTH (COLOR_ANSI_MAX_LENGTH + 6)

/*
    Структуры
*/

#pragma pack (push, 1)

/// @brief      Структура цветов файла и цели символической ссылки
typedef struct colorFileTargetStruct
{
    char file  [COLOR_ESC_MAX_LENGTH]; ///< Escape-последовательность с цветом файла
    char target[COLOR_ESC_MAX_LENGTH]; ///< Escape-последовательность с цветом цели символической ссылки
}colorFileTargetStruct;

#pragma pack (pop)

/*
    Прототипы функций
*/

/// @brief      Функция обновления списка цветов
/// @details    Данная функция выполняет обновление colorList,
///                 используя информацию из переменной окружения LS_COLORS
/// @note       Отсутствующие Ansi коды берутся из colorListDefault
/// @return     Возвращает true в случае успешного обновления colorList.
///                 В противном случае, возвращает false
bool colorUpdateColorsList(void);

/// @brief      Функция получения escape-последовательности, сбрасывающей цвета
/// @return     Возвращает escape-последовательность, сбрасывающую цвета
const char *colorGetReset(void);

/// @brief      Функция получения цветов файла и цели символической ссылки
/// @details    Данная функция выполняет определение цветов, которыми необходимо раскрасить 
///                 файл и цель символической ссылки и возврвщает эти цвета в виде
///                 escape-последовательности
/// @param[in]  fileInfoPtr  Указатель на информацию о файле
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает цвета файла и цели символической ссылки
colorFileTargetStruct colorFileToESC(const fileInfoStruct *fileInfoPtr, bool *isOkPtr);

// _COLOR_H_
#endif