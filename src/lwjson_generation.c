#include "lwjson.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static int lwJsonCalculateValueStringLength(LwJsonValueType type, LwJsonValue *value);
static int lwJsonCheckWriteError(LwJsonMsg *msg);
static int lwJsonAddNameAndValuePair(LwJsonMsg *msg, const char *name, LwJsonValueType type, LwJsonValue *value);
static int lwJsonAddValueToArray(LwJsonMsg *msg, LwJsonValueType type, LwJsonValue *value);
static int lwJsonIsValidUTF8String(const char *string);

int lwJsonWriteStart(LwJsonMsg *msg) {
    if (msg == NULL) {
        return -EINVAL;
    }
    if (msg->string == NULL) {
        return -EINVAL;
    }

    msg->_offset = 0;
    msg->_lastError = 0;

    return 0;
}

int lwJsonWriteEnd(LwJsonMsg *msg) {
    if (msg == NULL) {
        return -EPERM;
    }
    if (msg->string == NULL) {
        return -EINVAL;
    }

    if (msg->_lastError < 0) {
        return msg->_lastError;
    }

    msg->string[msg->_offset] = 0;

    return 0;
}

void LwJsonWriteApplyOffset(LwJsonMsg *msg, uint32_t offset) {
    if (msg == NULL) {
        return;
    }

    msg->_offset += offset;
}

int lwJsonStartObject(LwJsonMsg *msg) {
    if (lwJsonCheckWriteError(msg) != 0) {
        return msg->_lastError;
    }

    msg->string[msg->_offset] = '{';
    msg->_offset++;
    return 0;
}

int lwJsonStartArray(LwJsonMsg *msg) {
    if (lwJsonCheckWriteError(msg) != 0) {
        return msg->_lastError;
    }

    msg->string[msg->_offset] = '[';
    msg->_offset++;
    return 0;
}

int lwJsonCloseObject(LwJsonMsg *msg) {
    if (lwJsonCheckWriteError(msg) != 0) {
        return msg->_lastError;
    }

    msg->string[msg->_offset] = '}';
    msg->_offset++;
    return 0;
}

int lwJsonCloseArray(LwJsonMsg *msg) {
    if (lwJsonCheckWriteError(msg) != 0) {
        return msg->_lastError;
    }

    msg->string[msg->_offset] = ']';
    msg->_offset++;
    return 0;
}

int lwJsonAddStringToObject(LwJsonMsg *msg, const char *name, const char *string) {
    char defaultString[] = "";
    LwJsonValueType type = LWJSON_VAL_STRING;
    LwJsonValue jsonValue;
    if (lwJsonIsValidUTF8String(string)) {
        jsonValue.valueString = (char*)string;
    } else {
        jsonValue.valueString = (char*)defaultString;
    }

    return  lwJsonAddNameAndValuePair(msg, name, type, &jsonValue);
}

int lwJsonAddStringToArray(LwJsonMsg *msg, const char *string) {
    char defaultString[] = "";
    LwJsonValueType type = LWJSON_VAL_STRING;
    LwJsonValue jsonValue;
    if (lwJsonIsValidUTF8String(string)) {
        jsonValue.valueString = (char*)string;
    } else {
        jsonValue.valueString = (char*)defaultString;
    }

    return lwJsonAddValueToArray(msg, type, &jsonValue);
}

int lwJsonAddIntToObject(LwJsonMsg *msg, const char *name, long long value) {
    LwJsonValueType type = LWJSON_VAL_NUMBER;
    LwJsonValue jsonValue;
    jsonValue.valueInt = value;
    return lwJsonAddNameAndValuePair(msg, name, type, &jsonValue);
}

int lwJsonAddBooleanToObject(LwJsonMsg *msg, const char *name, bool boolean) {
    LwJsonValueType type = LWJSON_VAL_BOOLEAN;
    LwJsonValue jsonValue;
    jsonValue.valueBool = boolean;
    return lwJsonAddNameAndValuePair(msg, name, type, &jsonValue);
}

int lwJsonAddIntToArray(LwJsonMsg *msg, long long value) {
    LwJsonValueType type = LWJSON_VAL_NUMBER;
    LwJsonValue jsonValue;
    jsonValue.valueInt = value;
    return lwJsonAddValueToArray(msg, type, &jsonValue);
}

int lwJsonAddBooleanToArray(LwJsonMsg *msg, bool boolean) {
    LwJsonValueType type = LWJSON_VAL_BOOLEAN;
    LwJsonValue jsonValue;
    jsonValue.valueBool = boolean;
    return lwJsonAddValueToArray(msg, type, &jsonValue);
}

int lwJsonAddObjectToObject(LwJsonMsg *msg, const char *name) {
    LwJsonValueType type = LWJSON_VAL_OBJECT;
    return lwJsonAddNameAndValuePair(msg, name, type, NULL);
}

int lwJsonAddObjectToArray(LwJsonMsg *msg) {
    LwJsonValueType type = LWJSON_VAL_OBJECT;
    return lwJsonAddValueToArray(msg, type, NULL);
}

