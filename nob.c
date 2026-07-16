#define NOB_IMPLEMENTATION
#include <stdbool.h>
#include "nob.h"

#define COMPILER "g++"
#define EXECUTABLE "myExec"
#define BUILD_FOLDER "build/"
#define BUILD_LIBS_FOLDER "build/libs/"
#if defined(_WIN32)
#define OUTPUT_EXEC BUILD_FOLDER EXECUTABLE".exe"
#elif defined(__linux__)
#define OUTPUT_EXEC BUILD_FOLDER EXECUTABLE
#endif

Procs procs = {0};

typedef struct {
	const char* sourcePath;
} sourceObject;

typedef struct {
	const char** links;
	size_t count;
} linkerFlags;

typedef struct {
	const char* sourcePath;
	const char* buildPath;
	Nob_Cmd cmd;
} buildScript;

typedef struct {
	union {
		buildScript* items;
		buildScript* scripts;
	};
	size_t count;
	size_t capacity;
} buildScripts;

typedef struct {
	const char* buildPath;
} buildObject;

typedef struct {
	union {
		buildObject* items;
		buildObject* objects;
	};
	size_t count;
	size_t capacity;
} buildObjects;

typedef struct {
	const char** flags;
	size_t count;
} compilerFlags;

typedef enum {
    LIB_STATIC,
    LIB_SHARED
} LibType;

buildScripts generateBuildScripts(const char* sources[], size_t sourcesCount, const char* includes[], size_t includesCount, bool pic) {
    buildScripts objects = {0};

    for (size_t i = 0; i < sourcesCount; ++i) {
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, COMPILER);

        const char* sourcePath = sources[i];
        const char* buildPath = nob_temp_sprintf(BUILD_FOLDER"%s.o", nob_path_name(sources[i]));
        nob_cc_inputs(&cmd, sourcePath);
        nob_cmd_append(&cmd, "-c");
        nob_cc_output(&cmd, buildPath);

#if !defined(_WIN32)
        if (pic) nob_cmd_append(&cmd, "-fPIC");
#endif

        for (size_t j = 0; j < includesCount; ++j) {
            nob_cmd_append(&cmd, includes[j]);
        }

        buildScript obj = {0};
        obj.buildPath = buildPath;
        obj.sourcePath = sourcePath;
        obj.cmd = cmd;

        nob_da_append(&objects, obj);
    }

    return objects;
}

buildObjects executeBuildScripts(buildScripts scripts, bool async) {
	buildObjects objects = {0};

	nob_da_foreach(buildScript, script, &scripts) {
		int rebuild_is_needed = nob_needs_rebuild(script->buildPath, &script->sourcePath, 1);
		if (rebuild_is_needed) {
			nob_log(NOB_INFO, "Building Object: %s", script->buildPath);
			if(async){
				if(!nob_cmd_run(&script->cmd, .async=&procs)) {
					nob_log(NOB_ERROR, "Failed to build %s", script->buildPath);
					return objects;
				}
			}else{
				if(!nob_cmd_run(&script->cmd)) {
					nob_log(NOB_ERROR, "Failed to build %s", script->buildPath);
					return objects;
				}
			}
		} else {
			nob_log(NOB_INFO, "%s is up to date, skipping", script->buildPath);
		}

		buildObject obj = {0};
		obj.buildPath = script->buildPath;
		nob_da_append(&objects, obj);
	}

	return objects;
}

bool compileObjects(buildObjects objects, linkerFlags links, compilerFlags flags, const char* outputExec) {
	Nob_Cmd cmd = {0};

	if(!objects.count){
		nob_log(NOB_WARNING, "No input files for compilation!");
		return false;
	}
	
	nob_cmd_append(&cmd, COMPILER);
	nob_cc_output(&cmd, outputExec);

	nob_da_foreach(buildObject, obj, &objects) {
		nob_cc_inputs(&cmd, obj->buildPath);
	}

	for(size_t i = 0; i < links.count; ++i) {
		nob_cmd_append(&cmd, links.links[i]);
	}

	for(size_t i = 0; i < flags.count; ++i) {
		nob_cmd_append(&cmd, flags.flags[i]);
	}

	return nob_cmd_run(&cmd);
}

// Generated from Claude
void json_escape_fputs(const char *s, FILE *f)
{
    for (; *s; ++s) {
        switch (*s) {
            case '"':  fputs("\\\"", f); break;
            case '\\': fputs("\\\\", f); break;
            case '\n': fputs("\\n", f);  break;
            case '\t': fputs("\\t", f);  break;
            default:   fputc(*s, f);
        }
    }
}

