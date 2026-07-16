# My own `nob.h` template
- depends on `https://github.com/tsoding/nob.h/`

## adding sources
```c
const char* sourcesToBuild[] = {
//	"src/main.c",
//	"...",
};
```

## adding includes
```c
const char* paths[] = {
    // "-Iinclude/",
#if defined(_WIN32)
    // "-Iinclude/",
    // ...
#elif defined(__linux__)
    // "-Iinclude/",
    // ...
#endif
};
```

## linking
```c
const char* toLink[] =
{
    // "-Llinkdir", "-llibrary",
#if defined(_WIN32)
    // "-Llinkdir", "-llibrary",
    // ...
#elif defined(__linux__)
    // "-Llinkdir", "-llibrary"
    // ...
#endif
};
```
