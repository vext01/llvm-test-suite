file(GLOB llsources ${CMAKE_CURRENT_SOURCE_DIR}/*.ll)
SET_SOURCE_FILES_PROPERTIES(${llsources} PROPERTIES LANGUAGE CXX)

llvm_multisource(stackmaps
    ${CMAKE_CURRENT_SOURCE_DIR}/Harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StackMapParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/input_${ARCH}.ll
)
