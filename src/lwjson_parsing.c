#include "lwjson.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

typedef enum {
    LWJSON_PARENT_OBJECT,           // Tipo objeto
    LWJSON_PARENT_ARRAY             // Tipo array
} LwJsonParentType;

typedef enum {
    LWJSON_SM_START,                // Comienzo de parsing
    LWJSON_SM_OBJECT,               // Objeto
    LWJSON_SM_NAME,                 // Nombre de variable
    LWJSON_SM_NAME_END,             // Fin de nombre de variable
    LWJSON_SM_VALUE,                // Valor
    LWJSON_SM_ARRAY,                // Array
    LWJSON_SM_STRING,               // String
    LWJSON_SM_VALUE_END,            // Fin de valor
    LWJSON_SM_NUMBER,               // Número
    LWJSON_SM_OBJECT_END,           // Fin de objeto
    LWJSON_SM_ARRAY_END,            // Fin de array
    LWJSON_SM_END,                  // Fin de máquina de estados
    LWJSON_SM_ERROR                 // Error de parsing
} LwJsonParserSM;

typedef struct {
    uint32_t offset;
    uint32_t len;
    LwJsonValueType type;
} LwJsonFindResult;

typedef struct {
    const char **path;
    const LwJsonMsg *msg;
    LwJsonParentType stack[LWJSON_DEPTH_MAX + 1];   // Array con el tipo de padre del objeto actual (útil en parsing)
    LwJsonParserSM state;                           // Estado actual para la máquina de estados
    uint32_t depth;                                 // Valor de profundidad actual en la búsqueda o parsing
    uint32_t findDepth;                             // Valor de profundidad de la última coincidencia con path
    uint32_t searchDepth;                           // Profundidad de path
    bool searchValuePending;                        // Path encontrado. Lectura de valor pendiente
    bool found;                                     // Flag que indica si se ha encontrado
    char *lastName;                                 // Puntero a último nombre de propiedad
    uint32_t currentArrayIndex;                     // Índice de elemento en array
    char *p;                                        // Puntero al caracter actual
    LwJsonFindResult *findResult;
} LwJsonParser;



static int lwJsonFindValue(const char **path, const LwJsonMsg *msg, LwJsonValueType expectedType, LwJsonMsg *value);
static int lwJsonFind(const char **path, const LwJsonMsg *msg, LwJsonFindResult *findResult);
static uint32_t lwJsonCalculatePathDepth(const char **path);
static void PrefilterChar(char *c);
static bool SkippableChar(char c);
static void FindSmStartHandler(LwJsonParser *parser);
static void FindSmObjectHandler(LwJsonParser *parser);
static void FindSmArrayHandler(LwJsonParser *parser);
static void FindSmNameHandler(LwJsonParser *parser);
static void FindSmNameEndHandler(LwJsonParser *parser);
static void FindSmValueHandler(LwJsonParser *parser);
static void FindSmStringHandler(LwJsonParser *parser);
static void FindSmNumberHandler(LwJsonParser *parser);
static void FindSmValueEndHandler(LwJsonParser *parser);
static void FindSmLevelEndHandler(LwJsonParser *parser);
static void lwJsonParserPush(LwJsonParser *parser, LwJsonParentType parent);
static void lwJsonParserPop(LwJsonParser *parser, LwJsonParentType expectedParent);
static int GetArrayLen(const LwJsonMsg *jsonArray);
static int GetIntArray(const LwJsonMsg *jsonArray, int *intArray, uint32_t intArrayLen);
static int GetStringArray(const LwJsonMsg *jsonArray, char **stringArray, uint32_t *stringLenArray, uint32_t arrayLen);


int lwJsonGetObject(const char **path, const LwJsonMsg *msg, LwJsonMsg *object) {

    return lwJsonFindValue(path, msg, LWJSON_VAL_OBJECT, object);
}

int lwJsonGetArray(const char **path, const LwJsonMsg *msg, LwJsonMsg *array) {
    return lwJsonFindValue(path, msg, LWJSON_VAL_ARRAY, array);
}

