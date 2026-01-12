/// @file       fileInfo.h
/// @brief      Файл с объявлениями модуля получения данных о файле
/// @details    Порядок работы с модулем: <br>
///                 1) fileInfoIsExists() для проверки существования файла <br>
///                 2) fileInfoGet() для получения всей информации о файле <br>
///                 3) fileInfoSetActiveFile() для установки активного файла <br>
///                 4) fileInfoClearActiveFile() для сброса активного файла <br>
///                 5) Функции с префиксом fileInfoGet для получения информации об активном файле <br>
///                 6) fileInfoToString() для получения строкового представления всей информации о файле <br>
///                 7) Функции с префиксом fileInfoToString для получения строкового представления информации о файле
/// @author     Тузиков Г.А. janisrus35@gmail.com

#ifndef _FILE_INFO_H_
#define _FILE_INFO_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/*
    Макроподстановки
*/

/// @brief      Максимальный размер строки fileInfoStruct.targetPtr с учетом \0
#define FILE_INFO_TARGET_LENGTH_MAX 256

/// @brief      Символ-разделитель полей функци fileInfoToString()
#define FILE_INFO_TO_STRING_DELIMER '\t'

/*
    Перечисления
*/

/// @brief      Перечисление типов файлов
typedef enum fileInfoTypesEnum
{
    fileInfoTypeUnknown = 0, ///< Неизвестный тип файла
    fileInfoTypeDirectory,   ///< Директория
    fileInfoTypeChar,        ///< Символьное устройство
    fileInfoTypeBlock,       ///< Блочное устройство
    fileInfoTypeFile,        ///< Файл
    fileInfoTypeFIFO,        ///< FIFO
    fileInfoTypeLink,        ///< Символическая ссылка
    fileInfoTypeSock,        ///< Сокет
    fileInfoTypeCount        ///< Количество типов файлов
}fileInfoTypesEnum;

/*
    Структуры
*/

#pragma pack (push, 1)

/// @brief      Структура ячейки доступа к файлу
typedef struct fileInfoAccessBitsStruct
{
    uint8_t special : 1; ///< Специальные биты
    uint8_t execute : 1; ///< Доступ на исполнение
    uint8_t write   : 1; ///< Доступ на запись
    uint8_t read    : 1; ///< Доступ на чтение
}fileInfoAccessBitsStruct;

/// @brief      Объединение доступа к файлу
typedef union fileInfoAccessUnion
{
    uint8_t                  value : 4; ///< Численное представление доступа к файлу
    fileInfoAccessBitsStruct bits;      ///< Битовое   представление доступа к файлу
}fileInfoAccessUnion;

/// @brief      Структура прав доступа
typedef struct fileInfoAccessStruct
{
    fileInfoAccessUnion owner; ///< Доступ владельца
    fileInfoAccessUnion group; ///< Доступ группы
    fileInfoAccessUnion other; ///< Доступ прочих
}fileInfoAccessStruct;

/// @brief      Структура информации о цели
typedef struct fileInfoTargetStruct
{
    fileInfoTypesEnum    type;           ///< Тип файла
    fileInfoAccessStruct access;         ///< Права доступа
    char                *filePathPtr;    ///< Указатель на строку с полным путем к файлу
    char                *fileNamePtr;    ///< Указатель на позицию имени файла в filePathPtr
    bool                 isTargetExists; ///< Флаг существования цели ссылки
}fileInfoTargetStruct;

/// @brief      Структура информации о файле
typedef struct fileInfoStruct
{
    fileInfoTypesEnum    type;           ///< Тип файла
    fileInfoAccessStruct access;         ///< Права доступа
    uint32_t             linksCount;     ///< Количество жестких ссылок
    uint32_t             ownerId;        ///< Id владельца файла
    uint32_t             groupId;        ///< Id группы файла
    int64_t              size;           ///< Размер файла
    time_t               timeEdit;       ///< Время последнего изменения файла
    char                *fileNamePtr;    ///< Указатель на строку с именем файла
    fileInfoTargetStruct targetInfo;     ///< Информация о цели ссылки
    int64_t              blocks;         ///< Количество занимаемых файлом 512 байтовых блоков
    __uint64_t           deviceNumber;   ///< Номер устройства
}fileInfoStruct;

