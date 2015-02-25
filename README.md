# lwjson
Lightweight JSON Parser and Generator for Embedded Projects

lwjson is a c library that allows to parse and generate JSON strings. It has been specially designed to target bare-metal microcontroller development.

## Why lwjson?
You may find lwjson specially useful in case the following conditions are met:
* Use of malloc must be avoided.
* The schema of the JSON strings to be parsed is known in advance.
* The size of the JSON strings to be parsed is not too big (a few KiloBytes should be ok).

## Quick parsing example
```c
// String to be parsed
char jsonExample[] = "{\"integer\":2}";
// Variables needed for parsing
LwJsonMsg jsonMsg;
char *path[2];
int parsedInt;

// STEP 1. Prepare LwJsonMsg object with pointer and string length properties
jsonMsg.string = jsonExample;
jsonMsg.len = strlen(jsonExample);

// STEP 2. Set path. It is a NULL terminated sequence of strings
path[0] = "integer";
path[1] = NULL;

// STEP 3. Parse data
lwJsonParseInt(path, &jsonMsg, &parsedInt);
```

## Quick generation example
```c
// String buffer
const uint32_t MAX_STRING_LEN = 1024;
char buffer[MAX_STRING_LEN + 1];
// Variable needed for generation
LwJsonMsg jsonMsg;

// STEP 1. Set LwJsonMsg object with pointer to buffer and maxLen
jsonMsg.string = buffer;
jsonMsg.len = MAX_STRING_LEN;

// STEP 2. Start generation
lwJsonWriteStart(&jsonMsg);

// STEP 3. Add data using the API
// 3.1 Open object
lwJsonStartObject(&jsonMsg);
// 3.2 Add Integer
lwJsonAddIntToObject(&jsonMsg, "integer", 2);
// 3.3 Close object
lwJsonCloseObject(&jsonMsg);

// STEP 4. Stop generation and check errors
if (lwJsonWriteEnd(&jsonMsg) != 0) {
    printf("An error ocurred\n");
} else {
    printf("Generated JSON: %s\n", jsonMsg.string);
}
```

## API
In construction...
