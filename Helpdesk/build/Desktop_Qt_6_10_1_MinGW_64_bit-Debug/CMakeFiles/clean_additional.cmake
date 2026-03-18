# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Helpdesk_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Helpdesk_autogen.dir\\ParseCache.txt"
  "Helpdesk_autogen"
  )
endif()
