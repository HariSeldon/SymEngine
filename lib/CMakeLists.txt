set(LIB_SRC_FILES "NDRange.cpp" 
                  "OCLEnv.cpp" 
                  "Utils.cpp" 
                  "YAMLReader.cpp"
                  "NDRangePoint.cpp" 
                  "SubscriptAnalysis.cpp" 
                  "Warp.cpp" 
                  "YAMLTraits.cpp"
                  "MemoryAccessesAnalyzer.cpp" 
                  "NDRangeSpace.cpp" 
                  "YAMLParser.cpp"
                  "BlockMask.cpp")

# Files registering passes must be linked in the final module library.
set(SYM_EXE_FILE "SymbolicExecution.cpp" "ControlDependenceAnalysis.cpp")

add_library(${SYM_ENGINE} MODULE ${SYM_EXE_FILE})
add_library(${SYM_ENGINE_CORE} ${LIB_SRC_FILES})

set_target_properties(${SYM_ENGINE} PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")
set_target_properties(${SYM_ENGINE_CORE} PROPERTIES COMPILE_FLAGS "-fno-rtti -fPIC")

target_link_libraries(${SYM_ENGINE} ${SYM_ENGINE_CORE})

install_targets("/${LIB_DIR}/" ${SYM_ENGINE})
