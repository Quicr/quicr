

add_executable( qbroadcast qbroadcast.cc)
target_link_libraries( qbroadcast PUBLIC quicr gsl sframe)
target_include_directories( qbroadcast PRIVATE ../include )


add_executable( qpub qpub.cc)
target_link_libraries( qpub PUBLIC quicr gsl sframe)
target_include_directories( qpub PRIVATE ../include )


add_executable( qsub qsub.cc)
target_link_libraries( qsub PUBLIC quicr gsl sframe)
target_include_directories( qsub PRIVATE ../include )


add_executable( qspeed qspeed.cc)
target_link_libraries( qspeed PUBLIC quicr gsl sframe)
target_include_directories( qspeed PRIVATE ../include )


add_subdirectory(relay)
