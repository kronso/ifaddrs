#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG
#ifdef WIN_PLATFORM
    #include <windows.h>
#endif
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>

    // When the user chooses an invalid ERROR_TYPE below
    #define INVALID_ERROR_TYPE (-2)  
    enum ERROR_TYPE
    {
        SYSTEM_ERR = 1,
        SOCKET_ERR,
    };

    int get_error(enum ERROR_TYPE err_type);
    char* error_str(int err_code);

    // ok lol ??~!
    // If err_type arg in ASSERT is not in ERROR_TYPE, it will fail to assert.
    #define ASSERT_ASSERT(err_type) ({if (get_error(err_type) == INVALID_ERROR_TYPE)                        \
        {fprintf(stderr, "Failed to assert (%s:%d): Invalid error type in assert.\n", __FILE__, __LINE__);  \
        exit(EXIT_FAILURE);}})

    #define ASSERT(exp, err_type) (ASSERT_ASSERT(err_type), (!(exp) ?           \
        ({fprintf(stderr, "Error %d (%s:%d): -- %s\n", get_error(err_type),     \
        __FILE__, __LINE__, error_str(get_error(err_type)));                    \
        exit(EXIT_FAILURE);})                                                   \
        : (exp)))
#else
    #define ASSERT(exp, err_code, err_msg)
#endif
