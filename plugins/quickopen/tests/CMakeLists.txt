if(BUILD_TESTING)
    add_library(quickopentestbase STATIC
        quickopentestbase.cpp
        ../projectfilequickopen.cpp)

    target_link_libraries(quickopentestbase PUBLIC
        Qt5::Test KF5::IconThemes KDev::Tests KDev::Project KDev::Language)
endif()

ecm_add_test(test_quickopen.cpp LINK_LIBRARIES quickopentestbase)

if(NOT COMPILER_OPTIMIZATIONS_DISABLED)
    ecm_add_test(bench_quickopen.cpp LINK_LIBRARIES quickopentestbase)
    set_tests_properties(bench_quickopen PROPERTIES TIMEOUT 30)
endif()