int lwJsonGetArrayLen(const char **path, const LwJsonMsg *msg) {
    int result;
    LwJsonMsg jsonArray;

    result = lwJsonFindValue(path, msg, LWJSON_VAL_ARRAY, &jsonArray);
    if (result != 0) {
        return result;
    }

    return GetArrayLen(&jsonArray);
}

int lwJsonGetIntArray(const char **path, const LwJsonMsg *msg, int *intArray, uint32_t intArrayLen) {
    int result;
    LwJsonMsg jsonArray;

    result = lwJsonFindValue(path, msg, LWJSON_VAL_ARRAY, &jsonArray);
    if (result != 0) {
        return result;
    }

    return GetIntArray(&jsonArray, intArray, intArrayLen);
}

int lwJsonGetStringArray(const char **path, const LwJsonMsg *msg, char **stringArray, uint32_t *stringLenArray, uint32_t arrayLen) {
    int result;
    LwJsonMsg jsonArray;

    result = lwJsonFindValue(path, msg, LWJSON_VAL_ARRAY, &jsonArray);
    if (result != 0) {
        return result;
    }

    return GetStringArray(&jsonArray, stringArray, stringLenArray, arrayLen);
}

int lwJsonGetString(const char **path, const LwJsonMsg *msg, char *value, uint32_t valueLen) {
    int result;
    uint32_t stringLen;
    LwJsonMsg jsonString;

    result = lwJsonFindValue(path, msg, LWJSON_VAL_STRING, &jsonString);
    if (result != 0) {
        return result;
    }

    // Check size
    stringLen = jsonString.len - 2;
    if (stringLen > valueLen) {
        return -ENOMEM;
    }

    // Get String
    strncpy(value, jsonString.string + 1, stringLen);
    value[stringLen] = 0;

    return 0;
}

int lwJsonGetInt(const char **path, const LwJsonMsg *msg, int *value) {
    int result;
    LwJsonMsg jsonNumber;

    result = lwJsonFindValue(path, msg, LWJSON_VAL_NUMBER, &jsonNumber);
    if (result != 0) {
        return result;
    }

    // Get integer
    (*value) = atoi(jsonNumber.string);

    return 0;
}

int lwJsonGetBool(const char **path, const LwJsonMsg *msg, bool *value) {
    int result;
    LwJsonMsg jsonBoolean;

    result = lwJsonFindValue(path, msg, LWJSON_VAL_BOOLEAN, &jsonBoolean);
    if (result != 0) {
        return result;
    }

    // Get Value
    if (strncmp(jsonBoolean.string, "true", strlen("true")) == 0) {
        (*value) = true;
    } else if (strncmp(jsonBoolean.string, "false", strlen("false")) == 0) {
        (*value) = false;
    } else {
        return -EPERM;
    }

    return 0;
}


static int lwJsonFindValue(const char **path, const LwJsonMsg *msg, LwJsonValueType expectedType, LwJsonMsg *value) {
    int result;
    LwJsonFindResult findResult;

    if (msg == NULL || value == NULL) {
        return -EINVAL;
    }

    result = lwJsonFind(path, msg, &findResult);
    if (result != 0) {
        return result;
    }
    if (findResult.type != expectedType) {
        return -EPERM;
    }

    value->string = &msg->string[findResult.offset];
    value->len = findResult.len;

    return 0;
}

