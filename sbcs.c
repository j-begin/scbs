#define XSTR(X)  #X
#define STR(X) XSTR(X)
#define WAIT(TEXT) TEXT
#define MESSAGE_HEAD WAIT(__FILE__)":"WAIT(STR(__LINE__))">"

#include "sbcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Compiler {
	COMPILER_UNKNOWN = 0,
	COMPILER_GCC,
	COMPILER_CLANG
};

static char* OBJ_OUTPUT_DIR = NULL;

static enum Compiler internal_getCompiler() {
	static enum Compiler compiler = COMPILER_UNKNOWN;

	if (compiler == COMPILER_UNKNOWN) {
		if (system("gcc --version") == 0) {
			compiler = COMPILER_GCC;
		} else if (system("clang --version") == 0) {
			compiler = COMPILER_CLANG;
		} else {
			fputs(MESSAGE_HEAD" Failed to find C compiler on the system\n", stderr);
		}
	}
	
	return compiler;
}

/* you have to free this */
static char* internal_generateObjectName(const char* const mod_name, const char* const file_path) {
	/* TODO this needs to be implemented */
	int i = 0;
	size_t read_start = 0;
	size_t read_stop = 0;
	char* obj_name = NULL;
	char* file_name = NULL;
	char* prepend_text = NULL;
	char* postpend_text = ".o";

	if (mod_name == NULL) {
		fputs(MESSAGE_HEAD" Cannot generate an object name for a module that does not have a valid name", stderr);
		return NULL;
	}

	if (file_path == NULL) {
		fprintf(stderr, MESSAGE_HEAD" Cannot generate an object name for module \"%s\" without valid path\n", mod_name);
		return NULL;
	}

	if (mod_name[0] == '\0') {
		fputs(MESSAGE_HEAD" Cannot generate an object name for a module that does not have a valid name", stderr);
		return NULL;
	}

	if (file_path[0] == '\0') {
		fprintf(stderr, MESSAGE_HEAD" Cannot generate an object name for module \"%s\" without valid path\n", mod_name);
		return NULL;
	}

	for (i = strlen(file_path) - 1; i > 0; i--) {
		if (file_path[i] == '/' || file_path[i] == '\\') {
			if (file_path[i+1] != '\0') {
				read_start = i + 1;
				break;
			} else {
				fputs(MESSAGE_HEAD" Cannot generate an object name for a directory", stderr);
				return NULL;
			}
		}

	}
	
	read_stop = strlen(file_path);
	for (i = read_stop; i != read_start; i--) {
		if (file_path[i] == '.') {
			read_stop = i;
			continue;
		}
		if (file_path[i] == '/' || file_path[i] == '\\') {
			break;
		}
	}

	file_name = calloc((read_stop - read_start) + 1, sizeof(char));
	if (file_name == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate memory for file_name", stderr);
		return NULL;
	}
	read_start *= sizeof(char);
	read_stop = (read_stop - read_start) * sizeof(char);
	memcpy(file_name, file_path + read_start, read_stop);
	
	prepend_text = calloc(((OBJ_OUTPUT_DIR != NULL) ? strlen(OBJ_OUTPUT_DIR) + 1 : 0) +  7 + strlen(mod_name) + 1, sizeof(char));
	if (prepend_text == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate memory for prepend_text", stderr);
		free(file_name);
		return NULL;
	}
	if (OBJ_OUTPUT_DIR != NULL) {
		strcat(prepend_text, OBJ_OUTPUT_DIR);
	}
	strcat(prepend_text, "module_");
	strcat(prepend_text, mod_name);

	obj_name = calloc(strlen(prepend_text) + 1 + strlen(file_name) + strlen(postpend_text) + 1, sizeof(char));
	if (obj_name == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate memory for obj_name", stderr);
		free(prepend_text);
		return NULL;
	}
	strcat(obj_name, prepend_text);
	strcat(obj_name, "_");
	strcat(obj_name, file_name);
	strcat(obj_name, postpend_text);

	free(file_name);
	free(prepend_text);
	return obj_name;
}

static int internal_readyToLink(const struct Module* const mod) {
	int i = 0;

	if (!mod->_was_compiled) {
		return 0;
	}

	for (i = 0; i < mod->_deps.count; i++) {
		internal_readyToLink(mod->_deps.data[i]);
	}
	return 1;
}

