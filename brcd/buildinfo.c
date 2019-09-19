

#include "buildinfo.h"


#ifndef GIT_VERSION
#define GIT_VERSION "unkown"
#endif




const struct buildinfo* brcd_get_buildinfo()
{
    static const struct buildinfo buildinfo = {
        .project_name = "brcd",
        .project_version = "0.9.4.1",
        .git_commit_hash = GIT_VERSION,
        .system_name = SYSTEM_NAME,
        .system_processor = SYSTEM_PROCESSOR,
        .compiler_id = COMPILER_ID,
        .compiler_version = BUILD_COMPILER_VERSION,
        .build_type = BUILD_TYPE,
    };
    return &buildinfo;
}
