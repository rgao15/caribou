#ifndef CARIBOU_GEOMETRY_HEXAHEDRON_H
#define CARIBOU_GEOMETRY_HEXAHEDRON_H

#include <Caribou/config.h>
#include <Caribou/Algebra/Vector.h>
#include <Caribou/Geometry/Node.h>
#include <Caribou/Geometry/Quad.h>
#include <Caribou/Geometry/Interpolation/Hexahedron.h>

namespace caribou {
namespace geometry {

template <typename CanonicalElementType>
struct Hexahedron : public CanonicalElementType
{
    static constexpr INTEGER_TYPE NumberOfNodes = CanonicalElementType::NumberOfNodes;
    using NodeType = caribou::geometry::Node<3>;
    using QuadType = Quad<3, typename CanonicalElementType::QuadType>;
    using Index = std::size_t ;
    using Real = FLOATING_POINT_TYPE;

    constexpr
    Hexahedron()
            : p_nodes(CanonicalElementType::nodes)
    {}

    constexpr
    Hexahedron(const std::array<NodeType, NumberOfNodes> & nodes)
            : p_nodes(nodes)
    {}

    template <
            typename ...Nodes,
            REQUIRES(NumberOfNodes == sizeof...(Nodes)),
            REQUIRES(std::conjunction_v<std::is_same<NodeType, Nodes>...>)
    >
    constexpr
    Hexahedron(Nodes&&...remaining_nodes)
    : p_nodes {std::forward<Nodes>(remaining_nodes)...}
    {}

    /** Get the Node at given index */
    inline
    const NodeType &
    node(Index index) const
    {
        return p_nodes[index];
    }

    /** Get the Node at given index */
    inline
    NodeType &
    node(Index index)
    {
        return p_nodes[index];
    }

    /**
     * Get the ith quadrangle face.
     */
    inline
    QuadType
    face(Index index) const
    {
        const auto & face_indices = CanonicalElementType::faces[index];
        std::array<NodeType, QuadType::NumberOfNodes> quad_nodes;
        for (std::size_t i = 0; i < QuadType::NumberOfNodes; ++i)
            quad_nodes[i] = node(face_indices[i]);

        return QuadType(quad_nodes);
    }

    /** Compute the jacobian matrix evaluated at local position {u,v,w}
     * (see interpolation::CanonicalElement::Jacobian for more details).
     * */
    inline
    algebra::Matrix<3, 3, Real>
    jacobian (const Real & u, const Real & v, const Real & w) const
    {
        return CanonicalElementType::Jacobian({u,v, w}, p_nodes);
    }

    /**
     * Compute the transformation of a local position {u,v,w} to its world position {x,y,z}
     */
    inline
    NodeType
    T(const Real & u, const Real & v, const Real & w) const
    {
        return CanonicalElementType::interpolate_at_local_position({u,v,w}, p_nodes);
    }

    /**
     * Compute an integral approximation by gauss quadrature on the hexahedron of the given evaluation function.
     *
     * @example
     * \code{.cpp}
     * // Integrate the polynomial 1 + 2x + 2xy + 3*z on an hexahedron.
     * float result = LinearHexahedron(x1, x2, x3, x4, x5, x6, x7, x8).gauss_integrate(
     *   [] (const LinearHexahedron & hexa, const float & xi, const float & eta, const float & zeta) -> float {
     *     return 1 + 2*xi + 2*xi*eta + 3*zeta;
     *   }
     * );
     * \endcode
     *
     * @tparam ValueType The result type of the evaluation function.
     * This type must implement the assignment "=", assignment-addition "+=", and multiplication "*" with a scalar type (float, double) operators.
     * @tparam EvaluateFunctionType Callback function reference type. See evaluate parameter.
     *
     * @param f
     * Callback function of the signature
     *
     *     ValueType f (const LinearHexahedron & hexa, const float & u, const float & v, const float & w);
     *
     * Where hexa is a reference to the current hexahadron on which we integrate, and the coordinates u, v and w
     * forms the local position of a sample point on which we want to get the evaluation value of type ValueType.
     *
     * @return The value of the integral computed on this hexahedron.
     *
     */
    template <typename ValueType , typename EvaluateFunctor>
    inline
    ValueType
    gauss_quadrature(const ValueType & initial_value, EvaluateFunctor f) const
    {
        static_assert(CanonicalElementType::gauss_nodes.size() == CanonicalElementType::gauss_weights.size(),
                "Gauss nodes must have assigned weights.");

        ValueType result = initial_value;

        for (std::size_t i = 0; i < CanonicalElementType::gauss_nodes.size(); ++i) {
            const auto p = CanonicalElementType::gauss_nodes[i];
            const auto w = CanonicalElementType::gauss_weights[i];
            const auto detJ = jacobian(p[0], p[1], p[2]).determinant();
            const auto eval = f(*this, p[0], p[1], p[2]);
            result += eval * w * detJ;
        }

        return result;
    }

