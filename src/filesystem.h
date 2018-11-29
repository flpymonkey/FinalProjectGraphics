#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>  // defines FILENAME_MAX
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #ifndef GetCurrentDir
        #define GetCurrentDir _getcwd
    #endif
    #ifndef PLATFORM
        #define PLATFORM 0
    #endif
#else
    #include <unistd.h>
    #ifndef GetCurrentDir
        #define GetCurrentDir getcwd
    #endif
    #ifndef PLATFORM
        #define PLATFORM 1
    #endif
#endif

std::string static cwd() {
    char c[FILENAME_MAX];

    if (!GetCurrentDir(c, sizeof(c))) {
         return std::string("");
    }

    c[sizeof(c) - 1] = '\0';
    
    std::string s = std::string(c);
    
    if (PLATFORM == 0) {
        return s.substr(0, s.find("\\build\\src"));
    }

    return s.substr(0, s.find("/build/src"));
}

std::string static path(std::string file) {
    if (PLATFORM == 0) {
        std::string win_file = file;
        int pos = win_file.find("/");
        while (pos != -1) {
            win_file.replace(pos, 1, "\\");
            pos = win_file.find("/");
        }
        return cwd() + win_file;
    }
    return cwd() + file;
}

std::string static mtl_path(std::string file, std::string texture) {
    std::string path;
    if (PLATFORM == 0) {
        path = file.substr(0, file.find_last_of("\\") + 1);
        return path + texture;
    }
    path = file.substr(0, file.find_last_of("/") + 1);
    return path + texture;
}

#endif
