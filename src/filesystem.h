#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>  /* defines FILENAME_MAX */
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
    #define PLATFORM 0
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
    #define PLATFORM 1
#endif

std::string cwd() {
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

std::string path(std::string file) {
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

#endif
