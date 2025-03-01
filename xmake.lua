-- 设置项目名称和最小 xmake 版本要求
set_project("xarrow")
set_version("0.0.1")
set_xmakever("2.9.0")

set_languages("c++17")

set_policy("build.warning", true)
set_warnings("all", "extra")

add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

add_rules("mode.debug", "mode.release")
if is_mode("debug") then
    set_symbols("debug")
    add_defines("DEBUG")
    set_optimize("none")
else
    add_defines("NDEBUG")
    set_optimize("fastest")
end

add_requires("doctest")

set_pmxxheader("include/pch_stl.hpp")

target("xarrow_static")
    set_kind("static")
    add_files("src/lib/*.cpp|test_*.cpp")
    add_includedirs("include", {public = true})
    set_targetdir("build/lib")

target("xarrow_shared")
    set_kind("shared")
    add_files("src/lib/*.cpp|test_*.cpp")
    add_includedirs("include", {public = true})
    set_targetdir("build/lib")

target("xarrow_test")
    set_kind("binary")
    set_default(false)
    add_packages("doctest")
    set_pmxxheader("doctest/doctest.h")
    add_defines("TEST")
    add_deps("xarrow_static")
    add_files("src/lib/*.cpp")
    add_includedirs("test/")
    add_files("test/*.cpp")
    add_tests("default")