/* only allowed to be used in internal_getObjectNames */
static size_t internal_l1(struct Module* const mod) {
	size_t length = 0;
	char* text = NULL;
	int i = 0;

	if (!mod->_was_checked) {
		for (i = 0; i < mod->_files.count; i++) {
			text = internal_generateObjectName(mod->_name, mod->_files.paths[i]);
			length += strlen(text) + 1;
			free(text);
		}
		mod->_was_checked = 1;
	}

	for (i = 0; i < mod->_deps.count; i++) {
		length += internal_l1(mod->_deps.data[i]);
	}

	return length;
}

/* only allowed to be used in internal_getObjectNames */
static void internal_l2(struct Module* const mod, char* string) {
	int i = 0;
	char* text = NULL;

	if (!mod->_was_checked) {
		for (i = 0; i < mod->_files.count; i++) {
			text = internal_generateObjectName(mod->_name, mod->_files.paths[i]);
			strcat(string, text);
			strcat(string, " ");
			free(text);
		}
		mod->_was_checked = 1;
	}

	for (i = 0; i < mod->_deps.count; i++) {
		internal_l2(mod->_deps.data[i], string);
	}

	return;
}

static void internal_l3(struct Module* const mod) {
	int i = 0;

	mod->_was_checked = 0;
	for (i = 0; i < mod->_deps.count; i++) {
		internal_l3(mod->_deps.data[i]);
	}
}

/* you need to free this */
static char* internal_getObjectNames(struct Module* const mod) {
	char* text = NULL;

	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot get object names for a module that doesn't exist", stderr);
		return NULL;
	}

	text = calloc(internal_l1(mod) + 1, sizeof(char));
	internal_l3(mod);
	internal_l2(mod, text);
	internal_l3(mod);

	return text;
}

struct Module* Module_new(const char* const name) {
	struct Module* new = NULL;
	size_t name_length = 0;

	if (name == NULL) {
		fputs(MESSAGE_HEAD" Cannot create a module without a name", stderr);
		return NULL;
	}

	if (name[0] == '\0') {
		fputs(MESSAGE_HEAD" Cannot create a module without a name", stderr);
		return NULL;
	}

	name_length = strlen(name);

	new = malloc(sizeof(*new));
	if (new == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate memory for new", stderr);
		return NULL;
	}
	new->_name = malloc((name_length + 1) * sizeof(char));
	if (new->_name == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate memory for new->_name", stderr);
		free(new);
		return NULL;
	}
	strcpy(new->_name, name);
	new->_files.paths = NULL;
	new->_files.count = 0;
	new->_deps.data = NULL;
	new->_deps.count = 0;
	new->_was_compiled = 0;
	new->_was_checked = 0;

	return new;
}

void Module_free(struct Module* mod) {
	size_t i = 0;

	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Tried to free non-existant module. Ignoring!!!", stderr);
		return;
	}

	free(mod->_name);
	for (i = 0; i < mod->_files.count; i++) {
		if (mod->_files.paths[i] != NULL) {
			free(mod->_files.paths[i]);
		}
	}
	free(mod->_deps.data);
	free(mod->_files.paths);

	free(mod);
}

const char* const Module_getName(const struct Module* const mod) {
	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot get name of a non existant module", stderr);
		return NULL;
	} else {
		return mod->_name;
	}
}

int Module_addFile(struct Module* const mod, const char* const file_name) {
	size_t file_name_length = 0;
	int i = 0;

	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot add a file to a non-existant module", stderr);
		return 0;
	}

	if (file_name == NULL) {
		fputs(MESSAGE_HEAD" Cannot add a file without a name to a module", stderr);
		return 0;
	}

	if (file_name[0] == '\0') {
		return 0;
	}

	file_name_length = strlen(file_name);

	mod->_files.count += 1;
	if (mod->_files.paths == NULL) {
		mod->_files.paths = malloc(sizeof(char**));
		if (mod->_files.paths == NULL) {
			fputs(MESSAGE_HEAD" Failed to allocate memory for mod->_files.paths", stderr);
			mod->_files.count = 0;
			return 0;
		}
	} else {
		mod->_files.paths = realloc(mod->_files.paths, sizeof(char**) * mod->_files.count);
		if (mod->_files.paths == NULL) {
			fputs(MESSAGE_HEAD" Failed to reallocate memory for mod->_files.paths", stderr);
			mod->_files.paths = 0;
			return 0;
		}
	}

	mod->_files.paths[mod->_files.count - 1] = calloc(file_name_length + 1, sizeof(char));
	if (mod->_files.paths[mod->_files.count - 1] == NULL) {
		fputs(MESSAGE_HEAD" Failed to reallocate memory for mod->_files.paths[mod->_files.count - 1]; module is going to break", stderr);
		for (i = 0; i < mod->_files.count; i++) {
			if (i != mod->_files.count - 1) {
				free(mod->_files.paths[i]);
			}
		}
		free(mod->_files.paths);
		mod->_files.count = 0;
		return 0;
	}
	strncpy(mod->_files.paths[mod->_files.count - 1], file_name, file_name_length);

	return 1;
}

