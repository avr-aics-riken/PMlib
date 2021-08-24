#ifndef _PM_MPI_STUBS_H_
#define _PM_MPI_STUBS_H_

/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2020 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2020 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
 */

/**
 * @file   mpi_stub.h
 * @brief  stub for serial
 * @author aics
 */

#include <cstdlib>
//	#include <cstdio>

namespace pm_lib {

#define MPI_COMM_WORLD 0
#define MPI_INT  1
#define MPI_CHAR 2
#define MPI_DOUBLE 3
#define MPI_UNSIGNED_LONG 4
#define MPI_LONG 4

  typedef int MPI_Comm;
  typedef int MPI_Datatype;
  typedef int MPI_Op;
  typedef int MPI_Group;

#define MPI_SUCCESS true
#define MPI_SUM (MPI_Op)(0x58000003)


  inline bool MPI_Init(int* argc, char*** argv) { return true; }

  inline int MPI_Abort(MPI_Comm comm, int ier)
	{
	std::exit(ier);
	//	std::printf("*** MPI_Abort is called. error code %d ***\n", ier);
	}

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

  inline int MPI_Reduce(void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
  {
    return 0;
  }

  inline int MPI_Allreduce(void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
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

  inline int MPI_Group_size(MPI_Group group, int *size)
  {
    *size = 1;
    return 0;
  }

  inline int MPI_Group_rank(MPI_Group group, int *rank)
  {
    *rank = 0;
    return 0;
  }

  inline int MPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup)
  {
    *newgroup = 0;
    return 0;
  }

  inline int MPI_Comm_group(MPI_Comm comm, int *group)
  {
    *group = 0;
    return 0;
  }

}

#endif /* _PM_MPI_STUBS_H_ */

