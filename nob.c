#define NOB_IMPLEMENTATION
#include <stdbool.h>
#include "nob.h"

#define COMPILER "g++"
#define BUILD_FOLDER "build/"
#if defined(_WIN32)
#define OUTPUT_EXEC "build/myExec.exe"
#elif defined(__linux__)
#define OUTPUT_EXEC "build/myExec"
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
	const char** paths;
	size_t count;
} includePaths;

typedef struct {
	const char* buildPath;
	Nob_Cmd cmd;
} buildScript;

typedef struct {
	buildScript* items;
	size_t count;
	size_t capacity;
} buildScripts;

typedef struct {
	const char* buildPath;
} buildObject;

typedef struct {
	buildObject* items;
	size_t count;
	size_t capacity;
} buildObjects;

buildScripts generateBuildScripts(const char* sources[], size_t sourcesCount, includePaths includes) {
	buildScripts objects = {0};

	for(size_t i = 0; i < sourcesCount; ++i) {
		Nob_Cmd cmd = {0};
		nob_cmd_append(&cmd, COMPILER);
		
		const char* sourcePath = sources[i];
		const char* buildPath = nob_temp_sprintf(BUILD_FOLDER"%s.o", nob_path_name(sources[i]));
		nob_cc_inputs(&cmd, sourcePath);
		nob_cmd_append(&cmd, "-c");
		nob_cc_output(&cmd, buildPath);

		for(size_t j = 0; j < includes.count; ++j) {
			nob_cmd_append(&cmd, includes.paths[j]);
		}

		buildScript obj = {0};
		obj.buildPath = buildPath;
		obj.cmd = cmd;

		da_append(&objects, obj);
	}

	return objects;
}


buildObjects executeBuildScripts(buildScripts scripts, bool async) {
	buildObjects objects = {0};

	da_foreach(buildScript, script, &scripts) {
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

		buildObject obj = {0};
		obj.buildPath = script->buildPath;

		da_append(&objects, obj);
	}


	return objects;
}


bool compileObjects(buildObjects objects, linkerFlags links, const char* outputExec) {
	Nob_Cmd cmd = {0};

	if(!objects.count){
		nob_log(WARNING, "No input files for compilation!");
		return false;
	}
	
	nob_cmd_append(&cmd, COMPILER);
	nob_cc_output(&cmd, outputExec);

	da_foreach(buildObject, obj, &objects) {
		nob_cc_inputs(&cmd, obj->buildPath);
	}

	for(size_t i = 0; i < links.count; ++i) {
		nob_cmd_append(&cmd, links.links[i]);
	}

	return nob_cmd_run(&cmd);
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

	includePaths includes = {
		.paths = paths,
		.count = ARRAY_LEN(paths)
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

	buildScripts scripts = generateBuildScripts(sourcesToBuild, ARRAY_LEN(sourcesToBuild), includes);
	buildObjects objects = executeBuildScripts(scripts, asnyc);

	if(asnyc)
		nob_procs_wait(procs);

	if(compileObjects(objects, links, OUTPUT_EXEC))
		nob_log(NOB_INFO, "Compilation Succesful!");
	else
		nob_log(NOB_ERROR, "Compilation Unsuccesful!");
}
