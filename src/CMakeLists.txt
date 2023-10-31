# -
add_executable(swzlisp swzlisp.c swzcore.c swztypes.c swzutils.c swzparser.c swzlisp.h)
target_link_libraries(swzlisp PRIVATE edit)