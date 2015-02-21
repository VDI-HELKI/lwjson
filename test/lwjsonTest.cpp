#include "CppUTest/TestHarness.h"
#include "lwjson.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

TEST_GROUP(lwjson)
{
    void setup()
    {

    }

    void teardown()
    {

    }
};

TEST(lwjson, ParsePositiveInt)
{
    char testString[] = "{\"value\":100}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    int value;

    path[0] = (char*)"value";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(100, value);
}

TEST(lwjson, ParseNegativeInt)
{
    char testString[] = "{\"value\":-100}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    int value;

    path[0] = (char*)"value";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(callResult, 0);
    CHECK_EQUAL(value, -100);
}

TEST(lwjson, ParseBigInt)
{
    char testString[] = "{\"value\":2147483647}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    int value;

    path[0] = (char*)"value";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(callResult, 0);
    CHECK_EQUAL(value, 2147483647);
}

TEST(lwjson, ParseIntFromHigherDepth)
{
    char testString[] = "{\"object\":{\"value\":100}}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL, NULL};
    int callResult;
    int value;

    path[0] = (char*)"object";
    path[1] = (char*)"value";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(100, value);
}

TEST(lwjson, FailToParseIntFromMalFormedString)
{
    char testString[] = "{\"value\":100";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    int value;

    path[0] = (char*)"value";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(-EPERM, callResult);
}

TEST(lwjson, ParseString)
{
    char testString[] = "{\"value\":\"testing\"}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int VALUE_LEN = 9;
    char value[VALUE_LEN + 1];

    path[0] = (char*)"value";
    callResult = lwJsonGetString((const char**)path, &testMsg, value, VALUE_LEN);
    CHECK_EQUAL(0, callResult);
    STRCMP_EQUAL("testing", value);
}

TEST(lwjson, ParseLongerThanExpectedString)
{
    char testString[] = "{\"value\":\"testing\"}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int VALUE_LEN = 4;
    char value[VALUE_LEN + 1];

    path[0] = (char*)"value";
    callResult = lwJsonGetString((const char**)path, &testMsg, value, VALUE_LEN);
    CHECK_EQUAL(-ENOMEM, callResult);
}

TEST(lwjson, ParseEmptyString)
{
    char testString[] = "{\"value\":\"\"}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int VALUE_LEN = 4;
    char value[VALUE_LEN + 1];

    path[0] = (char*)"value";
    callResult = lwJsonGetString((const char**)path, &testMsg, value, VALUE_LEN);
    CHECK_EQUAL(0, callResult);
    STRCMP_EQUAL("", value);
}

TEST(lwjson, ParseIntArray)
{
    char testString[] = "{\"array\":[0,1,2,3,4,5,6,7,8,9]}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int ARRAY_LEN = 10;
    int value[ARRAY_LEN];
    int i;

    path[0] = (char*)"array";
    callResult = lwJsonGetIntArray((const char**)path, &testMsg, value, ARRAY_LEN);
    CHECK_EQUAL(10, callResult);
    for (i = 0; i < 10; i++) {
        CHECK_EQUAL(i, value[i]);
    }
}

TEST(lwjson, ParseLongerThanExpectedIntArray)
{
    char testString[] = "{\"array\":[0,1,2,3,4,5,6,7,8,9]}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int ARRAY_LEN = 5;
    int value[ARRAY_LEN];

    path[0] = (char*)"array";
    callResult = lwJsonGetIntArray((const char**)path, &testMsg, value, ARRAY_LEN);
    CHECK_EQUAL(-EPERM, callResult);
}

TEST(lwjson, ParseStringArray)
{
    char testString[] = "{\"array\":[\"0\",\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"7\",\"8\",\"9\"]}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int ARRAY_LEN = 10;
    char* value[ARRAY_LEN];
    unsigned int len[ARRAY_LEN];
    int i;
    char expectedString[4];
    char actualString[4];

    path[0] = (char*)"array";
    callResult = lwJsonGetStringArray((const char**)path, &testMsg, value, len, ARRAY_LEN);
    CHECK_EQUAL(10, callResult);
    for (i = 0; i < 10; i++) {
        // Comprobar longitud (se incluyen comillas)
        CHECK_EQUAL(3, len[i]);

        // Comprobar string
        sprintf(expectedString, "\"%u\"", i);
        strncpy(actualString, value[i], 3);
        actualString[3] = 0;
        STRCMP_EQUAL(expectedString, actualString);
    }
}

TEST(lwjson, ParseLongerThanExpectedStringArray)
{
    char testString[] = "{\"array\":[\"0\",\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"7\",\"8\",\"9\"]}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    const int ARRAY_LEN = 5;
    char* value[ARRAY_LEN];
    unsigned int len[ARRAY_LEN];

    path[0] = (char*)"array";
    callResult = lwJsonGetStringArray((const char**)path, &testMsg, value, len, ARRAY_LEN);
    CHECK_EQUAL(-EPERM, callResult);
}

