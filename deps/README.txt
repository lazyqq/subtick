The root CS2Bhop.sln / .vcxproj now compiles MinHook .c files directly (no minhook.lib needed) and references ImGui sources.

It looks for them under build\_deps\ (populated by running cmake).

Easiest way:

    cmake -G "Visual Studio 17 2022" -A x64 -B build

Then open the generated solution inside the build\ folder.

If you want a completely standalone project without ever running cmake, clone:

- minhook into deps/minhook   (we need the src/*.c + include/MinHook.h)
- imgui   into deps/imgui     (the main .cpp + backends)

Then update the <ClCompile Include=...> and <AdditionalIncludeDirectories> paths in CS2Bhop.vcxproj to point at deps\ instead of build\_deps\.

After that you can open build/CS2Bhop.sln (or the generated name) and build Release x64.