int lwJsonAddArrayToObject(LwJsonMsg *msg, const char *name) {
    LwJsonValueType type = LWJSON_VAL_ARRAY;
    return lwJsonAddNameAndValuePair(msg, name, type, NULL);
}

int lwJsonAddArrayToArray(LwJsonMsg *msg) {
    LwJsonValueType type = LWJSON_VAL_ARRAY;
    return lwJsonAddValueToArray(msg, type, NULL);
}

int lwJsonAddNullToObject(LwJsonMsg *msg, const char *name) {
    LwJsonValueType type = LWJSON_VAL_NULL;
    return lwJsonAddNameAndValuePair(msg, name, type, NULL);
}

int lwJsonAddNullToArray(LwJsonMsg *msg) {
    LwJsonValueType type = LWJSON_VAL_NULL;
    return lwJsonAddValueToArray(msg, type, NULL);
}

int lwJsonAppendObject(LwJsonMsg *msg, const char *name, const char *objString) {
    uint32_t objLen;
    uint32_t nameLen;
    uint32_t requiredLen;
    bool separator = false;

    if (name == NULL || objString == NULL) {
        return -EINVAL;
    }

    objLen = strlen(objString);
    nameLen = strlen(name);
    requiredLen = nameLen + objLen + 3;
    if (msg->string[msg->_offset - 1] != '{' && msg->string[msg->_offset - 1] != '[') {
        requiredLen++;
        separator = true;
    }

    // Check there is enough space
    if (msg->_offset > (msg->len - requiredLen) || requiredLen > msg->len) {
        msg->_lastError = (-ENOMEM);
        return -ENOMEM;
    }

    // Add separator if necessary
    if (separator) {
        msg->string[msg->_offset] = ',';
        msg->_offset++;
    }

    // Add name
    msg->string[msg->_offset] = '"';
    msg->_offset++;
    strcpy(msg->string + msg->_offset, name);
    msg->_offset += nameLen;
    strcpy(msg->string + msg->_offset, "\":");
    msg->_offset += 2;

    // Add object
    memcpy(&msg->string[msg->_offset], objString, objLen);
    msg->_offset += objLen;

    return 0;
}


static int lwJsonCalculateValueStringLength(LwJsonValueType type, LwJsonValue *value) {
    unsigned int valueLen = 0;
    char auxString[50];

    switch (type) {
    case LWJSON_VAL_STRING:
        if (value == NULL) {
            return -EINVAL;
        }
        valueLen = strlen(value->valueString) + 2;
        break;
    case LWJSON_VAL_NUMBER:
        if (value == NULL) {
            return -EINVAL;
        }
        sprintf(auxString, "%lld", value->valueInt);
        valueLen = strlen(auxString);
        break;
    case LWJSON_VAL_BOOLEAN:
        if (value == NULL) {
            return -EINVAL;
        }
        if(value->valueBool == true) {
            // true
            valueLen += 4;
        } else {
            // false
            valueLen += 5;
        }
        break;
    case LWJSON_VAL_OBJECT:
    case LWJSON_VAL_ARRAY:
        valueLen = 1;
        break;
    case LWJSON_VAL_NULL:
        valueLen = 4;
        break;
    }

    return valueLen;
}

static int lwJsonCheckWriteError(LwJsonMsg *msg) {
    if (msg == NULL) {
        msg->_lastError = (-EINVAL);
        return -EINVAL;
    }
    if (msg->_offset >= msg->len) {
        msg->_lastError = (-ENOMEM);
        return -ENOMEM;
    }

    return 0;
}

static int lwJsonAddNameAndValuePair(LwJsonMsg *msg, const char *name, LwJsonValueType type, LwJsonValue *value) {
    unsigned int entryLen = 0;
    unsigned int nameLen;
    unsigned int valueLen = 0;
    char auxString[50];
    unsigned char flagSeparator = 0;

    if (lwJsonCheckWriteError(msg) != 0) {
        return msg->_lastError;
    }
    if (name == NULL) {
        msg->_lastError = (-EINVAL);
        return -EINVAL;
    }

    // Calculate and check len
    valueLen = lwJsonCalculateValueStringLength(type, value);
    if (valueLen < 0) {
        return -EINVAL;
    }

    // Calculate space
    nameLen = strlen(name);
    entryLen = nameLen + valueLen + 3;
    if (msg->string[msg->_offset - 1] != '{') {
        entryLen ++;
        flagSeparator = 1;
    }

    // Check space
    if (msg->_offset > (msg->len - entryLen)) {
        msg->_lastError = (-ENOMEM);
        return -ENOMEM;
    }

    // Add separator if necessary
    if (flagSeparator != 0) {
        msg->string[msg->_offset] = ',';
        msg->_offset++;
    }

    // Add name
    msg->string[msg->_offset] = '"';
    msg->_offset++;
    strcpy(msg->string + msg->_offset, name);
    msg->_offset += nameLen;
    strcpy(msg->string + msg->_offset, "\":");
    msg->_offset += 2;

    // Add value
    switch (type) {
    case LWJSON_VAL_STRING:
        msg->string[msg->_offset] = '"';
        msg->_offset++;
        strcpy(msg->string + msg->_offset, value->valueString);
        msg->_offset += valueLen - 2;
        msg->string[msg->_offset] = '"';
        msg->_offset++;
        break;
    case LWJSON_VAL_NUMBER:
        sprintf(auxString, "%lld", value->valueInt);
        strcpy(msg->string + msg->_offset, auxString);
        msg->_offset += valueLen;
        break;
    case LWJSON_VAL_BOOLEAN:
        if (value->valueBool == true) {
            strcpy(msg->string + msg->_offset, "true");
        } else {
            strcpy(msg->string + msg->_offset, "false");
        }
        msg->_offset += valueLen;
        break;
    case LWJSON_VAL_OBJECT:
        msg->string[msg->_offset] = '{';
        msg->_offset++;
        break;
    case LWJSON_VAL_ARRAY:
        msg->string[msg->_offset] = '[';
        msg->_offset++;
        break;
    case LWJSON_VAL_NULL:
        strcpy(msg->string + msg->_offset, "null");
        msg->_offset += valueLen;
        break;
    }

    return 0;
}

