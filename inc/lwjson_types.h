#ifndef LWJSON_TYPES_H
#define LWJSON_TYPES_H

#ifdef __cplusplus
extern "C"{
#endif

typedef enum {
    LWJSON_VAL_STRING,              // String
    LWJSON_VAL_NUMBER,              // Número
    LWJSON_VAL_BOOLEAN,             // Boolean
    LWJSON_VAL_OBJECT,              // Objeto
    LWJSON_VAL_ARRAY,               // Array
    LWJSON_VAL_NULL                 // Null
} LwJsonValueType;

typedef union {
    char *valueString;              // Puntero a cadena
    int64_t valueInt;               // Valor numérico
    bool valueBool;                 // Valor booleano
} LwJsonValue;

#ifdef __cplusplus
}
#endif

#endif
