
# c

Place where I put my useful C files

## Tipes for programming C

[video](https://www.youtube.com/watch?v=9UIIMBqq1D4)

### version

Use c99 and up.  
C89 has some cumbersome syntax.

### stint.h

Use the stdint.h for the int variables.  
eg: `int8_t`, `uint8_t`  

It also hold useful constants.  
eg: `INT32_MAX`

### designated initializers

```C
typedef struct {
    int id;
    int age;
    char* name;
} User;

// doesn't have to be in order
User jane = {
    .name = "Jane"
    .id = 557,
    .age = 34,
}
```

### compound literals

```C
// push a point old way
Point point = { 25, 42 };
pushPoint(point);

// compound way
puShPoint((Point) {25, 42});
```

### compiler flags

Compiler flags are good. Use them.  

`gcc -std=c99 -Wall -Werror -fsanitize=address`  

| flag     | meaning                                   |
|----------|-------------------------------------------|
| -std=c99 | set the standard to the one you are using |
| -Wall    | enable all warnings                       |
| -Werror  | treat all warnings as errors.             |
|-fsanitize=address | use the [ASan](#asan) |


### build

Use a single translation unit, aka: Unity Build.  

`#include`all of your c files at the top of your main file.  
That way you don't have to use any convoluted build systems
like cmake. You don't even really need h files.  
This way you only need to call one file in the compiler.  

### Debugger

Use a debugger!

### ASan

Address Sanitization.  
This will help you catch Memory corruption issues.  
It makes sure you don't access memory outside of the range
you specified.  

Use the `-fsanitize=address`flag for the compiler.  

**!!! Turn this off for production build !!!**

### Arrays and Strings

All that an array / String really is in C, is a pointer.  
You have to track the length of an array yourself.  

Use runtime bounds checking!  

Implement your own arrays.

```C
typedef struct {
    int32_t* items;
    int32_t length;
    int32_t capacity;
} Int32Array;


// Use a getter function that checks the index is
// within the array.
int32_t Int32Array_Get(Int32Array array, int32_t index){
    if( index >= 0 && index < array.length){
        return array.items[index];
    }
    // add a debug break point here to catch when
    // you try an access a wrong index
    return 0;
}
```

A string is just an array of characters.  

**Avoid using standard library string functions.**  

Create your own similar to the Arrays above.  

```C
typedef struct {
    char* chars;
    int32_t length;
} String;
```

This way you don't have to rely on Null terminators.  
It's also much faster to get the length.

This way you can also create string slices without having to
copy memory.

### Index and Pointers

If you have a lot of data in an array, don't use
pointers to reference data in the array.  
Use indexes. This makes it easier to serialize data
and resize an array without creating dangling pointers.

### Arenas

Use arenas for memory management.
