
add_airliner_app_def(hmc5883
    FILE HMC5883
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_app.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_app.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_events.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_msg.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_version.h
   
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/

    USER_DOCS   ${CMAKE_CURRENT_SOURCE_DIR}/user_doxy.in
    DESIGN_DOCS ${CMAKE_CURRENT_SOURCE_DIR}/detail_doxy.in
)

add_airliner_app_unit_test(hmc5883-ut
    UTASSERT
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_app_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_app_test.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_cmds_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_cmds_test.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_testrunner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_test_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/hmc5883_test_utils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_app.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_app.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_events.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_msg.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/hmc5883_version.h
        
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/
        
    VALGRIND_SUPPRESSION_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/valgrind.supp
)