int Module_addDependency(struct Module* const mod, struct Module* to_add) {
	/* TODO this needs to be implemented */
	int i = 0;

	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot dependency to non-existant module", stderr);
		return 0;
	}

	if (to_add == NULL) {
		fputs(MESSAGE_HEAD" Cannot add a dependency that does not-exist", stderr);
		return 0;
	}

	if (mod->_deps.count == 0) {
		mod->_deps.data = malloc(sizeof(struct Module**));
		if (mod->_deps.data == NULL) {
			fputs(MESSAGE_HEAD" Failed to allocate memory for mod->_deps.data", stderr);
			mod->_deps.count = 0;
			return 0;
		}
	} else {
		mod->_deps.data = realloc(mod->_deps.data, sizeof(struct Module**) * (mod->_deps.count + 1));
		if (mod->_deps.data == NULL) {
			fputs(MESSAGE_HEAD" Failed to allocate memory for mod->_deps.data", stderr);
			mod->_deps.count = 0;
			return 0;
		}
	}

	mod->_deps.count += 1;
	if (mod->_deps.count == 1) {
		mod->_deps.data[0] = to_add;
	} else {
		mod->_deps.data[mod->_deps.count - 1] = to_add;
	}
	return 1;
}


int Module_wasCompiled(const struct Module* const mod) {
	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot check if a non-existant module was compiled", stderr);
		return 0;
	}

	return mod->_was_compiled;
}

int Module_compile(struct Module* const mod) {
	int i = 0;

	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot compile a module that does not exist", stderr);
		return 0;
	}

	if (mod->_files.count == 0) {
		fputs(MESSAGE_HEAD" Cannot compile a module with no files", stderr);
		return 0;
	}

	if (mod->_was_compiled) {
		printf(MESSAGE_HEAD" module \"%s\" was already compiled; Skipping!!!\n", mod->_name);
		return 1;
	}

	for (i = 0; i < mod->_deps.count; i++) {
		if (!mod->_deps.data[i]->_was_compiled) {
			printf(MESSAGE_HEAD" compiling submodule \"%s\"\n", mod->_deps.data[i]->_name);
			Module_compile(mod->_deps.data[i]);
		}
	}

	switch (internal_getCompiler()) {
		case COMPILER_GCC: {

				for (i = 0; i < mod->_files.count; i++) {
					char* command = NULL;
					char* obj_name = NULL;

					printf(MESSAGE_HEAD" compiling \"%s\"\n", mod->_files.paths[i]);

					obj_name = internal_generateObjectName(mod->_name, mod->_files.paths[i]);
					if (obj_name == NULL) {
						return 0;
					}

					command = calloc(23 + strlen(mod->_files.paths[i]) + 4 + strlen(obj_name) + 1, sizeof(char));
					if (command == NULL) {
						fputs(MESSAGE_HEAD" Failed to allocate memory for command", stderr);
						free(obj_name);
						return 0;
					}
					strcat(command, "gcc -ansi -pedantic -c ");
					strcat(command, mod->_files.paths[i]);
					strcat(command, " -o ");
					strcat(command, obj_name);
					
					free(obj_name);

					printf(MESSAGE_HEAD" %s\n", command);
					if (system(command) != 0) {
						free(command);
						return 0;
					} else {
						free(command);
					}

					printf(MESSAGE_HEAD" finished compiling file \"%s\"\n", mod->_files.paths[i]);
				}
			} break;
		case COMPILER_CLANG: {
				for (i = 0; i < mod->_files.count; i++) {
					char* command = NULL;
					char* obj_name = NULL;

					printf(MESSAGE_HEAD" compiling \"%s\"\n", mod->_files.paths[i]);

					obj_name = internal_generateObjectName(mod->_name, mod->_files.paths[i]);
					if (obj_name == NULL) {
						return 0;
					}

					command = calloc(18 + strlen(mod->_files.paths[i]) + 4 + strlen(obj_name) + 1, sizeof(char));
					if (command == NULL) {
						fputs(MESSAGE_HEAD" Failed to allocate memory for command", stderr);
						free(obj_name);
						return 0;
					}
					strcat(command, "clang -std=c89 -c ");
					strcat(command, mod->_files.paths[i]);
					strcat(command, " -o ");
					strcat(command, obj_name);
					
					free(obj_name);

					printf(MESSAGE_HEAD" %s\n", command);
					if (system(command) != 0) {
						free(command);
						return 0;
					} else {
						free(command);
					}

					printf(MESSAGE_HEAD" Finished compiling file \"%s\"\n", mod->_files.paths[i]);
				} break;
		case COMPILER_UNKNOWN: {
				return 0;
			} break;
		}
	}

	mod->_was_compiled = 1;
	return 1;
}

