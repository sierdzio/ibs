#pragma once

#include <QLatin1String>

/*!
 * Tags contains all the repeating strings, put in a single place for convenience.
 * Whenever you use some raw string somewhere in gibs, consider adding it to Tags,
 * especially if you need to repeat it elsewhere.
 *
 * The most important benefit of using such named strings is that it is much
 * harder to make a typo - your compiler will make sure the Tag is correct.
 *
 * It also makes it easier to change some strings or gibs commands - you change
 * it in one place only.
 */
namespace Tags {
// Command line flags
const QLatin1String run("run");
const QLatin1String clean("clean");
const QLatin1String quick_flag("quick");
const QLatin1String qt_dir_flag("qt-dir");
const QLatin1String jobs("jobs");
const QLatin1String commands("commands");
const QLatin1String commandSeparator(";");
const QLatin1String prefix("prefix");
const QLatin1String auto_include_flag("auto-include");
const QLatin1String auto_qt_modules_flag("auto-qt-modules");
const QLatin1String parse_whole_files("parse-whole-files");
const QLatin1String release("release");
const QLatin1String debug("debug");
const QLatin1String deployer_tool("deployer");
const QLatin1String deployer_path("deployer-path");
const QLatin1String compiler_tool("compiler");
const QLatin1String cross_compile_flag("cross-compile");
const QLatin1String sysroot("sysroot");
const QLatin1String toolchain("toolchain");
const QLatin1String androidNdkPath("android-ndk-path");
const QLatin1String androidNdkApi("android-ndk-api");
const QLatin1String androidNdkAbi("android-ndk-abi");
const QLatin1String androidSdkPath("android-sdk-path");
const QLatin1String androidSdkApi("android-sdk-api");
const QLatin1String jdkPath("jdk-path");
const QLatin1String pipe_flag("pipe");
// General
const QLatin1String gibsCacheFileName(".gibs.cache");
const QLatin1String gibsConfigFileName(".gibsPathConfig.ini");
const QLatin1String inputFile("inputFile");
const QLatin1String globalScope("Global");
// GIBS comment scope
const QLatin1String scopeOneLine("//i");
const QLatin1String scopeBegin("/*i");
const QLatin1String scopeEnd("*/");
// GIBS commands
const QLatin1String source("source");
const QLatin1String targetCommand("target");
const QLatin1String targetName("name");
const QLatin1String targetType("type");
const QLatin1String targetApp("app");
const QLatin1String targetLib("lib");
const QLatin1String targetLibStatic("static");
const QLatin1String targetLibDynamic("dynamic");
const QLatin1String defines("define");
const QLatin1String include("include");
const QLatin1String includes("includes");
const QLatin1String libs("lib");
const QLatin1String tool("tool");
const QLatin1String subprojects("subprojects");
const QLatin1String subproject("subproject");
const QLatin1String depends("depends on");
const QLatin1String link("link(");
const QLatin1String linkEnd(")");
const QLatin1String version("version");
const QLatin1String feature("feature");
const QLatin1String featureDefault("default");
const QLatin1String featureOn("on");
const QLatin1String featureOff("off");
// Qt support
const QLatin1String qtDir("qtDir");
// Qt modules
const QLatin1String qtModules("qt");
const QLatin1String core("core");
const QLatin1String gui("gui");
const QLatin1String network("network");
const QLatin1String concurrent("concurrent");
const QLatin1String widgets("widgets");
const QLatin1String qml("qml");
const QLatin1String quick("quick");
const QLatin1String quickcontrols2("quickcontrols2");
const QLatin1String quickwidgets("quickwidgets");
const QLatin1String svg("svg");
const QLatin1String sql("sql");
const QLatin1String xml("xml");
const QLatin1String dbus("dbus");
const QLatin1String script("script");
const QLatin1String multimedia("multimedia");
const QLatin1String location("location");
const QLatin1String gamepad("gamepad");
const QLatin1String bluetooth("bluetooth");
// Qt tools
const QLatin1String rcc("rcc");
const QLatin1String uic("uic");
// Cache file tags
const QLatin1String parsedFiles("parsedFiles");
const QLatin1String fileChecksum("fileChecksum");
const QLatin1String fileModificationDate("fileModificationDate");
const QLatin1String scopeId("scopeId");
const QLatin1String scopeName("scopeName");
const QLatin1String scopeTargetName("targetName");
const QLatin1String scopes("scopes");
const QLatin1String scopeDependencies("scopeDependencies");
const QLatin1String relativePath("relativePath");
const QLatin1String targetLibType("targetLibType");
// Platform ifdefs
// TODO: keeping them stored here is a horrible idea. Gibs should understand
// ifdefs dynamically!
const QLatin1String osUnix("Q_OS_UNIX");
const QLatin1String osLinux("Q_OS_LINUX");
const QLatin1String osHurd("Q_OS_HURD");
const QLatin1String osAndroid("Q_OS_ANDROID");
const QLatin1String osWin("Q_OS_WIN");
const QLatin1String osWin32("Q_OS_WIN32");
const QLatin1String osWin64("Q_OS_WIN64");
const QLatin1String osDarwin("Q_OS_DARWIN");
const QLatin1String osMac("Q_OS_MAC");
const QLatin1String osMacOs("Q_OS_MACOS");
// Compilers
const QLatin1String compilerName("name");
const QLatin1String compiler("compiler");
const QLatin1String ccompiler("ccompiler");
const QLatin1String compilerFlags("flags");
const QLatin1String compilerDebugFlags("debugFlags");
const QLatin1String compilerReleaseFlags("releaseFlags");
const QLatin1String linker("linker");
const QLatin1String staticArchiver("staticArchiver");
const QLatin1String libraryPrefix("libraryPrefix");
const QLatin1String librarySuffix("librarySuffix");
const QLatin1String staticLibrarySuffix("staticLibrarySuffix");
const QLatin1String linkerFlags("linkerFlags");
const QLatin1String linkerStaticFlags("linkerStaticFlags");
const QLatin1String linkerDynamicFlags("linkerDynamicFlags");
const QLatin1String toolPrefix("toolPrefix");
const QLatin1String crossCompile("crossCompile");
// Deployers
const QLatin1String deployerName("name");
const QLatin1String deployerSuffix("suffix");
const QLatin1String deployerFlags("flags");
}