// Generated from Claude
void json_escape_path_fputs(const char *s, FILE *f)
{
    for (; *s; ++s) {
        char c = (*s == '\\') ? '/' : *s;
        if (c == '"') fputs("\\\"", f);
        else          fputc(c, f);
    }
}

// Generated from Claude
bool generate_compile_commands(buildScripts scripts)
{
    FILE *f = fopen("compile_commands.json", "w");
    if (!f) return false;

    const char *cwd = nob_get_current_dir_temp(); // hoisted, doesn't change per-iteration

    fprintf(f, "[\n");

    for (size_t i = 0; i < scripts.count; ++i) {
        buildScript *script = &scripts.items[i];

        fprintf(f, "  {\n    \"directory\": \"");
        json_escape_fputs(cwd, f);
        fprintf(f, "\",\n    \"file\": \"");
        json_escape_fputs(script->sourcePath, f);
        fprintf(f, "\",\n    \"arguments\": [");

        for (size_t j = 0; j < script->cmd.count; ++j) {
            if (j) fprintf(f, ", ");
            fputc('"', f);
            json_escape_fputs(script->cmd.items[j], f);
            fputc('"', f);
        }

        fprintf(f, "]\n  }%s\n", i + 1 < scripts.count ? "," : "");
    }

    fprintf(f, "]\n");
    fclose(f);
    return true;
}


// Generated from Claude
static const char* libFileName(const char* libName, LibType type, char* buf, size_t bufSize) {
#if defined(_WIN32)
    const char* prefix = "";
    const char* ext = (type == LIB_STATIC) ? ".lib" : ".dll";
#else
    const char* prefix = "lib";
    const char* ext = (type == LIB_STATIC) ? ".a" : ".so";
#endif
    snprintf(buf, bufSize, "%s%s%s%s", BUILD_FOLDER, prefix, libName, ext);
    return buf;
}

// Generated from Claude
bool createLibrary(buildObjects objects, const char* libName, LibType type) {
    Nob_Cmd cmd = {0};

    if (!objects.count) {
        nob_log(NOB_WARNING, "No input files for library creation!");
        return false;
    }

    const char* outputPath = nob_temp_sprintf(
#if defined(_WIN32)
        BUILD_LIBS_FOLDER"%s%s", libName, type == LIB_STATIC ? ".lib" : ".dll"
#else
        BUILD_LIBS_FOLDER"lib%s%s", libName, type == LIB_STATIC ? ".a" : ".so"
#endif
    );

    if (type == LIB_STATIC) {
#if defined(_WIN32)
        nob_cmd_append(&cmd, "llvm-ar", "rcs", outputPath);
#else
        nob_cmd_append(&cmd, "ar", "rcs", outputPath);
#endif
        nob_da_foreach(buildObject, obj, &objects) {
            nob_cmd_append(&cmd, obj->buildPath);
        }
    } else {
        nob_cmd_append(&cmd, COMPILER, "-shared");
        nob_cc_output(&cmd, outputPath);
        nob_da_foreach(buildObject, obj, &objects) {
            nob_cmd_append(&cmd, obj->buildPath);
        }
    }

    bool ok = nob_cmd_run(&cmd);
    if (ok) nob_log(NOB_INFO, "Created library: %s", outputPath);
    return ok;
}


int main(int argc, char** argv){
	NOB_GO_REBUILD_URSELF(argc, argv);
	if(!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

	bool asnyc = false;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-j") == 0) {
			asnyc = true;
		} else {
			nob_log(NOB_ERROR, "Unknown flag: %s", argv[i]);
			return 1;
		}
	}

	
	const char* sourcesToBuild[] = {
	//	"src/main.cpp",
		"nob.c"
	};

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

	linkerFlags links = {
		.links = toLink,
		.count = ARRAY_LEN(toLink)
	};

	const char* flags[] = {
		"-O3",
	};

	compilerFlags cFlags = {
		.flags = flags,
		.count = ARRAY_LEN(flags)
	};

	buildScripts scripts = generateBuildScripts(
			sourcesToBuild, ARRAY_LEN(sourcesToBuild),
			paths,          ARRAY_LEN(paths),
		true);

	if(!generate_compile_commands(scripts)) return 1;

	buildObjects objects = executeBuildScripts(scripts, asnyc);

	if(asnyc)
		nob_procs_wait(procs);

	bool success = compileObjects(objects, links, cFlags, OUTPUT_EXEC);
	if(success)
		nob_log(NOB_INFO, "Compilation Succesful!");
	else
		nob_log(NOB_ERROR, "Compilation Unsuccesful!");
}