TEST(lwjson, ParseTrue)
{
    char testString[] = "{\"value\":true}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    bool value;

    path[0] = (char*)"value";
    callResult = lwJsonGetBool((const char**)path, &testMsg, &value);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(true, value);
}

TEST(lwjson, ParseFalse)
{
    char testString[] = "{\"value\":false}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    bool value;

    path[0] = (char*)"value";
    callResult = lwJsonGetBool((const char**)path, &testMsg, &value);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(false, value);
}

TEST(lwjson, ParseStringWithSpacesTabsLRAndCF)
{
    char testString[] = "{\"object\" : \r\n {\r\n\t\"boolean\": true,\n\"string\":\t\"testing\",\r\"integer\": 100}}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL, NULL};
    int callResult;
    bool boolean;
    const int STRING_LEN = 9;
    char string[STRING_LEN + 1];
    int integer;

    path[0] = (char*)"object";
    path[1] = (char*)"boolean";
    callResult = lwJsonGetBool((const char**)path, &testMsg, &boolean);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(true, boolean);

    path[1] = (char*)"string";
    callResult = lwJsonGetString((const char**)path, &testMsg, string, STRING_LEN);
    CHECK_EQUAL(0, callResult);
    STRCMP_EQUAL("testing", string);

    path[1] = (char*)"integer";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &integer);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(100, integer);
}

TEST(lwjson, ParseAcceptEmptyObjects)
{
    char testString[] = "{\"object\":{\"boolean\":true},\"body\":{}}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL, NULL};
    int callResult;
    bool boolean;

    path[0] = (char*)"object";
    path[1] = (char*)"boolean";
    callResult = lwJsonGetBool((const char**)path, &testMsg, &boolean);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(true, boolean);
}

TEST(lwjson, GetGenericArrayLen)
{
    char testString1[] = "{\"array\":[{\"addr\":2},{\"addr\":3}]}";
    char testString2[] = "{\"array\":[{\"object\":{\"value\":1,\"addr\":2}},{\"object\":{\"value\":2,\"addr\":3}}]}";
    char testString3[] = "{\"array\": \t[{ \"object\": {\"value\":1,\r\n\"addr\":2}} , {\"object\":{\"value\":2,\"addr\":3}}]}";
    char testString4[] = "{\"array\":[]}";
    char testString5[] = "{\"array\": \t[\t ]}";
    char testString6[] = "{\"method\":\"POST\",\"path\":\"/mgr/replace\",\"body\":{\"nodes\":[{\"type\":\"pmo\",\"addr\":3,\"installed\":true,\"name\":\"\"}],\"network_addr\":\"1A2D\"}}";
    char testString7[] = "{\"method\":\"POST\",\"path\":\"/mgr/replace\",\"body\":{\"nodes\":[{\"type\":\"pmo\",\"addr\":3,\"installed\":true,\"name\":\"\"},{\"type\":\"thm\",\"addr\":4,\"installed\":true,\"name\":\"\"},{\"type\":\"htr\",\"addr\":5,\"installed\":true,\"name\":\"\"}],\"network_addr\":\"1A2D\"}}";
    LwJsonMsg testMsg;
    char* path[] = {NULL, NULL,NULL};
    int callResult;

    path[0] = (char*)"array";
    testMsg.string = testString1;
    testMsg.len = sizeof(testString1) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(2, callResult);

    testMsg.string = testString2;
    testMsg.len = sizeof(testString2) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(2, callResult);

    testMsg.string = testString3;
    testMsg.len = sizeof(testString3) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(2, callResult);

    testMsg.string = testString4;
    testMsg.len = sizeof(testString4) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(0, callResult);

    testMsg.string = testString5;
    testMsg.len = sizeof(testString5) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(0, callResult);

    path[0] = (char*)"body";
    path[1] = (char*)"nodes";
    testMsg.string = testString6;
    testMsg.len = sizeof(testString6) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(1, callResult);

    testMsg.string = testString7;
    testMsg.len = sizeof(testString7) - 1;
    callResult = lwJsonGetArrayLen((const char**)path, &testMsg);
    CHECK_EQUAL(3, callResult);
}

TEST(lwjson, ParseValueInArrayObject)
{
    char testString[] = "{\"array\":[{\"addr\":2},{\"addr\":3}]}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL, NULL, NULL};
    int callResult;
    int value;

    path[0] = (char*)"array";
    path[1] = (char*)"[0]";
    path[2] = (char*)"addr";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(2, value);

    path[0] = (char*)"array";
    path[1] = (char*)"[1]";
    path[2] = (char*)"addr";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(0, callResult);
    CHECK_EQUAL(3, value);

    path[0] = (char*)"array";
    path[1] = (char*)"[2]";
    path[2] = (char*)"addr";
    callResult = lwJsonGetInt((const char**)path, &testMsg, &value);
    CHECK_EQUAL(-ENOENT, callResult);
}