#pragma pack (pop)

/*
    Прототипы функций получения информации о файле
*/

/// @brief      Функция проверки существования файла
/// @details    Данная функция выполняет проверку существования файла
/// @param[in]  filePtr Указатель на файл
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает true если файл существует.
///                 В противном случае, возвращает false
bool fileInfoIsExists(const char *filePtr, bool *isOkPtr);

/// @brief      Функция получения всей информации о файле filePtr
/// @details    Данная функция выполняет вызов fileInfoSetActiveFile() с filePtr в качестве аргумента,
///                 затем последовательно заполняет структуру fileInfoPtr,
///                 выполняя вызовы соответствующих fileInfoGet функций
/// @param[in]  filePtr      Указатель на путь к файлу
/// @param[out] fileInfoPtr  Указатель на информацию о файле
/// @param[in]  isFollowLink Флаг следования по ссылке до конца
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @warning    Для инициализации fileNamePtr и targetInfo.filePathPtr используется malloc!
///                 Не забудьте очистить память при удалении fileInfoPtr, если функция вернула true!
void fileInfoGet(const char *filePtr, fileInfoStruct *fileInfoPtr, bool isFollowLink, bool *isOkPtr);

/// @brief      Функция установки активного файла
/// @details    Данная функция выполняет запись filePtr и полученных при помощи lstat() данных в 
///                 fileInfoPath и fileInfoStat соответственно
/// @warning    Данная функция использует malloc!
///                 Не забудьте очистить память при выходе из программы, вызвав функцию fileInfoClearActiveFile()! 
/// @param[in]  filePtr Указатель на путь к файлу
/// @return     Возвращает true если задать активный файл как filePtr удалось.
///                 В противном случае, возвращает false
bool fileInfoSetActiveFile(const char *filePtr);

/// @brief      Функция сброса активного файла
/// @details    Данная функция выполняет сброс активного файла и очистку занятых ресурсов
void fileInfoClearActiveFile(void);

/// @brief      Функция получения типа файла
/// @details    Данная функция выполняет получения типа активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает тип активного файла
fileInfoTypesEnum fileInfoGetType(bool *isOkPtr);

/// @brief      Функция получения прав доступа файла
/// @details    Данная функция выполняет получения прав доступа активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает прав доступа активного файла
fileInfoAccessStruct fileInfoGetAccess(bool *isOkPtr);

/// @brief      Функция получения количества жестких ссылок на файл
/// @details    Данная функция выполняет получение количества жестких ссылок на активный файл
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает количество жестких ссылок на активный файл
uint32_t fileInfoGetLinksCount(bool *isOkPtr);

/// @brief      Функция получения Id владельца файла
/// @details    Данная функция выполняет получение Id владельца активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает Id владельца активного файла
uint32_t fileInfoGetOwnerId(bool *isOkPtr);

/// @brief      Функция получения Id группы файла
/// @details    Данная функция выполняет получение Id группы активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает Id группы активного файла
uint32_t fileInfoGetGroupId(bool *isOkPtr);

/// @brief      Функция получения размера файла
/// @details    Данная функция выполняет получение размера активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает размер активного файла
uint32_t fileInfoGetSize(bool *isOkPtr);

/// @brief      Функция получения номера устройства
/// @details    Данная функция выполняет получение номера устройства активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает номер устройства активного файла
__uint64_t fileInfoGetDeviceNumber(bool *isOkPtr);

/// @brief      Функция получения размера файла
/// @details    Данная функция выполняет получение размера активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает размер активного файла
uint32_t fileInfoGetSize(bool *isOkPtr);

