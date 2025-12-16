#define NOB_IMPLEMENTATION
#include "nob.h"

// Define a safe max path length if it's not in your nob.h
#ifndef NOB_MAX_PATH
#define NOB_MAX_PATH 4096
#endif

// --- Configuration ---
#define CC "gcc"
#define BUILD_DIR "build"
#define SRC_DIR "src"
#define BIN "GorbinosQuest"
#define SO_NAME "gamehot.so"

//TODO simplify all of this learn more nob stuff specifically string builder

const char *cflags[] = {
    "-Wall", "-Wextra", "-O0", "-g", "-fPIC"
};

const char *ldflags[] = {
    "-ldl", "-lncurses", "-lpthread", "-rdynamic"
};

// TODO replace with nob's built in
bool check_ends_with(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int compile_obj(const char *src_path, const char *obj_path) {
    if (nob_needs_rebuild(obj_path, &src_path, 1)) {
        nob_log(NOB_INFO, "Compiling: %s", src_path);
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, CC);
        
        // Append all CFLAGS
        for (size_t i = 0; i < NOB_ARRAY_LEN(cflags); ++i) {
            nob_cmd_append(&cmd, cflags[i]);
        }

        nob_cmd_append(&cmd, "-c", src_path, "-o", obj_path);
        
        if (!nob_cmd_run_sync(cmd)) {
            nob_cmd_free(cmd);
            return 0;
        }
        nob_cmd_free(cmd);
    }
    return 1;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "clean") == 0) {
        nob_log(NOB_INFO, "Cleaning build artifacts...");
        if (system("rm -rf " BUILD_DIR) != 0) return 1;
        if (system("rm -f " BIN) != 0) return 1;
        return 0;
    }

    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    // Read all files from the source directory
    Nob_File_Paths src_files = {0};
    if (!nob_read_entire_dir(SRC_DIR, &src_files)) return 1;

    Nob_File_Paths static_objs = {0}; // For main executable
    Nob_File_Paths hot_objs = {0};    // For shared library

    // --- Compilation Phase ---
    for (size_t i = 0; i < src_files.count; ++i) {
        const char *filename = src_files.items[i];

        if (filename[0] == '.') continue;
        if (!check_ends_with(filename, ".c")) continue;

        // 2. Construct Source Path (src/foo.c)
        const char *src_path = nob_temp_sprintf("%s/%s", SRC_DIR, filename);
        
        char obj_buffer[NOB_MAX_PATH];
        size_t len = strlen(filename);

        if (len + strlen(BUILD_DIR) + 2 >= NOB_MAX_PATH) {
            nob_log(NOB_ERROR, "Path too long: %s", filename);
            return 1;
        }
        
        // Copy "build/"
        size_t offset = snprintf(obj_buffer, NOB_MAX_PATH, "%s/", BUILD_DIR);
        
        // Append "filename" (without .c) + ".o"
        // strncpy copies (len - 2) characters (i.e., "foo" from "foo.c")
        strncpy(obj_buffer + offset, filename, len - 2); 
        // strcpy appends the ".o" and null terminator
        strcpy(obj_buffer + offset + len - 2, ".o");
        
        // Store the final object path in a temporary string for the array
        const char *final_obj_path = nob_temp_sprintf("%s", obj_buffer);

        if (!compile_obj(src_path, final_obj_path)) return 1;

        // 4. Categorize into Static vs Hot based on filename
        if (strcmp(filename, "main.c") == 0 || strcmp(filename, "util.c") == 0) {
            nob_da_append(&static_objs, final_obj_path);
        } else {
            nob_da_append(&hot_objs, final_obj_path);
        }
    }

    // --- Link Main Executable (GorbinosQuest) ---
    if (nob_needs_rebuild(BIN, static_objs.items, static_objs.count)) {
        nob_log(NOB_INFO, "Linking %s", BIN);
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, CC);
        
        // CFLAGS
        for (size_t i = 0; i < NOB_ARRAY_LEN(cflags); ++i) nob_cmd_append(&cmd, cflags[i]);
        
        nob_cmd_append(&cmd, "-o", BIN);
        
        // Object files
        for (size_t i = 0; i < static_objs.count; ++i) nob_cmd_append(&cmd, static_objs.items[i]);
        
        // LDFLAGS
        for (size_t i = 0; i < NOB_ARRAY_LEN(ldflags); ++i) nob_cmd_append(&cmd, ldflags[i]);

        if (!nob_cmd_run_sync(cmd)) return 1;
        nob_cmd_free(cmd);
    }

    // --- Link Shared Library (gamehot.so) ---
    const char *so_path = nob_temp_sprintf("%s/%s", BUILD_DIR, SO_NAME);
    const char *so_tmp_path = nob_temp_sprintf("%s.tmp", so_path);

    if (nob_needs_rebuild(so_path, hot_objs.items, hot_objs.count)) {
        nob_log(NOB_INFO, "Linking shared library %s", so_path);
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, CC, "-shared", "-o", so_tmp_path);
        
        for (size_t i = 0; i < hot_objs.count; ++i) nob_cmd_append(&cmd, hot_objs.items[i]);
        
        if (!nob_cmd_run_sync(cmd)) return 1;
        nob_cmd_free(cmd);

        if (!nob_rename(so_tmp_path, so_path)) return 1;
    }

    nob_da_free(static_objs);
    nob_da_free(hot_objs);
    nob_da_free(src_files);

    return 0;
}