TEST(lwjson, ParseObject) {
    char testString[] = "{\"array\":[0,{\"bool\":true},2,3,4,5,6,7,8,9]}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    LwJsonMsg array;

    path[0] = (char*)"array";
    callResult = lwJsonGetArray((const char**)path, &testMsg, &array);

    CHECK_EQUAL(0, callResult);
    POINTERS_EQUAL(&testString[9], array.string);
    CHECK_EQUAL(33, array.len);
}

TEST(lwjson, ParseArray) {
    char testString[] = "{\"object\":{\"boolean\":true}}";
    LwJsonMsg testMsg = {testString, sizeof(testString) - 1};
    char* path[] = {NULL, NULL};
    int callResult;
    LwJsonMsg obj;

    path[0] = (char*)"object";
    callResult = lwJsonGetObject((const char**)path, &testMsg, &obj);

    CHECK_EQUAL(0, callResult);
    POINTERS_EQUAL(&testString[10], obj.string);
    CHECK_EQUAL(16, obj.len);
}

TEST(lwjson, GenerateEmptyObject)
{
    const unsigned int STRING_LEN = 2;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{}", string);
}

TEST(lwjson, NotEnoughSpace)
{
    const unsigned int STRING_LEN = 1;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GeneratePositiveInteger)
{
    const unsigned int STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddIntToObject(&testMsg, "value", 100);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"value\":100}", string);
}

TEST(lwjson, GeneratePositiveIntegerRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 12;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddIntToObject(&testMsg, "value", 100);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateNegativeInteger)
{
    const unsigned int STRING_LEN = 14;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddIntToObject(&testMsg, "value", -100);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"value\":-100}", string);
}

TEST(lwjson, GenerateNegativeIntegerRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddIntToObject(&testMsg, "value", -100);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateString)
{
    const unsigned int STRING_LEN = 17;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddStringToObject(&testMsg, "value", "hello");
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"value\":\"hello\"}", string);
}

TEST(lwjson, GenerateStringRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 16;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddStringToObject(&testMsg, "value", "hello");
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateBooleanTrue)
{
    const unsigned int STRING_LEN = 14;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddBooleanToObject(&testMsg, "value", true);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"value\":true}", string);
}

TEST(lwjson, GenerateBooleanTrueRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddBooleanToObject(&testMsg, "value", true);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateBooleanFalse)
{
    const unsigned int STRING_LEN = 15;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddBooleanToObject(&testMsg, "value", false);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"value\":false}", string);
}

TEST(lwjson, GenerateBooleanFalseRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 14;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddBooleanToObject(&testMsg, "value", false);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);
    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateNull)
{
    const unsigned int STRING_LEN = 14;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddNullToObject(&testMsg, "value");
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"value\":null}", string);
}

TEST(lwjson, GenerateNullRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddNullToObject(&testMsg, "value");
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateArray)
{
    const unsigned int STRING_LEN = 12;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"array\":[]}", string);
}

TEST(lwjson, GenerateArrayRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 11;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateObject)
{
    const unsigned int STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddObjectToObject(&testMsg, "object");
    lwJsonCloseObject(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"object\":{}}", string);
}

TEST(lwjson, GenerateObjectRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 12;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddObjectToObject(&testMsg, "object");
    lwJsonCloseObject(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateIntArray)
{
    const unsigned int ARRAY_LEN = 5;
    const unsigned int STRING_LEN = 38;
    int64_t intArray[ARRAY_LEN] = {-5, 234, 21, -5643, 1234567890};
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    unsigned int i;
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    for (i = 0; i < ARRAY_LEN; i++) {
        lwJsonAddIntToArray(&testMsg, intArray[i]);
    }
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"array\":[-5,234,21,-5643,1234567890]}", string);
}

TEST(lwjson, GenerateIntArrayRunsOutOfSpace)
{
    const unsigned int ARRAY_LEN = 5;
    const unsigned int STRING_LEN = 37;
    int64_t intArray[ARRAY_LEN] = {-5, 234, 21, -5643, 1234567890};
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    unsigned int i;
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    for (i = 0; i < ARRAY_LEN; i++) {
        lwJsonAddIntToArray(&testMsg, intArray[i]);
    }
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateStringArray)
{
    const unsigned int ARRAY_LEN = 5;
    const unsigned int STRING_LEN = 46;
    const char* stringArray[ARRAY_LEN] = {"Hello", "world", "How", "are", "you?"};
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    unsigned int i;
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    for (i = 0; i < ARRAY_LEN; i++) {
        lwJsonAddStringToArray(&testMsg, stringArray[i]);
    }
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"array\":[\"Hello\",\"world\",\"How\",\"are\",\"you?\"]}", string);
}

