add_subdirectory(test)
IF(TBB_FOUND)
  add_subdirectory(pso)
ENDIF(TBB_FOUND)