static int lwJsonFind(const char **path, const LwJsonMsg *msg, LwJsonFindResult *findResult) {
    LwJsonParser parser;

    if (path == NULL || msg == NULL || findResult == NULL) {
        return -EINVAL;
    }

    // Init Parser
    parser.searchDepth = lwJsonCalculatePathDepth(path);
    if (parser.searchDepth > LWJSON_DEPTH_MAX) {
        return -EPERM;
    }
    parser.path = path;
    parser.msg = msg;
    parser.depth = 0;
    parser.findDepth = 0;
    parser.state = LWJSON_SM_START;
    parser.findResult = findResult;

    for (parser.p = msg->string; (parser.p[0] != 0) && ((parser.p - msg->string) < msg->len); parser.p++) {
        // Filter chars
        PrefilterChar(parser.p);
        if (SkippableChar(parser.p[0])) {
            continue;
        }

        switch (parser.state) {
        case LWJSON_SM_START:
            FindSmStartHandler(&parser);
            break;
        case LWJSON_SM_OBJECT:
            FindSmObjectHandler(&parser);
            break;
        case LWJSON_SM_ARRAY:
            FindSmArrayHandler(&parser);
            break;
        case LWJSON_SM_NAME:
            FindSmNameHandler(&parser);
            break;
        case LWJSON_SM_NAME_END:
            FindSmNameEndHandler(&parser);
            break;
        case LWJSON_SM_VALUE:
            FindSmValueHandler(&parser);
            break;
        case LWJSON_SM_STRING:
            FindSmStringHandler(&parser);
            break;
        case LWJSON_SM_NUMBER:
            FindSmNumberHandler(&parser);
            break;
        case LWJSON_SM_VALUE_END:
            FindSmValueEndHandler(&parser);
            break;
        case LWJSON_SM_OBJECT_END:
        case LWJSON_SM_ARRAY_END:
            FindSmLevelEndHandler(&parser);
            break;
        default:
            return -EPERM;
        }
    }

    if (parser.state != LWJSON_SM_END) {
        return -EPERM;
    }
    //Comprobar si no se ha encontrado
    if (parser.findDepth < parser.searchDepth) {
        return -ENOENT;
    }
    return 0;
}

static void PrefilterChar(char *p) {
    // Substitute LF and CR with space. It avoids problems in array parsing
    if ((*p) == '\r' || (*p) == '\n') {
        (*p) = ' ';
    }
}

static bool SkippableChar(char c) {

    if (c == '\t' || c == ' ') {
        return true;
    }

    return false;
}

static void FindSmStartHandler(LwJsonParser *parser) {
    // Inicio. Se debe encontrar '{'
    if (parser->p[0] == '{') {
        parser->state = LWJSON_SM_OBJECT;
        lwJsonParserPush(parser, LWJSON_PARENT_OBJECT);
    } else if (parser->p[0] == '[') {
        parser->state = LWJSON_SM_ARRAY;
        lwJsonParserPush(parser, LWJSON_PARENT_ARRAY);
    } else {
        parser->state = LWJSON_SM_ERROR;
    }
}

static void FindSmObjectHandler(LwJsonParser *parser) {

    // Inicio de objeto. Se debe encontrar el incio del nombre '"'
    if (parser->p[0] == '"') {
        parser->state = LWJSON_SM_NAME;
        parser->lastName = &parser->p[1];
    } else if (parser->p[0] == '}') {
        parser->state = LWJSON_SM_OBJECT_END;
        parser->p--;
    } else {
        parser->state = LWJSON_SM_ERROR;
    }
}

static void FindSmArrayHandler(LwJsonParser *parser) {

    if (parser->p[0] == ']') {
        parser->state = LWJSON_SM_ARRAY_END;
    } else {
        parser->state = LWJSON_SM_VALUE;
    }
    parser->p--;
}

static void FindSmNameHandler(LwJsonParser *parser) {
    uint32_t nameLen;

    while (parser->state == LWJSON_SM_NAME) {
        // Nombre. Puede encontrarse un carácter válido o el fin de nombre
        if ((parser->p[0]) == '"') {
            parser->state = LWJSON_SM_NAME_END;
            if ((parser->depth == (parser->findDepth + 1)) && (parser->findDepth < parser->searchDepth)) {
                // Comprobar que las longitudes coinciden
                nameLen = parser->p - parser->lastName;
                if (strlen(parser->path[parser->findDepth]) == nameLen) {
                    // Comprobar que las cadenas coinciden
                    if (strncmp(parser->path[parser->findDepth], parser->lastName, nameLen) == 0) {
                        parser->findDepth++;
                        if (parser->findDepth == parser->searchDepth) {
                            parser->searchValuePending = true;
                        }
                    }
                }
            }
        } else if ((parser->p[0]) < 32) {
            parser->state = LWJSON_SM_ERROR;
        } else {
            parser->p++;
        }
    }
}