TEST(lwjson, GenerateObjectArray)
{
    const unsigned int ARRAY_LEN = 5;
    const unsigned int STRING_LEN = 26;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    unsigned int i;
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    for (i = 0; i < ARRAY_LEN; i++) {
        lwJsonAddObjectToArray(&testMsg);
        lwJsonCloseObject(&testMsg);
    }
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"array\":[{},{},{},{},{}]}", string);
}

TEST(lwjson, GenerateObjectArrayRunsOutOfSpace)
{
    const unsigned int ARRAY_LEN = 5;
    const unsigned int STRING_LEN = 25;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    unsigned int i;
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    for (i = 0; i < ARRAY_LEN; i++) {
        lwJsonAddObjectToArray(&testMsg);
        lwJsonCloseObject(&testMsg);
    }
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateStringArrayRunsOutOfSpace)
{
    const unsigned int ARRAY_LEN = 5;
    const unsigned int STRING_LEN = 45;
    const char* stringArray[ARRAY_LEN] = {"Hello", "world", "How", "are", "you?"};
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    unsigned int i;
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddArrayToObject(&testMsg, "array");
    for (i = 0; i < ARRAY_LEN; i++) {
        lwJsonAddStringToArray(&testMsg, stringArray[i]);
    }
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, GenerateMiscellaneousObject)
{
    const unsigned int STRING_LEN = 117;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddObjectToObject(&testMsg, "object");
    lwJsonAddBooleanToObject(&testMsg, "boolean", true);
    lwJsonAddStringToObject(&testMsg, "string", "testing");
    lwJsonAddIntToObject(&testMsg, "integer", 100);
    lwJsonAddArrayToObject(&testMsg, "array");
    lwJsonAddObjectToArray(&testMsg);
    lwJsonAddIntToObject(&testMsg, "attr1", 10);
    lwJsonAddIntToObject(&testMsg, "attr2", 20);
    lwJsonCloseObject(&testMsg);
    lwJsonAddStringToArray(&testMsg, "arrayString");
    lwJsonAddIntToArray(&testMsg, -5);
    lwJsonAddBooleanToArray(&testMsg, false);
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("{\"object\":{\"boolean\":true,\"string\":\"testing\",\"integer\":100,\"array\":[{\"attr1\":10,\"attr2\":20},\"arrayString\",-5,false]}}", string);
}

TEST(lwjson, GenerateMiscellaneousObjectRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 116;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAddObjectToObject(&testMsg, "object");
    lwJsonAddBooleanToObject(&testMsg, "boolean", true);
    lwJsonAddStringToObject(&testMsg, "string", "testing");
    lwJsonAddIntToObject(&testMsg, "integer", 100);
    lwJsonAddArrayToObject(&testMsg, "array");
    lwJsonAddObjectToArray(&testMsg);
    lwJsonAddIntToObject(&testMsg, "attr1", 10);
    lwJsonAddIntToObject(&testMsg, "attr2", 20);
    lwJsonCloseObject(&testMsg);
    lwJsonAddStringToArray(&testMsg, "arrayString");
    lwJsonAddIntToArray(&testMsg, -5);
    lwJsonAddBooleanToArray(&testMsg, false);
    lwJsonCloseArray(&testMsg);
    lwJsonCloseObject(&testMsg);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}

TEST(lwjson, AppendExistingObject)
{
    const unsigned int STRING_LEN = 24;
    const unsigned int OBJ_STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    const char objString[OBJ_STRING_LEN + 1] = "{\"value\":100}";
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAppendObject(&testMsg, "object", objString);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL(string, "{\"object\":{\"value\":100}}");
}

TEST(lwjson, AppendExistingObjectRunsOutOfSpace)
{
    const unsigned int STRING_LEN = 23;
    const unsigned int OBJ_STRING_LEN = 13;
    char string[STRING_LEN + 1];
    LwJsonMsg testMsg = {string, STRING_LEN};
    const char objString[OBJ_STRING_LEN + 1] = "{\"value\":100}";
    int result;

    lwJsonWriteStart(&testMsg);
    lwJsonStartObject(&testMsg);
    lwJsonAppendObject(&testMsg, "object", objString);
    lwJsonCloseObject(&testMsg);
    result = lwJsonWriteEnd(&testMsg);

    CHECK_EQUAL(-ENOMEM, result);
}