    /**
     * Compute an integral approximation by gauss quadrature on the hexahedron of the given evaluation function.
     *
     * @example
     * \code{.cpp}
     * // Integrate the polynomial 1 + 2x + 2xy + 3*z on an hexahedron.
     * float result = Hexahedron(x1, x2, x3, x4, x5, x6, x7, x8).gauss_integrate(
     *   [] (const LinearHexahedron & hexa, const float & xi, const float & eta, const float & zeta) -> float {
     *     return 1 + 2*xi + 2*xi*eta + 3*zeta;
     *   }
     * );
     * \endcode
     *
     * @tparam EvaluateFunctionType Callback function reference type. See f parameter.
     *
     * @param f
     * Callback function of the signature
     *
     *     ValueType f (const Hexahedron & hexa, const float & u, const float & v, const float & w);
     *
     * Where hexa is a reference to the current hexahadron on which we integrate, and the coordinates u, v and w
     * forms the local position of a sample point on which we want to get the evaluation value.
     *
     * @return The value of the integral computed on this hexahedron.
     *
     */
    template <typename EvaluateFunctor>
    inline
    auto
    gauss_quadrature(EvaluateFunctor f) const
    {
        static_assert(CanonicalElementType::gauss_nodes.size() == CanonicalElementType::gauss_weights.size(),
                      "Gauss nodes must have assigned weights.");

        const auto p0 = CanonicalElementType::gauss_nodes[0];
        const auto w0 = CanonicalElementType::gauss_weights[0];
        const auto detJ0 = jacobian(p0[0], p0[1], p0[2]).determinant();
        const auto eval0 = f(*this, p0[0], p0[1], p0[2]);
        auto result = eval0 * w0 * detJ0;

        for (std::size_t i = 1; i < CanonicalElementType::gauss_nodes.size(); ++i) {
            const auto p = CanonicalElementType::gauss_nodes[i];
            const auto w = CanonicalElementType::gauss_weights[i];
            const auto detJ = jacobian(p[0], p[1], p[2]).determinant();
            const auto eval = f(*this, p[0], p[1], p[2]);
            result += eval * w * detJ;
        }

        return result;
    }

    /**
     * Check whether the hexahedron is a parallelepiped.
     *
     * A parallelepiped:
     * - Has six faces, each of which is a parallelogram
     * - Has three pairs of parallel faces
     *
     * The transformation from its elemental frame to its world frame can be defined as
     *                      | x1 + 1/2 (1 + u) hx |
     * (x,y,z) = Q(u,v,w) = | y1 + 1/2 (1 + v) hy |
     *                      | z1 + 1/2 (1 + w) hz |
     *
     * where (x1,y1,z1) are the world coordinates of the node #0 on the hexahedron, and (hx,hy,hz) denote the hexahedron
     * size w.r.t the x,y and z directions.
     *
     * And the Jacobian of this transformation is constant and defined as
     *                          | hx 0  0  |
     * J = gradQ^T(u,v,w) = 1/2 | 0  hy 0  |
     *                          | 0  0  hz |
     */
    inline
    bool
    is_a_parallelepiped() const noexcept
    {
        const auto lx = node(1) - node(0);
        const auto lz = node(4) - node(0);

        return (
                (node(3)-node(2) + lx).length_squared() < EPSILON && // edges 0-1 and 2-3 have the same length
                (node(1)-node(5) + lz).length_squared() < EPSILON && // edges 0-4 and 1-5 have the same length
                (node(2)-node(6) + lz).length_squared() < EPSILON && // edges 0-4 and 2-6 have the same length
                (node(3)-node(7) + lz).length_squared() < EPSILON    // edges 0-4 and 3-7 have the same length
        );
    }

    /**
     * Extract the frame positioned at the center of the hexahedron by computing the cross product of the unit vectors
     * from the center of the hexahedron to the center of the opposite faces.
     *
     * This function will return a matrix of the form:
     * | ux vx wx |
     * | uy vy wy |
     * | uz vz wz |
     *
     * Where [ux uy uz], [vx, vy, vz] and [wx, wy, wz] are orthogonal unitary vectors representing the u, v and w frame
     * in the current hexahedron. If the hexahedron is regular and not rotated, this matrix is the Identity matrix.
     * If it is regular but rotated, rotating the hexa by the transposed of this frame should align the u,v,w axis to the
     * x,y,z world frame (identity matrix).
     */
    inline
    algebra::Matrix<3,3,FLOATING_POINT_TYPE>
    extract_frame_by_cross_products() const
    {
        const auto hexa_center = T(0,0,0); // Hexahedron's center position

        const auto quad_faced_to_u_axis = face(2); // Quad that lies in front of the u axis
        const auto face_u_center = quad_faced_to_u_axis.T(0,0); // Center position of this quad

        const auto quad_faced_to_v_axis = face(4); // Quad that lies in front of the v axis
        const auto face_v_center = quad_faced_to_v_axis.T(0,0); // Center position of this quad

        /* @todo(jnbrunet2000@gmail.com): select between the pairs of axis (center-to-u, center-to-v),
          (center-to-u, center-to-w) and (center-to-v, center-to-w) to find the best match (closer to orthogonal) */

        // Vector from the hexa's center to the center of the quad faced to the u axis
        const auto center_to_u = face_u_center - hexa_center;

        // Vector from the hexa's center to the center of the quad faced to the v axis
        const auto center_to_v = face_v_center - hexa_center;

        // u-axis
        const auto u = center_to_u.unit();

        // v-axis
        auto v = center_to_v.unit();

        // w-axis
        const auto w = u.cross(v).unit();

        // v-axis (recompute the v-axis in case u and v aren't orthogonal
        v = w.cross(u).unit();

        return {u,v,w};
    }

private:
    std::array<NodeType, NumberOfNodes> p_nodes;
};

template <
        typename ...Nodes,
        REQUIRES(8 == sizeof...(Nodes)),
        REQUIRES(std::conjunction_v<std::is_same<caribou::geometry::Node<3>, Nodes>...>)
>
Hexahedron(Nodes&&...remaining_nodes) -> Hexahedron<interpolation::Hexahedron8>;

Hexahedron() -> Hexahedron<interpolation::Hexahedron8>;

} // namespace geometry
} // namespace caribou
#endif //CARIBOU_GEOMETRY_HEXAHEDRON_H