static void FindSmNameEndHandler(LwJsonParser *parser) {
    if((parser->p[0]) == ':') {
        parser->state = LWJSON_SM_VALUE;
    } else {
        parser->state = LWJSON_SM_ERROR;
    }
}

static void FindSmValueHandler(LwJsonParser *parser) {
    char currentChar;
    LwJsonValueType tempType;
    char findArrayString[10];

    // Guardar offset si es necesario
    if (parser->searchValuePending) {
        parser->findResult->offset = (parser->p - parser->msg->string);
    }

    currentChar = parser->p[0];

    // Valor. Varias opciones
    if (currentChar == '\"') {
        parser->state = LWJSON_SM_STRING;
        tempType = LWJSON_VAL_STRING;
    } else if ((currentChar == '-') || ((currentChar >= '0') && (currentChar <='9'))) {
        // Sólo soporta enteros
        parser->state = LWJSON_SM_NUMBER;
        tempType = LWJSON_VAL_NUMBER;
    } else if (currentChar == '[') {
        tempType = LWJSON_VAL_ARRAY;
        parser->state = LWJSON_SM_ARRAY;
        lwJsonParserPush(parser, LWJSON_PARENT_ARRAY);
        if ((parser->depth == (parser->findDepth + 1)) && (parser->findDepth < parser->searchDepth)) {
            // Comprobar si se busca este índice de array
            sprintf(findArrayString, "[%u]", parser->currentArrayIndex);
            if (strcmp(parser->path[parser->findDepth], findArrayString) == 0) {
                parser->findDepth++;
            }
        }
    } else if (currentChar == '{') {
        tempType = LWJSON_VAL_OBJECT;
        parser->state = LWJSON_SM_OBJECT;
        lwJsonParserPush(parser, LWJSON_PARENT_OBJECT);
    } else if (strncmp(parser->p, "true", strlen("true")) == 0) {
        tempType = LWJSON_VAL_BOOLEAN;
        parser->p += strlen("true") - 1;
        parser->state = LWJSON_SM_VALUE_END;
    } else if (strncmp(parser->p, "false", strlen("false")) == 0) {
        tempType = LWJSON_VAL_BOOLEAN;
        parser->p += strlen("false") - 1;
        parser->state = LWJSON_SM_VALUE_END;
    } else {
        parser->state = LWJSON_SM_ERROR;
    }

    // Actualizar tipo e indicar fin
    if (parser->searchValuePending) {
        parser->searchValuePending = false;
        parser->found = true;
        parser->findResult->type = tempType;
    }
}

static void FindSmStringHandler(LwJsonParser *parser) {
    // Valor string. Puede encontrarse un carácter válido o el fin de nombre
    while (parser->state == LWJSON_SM_STRING) {
        if ((parser->p[0]) == '"') {
            parser->state = LWJSON_SM_VALUE_END;
        } else if ((parser->p[0]) < 32) {
            parser->state = LWJSON_SM_ERROR;
        } else {
            parser->p++;
        }
    }
}

static void FindSmNumberHandler(LwJsonParser *parser) {
    while (parser->state == LWJSON_SM_NUMBER) {
        // Sólo se soportan enteros. Si no se encuentra un entero, se pasa directamente a VALUE_END
        if (((parser->p[0]) >= '0') && ((parser->p[0]) <= '9')) {
            parser->p++;
        } else {
            parser->state = LWJSON_SM_VALUE_END;
            parser->p--;
        }
    }
}