static int lwJsonAddValueToArray(LwJsonMsg *msg, LwJsonValueType type, LwJsonValue *value) {
    unsigned int entryLen = 0;
    unsigned int valueLen = 0;
    char auxString[50];
    unsigned char flagSeparator = 0;

    if (lwJsonCheckWriteError(msg) != 0) {
        return msg->_lastError;
    }

    // Calculate and check len
    valueLen = lwJsonCalculateValueStringLength(type, value);
    if (valueLen < 0) {
        msg->_lastError = (-EINVAL);
        return -EINVAL;
    }

    entryLen = valueLen;
    if (msg->string[msg->_offset - 1] != '[') {
        entryLen ++;
        flagSeparator = 1;
    }
    if (msg->_offset > (msg->len - entryLen)) {
        msg->_lastError = (-ENOMEM);
        return -ENOMEM;
    }

    // Add separator if necessary
    if (flagSeparator != 0) {
        msg->string[msg->_offset] = ',';
        msg->_offset++;
    }

    // Add value
    switch (type) {
    case LWJSON_VAL_STRING:
        msg->string[msg->_offset] = '"';
        msg->_offset++;
        strcpy(msg->string + msg->_offset, value->valueString);
        msg->_offset += valueLen - 2;
        msg->string[msg->_offset] = '"';
        msg->_offset++;
        break;
    case LWJSON_VAL_NUMBER:
        sprintf(auxString, "%lld", value->valueInt);
        strcpy(msg->string + msg->_offset, auxString);
        msg->_offset += valueLen;
        break;
    case LWJSON_VAL_BOOLEAN:
        if (value->valueBool == true) {
            strcpy(msg->string + msg->_offset, "true");
        } else {
            strcpy(msg->string + msg->_offset, "false");
        }
        msg->_offset += valueLen;
        break;
    case LWJSON_VAL_OBJECT:
        msg->string[msg->_offset] = '{';
        msg->_offset++;
        break;
    case LWJSON_VAL_ARRAY:
        msg->string[msg->_offset] = '[';
        msg->_offset++;
        break;
    case LWJSON_VAL_NULL:
        strcpy(msg->string + msg->_offset, "null");
        msg->_offset += valueLen;
        break;
    }

    return 0;
}

static int lwJsonIsValidUTF8String(const char *string) {
    unsigned char *bytes = (unsigned char*)string;

    if (string == NULL) {
        return 0;
    }

    while (*bytes) {
        if (bytes[0] == 0x09 || bytes[0] == 0x0A || bytes[0] == 0x0D || (0x20 <= bytes[0] && bytes[0] <= 0x7E)) {
            // Allow ASCII, TAB, LF y CR
            bytes += 1;
            continue;
        }

        if ((0xC2 <= bytes[0] && bytes[0] <= 0xDF) && (0x80 <= bytes[1] && bytes[1] <= 0xBF)) {
            // 2 byte chars (see http://tools.ietf.org/html/rfc3629)
            bytes += 2;
            continue;
        }

        if ( (// exclude overlongs
                bytes[0] == 0xE0 &&
                (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            ) ||
            (// 3 byte regular sequence
                ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
                    bytes[0] == 0xEE ||
                    bytes[0] == 0xEF) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            ) ||
            (// exclude surrogates
                bytes[0] == 0xED &&
                (0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            )
        ) {
            // 3 byte chars (see http://tools.ietf.org/html/rfc3629)
            bytes += 3;
            continue;
        }

        if( (// planes 1-3
                bytes[0] == 0xF0 &&
                (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            ) ||
            ( // planes 4-15
                (0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            ) ||
            ( // plane 16
                bytes[0] == 0xF4 &&
                (0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            )
        ) {
            // 4 bytes chars (see http://tools.ietf.org/html/rfc3629)
            bytes += 4;
            continue;
        }

        return 0;
    }

    return 1;
}