int Module_link(struct Module* const mod, const char* const exe_name) {
	char* object_names = NULL;

	if (mod == NULL) {
		fputs(MESSAGE_HEAD" Cannot link a module that does not exist", stderr);
		return 0;
	}

	if (exe_name == NULL) {
		fputs(MESSAGE_HEAD" Cannot link a module that does not have a name", stderr);
		return 0;
	}

	/* check that all modules are compiled */
	if (!internal_readyToLink(mod)) {
		fprintf(stderr, MESSAGE_HEAD" Cannot link module \"%s\" because submodules are not compiled\n", mod->_name);
		return 0;
	}

	/* generate a list of all module names */
	object_names = internal_getObjectNames(mod);
	if (object_names == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate memory for object_names", stderr);
		return 0;
	}

	/* generate the cc command */
	switch (internal_getCompiler()) {
		case COMPILER_GCC: {
			char* command = NULL;

			command = calloc(4 + strlen(object_names) + 4 + ((OBJ_OUTPUT_DIR != NULL) ? strlen(OBJ_OUTPUT_DIR) + 1 : 0) + strlen(exe_name) + 1, sizeof(char));
			if (command == NULL) {
				free(object_names);
				fputs(MESSAGE_HEAD" Failed to allocate memory for command", stderr);
				return 0;
			}
			strcat(command, "gcc ");
			strcat(command, object_names);
			strcat(command, " -o ");
			if (OBJ_OUTPUT_DIR != NULL) {
				strcat(command, OBJ_OUTPUT_DIR);
			}
			strcat(command, exe_name);
			printf(MESSAGE_HEAD" %s\n", command);
			if (system(command) == 0) {
				puts(MESSAGE_HEAD" Finished linking!");
			}
			free(command);
		} break;
		case COMPILER_CLANG: {
			char* command = NULL;

			command = calloc(6 + strlen(object_names) + 4 + strlen(exe_name) + 1, sizeof(char));
			if (command == NULL) {
				free(object_names);
				fputs(MESSAGE_HEAD" Failed to allocate memory for command", stderr);
				return 0;
			}
			strcat(command, "clang ");
			strcat(command, object_names);
			strcat(command, " -o ");
			strcat(command, exe_name);
			printf(MESSAGE_HEAD" %s\n", command);
			if (system(command) == 0) {
				puts(MESSAGE_HEAD" Finished linking!");
			}
			free(command);
		} break;
		case COMPILER_UNKNOWN: {
			free(object_names);
			return 0;
		} break;
	}

	/* execute the command & cleanup */

	free(object_names);

	return 1;
}

int SBCS_outputDirectory(const char* const text) {
	int not_dir = 0;

	if (OBJ_OUTPUT_DIR != NULL) {
		free(OBJ_OUTPUT_DIR);
	}

	if (text != NULL) {
		if (text[0] != '\0') {
			if(text[strlen(text) - 1] != '/' && text[strlen(text) - 1] != '\\') {
				not_dir = 1;
			}
		}
	}

	OBJ_OUTPUT_DIR = calloc(strlen(text) + not_dir + 1, sizeof(char));
	if (OBJ_OUTPUT_DIR == NULL) {
		fputs(MESSAGE_HEAD" Failed to allocate OBJ_OUTPUT_DIR", stderr);
		return 0;
	}
	strcpy(OBJ_OUTPUT_DIR, text);
	if (not_dir) {
		strcat(OBJ_OUTPUT_DIR, "/");
	}
	return 1;
}

void SBCS_cleanup() {
	if (OBJ_OUTPUT_DIR != NULL) {
		free(OBJ_OUTPUT_DIR);
	}
}