static void FindSmValueEndHandler(LwJsonParser *parser) {
    char findArrayString[10];
    char c;

    if (parser->found) {
        if (parser->depth == parser->searchDepth) {
            parser->findResult->len = (parser->p - parser->msg->string) - parser->findResult->offset;
            parser->found = false;
        }
    }

    // Sanity check
    if (parser->depth == 0) {
        return;
    }

    c = parser->stack[parser->depth - 1];
    if (c == LWJSON_PARENT_OBJECT) {
        // Another attribute or object end accepted
        if ((*parser->p) == ',') {
            parser->state = LWJSON_SM_OBJECT;
        } else if((*parser->p) == '}') {
            parser->state = LWJSON_SM_OBJECT_END;
            lwJsonParserPop(parser, LWJSON_PARENT_OBJECT);
        } else {
            parser->state = LWJSON_SM_ERROR;
        }
    } else if (c == LWJSON_PARENT_ARRAY) {
        // Another item or array end accepted
        if ((*parser->p) == ',') {
            parser->state = LWJSON_SM_VALUE;
            if ((parser->depth == (parser->findDepth + 1)) && (parser->findDepth < parser->searchDepth)) {
                // Update array search Index
                parser->currentArrayIndex++;
                // Comprobar si se busca este índice de array
                sprintf(findArrayString, "[%u]", parser->currentArrayIndex);
                if (strcmp(parser->path[parser->findDepth], findArrayString) == 0) {
                    parser->findDepth++;
                }
            }
        } else if ((*parser->p) == ']') {
            parser->state = LWJSON_SM_ARRAY_END;
            lwJsonParserPop(parser, LWJSON_PARENT_ARRAY);
        } else {
            parser->state = LWJSON_SM_ERROR;
        }
    }
}

static void FindSmLevelEndHandler(LwJsonParser *parser) {
    if ((parser->findDepth < parser->searchDepth) && (parser->depth == parser->findDepth)) {
        // Start search again
        parser->findDepth = 0;
    }
    if(parser->found) {
        if(parser->depth == parser->searchDepth) {
            parser->findResult->len = (parser->p - parser->msg->string) - parser->findResult->offset;
            parser->found = false;
        }
    }
    parser->p--;
    parser->state = LWJSON_SM_VALUE_END;
}

static uint32_t lwJsonCalculatePathDepth(const char **path) {
    uint32_t result = 0;

    while (result <= LWJSON_DEPTH_MAX) {
        if (path[result] == NULL) {
            break;
        }
        result++;
    }

    return result;
}

static void lwJsonParserPush(LwJsonParser *parser, LwJsonParentType parent) {
    // Actualizar parser path
    parser->stack[parser->depth] = parent;
    parser->depth++;

    if (parent == LWJSON_PARENT_ARRAY) {
        parser->currentArrayIndex = 0;
    }

    if (parser->depth > LWJSON_DEPTH_MAX) {
        parser->state = LWJSON_SM_ERROR;
    }
}

static void lwJsonParserPop(LwJsonParser *parser, LwJsonParentType expectedParent) {
    if (parser->depth == 0) {
        parser->state = LWJSON_SM_ERROR;
    }
    parser->depth--;

    if (parser->stack[parser->depth] != expectedParent) {
        parser->state = LWJSON_SM_ERROR;
    } else if (parser->depth == 0) {
        // End
        parser->state = LWJSON_SM_END;
    }
}

static int GetArrayLen(const LwJsonMsg *jsonArray) {
    uint32_t level = 0;
    uint32_t items = 0;
    char *p, *arrayEnd;

    p = jsonArray->string;
    arrayEnd = &p[jsonArray->len];

    // Go to array start
    for (; p < arrayEnd; p++) {
        if (*p == '[') {
            p++;
            break;
        }
    }
    // Look for first item
    for(; p < arrayEnd; p++) {
        if (*p == '\t' || *p == '\r' || *p == '\n' || *p == ' ') {
            continue;
        } else if (*p != ']') {
            items = 1;
            break;
        }
    }
    // Count rest of items
    for (; p < arrayEnd; p++) {
        if (*p == '{' || *p == '[') {
            level++;
        } else if (*p == '}' || *p == ']') {
            level--;
        } else if (*p == ',' && level == 0) {
            items++;
        }
    }

    return items;
}

