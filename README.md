# My own `nob.h` template
- depends on `https://github.com/tsoding/nob.h/`

### Features
- a bit more managable building paradigm
- easy to use `building` scripts, enumrated as follows:
    - `generateBuildScripts(...)`
    - `executeBuildScripts(...)`
    - `compileObjects(...)`
    - `createLibrary(...)`
    - `generate_compile_commands(...)`

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

## creating libs
```c
buildObjects objects = ...;
const char* myLibName = "libMyLib";
LibType libtype = LIB_STATIC; // LIB_SHARED

bool success = createLibrary(objects, "libMyLib", libtype);
if(success) ...
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

All together
===
```c
/*
* creating lib
*/
const char* myLibSources[] = { "myLib/src/myLib.c", ... };
size_t myLibSourcesCount = ARRAY_LEN(myLibSources);

const char* myLibIncludes[] = { "myLib/include/", ... };
size_t myLibIncludesCount = ARRAY_LEN(myLibIncludes);

buildScripts myLibScripts = generateBuildScripts(
                myLibSources, myLibSourcesCount,
                myLibIncludes, myLibIncludesCount,
            /* pic=*/true);

// building the lib
bool async = true; // for async building
buildObjects myLibObjects = executeBuildScripts(myLibScripts, async);
if(async)
    nob_procs_wait(global_procs); // `procs` is global

// compiling lib
if(!createLibrary(myLibObjects, "myLib", LIB_STATIC)) return 1;

/*
* creating main app
*/

const char* myAppSources[] = { "src/main.c", ... };
size_t myAppSourcesCount = ARRAY_LEN(myAppSources);

const char* myAppIncludes[] = { "include/", ... };
size_t myAppIncludesCount = ARRAY_LEN(myAppIncludes);

const char* links[] = { 
    "-Lbuild/libs", "-lmylib",
    "-lm", "-l...", ...
};

linkerFlags lFlags = {
    .links = links,
    .count = ARRAY_LEN(links)
};

const char* flags[] = {
    "-O3",
    "-static",
};

compilerFlags cFlags = {
    .flags = flags,
    .count = ARRAY_LEN(flags)
};

buildScripts scripts = generateBuildScripts(myAppSources, myAppSourcesCount, myAppIncludes, myAppIncludesCount, true);

// for LSP
if(!generate_compile_commands(scripts)) return 1;

// building the main App
async = true;
buildObjects objects = executeBuildScripts(scripts, async);
if(async)
    nob_procs_wait(global_procs);

// compiling App
bool success = compileObjects(objects, lFlags, cFlags, "build/myApp.exe");
if(success)
    nob_log(NOB_INFO, "Compilation Succesful!");
else
    nob_log(NOB_ERROR, "Compilation Unsuccesful!");
```
