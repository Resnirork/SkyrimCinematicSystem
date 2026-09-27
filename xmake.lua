includes("lib/commonlibsse-ng")

set_project("SkyrimCinematicSystem")
set_version("0.3.3")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

target("SkyrimCinematicSystem")
    add_rules("commonlibsse-ng.plugin", {
        name = "Skyrim Cinematic System",
        author = "Fa'Rihr",
        description = "Skyrim Cinematic System for Skyrim SE/AE using CommonLibSSE-NG"
    })
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    add_installfiles("Data/Source/Scripts/*.psc", { prefixdir = "Scripts" })
    add_installfiles("CinematicScripts/*.scs", { prefixdir = "CinematicScripts" })
    add_installfiles("Data/SKSE/Plugins/SkyrimCinematicSystem.ini", { prefixdir = "SKSE/Plugins" })
