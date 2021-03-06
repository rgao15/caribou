#pragma once

#include <Caribou/config.h>
#include <sofa/core/behavior/LinearSolver.h>
#include <SofaBaseLinearSolver/DefaultMultiMatrixAccessor.h>
#include <Eigen/Sparse>

namespace SofaCaribou::solver {

namespace internal {
template <typename T>
struct solver_traits {};
}

/**
 * Base class for sparse direct solvers using Eigen as a backend solver.
 *
 * Note that the complete system matrix has to be assemble for any of the derivated solvers.
 * This means that the mass, damping and forcefield components in the scene graph need to implement
 * the addMtoMatrix, addBtoMatrix and addMtoMatrix methods respectively.
 *
 * @tparam EigenSolver Eigen solver type (eg.: SimplicialLLT, SparseLU, PardisoLLT, etc.)
 */
template <class EigenSolver>
class EigenSparseSolver : public sofa::core::behavior::LinearSolver {
public:
    SOFA_CLASS(SOFA_TEMPLATE(EigenSparseSolver, EigenSolver), LinearSolver);
    using SparseMatrix = typename EigenSolver::MatrixType;
    using Vector = Eigen::Matrix<FLOATING_POINT_TYPE, Eigen::Dynamic, 1>;

    /// Backend solvers
    enum class Backend : unsigned int {
        // Default solvers provided by Eigen (SimplicialLLT, SimplicialLDLT, SparseLU, SparseQR)
        EigenDefault = 0,

#ifdef CARIBOU_WITH_MKL
        // Pardiso MKL solvers (PardisoLLT, PardisoLDLT, PardisoLU)
        Pardiso = 1
#endif
    };

    /**
     * Assemble the system matrix A = (mM + bB + kK) inside the SparseMatrix p_A.
     * @param mparams Mechanical parameters containing the m, b and k factors.
     */
    void assemble (const sofa::core::MechanicalParams* mparams);

    /**
     * Reset the complete system (A, x and b are cleared).
     *
     * When using no preconditioner (None), this does absolutely nothing here since the complete
     * system is never built.
     *
     * This method is called by the MultiMatrix::clear() and MultiMatrix::reset() methods.
     */
    void resetSystem() final;

    /**
     * Set the linear system matrix A = (mM + bB + kK), storing the coefficients m, b and k of
     * the mechanical M,B,K matrices.
     *
     * @param mparams Contains the coefficients m, b and k of the matrices M, B and K
     */
    void setSystemMBKMatrix(const sofa::core::MechanicalParams* mparams) final;

    /**
     * Gives the identifier of the right-hand side vector b. This identifier will be used to find the actual vector
     * in the mechanical objects of the system. The complete dense vector is accumulated from the mechanical objects
     * found in the graph subtree of the current context.
     */
    void setSystemRHVector(sofa::core::MultiVecDerivId b_id) final;

    /**
     * Gives the identifier of the left-hand side vector x. This identifier will be used to find the actual vector
     * in the mechanical objects of the system. The complete dense vector is accumulated from the mechanical objects
     * found in the graph subtree of the current context.
     */
    void setSystemLHVector(sofa::core::MultiVecDerivId x_id) final;

    /**
     * Solves the system using the Eigen solver.
     */
    void solveSystem() override;

    // SOFA overrides
    static std::string GetCustomTemplateName();
    static auto canCreate(EigenSparseSolver<EigenSolver>* o, sofa::core::objectmodel::BaseContext* context, sofa::core::objectmodel::BaseObjectDescription* arg) -> bool;

private:
    /// Private members

    ///< The Eigen solver used (its type is passed as a template parameter and must be derived from Eigen::SparseSolverBase)
    EigenSolver p_solver;

    ///< The mechanical parameters containing the m, b and k coefficients.
    sofa::core::MechanicalParams p_mechanical_params;

    ///< Accessor used to determine the index of each mechanical object matrix and vector in the global system.
    sofa::component::linearsolver::DefaultMultiMatrixAccessor p_accessor;

    ///< The identifier of the b vector
    sofa::core::MultiVecDerivId p_b_id;

    ///< The identifier of the x vector
    sofa::core::MultiVecDerivId p_x_id;

    ///< Global system matrix (only built when a preconditioning method needs it)
    SparseMatrix p_A;

    ///< Global system solution vector (usually filled with an initial guess or the previous solution)
    Vector p_x;

    ///< Global system right-hand side vector
    Vector p_b;

    ///< True if the solver has successfully factorize the system matrix
    bool p_A_is_factorized;
};

} // namespace SofaCaribou::solver
