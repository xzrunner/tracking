cmake_minimum_required(VERSION 3.8)

project(tracking)

################################################################################
# Source groups
################################################################################
set(graph
    "include/tracking/Graph.h"
    "include/tracking/Node.h"
    "source/Graph.cpp"
    "source/Node.cpp"
)
source_group("graph" FILES ${graph})

set(nodes
    "include/tracking/OpNode.h"
    "include/tracking/OpType.h"
    "include/tracking/RegNode.h"
    "include/tracking/Trace.h"
    "source/OpNode.cpp"
    "source/RegNode.cpp"
    "source/Trace.cpp"
)
source_group("nodes" FILES ${nodes})

set(tools
    "include/tracking/Compressor.h"
    "include/tracking/Serializer.h"
    "source/Compressor.cpp"
    "source/Serializer.cpp"
)
source_group("tools" FILES ${tools})

set(ALL_FILES
    ${graph}
    ${nodes}    
    ${tools}
)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(tracking STATIC ${ALL_FILES})
