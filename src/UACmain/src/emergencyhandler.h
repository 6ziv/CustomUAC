#ifndef ENERGENCYHANDLER_H
#define ENERGENCYHANDLER_H
#include <Windows.h>
#include <string>
void run_TI();
void try_restore();
void parse_args(int argc,char** argv);
LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS exception);
extern int error_level;
extern std::string arguments;
#endif // ENERGENCYHANDLER_H
