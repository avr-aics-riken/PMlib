#ifndef _PM_MPI_STUBS_H_
#define _PM_MPI_STUBS_H_

/* ############################################################################
 *
 * PMlib - Performance Monitor library
 *
 * Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
 * All rights reserved.
 *
 * Copyright (c) 2012-2015 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 * ############################################################################
 */

/**
 * @file   mpi_stub.h
 * @brief  stub for serial
 * @author aics
 */

namespace pm_lib {
  
#define MPI_COMM_WORLD 0
#define MPI_INT  1
#define MPI_CHAR 2
#define MPI_DOUBLE 3
#define MPI_UNSIGNED_LONG 4
  
#define MPI_SUCCESS true
  
  typedef int MPI_Comm;
  typedef int MPI_Datatype;

  
  inline bool MPI_Init(int* argc, char*** argv) { return true; }
  
  inline int MPI_Comm_rank(MPI_Comm comm, int *rank)
  {
    *rank = 0;
    return 0;
  }
  
  inline int MPI_Comm_size(MPI_Comm comm, int *size)
  {
    *size = 1;
    return 0;
  }
  
  inline int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                           MPI_Comm comm)
  {
    return 0;
  }
  
  inline int MPI_Gather(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                        void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                        int root, MPI_Comm comm)
  {
    return 0;
  }

  inline int MPI_Barrier(MPI_Comm comm)
  {
    return 0;
  }
  
  inline int MPI_Finalize(void)
  {
    return 0;
  }
  
}

#endif /* _PM_MPI_STUBS_H_ */
