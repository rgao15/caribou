#pragma once

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace SofaCaribou::forcefield::python {
void addFictitiousGridElasticForce(py::module &m);
}