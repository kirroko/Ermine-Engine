# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/dependencies/xstrtool"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-build"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix/tmp"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix/src/xstrtool-populate-stamp"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix/src"
  "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix/src/xstrtool-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix/src/xstrtool-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/kairu/OneDrive/Desktop/GAM300/Ermine-Engine/Ermine-ResourcePipeline/xresource_pipeline_v2-main/build/_deps/xstrtool-subbuild/xstrtool-populate-prefix/src/xstrtool-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
