#pragma once

#include <dpsim/MNASolver.h>

#include <cuda_runtime.h>
#include <cusolverDn.h>

#define CUDA_ERROR_HANDLER(func) {cudaError_t error; if((error = func) != cudaSuccess) std::cerr << cudaGetErrorString(error) << std::endl; }

/**
 * TODO:
 *    -Proper error-handling
 */

namespace DPsim {
	template <typename VarType>
    class MnaSolverGpu : public MnaSolver<VarType>{
	protected:

		// #### Attributes required for GPU ####
		/// Solver-Handle
		cusolverDnHandle_t mCusolverHandle;
		/// Stream
		cudaStream_t mStream;

		/// Variables for solving one Equation-system (All pointer are device-pointer)
		struct GpuData {
			/// Device copy of System-Matrix
			double *matrix;
			/// Size of one dimension
			UInt size;
			/// Device copy of Vector
			double *vector;

			/// Device-Workspace for getrf
			double *workSpace;
			/// Pivoting-Sequence
			int *pivSeq;
			/// Errorinfo
			int *errInfo;
		} mDeviceCopy;

		/// Initialize cuSolver-library
        void initialize();
        /// Allocate Space for Vectors & Matrices on GPU
        void allocateDeviceMemory();
		/// Copy Systemmatrix to Device
		void copySystemMatrixToDevice();
		/// LU factorization
		void LUfactorization();

	public:
		MnaSolverGpu(String name,
			CPS::Domain domain = CPS::Domain::DP,
			CPS::Logger::Level logLevel = CPS::Logger::Level::info);

		virtual ~MnaSolverGpu();

		CPS::Task::List getTasks();

		class SolveTask : public CPS::Task {
		public:
			SolveTask(MnaSolverGpu<VarType>& solver, Bool steadyStateInit) :
				Task(solver.mName + ".Solve"), mSolver(solver), mSteadyStateInit(steadyStateInit) {
				for (auto it : solver.mMNAComponents) {
					if (it->template attribute<Matrix>("right_vector")->get().size() != 0) {
						mAttributeDependencies.push_back(it->attribute("right_vector"));
					}
				}
				for (auto node : solver.mNodes) {
					mModifiedAttributes.push_back(node->attribute("v"));
				}
				mModifiedAttributes.push_back(solver.attribute("left_vector"));
			}

			void execute(Real time, Int timeStepCount);

		private:
			MnaSolverGpu<VarType>& mSolver;
			Bool mSteadyStateInit;
		};

		class LogTask : public CPS::Task {
		public:
			LogTask(MnaSolverGpu<VarType>& solver) :
				Task(solver.mName + ".Log"), mSolver(solver) {
				mAttributeDependencies.push_back(solver.attribute("left_vector"));
				mModifiedAttributes.push_back(Scheduler::external);
			}

			void execute(Real time, Int timeStepCount);

		private:
			MnaSolverGpu<VarType>& mSolver;
		};
    };
}