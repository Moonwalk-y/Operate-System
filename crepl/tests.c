#include <testkit.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

bool define_function(const char* function_def, char func_list[],size_t *count);
bool evaluate_expression(const char* expression, int* result, int *number,const char func_list[],const size_t *count);

UnitTest(test_compile_valid_function) {
    char func_list[2000] = {0};
    size_t count = 0;
    bool result = define_function("int test_func() { return 42; }",func_list,&count);
    tk_assert(result == true, "Should successfully compile valid function");
}

UnitTest(test_compile_function_using_previous) {
    char func_list[2000] = {0};
    size_t count = 0;
    define_function("int test_func() { return 42; }",func_list,&count);

    bool result = define_function("int test_func2() { return test_func(); }",func_list,&count);
    tk_assert(result == true, "Should successfully compile function using previous function");
}

UnitTest(test_compile_invalid_syntax) {
    char func_list[2000] = {0};
    size_t count = 0;
    bool result = define_function("int invalid_func() { return 42",func_list,&count);
    tk_assert(result == false, "Should fail on syntax error");
}

UnitTest(test_evaluate_simple_constant) {
    char func_list[2000] = {0};
    size_t count = 0;
    int number = 0;
    int result_value;
    bool result = evaluate_expression("42",&result_value,&number,func_list,&count);
    tk_assert(result == true, "Should evaluate simple constant");
    tk_assert(result_value == 42, "Result should be 42");
}

UnitTest(test_evaluate_arithmetic) {
    char func_list[2000] = {0};
    size_t count = 0;
    int number = 0;
    int result_value;
    bool result = evaluate_expression("21 + 21",&result_value,&number,func_list,&count);
    tk_assert(result == true, "Should evaluate arithmetic expression");
    tk_assert(result_value == 42, "Result should be 42");
}

UnitTest(test_evaluate_function_call) {
    char func_list[2000] = {0};
    size_t count = 0;
    int number = 0;
    define_function("int test_eval() { return 100; }",func_list,&count);

    int result_value;
    bool result = evaluate_expression("test_eval()",&result_value,&number,func_list,&count);
    tk_assert(result == true, "Should evaluate function call");
    tk_assert(result_value == 100, "Result should be 100");
}

UnitTest(test_evaluate_complex_expression) {
    char func_list[2000] = {0};
    size_t count = 0;
    int number = 0;
    define_function("int test_eval() { return 100; }",func_list,&count);

    int result_value;
    bool result = evaluate_expression("test_eval() / 2",&result_value,&number,func_list,&count);
    tk_assert(result == true, "Should evaluate complex expression");
    tk_assert(result_value == 50, "Result should be 50");
}

UnitTest(test_evaluate_undefined_function) {
    char func_list[2000] = {0};
    size_t count = 0;
    int number = 0;
    int result_value;
    bool result = evaluate_expression("undefined_function()",&result_value,&number,func_list,&count);
    tk_assert(result == false, "Should fail on undefined function");
}

UnitTest(test_evaluate_syntax_error) {
    char func_list[2000] = {0};
    size_t count = 0;
    int number = 0;
    int result_value;
    bool result = evaluate_expression("21 +",&result_value,&number,func_list,&count);
    tk_assert(result == false, "Should fail on syntax error");
}
