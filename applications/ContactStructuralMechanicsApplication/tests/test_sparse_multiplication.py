﻿from __future__ import print_function, absolute_import, division
import KratosMultiphysics
import KratosMultiphysics.StructuralMechanicsApplication as StructuralMechanicsApplication
import KratosMultiphysics.ContactStructuralMechanicsApplication as CSMA
import KratosMultiphysics.KratosUnittest as KratosUnittest
import os

def GetFilePath(fileName):
    return os.path.dirname(os.path.realpath(__file__)) + "/" + fileName

class TestSparseMatrixSum(KratosUnittest.TestCase):

    def setUp(self):
        pass

    def __sparse_matrix_sum(self):
        file_name = "../../../kratos/tests/A.mm"

        # Read the matrices
        A = KratosMultiphysics.CompressedMatrix()
        B = KratosMultiphysics.CompressedMatrix()
        KratosMultiphysics.ReadMatrixMarketMatrix(GetFilePath(file_name),A)
        KratosMultiphysics.ReadMatrixMarketMatrix(GetFilePath(file_name),B)

        try:
            from scipy import sparse, io
            import numpy as np
            missing_scipy = False
        except ImportError as e:
            missing_scipy = True

        if (missing_scipy == False):
            A_python = io.mmread(file_name)
            A_python.toarray()
            B_python = io.mmread(file_name)
            B_python.toarray()

            A_python = A_python + B_python

            # Solve
            solver = CSMA.SparseMatrixMultiplicationUtility
            solver.MatrixAdd(A, B, 1.0)

            for i, j in np.nditer(A_python.nonzero()):
                self.assertAlmostEqual(A[int(i), int(j)], A_python[int(i), int(j)])
        else:
            self.assertTrue(True)

    def test_sparse_matrix_sum(self):
        self.__sparse_matrix_sum()

class TestSparseMatrixMultiplication(KratosUnittest.TestCase):

    def setUp(self):
        pass

    def __sparse_matrix_multiplication(self, problem = "saad"):
        file_name = "../../../kratos/tests/A.mm"

        # Read the matrices
        A = KratosMultiphysics.CompressedMatrix()
        A2 = KratosMultiphysics.CompressedMatrix()
        KratosMultiphysics.ReadMatrixMarketMatrix(GetFilePath(file_name),A)

        try:
            from scipy import sparse, io
            import numpy as np
            missing_scipy = False
        except ImportError as e:
            missing_scipy = True

        if (missing_scipy == False):
            A_python = io.mmread(file_name)
            A_python.toarray()

            A2_python = np.dot(A_python, A_python)

            # Solve
            solver = CSMA.SparseMatrixMultiplicationUtility
            if (problem == "saad"):
                solver.MatrixMultiplicationSaad(A, A, A2)
            elif (problem == "rmerge"):
                solver.MatrixMultiplicationRMerge(A, A, A2)

            for i, j in np.nditer(A2_python.nonzero()):
                self.assertAlmostEqual(A2[int(i), int(j)], A2_python[int(i), int(j)], 1e-3)
        else:
            self.assertTrue(True)

    def test_sparse_matrix_multiplication_saad(self):
        self.__sparse_matrix_multiplication("saad")

    def test_sparse_matrix_multiplication_rmerge(self):
        self.__sparse_matrix_multiplication("rmerge")

if __name__ == '__main__':
    KratosUnittest.main()
