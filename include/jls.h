/// @file       jls.h
/// @brief      Файл с объявлениями модуля вывода данных о файле или содержимым директории
/// @details    Порядок работы с модулем: <br>
///                 1) jls() для сбора и вывода информации о файле/файлах в директории <br>
///                 2) jlsPrintFileInfo() для вывода информации о файле <br>
///                 3) jlsGetCommonInfo() для получения общей информации о файлах в директории <br>
///                 4) jlsGetFilesList() для получения списка файлов в директории <br>
///                 5) jlsSortFilesList() для сортировки списка файлов <br>
///                 6) jlsCountFilesInDirectory() для подсчета количества файлов в директории <br>
///                 7) jlsCalculateAlignment() для расчета максимальных размеров полей информации о файле <br>
/// @note       Для настройки вывода, модулем используются следующие переменные: <br>
///                 1) jlsIsSafeModeEnabled <br>
///                 2) jlsIsColorModeEnabled <br>
/// @author     Тузиков Г.А. janisrus35@gmail.com

#ifndef _JLS_H_
#define _JLS_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "color.h"

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

/// @brief      Перечисление типов безопасного режима
typedef enum jlsSafeTypesEnum
{
    jlsSafeTypeNone   = 0, ///< Безопасный режим не нужен
    jlsSafeTypeName   = 1, ///< Безопасный режим нужен для имени файла
    jlsSafeTypeTarget = 2, ///< Безопасный режим нужен для цели ссылки
    jlsSafeTypeBoth   = 3  ///< Безопасный режим нужен и для имени файла и для цели ссылки
}jlsSafeTypesEnum;

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

/// @brief      Структура общей информации о файле/файлах в директории
typedef struct jlsCommonInfoStruct
{
    jlsFilesListStruct files;     ///< Список файлов
    jlsAlignmentStruct alignment; ///< Максимальный размер полей информации о файле
    jlsSafeTypesEnum   safeType;  ///< Безопасный режим
    uint64_t           total;     ///< Количество занимаемых файлами 1024 байтовых блоков
}jlsCommonInfoStruct;

#pragma pack (pop)

/*
    Прототипы функций
*/

/// @brief      Функция вывода информации о файле/файлах в директории
/// @details    Данная функция выполняет получение информации о файле filePtr.
///                 Если filePtr - регулярный файл, выводит о нем информацию.
///                 Если filePtr - директория, последовательно выводит о информацию о каждом файле в ней
/// @param[in]  filePtr      Указатель на файл
/// @param[in]  alignmentPtr Указатель на структуру максимальных размеров полей информации о файле. Может быть равен 0
/// @param[in]  safeType     Тип безопасного режима
/// @warning    Значения alignmentPtr и safeType используются только если filePtr - регулярный файл
/// @return     Возвращает 0 в случае успешного выполнения функции.
///                 В противном случае, возвращет 1
int jls(const char *filePtr, const jlsAlignmentStruct *alignmentPtr, jlsSafeTypesEnum safeType);

/// @brief      Функция вывода информации о файле
/// @details    Данная функция выпоняет вывод fileInfoStringPtr с учетом значений из alignmentPtr
/// @param[in]  fileInfoStringPtr Указатель на строку с информацией о файле
/// @warning    Строка fileInfoStringPtr должна соответствовать строке, получаемой при помощи fileInfoToString()
/// @param[in]  alignmentPtr      Указатель на структуру максимальных размеров полей информации о файле
/// @param[in]  safeType          Тип безопасного режима
/// @note       Можно отключить при помощи сброса jlsIsSafeModeEnabled
/// @param[in]  colorsPtr         Указатель на структуру цветов. Может быть равен 0
/// @note       Можно отключить при помощи сброса jlsIsColorModeEnabled
/// @param[out] isOkPtr           Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @note       Указатель alignmentPtr может быть равен 0
void jlsPrintFileInfo(const char *fileInfoStringPtr, const jlsAlignmentStruct *alignmentPtr, jlsSafeTypesEnum safeType, const colorFileTargetStruct *colorsPtr, bool *isOkPtr);

/// @brief      Функция получения общей информациии о файлах в директории
/// @details    Данная функция выполняет получение общей информации о файлах в директории
/// @note       Список информации:<br>
///                 1) Список файлов<br>
///                 2) Максимальный размер полей информации о файлах<br>
///                 3) Необходимый безопасный режим<br>
///                 4) Количество занимаемых файлами 1024 байтовых блоков
/// @param[in]  dirPtr  Указатель на директорию
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает список общей информации о файлах в директории
jlsCommonInfoStruct jlsGetCommonInfo(const char *dirPtr, bool *isOkPtr);

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
void jlsSortFilesList(jlsFilesListStruct *filesListPtr, jlsSortEnum sort, bool *isOkPtr);

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

/// @brief      Функция вычисления безопасного режима
/// @details    Данная функция выполняет проверку всех имен файлов и целей ссылок.
///                 В зависимости от того, что из них требует вывода одинарных кавычек,
///                 возвращает нужный для их отображения безопасный режим
/// @param[in]  pathPtr   Указатель на директорию с файлами
/// @param[in]  filesList Список файлов в указанной директории
/// @param[out] isOkPtr   Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает необходимый для отображения filesList безопасный режим
jlsSafeTypesEnum jlsCalculateSafeType(const char *pathPtr, const jlsFilesListStruct *filesList, bool *isOkPtr);

/// @brief      Функция расчета количества занимаемых файлами 1024 байтовых блоков
/// @details    Данная функция выполняет последовательное получение информации о 
///                 количестве занимаемых файлами 512 байтовых блоках,
///                 суммирует эти значения и делит пополам
/// @param[in]  pathPtr   Указатель на директорию с файлами
/// @param[in]  filesList Список файлов в указанной директории
/// @param[out] isOkPtr   Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return    Возвращает количество занимаемых файлами 1024 байтовых блоков 
uint64_t jlsCalculate1024ByteBlocks(const char *pathPtr, const jlsFilesListStruct *filesList, bool *isOkPtr);

/// @brief      Функция преобразования строки в безопасный вариант
/// @details    Данная функция выполняет экранирование строки stringPtr по правилам: <br>
///                 -) Если небезопасных символов нет, ничего не делать <br>
///                 -) Если есть одинарные кавычки, экранировать stringPtr двойными кавычками <br>
///                 -) Если одинарных кавычек нет, экранировать stringPtr одинарными кавычками <br>
///                 Новая строка записывается в safePtr
/// @param[in]  stringPtr     Указатель на строку
/// @param[out] safePtr       Указатель на строку в безопасном варианте, куда будет записан результат с \0
/// @param[in]  safePtrLength Длина safePtrLength
/// @param[out] isOkPtr       Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return    Возвращает количество записанных в safePtr данных
size_t jlsMakeStringSafe(const char *stringPtr, char *safePtr, size_t safePtrLength, bool *isOkPtr);

/*
    Переменные
*/

/// @brief      Флаг вывода информации в безопасном режиме
/// @details    Если установлен, названия файлов с запрещенными символами будут выведены в экранирующих символах
/// @note       По умолчанию выключен
extern bool jlsIsSafeModeEnabled;

/// @brief      Флаг цветного вывода информации о файлах
/// @details    Если установлен, названия файлов будут раскрашены
/// @note       По умолчанию выключен
extern bool jlsIsColorModeEnabled;

// _JLS_H_
#endif