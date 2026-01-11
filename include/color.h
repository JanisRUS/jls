/// @file       color.h
/// @brief      Файл с объявлениями модуля работы с цветами
/// @details    Порядок работы с модулем: <br>
///                 1) TODO () для <br>
/// @author     Тузиков Г.А. janisrus35@gmail.com

#ifndef _COLOR_H_
#define _COLOR_H_

#include <stddef.h>
#include <stdbool.h>

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
#define COLOR_ANSI_MAX_LENGTH (COLOR_ANSI_MAX_COUNT * 3)

/// @brief      Максимальная escape-последовательности
/// @note       Длина \033[m
#define COLOR_ESC_MAX_LENGTH (COLOR_ANSI_MAX_LENGTH + 6)

/*
    Перечисления
*/

/*
    Структуры 
*/

/*
    Прототипы функций
*/

/// @brief      Функция обновления списка цветов
/// @details    Данная функция выполняет обновление colorsList,
///                 используя информацию из переменной окружения LS_COLORS
/// @note       Отсутствующие Ansi коды берутся из colorsListDefault
/// @return     Возвращает true в случае успешного обновления colorsList.
///                 В противном случае, возвращает false
bool colorUpdateColorsList(void);

/// @brief      Функция получения escape-последовательности, сбрасывающей цвета
/// @return     Возвращает escape-последовательность, сбрасывающую цвета
const char *colorGetReset(void);

/// @brief      Функция получения escape-последовательности, соответствующей fileInfoPtr
/// @details    Данная функция выполняет определение цвета, которым необходимо раскрасить fileInfoPtr, и
///                 записывает его escape-последовательность в stringPtr длиной stringLength
/// @param[in]  fileInfoPtr  Указатель на информацию о файле
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает количество записанных данных в stringPtr 
size_t colorFileToESC(const fileInfoStruct *fileInfoPtr, char *stringPtr, size_t *stringLength, bool *isOkPtr);

/*
    Переменные
*/

// _COLOR_H_
#endif