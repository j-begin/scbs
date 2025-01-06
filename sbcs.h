#ifndef SBCS_HEADER_GUARD
#define SBCS_HEADER_GUADER

#include <stddef.h>

struct Module {
	char* _name;
	struct {
		char** paths;
		size_t count;
	} _files;
	struct {
		/* this struct is not responsible for freeing the modules that are assigned
		 * as dependencies to it */
		struct Module** data;
		size_t count;
	} _deps;
	char _was_compiled;
	/* used only in internal_getObjectNames() and its internal functions */
	char _was_checked;
};

/* you need to free this */
extern struct Module*    Module_new(const char* const name);
extern void              Module_free(struct Module* mod);
extern const char* const Module_getName(const struct Module* const mod);
extern int               Module_addFile(struct Module* const mod, const char* const file_name);
extern int               Module_addDependency(struct Module* const mod, struct Module* to_add);
extern int               Module_wasCompiled(const struct Module* const mod);
/* this also compiles all of their dependencies */
extern int               Module_compile(struct Module* const mod);
extern int               Module_link(struct Module* const mod, const char* const exe_name);

extern int SBCS_outputDirectory(const char* const text);

/* shuts down all extra stuff that SBCS allocated itself */
extern void SBCS_cleanup();

#endif