static int GetIntArray(const LwJsonMsg *jsonArray, int *intArray, uint32_t intArrayLen) {
    uint32_t i, index = 0;
    uint32_t count;
    char *p;
    enum {
        INT_ARRAY_INIT,
        INT_ARRAY_OPEN,
        INT_ARRAY_NEW_ITEM,
        INT_ARRAY_ITEM_END,
        INT_ARRAY_CLOSE
    } sm = INT_ARRAY_INIT;

    if (intArray == NULL) {
        return -EINVAL;
    }

    // Extract array values
    p = jsonArray->string;
    for (i = 0; i < jsonArray->len; i++) {
        if(sm == INT_ARRAY_CLOSE) {
            break;
        }
        switch (sm) {
        case INT_ARRAY_INIT:
            if (p[i] == '[') {
                sm = INT_ARRAY_NEW_ITEM;
            }
            break;
        case INT_ARRAY_NEW_ITEM:
            if (p[i] >= '0' && p[i] <= '9') {
                // Sanity check
                if (index < intArrayLen) {
                    intArray[index] = atoi(&p[i]);
                    index++;
                    sm = INT_ARRAY_ITEM_END;
                } else {
                    return -EPERM;
                }
            } else if (p[i] != ' ' && p[i] != '\r' && p[i] != '\n' && p[i] != '\t') {
                return -EPERM;
            }
            break;
        case INT_ARRAY_ITEM_END:
            if (p[i] == ',') {
                sm = INT_ARRAY_NEW_ITEM;
            } else if (p[i] == ']') {
                sm = INT_ARRAY_CLOSE;
            }
            break;
        default:
            break;
        }
    }

    // Check end of array reached
    if (sm != INT_ARRAY_CLOSE) {
        return -EPERM;
    }

    // Return number of items
    count = index;
    return count;
}

static int GetStringArray(const LwJsonMsg *jsonArray, char **stringArray, uint32_t *stringLenArray, uint32_t arrayLen) {
    uint32_t i, index = 0;
    uint32_t count;
    char *p;
    enum {
        STR_ARRAY_INIT,
        STR_ARRAY_OPEN,
        STR_ARRAY_NEW_ITEM,
        STR_ARRAY_ITEM,
        STR_ARRAY_ITEM_CLOSE,
        STR_ARRAY_CLOSE
    } sm = STR_ARRAY_INIT;

    // Check inputs
    if ((stringArray == NULL) || (stringLenArray == NULL)) {
        return -EINVAL;
    }

    // Extract array values
    p = jsonArray->string;
    for (i = 0; i < jsonArray->len; i++) {
        if(sm == STR_ARRAY_CLOSE) {
            break;
        }
        switch (sm) {
        case STR_ARRAY_INIT:
            if (p[i] == '[') {
                sm = STR_ARRAY_NEW_ITEM;
            }
            break;
        case STR_ARRAY_NEW_ITEM:
            if (p[i] == '\"') {
                // Sanity check
                if (index < arrayLen) {
                    stringArray[index] = &p[i];
                    sm = STR_ARRAY_ITEM;
                } else {
                    return -EPERM;
                }
            } else if (p[i] != ' ' && p[i] != '\r' && p[i] != '\n' && p[i] != '\t') {
                return -EPERM;
            }
            break;
        case STR_ARRAY_ITEM:
            if (p[i] == '\"') {
                // Sanity check
                if (index < arrayLen) {
                    stringLenArray[index] = &p[i] - stringArray[index] + 1;
                    index++;
                    sm = STR_ARRAY_ITEM_CLOSE;
                } else {
                    return -EPERM;
                }
            }
            break;
        case STR_ARRAY_ITEM_CLOSE:
            if (p[i] == ',') {
                sm = STR_ARRAY_NEW_ITEM;
            } else if (p[i] == ']') {
                sm = STR_ARRAY_CLOSE;
            }
            break;
        default:
            break;
        }
    }

    // Check end of array reached
    if (sm != STR_ARRAY_CLOSE) {
        return -EPERM;
    }

    // Return number of items
    count = index;
    return count;

}

