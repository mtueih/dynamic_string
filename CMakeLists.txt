#===============================================================================
# CMakeLists.txt - 项目主 CMake 脚本
#===============================================================================
# 职责：
#   1. 项目声明；
#   2. 全局变量定义；
#   3. 全局模块引入；
#   4. 库构建；
#   5. 测试构建；
#   6. 库安装；
#   7. 包安装。
#===============================================================================


#-------------------------------------------------------------------------------
# 项目声明
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.24)
project(dynamic_string VERSION 2.0.0 LANGUAGES C)

if(NOT CMAKE_C_STANDARD)
	set(CMAKE_C_STANDARD 99)
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


#-------------------------------------------------------------------------------
# 全局变量定义
#-------------------------------------------------------------------------------

# 是否启用安装。
option(DYNAMIC_STRING_INSTALL "Whether to enable installation." OFF)

set(MAIN_PACKAGE_NAME ${PROJECT_NAME})
set(MAIN_LIB_TARGET_NAME ${PROJECT_NAME})


#-------------------------------------------------------------------------------
# 全局模块引入
#-------------------------------------------------------------------------------

include(cmake/CPM.cmake)


#-------------------------------------------------------------------------------
# 库构建
#-------------------------------------------------------------------------------
add_library(${MAIN_LIB_TARGET_NAME} STATIC src/${MAIN_LIB_TARGET_NAME}.c)
add_library(${MAIN_PACKAGE_NAME}::${MAIN_LIB_TARGET_NAME} ALIAS ${MAIN_LIB_TARGET_NAME})

target_sources(
		${MAIN_LIB_TARGET_NAME} PUBLIC
		FILE_SET ${MAIN_LIB_TARGET_NAME}_public_headers
		TYPE HEADERS
		BASE_DIRS include
		FILES include/${MAIN_PACKAGE_NAME}/${MAIN_LIB_TARGET_NAME}.h
)

CPMAddPackage("gh:mtueih/safe_calc#v1.0.1")
target_link_libraries(${MAIN_LIB_TARGET_NAME} PRIVATE safe_calc::safe_calc)


#-------------------------------------------------------------------------------
# 测试构建
#-------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
	include(CTest)

	if(BUILD_TESTING)
		# 主库的单元测试可执行文件的目标名称列表。
		set(TEST_NAMES ${MAIN_LIB_TARGET_NAME})

		# CPM.cmake 集成 Unity 测试框架。
		CPMAddPackage("gh:ThrowTheSwitch/Unity#v2.7.0")

		foreach(TEST_NAME ${TEST_NAMES})
			set(TEST_TARGET_NAME test_${TEST_NAME})

			# 构建测试可执行文件。
			add_executable(${TEST_TARGET_NAME} tests/${TEST_TARGET_NAME}.c)

			# 链接项目主库与 Unity 测试框架。
			target_link_libraries(${TEST_TARGET_NAME} PRIVATE ${MAIN_LIB_TARGET_NAME})
			target_link_libraries(${TEST_TARGET_NAME} PRIVATE unity::framework)

			# 注册测试可执行文件到 CTest。
			add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})
		endforeach()
	endif()
endif()


#-------------------------------------------------------------------------------
# 库安装
#-------------------------------------------------------------------------------
if(DYNAMIC_STRING_INSTALL)
	install(
			TARGETS ${MAIN_LIB_TARGET_NAME}
			EXPORT ${MAIN_PACKAGE_NAME}Targets
			FILE_SET ${MAIN_LIB_TARGET_NAME}_public_headers DESTINATION include
	)
endif()


#-------------------------------------------------------------------------------
# 包安装
#-------------------------------------------------------------------------------
if(DYNAMIC_STRING_INSTALL)
	include(CMakePackageConfigHelpers)

	# 生成并安装项目目标集文件。
	install(
			EXPORT ${MAIN_PACKAGE_NAME}Targets
			DESTINATION lib/cmake/${MAIN_PACKAGE_NAME}
			NAMESPACE ${MAIN_PACKAGE_NAME}::
	)

	# 从模板文件生成包配置文件。
	configure_package_config_file(
			# 模板文件（cmake/${MAIN_PACKAGE_NAME}Config.cmake.in），需要手动编写。
			cmake/${MAIN_PACKAGE_NAME}Config.cmake.in
			${MAIN_PACKAGE_NAME}Config.cmake
			INSTALL_DESTINATION lib/cmake/${MAIN_PACKAGE_NAME}
	)

	# 生成包版本文件。
	write_basic_package_version_file(
			${MAIN_PACKAGE_NAME}ConfigVersion.cmake
			VERSION ${PROJECT_VERSION}
			# 兼容性设置为 SameMajorVersion，表示主版本号相同则兼容。
			COMPATIBILITY SameMajorVersion
	)

	# 安装生成的包配置文件和包版本文件。
	install(
			FILES
			${CMAKE_CURRENT_BINARY_DIR}/${MAIN_PACKAGE_NAME}Config.cmake
			${CMAKE_CURRENT_BINARY_DIR}/${MAIN_PACKAGE_NAME}ConfigVersion.cmake
			DESTINATION lib/cmake/${MAIN_PACKAGE_NAME}
	)
endif()
