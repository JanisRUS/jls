/// @file       jls.h
/// @brief      Файл с объявлениями модуля вывода данных о файле или содержимым директории
/// @details    Порядок работы с модулем:
///                 1) jls() для сбора и вывода информации о файле/файлах в директории<br>
///                 2) jlsPrintFileInfo() для вывода информации о файле<br>
///                 3) jlsGetFilesList() для получения списка файлов в директории<br>
///                 4) jlsSortFilesList() для сортировки списка файлов<br>
///                 5) jlsCountFilesInDirectory() для подсчета количества файлов в директории<br>
///                 6) jlsCalculateAlignment() для расчета максимальных размеров полей информации о файле<br>
/// @author     Тузиков Г.А. janisrus35@gmail.com

#ifndef _JLS_H_
#define _JLS_H_

#include <stdlib.h>
#include <stdbool.h>

/*
    Макроподстановки
*/

/// @brief      Максимальная длина строки со всей информацией о файле
#define JLS_FILE_INFO_MAX_LENGTH 500

/*
    Перечисления
*/

/// @brief      Типы сортировки
typedef enum jlsSortEnum
{
    jlsSortNone,   ///< Без сортировки
    jlsSortAscend, ///< Сортировка по возрастанию
    jlsSortDescend ///< Сортировка по возрастанию
}jlsSortEnum;

/*
    Структуры
*/

#pragma pack (push, 1)

/// @brief      Структура максимальных размеров полей информации о файле
typedef struct jlsAlignmentStruct
{
    size_t linksCount; ///< Максимальная длина числа жестких ссылок
    size_t owner;      ///< Максимальная длина владельца файла
    size_t group;      ///< Максимальная длина группы файла
    size_t size;       ///< Максимальная длина размера файла
}jlsAlignmentStruct;

/// @brief      Стуктура списка файлов
typedef struct jlsFilesListStruct
{
    char **list;  ///< Список файлов
    size_t count; ///< Количество файлов
}jlsFilesListStruct;

#pragma pack (pop)

/*
    Прототипы функций
*/

/// @brief      Функция вывода информации о файле/файлах в директории
/// @details    Данная функция выполняет получение информации о файле filePtr.
///                 Если filePtr - регулярный файл, выводит о нем информацию.
///                 Если filePtr - директория, последовательно выводит о информацию о каждом файле в ней
/// @param[in]  filePtr Указатель на файл
/// @return     Возвращает 0 в случае успешного выполнения функции.
///                 В противном случае, возвращет 1
int jls(const char *filePtr);

/// @brief      Функция вывода информации о файле
/// @details    Данная функция выпоняет вывод fileInfoStringPtr с учетом значений из alignmentPtr
/// @param[in]  fileInfoStringPtr Указатель на строку с информацией о файле
/// @warning    Строка fileInfoStringPtr должна соответствовать строке, получаемой при помощи fileInfoToString()
/// @param[in]  alignmentPtr      Указатель структуру максимальных размеров полей информации о файле
/// @param[out] isOkPtr           Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @note       Указатель alignmentPtr может быть равен 0
void jlsPrintFileInfo(const char *fileInfoStringPtr, const jlsAlignmentStruct *alignmentPtr, bool *isOkPtr);

/// @brief      Функция получения списка файлов в указанной директории
/// @details    Данная функция выполняет последовательное формирование списка файлов, игнорируя . и ..
/// @warning    Данная функция использует malloc!
///                 Не забудьте очистить память при выходе из программы, очистив каждый элемент списка и сам список! 
/// @param[in]  dirPtr  Указатель на директорию
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает список файлов в указанной директории
jlsFilesListStruct jlsGetFilesList(const char *dirPtr, bool *isOkPtr);

/// @brief      Функция сортировки списка файлов
/// @details    Данная функция выполняет сортировку filesListPtr по sort
/// @param[in]  filesListPtr Указатель на список файлов
/// @param[in]  sort         Тип сортировки
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
void jlsSortFilesList(const jlsFilesListStruct *filesListPtr, jlsSortEnum sort, bool *isOkPtr);

/// @brief      Функция подсчета количества файлов в директории
/// @details    Данная функция выполняет последовательный подсчет количества файлов в директории, игнорируя . и .. 
/// @param[in]  dirPtr  Указатель на директорию
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает количество файлов в директории
size_t jlsCountFilesInDirectory(const char *dirPtr, bool *isOkPtr);

/// @brief      Функция расчета максимальных размеров полей информации о файле
/// @details    Данная функция выполняет последовательное получение информации о файлах, 
///                 получение строкового представления этой информации и расчет длины каждого поля информации о файле.
///                 Если какое то поле превысило прошлый максимум этого поля, он будет обновлен
/// @param[in]  pathPtr   Указатель на директорию с файлами
/// @param[in]  filesList Список файлов в указанной директории
/// @param[out] isOkPtr   Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return    Возвращает структуру с максимальными размерами всех полей информации о файле
jlsAlignmentStruct jlsCalculateAlignment(const char *pathPtr, const jlsFilesListStruct *filesList, bool *isOkPtr);

// _JLS_H_
#endif