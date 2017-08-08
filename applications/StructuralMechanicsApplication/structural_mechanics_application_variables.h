// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:		 BSD License
//					 license: structural_mechanics_application/license.txt
//
//  Main authors:    Riccardo Rossi
//

#if !defined(KRATOS_STRUCTURAL_MECHANICS_APPLICATION_VARIABLES_H_INCLUDED )
#define  KRATOS_STRUCTURAL_MECHANICS_APPLICATION_VARIABLES_H_INCLUDED

// System includes

// External includes

// Project includes
#include "includes/model_part.h"
#include "includes/define.h"
#include "includes/kratos_application.h"
//#include "structural_mechanics_application.h"
#include "includes/mat_variables.h"
#include "custom_utilities/shell_cross_section.hpp"

namespace Kratos
{
    typedef array_1d<double,3> Vector3;

    // Generalized eigenvalue problem
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, int, BUILD_LEVEL )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, EIGENVALUE_VECTOR)
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix , EIGENVECTOR_MATRIX )

    // Geometrical
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, AREA )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, IT )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, IY )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, IZ )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, CROSS_AREA )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, MEAN_RADIUS )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, int,    SECTION_SIDES )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix , GEOMETRIC_STIFFNESS )

    //fusseder
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, IZ_SENSITIVITY )

    // Truss generalized variables
    KRATOS_DEFINE_VARIABLE(double, TRUSS_PRESTRESS_PK2)
    KRATOS_DEFINE_VARIABLE(bool, TRUSS_IS_CABLE)

    // Beam generalized variables
    KRATOS_DEFINE_VARIABLE(double, AREA_EFFECTIVE_Y)
    KRATOS_DEFINE_VARIABLE(double, AREA_EFFECTIVE_Z)
    KRATOS_DEFINE_VARIABLE(double, INERTIA_ROT_Y)
    KRATOS_DEFINE_VARIABLE(double, INERTIA_ROT_Z)
    KRATOS_DEFINE_VARIABLE(Vector, LOCAL_AXES_VECTOR)
    KRATOS_DEFINE_VARIABLE(bool, LUMPED_MASS_MATRIX)

    // Shell generalized variables
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_STRAIN )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_STRAIN_GLOBAL )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_CURVATURE )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_CURVATURE_GLOBAL )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_FORCE )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_FORCE_GLOBAL )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_MOMENT )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, SHELL_MOMENT_GLOBAL )

    // Membrane1 variables
    KRATOS_DEFINE_APPLICATION_VARIABLE(STRUCTURAL_MECHANICS_APPLICATION, double, PRESTRESS_11)
    KRATOS_DEFINE_APPLICATION_VARIABLE(STRUCTURAL_MECHANICS_APPLICATION, double, PRESTRESS_22)
    KRATOS_DEFINE_APPLICATION_VARIABLE(STRUCTURAL_MECHANICS_APPLICATION, double, PRESTRESS_12)

    // Cross section
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, ShellCrossSection::Pointer, SHELL_CROSS_SECTION )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, int, SHELL_CROSS_SECTION_OUTPUT_PLY_ID )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, SHELL_CROSS_SECTION_OUTPUT_PLY_LOCATION )

    // Nodal stiffness for the nodal concentrated element
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS(STRUCTURAL_MECHANICS_APPLICATION, NODAL_STIFFNESS )
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS(STRUCTURAL_MECHANICS_APPLICATION, NODAL_DAMPING_RATIO)

    // CONDITIONS
    /* Beam conditions */
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS(STRUCTURAL_MECHANICS_APPLICATION, POINT_MOMENT )
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS(STRUCTURAL_MECHANICS_APPLICATION, LOCAL_POINT_MOMENT )
    /* Torque conditions */
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS(STRUCTURAL_MECHANICS_APPLICATION, POINT_TORQUE )

    // Adding the SPRISM EAS variables
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION,double, ALPHA_EAS);
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION,bool, EAS_IMP);
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION,bool, SPRISM_TL_UL);

    // Adding the SPRISM additional variables
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION,double, ANG_ROT); // TODO: Transform into a vector

    // Adding the Sprism number of transversal integration points
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION,int, NINT_TRANS);

    // Adding the SPRISM variable to deactivate the quadratic interpolation
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION,bool, QUAD_ON);

    // Additional strain measures
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, HENCKY_STRAIN_VECTOR);
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, HENCKY_STRAIN_TENSOR);

    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, VON_MISES_STRESS )  

    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Matrix, REFERENCE_DEFORMATION_GRADIENT);
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, REFERENCE_DEFORMATION_GRADIENT_DETERMINANT);
    
    // Rayleigh variables
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, RAYLEIGH_ALPHA )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, double, RAYLEIGH_BETA )

    // Nodal load variables
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS( STRUCTURAL_MECHANICS_APPLICATION, POINT_LOAD )
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS( STRUCTURAL_MECHANICS_APPLICATION, LINE_LOAD )
    KRATOS_DEFINE_3D_APPLICATION_VARIABLE_WITH_COMPONENTS( STRUCTURAL_MECHANICS_APPLICATION, SURFACE_LOAD )

    // Condition load variables
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, POINT_LOADS_VECTOR )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, LINE_LOADS_VECTOR )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, SURFACE_LOADS_VECTOR )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, POSITIVE_FACE_PRESSURES_VECTOR )
    KRATOS_DEFINE_APPLICATION_VARIABLE( STRUCTURAL_MECHANICS_APPLICATION, Vector, NEGATIVE_FACE_PRESSURES_VECTOR )

    // fusseder needed for sensitivity analysis
    KRATOS_DEFINE_3D_VARIABLE_WITH_COMPONENTS(  ADJOINT_NODE_COORD )
}

#endif	/* KRATOS_STRUCTURAL_MECHANICS_APPLICATION_VARIABLES_H_INCLUDED */
