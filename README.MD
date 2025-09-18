## Build Instructions (Windows)

This page has detailed information on building Chromium on Windows, including tips for troubleshooting and for speeding up the build.

### Contents

1.  [Build environment](#build-environment)
    1.  [Prerequisite software](#prerequisite-software)
        1.  [Important: As of May 2012 the preferred toolchain is Visual Studio 2010. Visual Studio 2008 support will be deprecated in the near future](#important-vs2010)
        2.  [Setting up the environment for Visual Studio 2008](#setting-up-for-vs2008)
        3.  [Setting up the environment for Visual Studio 2010](#setting-up-for-vs2010)
        4.  [Using Visual C++ compiler from Windows SDK 7.1](#using-vc-from-sdk)
2.  [Building Chromium](#building-chromium)
    1.  [Accelerating the build](#accelerating-the-build)
    2.  [Optional](#optional)
        1.  [Rebuild / Clean](#rebuild-clean)
        2.  [Build Chromium with Ninja](#build-with-ninja)
        3.  [Build Chromium using Visual Studio from the Command Line](#build-with-vs-cli)
        4.  [Build Chromium with MSBuild](#build-with-msbuild)
        5.  [Official/WPO/LTCG build](#official-build)
        6.  [Unit tests](#unit-tests)
        7.  [Other tricks](#other-tricks)
3.  [Running Chromium](#running-chromium)
    1.  [Packaging](#packaging)
4.  [Troubleshooting](#troubleshooting)
    1.  [Build failures on Vista](#build-failures-on-vista)
    2.  [Compilation failures](#compilation-failures)
    3.  [Cygwin access control issues](#cygwin-access-control-issues)
    4.  [Manually registering the Platform SDK](#manually-registering-sdk)
        1.  [VS2008](#sdk-vs2008)
        2.  [VS2010](#sdk-vs2010)
    5.  [Reinstalling Visual Studio SP1 (VS2008)](#reinstalling-vs2008-sp1)
    6.  [Special Service Pack Preparation Tool for Visual Studio 2008](#special-sp-prep-tool)
    7.  [Native Client Doesn't Run](#nacl-doesnt-run)
    8.  [NTFS recommended for building Native Client for Chrome](#ntfs-for-nacl)
    9.  [Installing VisualStudio 2010 SP1 after Windows SDK 7.1](#vs2010sp1-after-sdk71)
    10. [Installing Windows SDK 7.1 into non-default folder](#sdk71-non-default-folder)

---

### <a name="build-environment"></a>Build environment

#### <a name="prerequisite-software"></a>Prerequisite software

*   Windows XP SP3 or later.
*   A 64 bit OS is highly recommended as building on 32 bit OS is constantly becoming harder, is a lot slower and is not actively maintained.
*   (Optional) Visual Studio Express, Standard, or Pro 2008 or 2010.
    *   If you don't own a copy of Visual Studio, you can (freely) download [Visual Studio 2008 Express w/SP1](http://web.archive.org/web/20120510032223/http://download.microsoft.com/download/a/5/4/a54badb6-9c3f-478d-8657-93b3fc9fe62d/vcsetup.exe)
    *   For non-Express versions:
        *   Make sure that "X64 Compilers and Tools" are installed as part of Visual Studio - **Important!** If you don't install this now, you won't be able to correct your installation later without uninstalling due to a bug in VS2008 SP1.
    *   For VS 2008, if you're using the Standard Edition, make sure to also install C# development. Otherwise cl.exe won't be copied. See [KB969866](http://web.archive.org/web/20120510032223/http://support.microsoft.com/kb/969866).
    *   For Express versions, keep in mind that debugging is extremely limited.
*   Visual C++ compiler is included to Windows SDK 7.1 so it is possible to build Chromium without having Visual Studio installed. You will have to build from the command line, supply your favorite code editor and use [WinDBG](http://web.archive.org/web/20120510032223/http://www.microsoft.com/whdc/DevTools/Debugging/default.mspx) for debugging if you choose to go this way.
*   Additional downloads:
    *   [Windows 7.1 SDK](http://web.archive.org/web/20120510032223/https://www.microsoft.com/downloads/en/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b)
    *   [June 2010 DirectX SDK](http://web.archive.org/web/20120510032223/http://www.microsoft.com/download/en/details.aspx?displaylang=en&id=6812)
    *   (Optional) [WDK](http://web.archive.org/web/20120510032223/http://msdn.microsoft.com/en-us/windows/hardware/gg487463.aspx).
        *   WDK is required unless you are using Visual Studio Professional or above.
    *   KB articles: [KB957912](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=15363), [KB958842](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=16558), [KB960075](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=16927), [KB967631](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=17105), [KB971092](http://web.archive.org/web/20120510032223/http://www.microsoft.com/downloads/details.aspx?familyid=294de390-3c94-49fb-a014-9a38580e64cb), and [KB2519277](http://web.archive.org/web/20120510032223/https://www.microsoft.com/en-us/download/details.aspx?displaylang=en&id=4422).
    *   Install them according to the setup instructions below.
*   (Optional) [Cygwin](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/cygwin)

> #### <a name="important-vs2010"></a>Important: As of May 2012 the preferred toolchain is Visual Studio 2010. Visual Studio 2008 support will be deprecated in the near future

#### <a name="setting-up-for-vs2008"></a>Setting up the environment for Visual Studio 2008

1.  **Install Visual Studio 2008**
2.  **Install [VS2008 SP1](http://web.archive.org/web/20120510032223/https://www.microsoft.com/downloads/details.aspx?familyid=FBEE1648-7106-44A7-9649-6D9F6D58056E)** (unless you already have SP1, e.g. because you downloaded VS2008 Express w/SP1 via the link above).
3.  **Install [KB967631](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=17105), [KB960075](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=16927), and [KB957912](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=15363).**
4.  **Install the [Windows 7.1 SDK](http://web.archive.org/web/20120510032223/https://www.microsoft.com/downloads/en/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b)**
    *   Read the instructions on the SDK download page! Read it again!
    *   You might need to reboot. You can save space by not installing the documentation and code samples.
5.  **Integrate the SDK with Visual Studio**
    *   Start > All Programs > Microsoft Windows SDK > Visual Studio Registration > Windows SDK Configuration Tool.
    *   If this program crashes, try running it from the command line with: `windowssdkver -version:v7.1 -legacy`
    *   If build still fails, try [registering the SDK manually](#manually-registering-sdk).
    *   If you had a previous version of the SDK installed before installing, verify that your VC++ directories are set to use the newest and not an earlier SDK. See [registering the SDK manually](#manually-registering-sdk) for details.
6.  **If you are not using Visual Studio 2008 Express, install [KB971092](http://web.archive.org/web/20120510032223/http://www.microsoft.com/downloads/details.aspx?familyid=294de390-3c94-49fb-a014-9a38580e64cb).** Yes, that's a 365 MB update. :/
7.  **If you are using Visual Studio 2008 Team System and plan to use the profiler, install [KB958842](http://web.archive.org/web/20120510032223/https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=16558).**
8.  **Install the [June 2010 DirectX SDK](http://web.archive.org/web/20120510032223/http://www.microsoft.com/download/en/details.aspx?displaylang=en&id=6812)**
    *   Open Tools > Options > Projects and Solutions > VC++ Directories.
    *   Under "Include files" add `$(DXSDK_DIR)include` to the start of the list.
    *   Under "Library files" add `$(DXSDK_DIR)lib\x86` to the start of the list.
    *   Make sure the DirectX include and lib directories appear first in the search path, otherwise you may get build errors (e.g. error C3861: 'XInputEnable': identifier not found).
9.  **If you use Visual Studio 2008 Express:**
    *   Install the [WDK](http://web.archive.org/web/20120510032223/http://msdn.microsoft.com/en-us/windows/hardware/gg487463.aspx) for the ATL headers and libs.
    *   Verify MSVC|Tools|Options|Projects and Solutions|VC++ Directories has Include: `[WDK]include\atl71` and Lib: `[WDK]lib\Atl\i386` where `[WDK]` is the WDK install directory.
10. **(Optional) Install [cygwin](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/cygwin).**

#### <a name="setting-up-for-vs2010"></a>Setting up the environment for Visual Studio 2010

1.  **Install Visual Studio 2010 / Visual C++ 2010 Express**
2.  **Install [VS2010 SP1](http://web.archive.org/web/20120510032223/https://www.microsoft.com/downloads/en/details.aspx?FamilyID=75568aa6-8107-475d-948a-ef22627e57a5).**
3.  **Install the [Windows 7.1 SDK](http://web.archive.org/web/20120510032223/https://www.microsoft.com/downloads/en/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b).**
    *   You may need [this repair to restore the libraries](http://web.archive.org/web/20120510032223/https://www.microsoft.com/download/en/details.aspx?displaylang=en&id=4422) if the SDK was already installed when you installed SP1.
4.  **Specify the version of the Windows SDK** by setting the `msbuild_toolset` variable in `%USERPROFILE%\.gyp\include.gypi`. Create the file if needed:
    ```json
    {
      'target_defaults': {
        'msbuild_toolset': 'Windows7.1SDK',
      },
    }
    ```
5.  As of 2011-04-01, VS2010 is not detected automatically by GYP, so to generate the .sln/.vcxproj, you need to **set/export `GYP_MSVS_VERSION=2010`** (or create new Windows environment variable)
    *   Note: `2010e` for express versions.
6.  `gclient runhooks`
    *   There is an issue in gyp: [GYP-222](http://web.archive.org/web/20120510032223/http://code.google.com/p/gyp/issues/detail?id=222) AdditionalManifestFiles element should be in Manifest instead of Mt
    *   No tooltip due by lack of manifest file in EXE/DLL.
7.  **Install the [June 2010 DirectX SDK](http://web.archive.org/web/20120510032223/http://www.microsoft.com/download/en/details.aspx?displaylang=en&id=6812)**
    *   Add `$(DXSDK_DIR)\include;` to the beginning of the `IncludePath` property in `%LOCALAPPDATA%\Microsoft\MSBuild\v4.0\Microsoft.Cpp.Win32.user.props`.
    *   Add `$(DXSDK_DIR)\lib\x86;` to the beginning of the `LibraryPath` property in the same file. At this point the .props file will look like this:
        ```xml
        <?xml version="1.0" encoding="utf-8"?>
        <Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
          <PropertyGroup>
            <IncludePath>$(DXSDK_DIR)\include;$(IncludePath)</IncludePath>
            <LibraryPath>$(DXSDK_DIR)\lib\x86;$(LibraryPath)</LibraryPath>
          </PropertyGroup>
        </Project>
        ```
    *   Similarly, edit `%LOCALAPPDATA%\Microsoft\MSBuild\v4.0\Microsoft.Cpp.x64.user.props`:
        ```xml
        <?xml version="1.0" encoding="utf-8"?>
        <Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
          <PropertyGroup>
            <IncludePath>$(DXSDK_DIR)\include;$(IncludePath)</IncludePath>
            <LibraryPath>$(DXSDK_DIR)\lib\x64;$(LibraryPath)</LibraryPath>
          </PropertyGroup>
        </Project>
        ```
    *   Do not edit the files above via the Visual Studio UI (via View->Property Manager) because it may overwrite recursive `$(IncludePath)` and `$(LibraryPath)` breaking (potentially) Windows SDK selection via the `msbuild_toolset` variable.
    *   Make sure the DirectX include and lib directories appear first in the search path, otherwise you may get build errors (e.g. error C3861: 'XInputEnable': identifier not found).

8.  **If you're using Visual C++ 2010 Express:**
    *   Install the [WDK](http://web.archive.org/web/20120510032223/http://msdn.microsoft.com/en-us/windows/hardware/gg487463.aspx) for the ATL/MFC headers and libs (else you will get "Missing library atlthunk.lib" and "Missing header altbase.h" errors).
    *   After you've done this, create a system-wide environment variable named `WDK_DIR` via Run...->SystemPropertiesAdvanced->Environment Variables...->New...(in System variables), with the name of `WDK_DIR` and the location you installed the WDK (e.g. `C:\WinDDK\7600.16385.1`).
    *   Add the following paths (highlighted) to `%LOCALAPPDATA%\Microsoft\MSBuild\v4.0\Microsoft.Cpp.Win32.user.props`:
        ```xml
        <?xml version="1.0" encoding="utf-8"?>
        <Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
          <PropertyGroup>
            <IncludePath>$(DXSDK_DIR)\include;$(IncludePath);$(WDK_DIR)\inc\atl71;$(WDK_DIR)\inc\mfc42</IncludePath>
            <LibraryPath>$(DXSDK_DIR)\lib\x86;$(LibraryPath);$(WDK_DIR)\lib\ATL\i386</LibraryPath>
          </PropertyGroup>
        </Project>
        ```
    *   Add the following paths (highlighted) to `%LOCALAPPDATA%\Microsoft\MSBuild\v4.0\Microsoft.Cpp.x64.user.props`:
        ```xml
        <?xml version="1.0" encoding="utf-8"?>
        <Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
          <PropertyGroup>
            <IncludePath>$(DXSDK_DIR)\include;$(IncludePath);$(WDK_DIR)\inc\atl71;$(WDK_DIR)\inc\mfc42</IncludePath>
            <LibraryPath>$(DXSDK_DIR)\lib\x64;$(LibraryPath);$(WDK_DIR)\lib\ATL\amd64</LibraryPath>
          </PropertyGroup>
        </Project>
        ```
    *   Do not edit the files above via the Visual Studio UI (via View->Property Manager) because it may overwrite recursive `$(IncludePath)` and `$(LibraryPath)` breaking (potentially) Windows SDK selection via the `msbuild_toolset` variable.
9.  **(Optional) Install [cygwin](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/cygwin).**

#### <a name="using-vc-from-sdk"></a>Using Visual C++ compiler from Windows SDK 7.1

Follow the steps from "Setting up the environment for Visual Studio 2010" above as if you were using Visual Studio 2010 Express edition with exception of installing Visual Studio and the service pack. Refer to "Build Chromium with MSBuild" for Chromium building instructions.

---

### <a name="building-chromium"></a>Building Chromium

1.  [Get the Chromium depot_tools](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/install-depot-tools).
2.  [Check out the source code](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/get-the-code) using either the bootstrap tarball or a direct svn checkout.
3.  Open the `chrome/chrome.sln` solution file in Visual Studio and build the solution. This can take from 10 minutes to 2 hours. More likely 1 hour.
4.  If you just want the Chromium browser, and none of the tests, you can speed up your build by right-clicking the `chrome` project in the solution explorer and selecting Build. You may want to make sure this project is the Startup project (which will display as bold) by right-clicking it and selecting Set as Startup Project. This will make Chromium (as opposed to some random test) build and run when you press F5.

#### <a name="accelerating-the-build"></a>Accelerating the build

Be prepared: the build of whole solution can take hour(s) longer on a power horse if not configured correctly.

In decreasing order of speedup:

*   Use a true multicore processor, i.e. an Intel Core Duo or later (not a Pentium 4 HT).
*   Use an x64 OS (otherwise incremental linking is disabled).
*   Have at least 8 GB RAM. If you have less than 4 GB, you may find builds to be prohibitively slow.
*   Turn on [precompiled headers](http://web.archive.org/web/20120510032223/http://code.google.com/p/chromium/wiki/WindowsPrecompiledHeaders) and the [component build](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/component-build).
*   Use [`src/build/some.gyp`](http://web.archive.org/web/20120510032223/http://src.chromium.org/viewvc/chrome/trunk/src/build/some.gyp?view=markup) to only load the targets you care about.
*   Disable your anti-virus software for `.ilk`, `.pdb`, `.cc`, `.h` files and only check for viruses on modify. Disable scanning the directory where your sources reside.
*   Store and build the Chromium code on a second hard drive that does not have swap (Control Panel->System and Security->System->Advanced->Performance/Settings->Advanced->Virtual memory/Change). During compilation and linking there are a lot of memory been used and disk I/O happening. It's not a good idea to have all I/Os staying in the way on the same disk that memory manager doing its swap.
*   Consider using [ninja](http://web.archive.org/web/20120510032223/http://code.google.com/p/chromium/wiki/NinjaBuild) - it's an alternative command-line build tool that's much faster than Visual Studio, especially for incremental builds.
*   Reduce number of parallel builds. The Chromium requires a lot of memory to link; if you build to many sub-projects simultaneously your PC will slow to crawl during the linking phase because of the linkers fighting for the swap. Go into MSVC's Tools\Options\Projects and Solutions\Build and Run and reduce "maximum number of parallel project builds". By default it is equal to number of virtual processors (e. g. it will be 16 on the two I7s because of the hyper-threading). Recommended number for 12 GB of RAM is lower than 8 (see [discussions here](http://web.archive.org/web/20120510032223/http://groups.google.com/a/chromium.org/group/chromium-dev/browse_thread/thread/735ea0a2c0b7c26f/a2020eaf733615dd)). Reduce more for less RAM.
*   You may also want to tune your `include.gypi` file to limit number of `cl.exe` spawned and reduce context switches (see [discussions here](http://web.archive.org/web/20120510032223/http://groups.google.com/a/chromium.org/group/chromium-dev/browse_thread/thread/735ea0a2c0b7c26f/a2020eaf733615dd)). For example, to limit only 5 `cl.exe` and 5 linkers, the `include.gypi` under your home folder's `.gyp` should have:
    ```json
    {
      'variables': {
        'msvs_multi_core_compile': 0,
      },
      'target_defaults': {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'AdditionalOptions': ['/MP5'],
          },
        },
      },
    }
    ```
*   Defragment your hard drive regularly.
*   You may want to try [incremental linking](http://web.archive.org/web/20120510032223/http://code.google.com/p/chromium/wiki/WindowsIncrementalLinking) if you're adventurous. It makes linking very fast when you're working incrementally, but is somewhat flaky, and you may need to do cleans after syncs to avoid spurious errors.

#### <a name="optional"></a>Optional

##### <a name="rebuild-clean"></a>Rebuild / Clean

You can use Rebuild and Clean on individual projects to rebuild them. However, because of the large number of files generated by the build system, avoid using Rebuild All or Clean All - these do not erase everything! Instead delete the entire output directory (`chrome\Debug` or `chrome\Release`).

##### <a name="build-with-ninja"></a>Build Chromium with Ninja

If you like building from the command line, consider using [ninja](http://web.archive.org/web/20120510032223/http://code.google.com/p/chromium/wiki/NinjaBuild) instead - it's much faster, especially for incremental builds! The only downside is that the intermediate object files are not the same as Visual Studio, so you can't switch back and forth between the Visual Studio IDE and Ninja for building.

##### <a name="build-with-vs-cli"></a>Build Chromium using Visual Studio from the Command Line

You would need to have Visual Studio installed, but not running.

Use Visual Studio 2008 Command Prompt in your Start Menu instead of cmd. Navigate to source directory (`src/chrome`) and type the following command:

```
C:\trunk\src\chrome> devenv.exe /build Debug chrome.sln /project chrome.vcproj
```

You can also use `devenv.com` with the same command-line. This will show the output of the various build commands, which can be useful when trying to track down errors during building.

**Note for Windows Vista:** the devenv command may silently fail if Visual Studio has been configured to always run as Administrator. You can check or disable this by right clicking the Visual Studio application icon, then select Properties -> Compatibility -> Run this program as administrator.

##### <a name="build-with-msbuild"></a>Build Chromium with MSBuild

If you use Visual Studio 2010 or the compiler from Windows SDK 7.1 you can build Chromium with [MSBuild](http://web.archive.org/web/20120510032223/http://msdn.microsoft.com/en-us/library/wea2sca5(v=vs.90).aspx).

1.  Launch Visual Studio 2010 Command Prompt (for Visual Studio 2010) or Windows SDK 7.1 Command Prompt (if you don't have Visual Studio installed) instead of cmd.
2.  If you are using Windows SDK 7.1 Command Prompt run `"set TARGET_CPU="` to enable building x64 projects.
3.  Add targets you want to build to `src/build/some.gyp`.
4.  Run `gclient runhooks`.
5.  Run the following command:
    ```
    C:\trunk\src\build> msbuild.exe some.sln /p:Configuration:Debug /p:Platform=Win32 /m
    ```
6.  You can also customize logging parameters to reduce console spew and/or redirect verbose logs to a file:
    ```
    msbuild.exe some.sln /p:Configuration:Debug /p:Platform=Win32 /m /consoleloggerparameters:verbosity=minimal /l:FileLogger,Microsoft.Build.Engine;logfile="debug.log";verbosity=detailed
    ```

##### <a name="official-build"></a>Official/WPO/LTCG build

To enable whole program optimization / link-time code generation, make sure to set the environment variable:

1.  `set GYP_DEFINES=branding=Chromium buildtype=Official`
    *   If you have the internal checkout, use `Chrome` instead of `Chromium` to get the Google Chrome resources.
2.  `gclient runhooks`
3.  `src\build\all.sln`
4.  Select 'Release' configuration
5.  Right-click on 'chrome' project and click 'Build'. You don't want to build the tests.
6.  The WPO configuration settings are defined in [`release_impl_official.gypi`](http://web.archive.org/web/20120510032223/http://src.chromium.org/viewvc/chrome/trunk/src/build/internal/release_impl_official.gypi?&view=markup).

##### <a name="unit-tests"></a>Unit tests

Some unit tests (such as `gfx_unittests`) require at least Vista/2008 with [Platform Update](http://web.archive.org/web/20120510032223/http://support.microsoft.com/kb/971644) installed or Windows 7 to execute all of the tests completely.

##### <a name="other-tricks"></a>Other tricks

See [Visual Studio Macros](http://web.archive.org/web/20120510032223/http://code.google.com/p/chromium/wiki/WindowsVisualStudioMacros) page and [debugging](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/debugging) pages.
[Blog post](http://web.archive.org/web/20120510032223/http://terboven.spaces.live.com/Blog/cns!EA3D3C756483FECB!194.entry) on setting a right-hand print margin.

---

### <a name="running-chromium"></a>Running Chromium

The `chrome.exe` executable can be found at `src/build/Debug/chrome.exe` or `src/build/Release/chrome.exe`, depending on the selected build configuration.

Because of Chromium's unique architecture, there are a number of special challenges associated with development. See [Debugging Chromium](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/how-tos/debugging) for more information.

Once you're comfortable with building Chromium, read the [Contributing Code](http://web.archive.org/web/20120510032223/http://www.chromium.org/developers/contributing-code) page for information about writing code for Chromium and contributing it.

#### <a name="packaging"></a>Packaging

If you want to package your build into a zip file, do the following:

```bash
cd /path/to/chrome
./tools/build/win/make_zip.sh Release my-chromium
```

This will create `my-chromium.zip`. You can change `Release` to `Debug` if you want to zip up the debug build instead.

---

### <a name="troubleshooting"></a>Troubleshooting

#### <a name="build-failures-on-vista"></a>Build failures on Vista

If you build on Vista, watch out for security issues. Make sure that the folder where you checked out your files is writable for users and not only for admins.

#### <a name="compilation-failures"></a>Compilation failures

Some common things to think about when you have weird compilation failures:

*   If you happen to have checked out the source code before installing the build prerequisites (Visual Studio, the SDK, and all the patches), you'll need to re-generate the VS solution files by quitting Visual Studio and then running "`gclient runhooks`".
*   Sometimes Visual Studio does the wrong thing when building Chromium and gets stuck on a bogus error. A good indication of this is if it is only failing for one person but others (including the Buildbots) are not complaining. To resolve this, try the following steps:
    1.  Close Visual Studio.
    2.  Sync to the tip of tree and ensure there are no conflicts ("`svn status`" should not show any "`C`"s in front of files that you've changed).
    3.  If there were conflicts, sync again after resolving them.
    4.  Manually erase the output directory (`chrome\Debug` and `chrome\Release`. Using the command line, you can use "`erase /S /Q Debug Release`" from the chrome directory to do this, or "`rm -rf Debug Release`" if you have Unix-like tools installed).
    5.  Restart Visual Studio and open the Chromium solution.
    6.  Rebuild the solution.
*   If you get errors like these:
    *   `C:\Program Files\Microsoft SDKs\Windows\v6.1\Include\unknwn.idl(108) : error MIDL2025 : syntax error : expecting ] or , near "annotation"`
    *   `browser\download\download_util.cc(469) : error C2065: 'ITaskbarList3' : undeclared identifier`
    You either have the wrong Platform SDK or it is incorrectly registered. Try installing the [Windows 7 SDK](http://web.archive.org/web/20120510032223/http://www.microsoft.com/downloads/details.aspx?FamilyID=c17ba869-9671-4330-a63e-1fd44e0e2505) or, if already installed, try [registering the SDK manually](#manually-registering-sdk).
*   If you get errors like this:
    *   `error C2039: '_Swap_adl' : is not a member of 'std'`
    Then you've likely run into a conflict between the Windows SDK and a security patch. See this [MSDN blog post](http://web.archive.org/web/20120510032223/http://blogs.msdn.com/windowssdk/archive/2009/08/07/installing-windows-sdk-for-server-2008-v6-1-after-vs2008-sp1-causes-conflicts-with-security-update-kb971092.aspx) for more details try [reinstalling Visual Studio SP1](#reinstalling-vs2008-sp1).
*   If you get errors like this:
    ```
    3>c:\program files (x86)\microsoft directx sdk (june 2010)\include\d3d11shader.h(35) : error C2146: syntax error : missing ';' before identifier 'D3D11_RESOURCE_RETURN_TYPE'
    3>c:\program files (x86)\microsoft directx sdk (june 2010)\include\d3d11shader.h(35) : error C4430: missing type specifier - int assumed. Note: C++ does not support default-int
    3>c:\program files (x86)\microsoft directx sdk (june 2010)\include\d3d11shader.h(35) : error C4430: missing type specifier - int assumed. Note: C++ does not support default-int
    3>c:\program files (x86)\microsoft directx sdk (june 2010)\include\d3d11shader.h(37) : error C2146: syntax error : missing ';' before identifier 'D3D11_CBUFFER_TYPE'
    ```
    Then make sure the DirectX headers appear first in the include search path (MSVC|Tools|Options|Projects and Solutions|VC++ Directories). After making the change the retrying the build without clobber should work.

If it still doesn't work, repeating this process probably won't help.

#### <a name="cygwin-access-control-issues"></a>Cygwin access control issues

If, while building JavaScriptCore, you see errors like:

```
3>Error in tempfile() using /tmp/dftables-XXXXXXXX.in: Parent directory (/tmp/) is not writable
3> at /cygdrive/c/b/slave/WEBKIT~1/build/webkit/third_party/JavaScriptCore/pcre/dftables line 236
3>make: *** [chartables.c] Error 255
```

...it's because the Cygwin installation included in the Chromium source is having trouble mapping the NT ACL to POSIX permissions. This seems to happen when Chromium is checked out into a directory for which Cygwin can't figure out the permissions in the first place, possibly when the directory is created from within a Cygwin environment before running `mkpasswd`. Cygwin then imposes its own access control, which is incorrectly restrictive. As a workaround, do one of the following:

*   Edit the NT permissions on `third_party\cygwin\tmp` to allow Modify and Write actions for Everyone and machine\Users. Cygwin is able to figure this out. Or,
*   Figure out what went wrong with your checkout and try again - try doing the checkout from `cmd` instead of from a Cygwin shell, then verify that the permissions aren't completely blank in your Cygwin installation. Or,
*   Bypass Cygwin's access control (NT's will still be in effect) by editing `webkit\build\JavaScriptCore\prebuild.bat` and `webkit\build\WebCore\prebuild.bat` to include the following line before invoking anything that uses Cygwin:
    ```
    set CYGWIN=nontsec
    ```
    Note: the `nontsec` option is not supported in cygwin 1.7 and greater.
*   Edit the cygwin file `/etc/fstab` and add the `noacl` option
    ```
    none /cygdrive cygdrive binary,posix=0,user,noacl 0 0
    ```

Only one of these solutions should be needed.

#### <a name="manually-registering-sdk"></a>Manually registering the Platform SDK

##### <a name="sdk-vs2008"></a>VS2008

Open Visual Studio, Tools > Options... > Projects and Solutions > VC++ Directories.
Assuming your base SDK directory is `C:\Program Files\Microsoft SDKs\Windows\v7.1`, remove any existing entries v6.0 and v6.1 and add the following:

*   In Executable Files, insert on top `C:\Program Files\Microsoft SDKs\Windows\v7.1\bin`
*   In Include Files, insert on top `C:\Program Files\Microsoft SDKs\Windows\v7.1\Include` and `C:\Program Files\Microsoft SDKs\Windows\v7.1\Include\gl`
*   In Library Files, insert on top `C:\Program Files\Microsoft SDKs\Windows\v7.1\lib`
*   In Exclude directories, insert on top `C:\Program Files\Microsoft SDKs\Windows\v7.1\Include` and `C:\Program Files\Microsoft SDKs\Windows\v7.1\Include\gl`

##### <a name="sdk-vs2010"></a>VS2010

GYP picks up the proper version SDK automatically according to the 'msbuild_toolset' variable set in `%USERPROFILE%\.gyp\include.gypi`.

*   If the variable is not set 7.0A SDK (also known as 'v100') is used. (I.e. the one that is included with Visual Studio).
*   Windows 7.1 SDK is selected by setting the variable to '`Windows7.1SDK`'.

#### <a name="reinstalling-vs2008-sp1"></a>Reinstalling Visual Studio SP1 (VS2008)

If you get errors like this: `error C2039: '_Swap_adl' : is not a member of 'std'` then you've likely run into a conflict between the Windows SDK and a security patch. See this [MSDN blog post](http://web.archive.org/web/20120510032223/http://blogs.msdn.com/windowssdk/archive/2009/08/07/installing-windows-sdk-for-server-2008-v6-1-after-vs2008-sp1-causes-conflicts-with-security-update-kb971092.aspx) for more details. The workaround is to re-download and over-install SP1 for Visual Studio 2008. Note that at this point you're going to need to re-install all of the hotfixes from above and you should verify that the SDK directories are correct and fix them if not (see "Manually registering the Platform SDK" above).

#### <a name="special-sp-prep-tool"></a>Special Service Pack Preparation Tool for Visual Studio 2008

If you are having weird compilation problems on Visual Studio 2008, you may have missed some small prints when downloading the Service Pack 1. Here's an excerpt for you:
> If you previously installed a Visual Studio 2008 Hotfix or Visual Studio 2008 SP1 pre-release, you must run the [Service Pack Preparation tool](http://web.archive.org/web/20120510032223/http://www.microsoft.com/downloads/details.aspx?FamilyId=A494B0E0-EB07-4FF1-A21C-A4663E456D9D) before installing Visual Studio 2008 SP1.

#### <a name="nacl-doesnt-run"></a>Native Client Doesn't Run

If you're on 64-bit Windows, you need to build the `chrome_nacl_win64` project manually. It isn't a hard project dependency so that it doesn't break 32-bit builds.

If you try to build it and get this error: `LINK : fatal error LNK1104: cannot open file 'atlsd.lib'`

It means that you didn't install the 64-bit compiler and tools as described at the top of the page. Unfortunately, due to a bug in Visual Studio 2008 SP1, you can't add them after the fact once you've installed SP1. As a workaround, first uninstall SP1, then go to the Windows control panel and "uninstall/change" Visual Studio 2008. Open up the C++ feature and check the x64 compiler and tools feature. Once that install finishes, reinstall SP1. Now you should be able to build `chrome_dll_nacl_win64`.

**ALTERNATIVELY,** you may be able to avoid the SP1 uninstall/install using this update from Microsoft:
[https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=27638](https://connect.microsoft.com/VisualStudio/Downloads/DownloadDetails.aspx?DownloadID=27638)
See this post for more information:
[http://connect.microsoft.com/VisualStudio/feedback/details/487382/installing-visual-studio-2008-sp1-disables-ability-to-make-changes-to-visual-studio-installed-features](http://connect.microsoft.com/VisualStudio/feedback/details/487382/installing-visual-studio-2008-sp1-disables-ability-to-make-changes-to-visual-studio-installed-features)

#### <a name="ntfs-for-nacl"></a>NTFS recommended for building Native Client for Chrome

The Native Client toolchain requires hardlinks to install properly. We recommend that you only use NTFS for your project. In particular, ExFAT is known to not work properly, and will result in errors similar to this:

```
________ running 'c:\depot_tools\python_bin\python.exe src/build/download_nacl_toolchains.py --x86-version 6494 --nacl-newlib-only --file-hash mac_x86_newlib 1b0855435c03c435a011c6105a509624b2a4edaa --file-hash win_x86_newlib 5038a47b5a9a49acdc36cbe311aec7bce575c164 --file-hash linux_x86_newlib 01e245dc6dca16bea5cf840dbc77e3aa138f234f' in 'F:\src\k0'
tmptar/tar: sdk/nacl-sdk/bin/i686-nacl-ar.exe: Cannot hard link to `sdk/nacl-sdk/bin/i686-nacl-addr2line.exe': Operation not permitted
```

#### <a name="vs2010sp1-after-sdk71"></a>Installing VisualStudio 2010 SP1 after Windows SDK 7.1

There is a bug in VisualStudio 2010 SP1 that it will corrupt existing Windows SDK 7.1 installation. Microsoft KB 2519277 is the fix:
[http://support.microsoft.com/?kbid=2519277](http://web.archive.org/web/20120510032223/http://support.microsoft.com/?kbid=2519277).

#### <a name="sdk71-non-default-folder"></a>Installing Windows SDK 7.1 into non-default folder

There is a bug in Windows SDK 7.1 that it always configures its own environment into the default folder regardless what you specified during install. There's not a known way to get around it, so just install it into default folder.

#### First run of gclient fails to install python

If the first run of `gclient` produces the error:

`svn: Can't move 'C:\Users\username\depot_tools\python_bin\tcl\Tix8.4.3\.svn\tmp\entries' to 'C:\Users\rhashimoto\depot_tools\python_bin\tcl\Tix8.4.3\.svn\entries': The file or directory is corrupted and unreadable.`

`... Failed to checkout python automatically.`

`Please visit http://python.org to download the latest python 2.x client before continuing.`

`You can also get the "prebacked" version used at http://src.chromium.org/chrome/trunk/tools/third_party/`

See [this link](http://web.archive.org/web/20120510032223/http://tortoisesvn.tigris.org/faq.html#cantmove2) for instructions on turning off Windows Indexing.