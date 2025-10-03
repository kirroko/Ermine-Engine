# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-src"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-build"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix/tmp"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix/src/xcmake_tools-populate-stamp"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix/src"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix/src/xcmake_tools-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix/src/xcmake_tools-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xcmake_tools-subbuild/xcmake_tools-populate-prefix/src/xcmake_tools-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