/// @brief      Функция получения времени изменения файла
/// @details    Данная функция выполняет получение времени последнего изменения активного файла
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает время изменения активного файла
time_t fileInfoGetTimeEdit(bool *isOkPtr);

/// @brief      Функция получения цели символической ссылки
/// @details    Данная функция выполняет чтение цели символической ссылки и записывает ее в строку stringPtr длинной stringLength
/// @param[out] stringPtr     Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength  Длина строки stringPtr
/// @param[out] isOkPtr       Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoGetLinkTarget(char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения количества занимаемых блоков
/// @details    Данная функция выполняет получение количества занимаемых активным файлом 512 байтовых блоков
/// @param[out] isOkPtr Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает количество занимаемых активным файлом блоков
uint32_t fileInfoGet512BytesBlocks(bool *isOkPtr);

/*
    Прототипы функций получения строкового представления информации о файле
*/

/// @brief      Функция получения строкового представления всей информации о файле
/// @details    Данная функция выполняет запись fileInfoPtr в строку stringPtr длинной stringLength,
///                 выполняя вызовы соответствующих fileInfoToString функций.
///                 Порядок информации в stringPtr соответствует порядку полей в fileInfoPtr.
///                 Разделителем полей является FILE_INFO_TO_STRING_DELIMER
/// @param[in]  fileInfoPtr  Указатель на информацию о файле
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToString(const fileInfoStruct *fileInfoPtr, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового представления типа файла
/// @details    Данная функция выполняет запись type в строку stringPtr длинной stringLength
/// @param[in]  type         Тип файла
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @warning    Длина stringPtr должна быть хотя бы 2 байта. 1 на символ типа и 1 под \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringType(fileInfoTypesEnum type, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового представления структуры доступа к файлу
/// @details    Данная функция выполняет запись accessPtr в строку stringPtr длинной stringLength
/// @param[in]  accessPtr    Указатель на структуру доступа к файлу
/// @param[in]  type         Тип файла
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @warning    Длина stringPtr должна быть хотя бы 10 байт. 9 на биты доступа и 1 под \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringAccess(const fileInfoAccessStruct *accessPtr, fileInfoTypesEnum type, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового количества жестких ссылок на файл
/// @details    Данная функция выполняет запись linksCount в строку stringPtr длинной stringLength
/// @param[in]  linksCount   Количество жестких ссылок на файл
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringLinksCount(uint32_t linksCount, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового представления Id владельца файла
/// @details    Данная функция выполняет перевод ownerId в строку stringPtr длинной stringLength
/// @param[in]  ownerId      Id владельца файла
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringOwnerId(uint32_t ownerId, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового представления Id группы файла
/// @details    Данная функция выполняет перевод groupId в строку stringPtr длинной stringLength
/// @param[in]  groupId      Id группы файла
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringGroupId(uint32_t groupId, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового размера файла
/// @details    Данная функция выполняет запись size в строку stringPtr длинной stringLength
/// @param[in]  size         Размер файла
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringSize(uint32_t size, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового размера файла
/// @details    Данная функция выполняет запись size в строку stringPtr длинной stringLength
/// @param[in]  deviceNumber Номер устройства
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringDeviceNumber(__uint64_t deviceNumber, char *stringPtr, size_t stringLength, bool *isOkPtr);

/// @brief      Функция получения строкового представления времени последнего изменения файла
/// @details    Данная функция выполняет перевод timeEdit в строку stringPtr длинной stringLength
/// @param[in]  timeEdit     Время последнего изменения файла
/// @param[out] stringPtr    Указатель на строку, куда будет записан результат с \0
/// @param[in]  stringLength Длина строки stringPtr
/// @param[out] isOkPtr      Указатель на флаг успешного выполнения операции. Может быть равен 0
/// @return     Возвращает длинну stringPtr
size_t fileInfoToStringTimeEdit(time_t timeEdit, char *stringPtr, size_t stringLength, bool *isOkPtr);

// _FILE_INFO_H_
#endif